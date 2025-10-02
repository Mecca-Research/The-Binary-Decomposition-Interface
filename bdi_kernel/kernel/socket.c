
/**
 * @file socket.c
 * @brief Zero-copy socket implementation with MPSC rings
 * 
 * Phase 4: Zero-Copy IPC & Communication
 */

#include "socket.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global socket state */
static struct socket *g_sockets[SOCKET_MAX_SOCKETS] = {0};
static _Atomic uint32_t g_socket_count = 0;

/* ===================================================================
 * MPSC Ring Buffer Functions
 * =================================================================== */

/**
 * @brief Initialize MPSC ring buffer
 */
static int mpsc_ring_init(struct mpsc_ring *ring, size_t capacity)
{
    if (!ring || capacity < SOCKET_MIN_CAPACITY || 
        capacity > SOCKET_MAX_CAPACITY) {
        return IPC_ERROR_INVALID;
    }
    
    /* Ensure capacity is power of 2 */
    if ((capacity & (capacity - 1)) != 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate message array */
    ring->messages = alloc_memory(capacity * sizeof(struct socket_message), 64);
    if (!ring->messages) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Initialize ring */
    memset(ring->messages, 0, capacity * sizeof(struct socket_message));
    atomic_store_explicit(&ring->head, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0, memory_order_relaxed);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    
    return IPC_SUCCESS;
}

/**
 * @brief Destroy MPSC ring buffer
 */
static void mpsc_ring_destroy(struct mpsc_ring *ring)
{
    if (!ring || !ring->messages) {
        return;
    }
    
    /* Free any remaining message data */
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    for (size_t i = tail; i < head; i++) {
        struct socket_message *msg = &ring->messages[i & ring->mask];
        if (msg->data) {
            free_memory(msg->data, msg->size);
        }
    }
    
    free_memory(ring->messages, ring->capacity * sizeof(struct socket_message));
    ring->messages = NULL;
}

/**
 * @brief Get pending messages in ring
 */
static inline size_t mpsc_ring_pending(const struct mpsc_ring *ring)
{
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    return head - tail;
}

/**
 * @brief Enqueue message to MPSC ring (producer, lock-free)
 * 
 * BUGFIX (Phase 4): Use CAS loop to reserve slot, write message data FIRST,
 * then publish. Previous code incremented head before writing message data,
 * allowing consumer to read uninitialized data and causing crashes.
 */
static int mpsc_ring_enqueue(struct mpsc_ring *ring,
                             const void *data,
                             size_t size,
                             uint8_t priority,
                             uint64_t sender_tid)
{
    if (!ring || !data || size == 0 || size > SOCKET_MAX_MSG_SIZE) {
        return IPC_ERROR_INVALID;
    }
    
    /* Use CAS loop to reserve a slot atomically */
    size_t head;
    while (1) {
        head = atomic_load_explicit(&ring->head, memory_order_acquire);
        size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
        
        /* Check if ring is full */
        if (head - tail >= ring->capacity) {
            return IPC_ERROR_FULL;  /* Ring full */
        }
        
        /* Try to atomically increment head (reserve slot) */
        if (atomic_compare_exchange_weak_explicit(&ring->head, &head, head + 1,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire)) {
            /* Success! We reserved this slot */
            break;
        }
        /* CAS failed, retry */
    }
    
    /* Allocate message data (zero-copy: store pointer) */
    void *msg_data = alloc_memory(size, 64);
    if (!msg_data) {
        /* Failed to allocate - need to undo reservation */
        atomic_fetch_sub_explicit(&ring->head, 1, memory_order_release);
        return IPC_ERROR_NOMEM;
    }
    
    memcpy(msg_data, data, size);
    
    /* Calculate slot index using OLD head value (before increment) */
    size_t index = head & ring->mask;
    
    /* Write message into slot - this is now safe because we reserved it */
    struct socket_message *msg = &ring->messages[index];
    msg->data = msg_data;
    msg->size = size;
    msg->priority = priority;
    msg->sender_tid = sender_tid;
    msg->timestamp = 0; /* TODO: Get timestamp */
    
    /* Memory barrier to ensure writes are visible before consumer reads */
    atomic_thread_fence(memory_order_release);
    
    return IPC_SUCCESS;
}

/**
 * @brief Dequeue message from MPSC ring (consumer, wait-free)
 */
static int mpsc_ring_dequeue(struct mpsc_ring *ring,
                             struct socket_message *msg)
{
    if (!ring || !msg) {
        return IPC_ERROR_INVALID;
    }
    
    /* Load head and tail */
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    
    /* Check if ring is empty */
    if (tail >= head) {
        return IPC_ERROR_EMPTY;
    }
    
    /* Get message */
    struct socket_message *src = &ring->messages[tail & ring->mask];
    *msg = *src;
    
    /* Clear slot */
    src->data = NULL;
    src->size = 0;
    
    /* Update tail (release semantics for producers) */
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    
    return IPC_SUCCESS;
}

/* ===================================================================
 * Socket Functions
 * =================================================================== */

/**
 * @brief Allocate socket structure
 */
static struct socket *socket_alloc(void)
{
    struct socket *s = alloc_memory(sizeof(struct socket), 64);
    if (!s) {
        return NULL;
    }
    
    memset(s, 0, sizeof(struct socket));
    return s;
}

/**
 * @brief Free socket structure
 */
static void socket_free(struct socket *s)
{
    if (!s) {
        return;
    }
    
    free_memory(s, sizeof(struct socket));
}

/**
 * @brief Add socket to global table
 */
static int socket_add(struct socket *s)
{
    uint32_t count = atomic_load_explicit(&g_socket_count,
                                         memory_order_acquire);
    
    if (count >= SOCKET_MAX_SOCKETS) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Find empty slot */
    for (uint32_t i = 0; i < SOCKET_MAX_SOCKETS; i++) {
        if (g_sockets[i] == NULL) {
            g_sockets[i] = s;
            atomic_fetch_add_explicit(&g_socket_count, 1,
                                     memory_order_release);
            return IPC_SUCCESS;
        }
    }
    
    return IPC_ERROR_NOMEM;
}

/**
 * @brief Remove socket from global table
 */
static void socket_remove(struct socket *s)
{
    if (!s) {
        return;
    }
    
    for (uint32_t i = 0; i < SOCKET_MAX_SOCKETS; i++) {
        if (g_sockets[i] == s) {
            g_sockets[i] = NULL;
            atomic_fetch_sub_explicit(&g_socket_count, 1,
                                     memory_order_release);
            break;
        }
    }
}

/* ===================================================================
 * Public API Implementation
 * =================================================================== */

int socket_init(void)
{
    /* Clear socket table */
    memset(g_sockets, 0, sizeof(g_sockets));
    atomic_store_explicit(&g_socket_count, 0, memory_order_relaxed);
    
    printf("[SOCKET] Socket subsystem initialized\n");
    return IPC_SUCCESS;
}

void socket_shutdown(void)
{
    /* Destroy all sockets */
    uint32_t count = atomic_load_explicit(&g_socket_count,
                                         memory_order_acquire);
    
    for (uint32_t i = 0; i < SOCKET_MAX_SOCKETS && count > 0; i++) {
        struct socket *s = g_sockets[i];
        if (s) {
            socket_destroy(s);
            count--;
        }
    }
    
    printf("[SOCKET] Socket subsystem shutdown\n");
}

int socket_create(struct socket **socket,
                  const char *name,
                  size_t capacity,
                  uint32_t flags)
{
    if (!socket || capacity < SOCKET_MIN_CAPACITY || 
        capacity > SOCKET_MAX_CAPACITY) {
        return IPC_ERROR_INVALID;
    }
    
    /* Ensure capacity is power of 2 */
    if ((capacity & (capacity - 1)) != 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate socket structure */
    struct socket *s = socket_alloc();
    if (!s) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Create IPC handle */
    int ret = ipc_create(&s->ipc_handle, IPC_TYPE_SOCKET, name, flags);
    if (ret != IPC_SUCCESS) {
        socket_free(s);
        return ret;
    }
    
    /* Initialize MPSC ring */
    ret = mpsc_ring_init(&s->ring, capacity);
    if (ret != IPC_SUCCESS) {
        ipc_destroy(s->ipc_handle);
        socket_free(s);
        return ret;
    }
    
    /* Initialize socket */
    s->flags = flags;
    s->numa_node = (flags & SOCKET_FLAG_NUMA_LOCAL) ? 
                   numa_current_node() : -1;
    atomic_store_explicit(&s->consumer_tid, 0, memory_order_relaxed);
    atomic_store_explicit(&s->num_producers, 0, memory_order_relaxed);
    atomic_store_explicit(&s->consumer_blocked, false, memory_order_relaxed);
    
    /* Add to global table */
    ret = socket_add(s);
    if (ret != IPC_SUCCESS) {
        mpsc_ring_destroy(&s->ring);
        ipc_destroy(s->ipc_handle);
        socket_free(s);
        return ret;
    }
    
    /* Link IPC handle to socket */
    s->ipc_handle->data = s;
    
    printf("[SOCKET] Created socket: capacity=%zu, numa_node=%d\n",
           capacity, s->numa_node);
    
    *socket = s;
    return IPC_SUCCESS;
}

int socket_destroy(struct socket *socket)
{
    if (!socket) {
        return IPC_ERROR_INVALID;
    }
    
    /* Remove from global table */
    socket_remove(socket);
    
    /* Destroy MPSC ring */
    mpsc_ring_destroy(&socket->ring);
    
    /* Destroy IPC handle */
    if (socket->ipc_handle) {
        ipc_destroy(socket->ipc_handle);
    }
    
    /* Free socket structure */
    socket_free(socket);
    
    return IPC_SUCCESS;
}

int socket_open(struct socket **socket, const char *name)
{
    if (!socket || !name) {
        return IPC_ERROR_INVALID;
    }
    
    /* Open IPC handle */
    struct ipc_handle *handle;
    int ret = ipc_open(&handle, name, 0);
    if (ret != IPC_SUCCESS) {
        return ret;
    }
    
    /* Get socket from handle */
    struct socket *s = (struct socket *)handle->data;
    if (!s) {
        ipc_close(handle);
        return IPC_ERROR_INVALID;
    }
    
    /* Increment producer count */
    atomic_fetch_add_explicit(&s->num_producers, 1, memory_order_relaxed);
    
    *socket = s;
    return IPC_SUCCESS;
}

int socket_close(struct socket *socket)
{
    if (!socket || !socket->ipc_handle) {
        return IPC_ERROR_INVALID;
    }
    
    /* Decrement producer count */
    atomic_fetch_sub_explicit(&socket->num_producers, 1, memory_order_relaxed);
    
    return ipc_close(socket->ipc_handle);
}

int socket_send(struct socket *socket,
                const void *data,
                size_t size,
                uint8_t priority)
{
    if (!socket || !data || size == 0 || size > SOCKET_MAX_MSG_SIZE) {
        return IPC_ERROR_INVALID;
    }
    
    /* Enqueue message */
    uint64_t sender_tid = 0; /* TODO: Get current task ID */
    int ret = mpsc_ring_enqueue(&socket->ring, data, size, priority,
                                sender_tid);
    
    if (ret == IPC_SUCCESS) {
        /* Update statistics */
        atomic_fetch_add_explicit(&socket->total_sends, 1,
                                 memory_order_relaxed);
        atomic_fetch_add_explicit(&socket->total_bytes_sent, size,
                                 memory_order_relaxed);
        
        /* Wake up blocked consumer (TODO: integrate with scheduler) */
        if (atomic_load_explicit(&socket->consumer_blocked,
                                memory_order_acquire)) {
            atomic_store_explicit(&socket->consumer_blocked, false,
                                memory_order_release);
            /* TODO: Wake up consumer task */
        }
    } else if (ret == IPC_ERROR_FULL) {
        /* Queue full */
        atomic_fetch_add_explicit(&socket->dropped_messages, 1,
                                 memory_order_relaxed);
        
        if (socket->flags & SOCKET_FLAG_BLOCKING) {
            /* Block sender (TODO: integrate with scheduler) */
            atomic_fetch_add_explicit(&socket->blocked_sends, 1,
                                     memory_order_relaxed);
            /* TODO: Block current task */
        }
    }
    
    return ret;
}

int socket_recv(struct socket *socket, struct socket_message *msg)
{
    if (!socket || !msg) {
        return IPC_ERROR_INVALID;
    }
    
    /* Dequeue message */
    int ret = mpsc_ring_dequeue(&socket->ring, msg);
    
    if (ret == IPC_SUCCESS) {
        /* Update statistics */
        atomic_fetch_add_explicit(&socket->total_recvs, 1,
                                 memory_order_relaxed);
        atomic_fetch_add_explicit(&socket->total_bytes_recv, msg->size,
                                 memory_order_relaxed);
    } else if (ret == IPC_ERROR_EMPTY) {
        /* Queue empty */
        if (socket->flags & SOCKET_FLAG_BLOCKING) {
            /* Block consumer (TODO: integrate with scheduler) */
            atomic_store_explicit(&socket->consumer_blocked, true,
                                memory_order_release);
            atomic_fetch_add_explicit(&socket->blocked_recvs, 1,
                                     memory_order_relaxed);
            /* TODO: Block current task */
        }
    }
    
    return ret;
}

size_t socket_pending(const struct socket *socket)
{
    return socket ? mpsc_ring_pending(&socket->ring) : 0;
}

bool socket_is_empty(const struct socket *socket)
{
    return socket && (mpsc_ring_pending(&socket->ring) == 0);
}

bool socket_is_full(const struct socket *socket)
{
    return socket && (mpsc_ring_pending(&socket->ring) >= socket->ring.capacity);
}

void socket_print_stats(const struct socket *socket)
{
    if (socket) {
        printf("[SOCKET] Statistics for socket %p:\n", (void *)socket);
        printf("  Capacity: %zu messages\n", socket->ring.capacity);
        printf("  Pending: %zu messages\n", socket_pending(socket));
        printf("  Producers: %u\n",
               atomic_load_explicit(&socket->num_producers,
                                   memory_order_relaxed));
        printf("  Total sends: %lu\n",
               atomic_load_explicit(&socket->total_sends,
                                   memory_order_relaxed));
        printf("  Total recvs: %lu\n",
               atomic_load_explicit(&socket->total_recvs,
                                   memory_order_relaxed));
        printf("  Total bytes sent: %lu\n",
               atomic_load_explicit(&socket->total_bytes_sent,
                                   memory_order_relaxed));
        printf("  Total bytes recv: %lu\n",
               atomic_load_explicit(&socket->total_bytes_recv,
                                   memory_order_relaxed));
        printf("  Blocked sends: %lu\n",
               atomic_load_explicit(&socket->blocked_sends,
                                   memory_order_relaxed));
        printf("  Blocked recvs: %lu\n",
               atomic_load_explicit(&socket->blocked_recvs,
                                   memory_order_relaxed));
        printf("  Dropped messages: %lu\n",
               atomic_load_explicit(&socket->dropped_messages,
                                   memory_order_relaxed));
    } else {
        printf("[SOCKET] Global statistics:\n");
        printf("  Total sockets: %u\n",
               atomic_load_explicit(&g_socket_count, memory_order_relaxed));
    }
}
