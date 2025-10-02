
/**
 * @file pipe.c
 * @brief Zero-copy pipe implementation with SPSC rings
 * 
 * Phase 4: Zero-Copy IPC & Communication
 */

#include "pipe.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global pipe state */
static struct pipe *g_pipes[PIPE_MAX_PIPES] = {0};
static _Atomic uint32_t g_pipe_count = 0;

/* ===================================================================
 * SPSC Ring Buffer Functions
 * =================================================================== */

/**
 * @brief Initialize SPSC ring buffer
 */
static int spsc_ring_init(struct spsc_ring *ring, size_t capacity)
{
    if (!ring || capacity < PIPE_MIN_SIZE || capacity > PIPE_MAX_SIZE) {
        return IPC_ERROR_INVALID;
    }
    
    /* Ensure capacity is power of 2 */
    if ((capacity & (capacity - 1)) != 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate buffer */
    ring->buffer = alloc_memory(capacity, 64);
    if (!ring->buffer) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Initialize ring */
    atomic_store_explicit(&ring->head, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0, memory_order_relaxed);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    
    return IPC_SUCCESS;
}

/**
 * @brief Destroy SPSC ring buffer
 */
static void spsc_ring_destroy(struct spsc_ring *ring)
{
    if (!ring || !ring->buffer) {
        return;
    }
    
    free_memory(ring->buffer, ring->capacity);
    ring->buffer = NULL;
}

/**
 * @brief Get available bytes in ring
 */
static inline size_t spsc_ring_available(const struct spsc_ring *ring)
{
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    return head - tail;
}

/**
 * @brief Get free space in ring
 */
static inline size_t spsc_ring_free_space(const struct spsc_ring *ring)
{
    return ring->capacity - spsc_ring_available(ring);
}

/**
 * @brief Write to SPSC ring (producer)
 */
static ssize_t spsc_ring_write(struct spsc_ring *ring,
                               const void *data,
                               size_t size)
{
    if (!ring || !data || size == 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Load head and tail */
    size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    /* Check available space */
    size_t available = ring->capacity - (head - tail);
    if (size > available) {
        return IPC_ERROR_FULL;
    }
    
    /* Write data (may wrap around) */
    size_t pos = head & ring->mask;
    size_t remaining = ring->capacity - pos;
    
    if (size <= remaining) {
        /* Single contiguous write */
        memcpy(ring->buffer + pos, data, size);
    } else {
        /* Wrap-around write */
        memcpy(ring->buffer + pos, data, remaining);
        memcpy(ring->buffer, (const uint8_t *)data + remaining,
               size - remaining);
    }
    
    /* Update head (release semantics for consumer) */
    atomic_store_explicit(&ring->head, head + size, memory_order_release);
    
    return (ssize_t)size;
}

/**
 * @brief Read from SPSC ring (consumer)
 */
static ssize_t spsc_ring_read(struct spsc_ring *ring,
                              void *data,
                              size_t size)
{
    if (!ring || !data || size == 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Load head and tail */
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    
    /* Check available data */
    size_t available = head - tail;
    if (available == 0) {
        return IPC_ERROR_EMPTY;
    }
    
    /* Read at most 'size' bytes */
    size_t to_read = (size < available) ? size : available;
    
    /* Read data (may wrap around) */
    size_t pos = tail & ring->mask;
    size_t remaining = ring->capacity - pos;
    
    if (to_read <= remaining) {
        /* Single contiguous read */
        memcpy(data, ring->buffer + pos, to_read);
    } else {
        /* Wrap-around read */
        memcpy(data, ring->buffer + pos, remaining);
        memcpy((uint8_t *)data + remaining, ring->buffer,
               to_read - remaining);
    }
    
    /* Update tail (release semantics for producer) */
    atomic_store_explicit(&ring->tail, tail + to_read, memory_order_release);
    
    return (ssize_t)to_read;
}

/* ===================================================================
 * Pipe Functions
 * =================================================================== */

/**
 * @brief Allocate pipe structure
 */
static struct pipe *pipe_alloc(void)
{
    struct pipe *p = alloc_memory(sizeof(struct pipe), 64);
    if (!p) {
        return NULL;
    }
    
    memset(p, 0, sizeof(struct pipe));
    return p;
}

/**
 * @brief Free pipe structure
 */
static void pipe_free(struct pipe *p)
{
    if (!p) {
        return;
    }
    
    free_memory(p, sizeof(struct pipe));
}

/**
 * @brief Add pipe to global table
 */
static int pipe_add(struct pipe *p)
{
    uint32_t count = atomic_load_explicit(&g_pipe_count, memory_order_acquire);
    
    if (count >= PIPE_MAX_PIPES) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Find empty slot */
    for (uint32_t i = 0; i < PIPE_MAX_PIPES; i++) {
        if (g_pipes[i] == NULL) {
            g_pipes[i] = p;
            atomic_fetch_add_explicit(&g_pipe_count, 1, memory_order_release);
            return IPC_SUCCESS;
        }
    }
    
    return IPC_ERROR_NOMEM;
}

/**
 * @brief Remove pipe from global table
 */
static void pipe_remove(struct pipe *p)
{
    if (!p) {
        return;
    }
    
    for (uint32_t i = 0; i < PIPE_MAX_PIPES; i++) {
        if (g_pipes[i] == p) {
            g_pipes[i] = NULL;
            atomic_fetch_sub_explicit(&g_pipe_count, 1, memory_order_release);
            break;
        }
    }
}

/* ===================================================================
 * Public API Implementation
 * =================================================================== */

int pipe_init(void)
{
    /* Clear pipe table */
    memset(g_pipes, 0, sizeof(g_pipes));
    atomic_store_explicit(&g_pipe_count, 0, memory_order_relaxed);
    
    printf("[PIPE] Pipe subsystem initialized\n");
    return IPC_SUCCESS;
}

void pipe_shutdown(void)
{
    /* Destroy all pipes */
    uint32_t count = atomic_load_explicit(&g_pipe_count, memory_order_acquire);
    
    for (uint32_t i = 0; i < PIPE_MAX_PIPES && count > 0; i++) {
        struct pipe *p = g_pipes[i];
        if (p) {
            pipe_destroy(p);
            count--;
        }
    }
    
    printf("[PIPE] Pipe subsystem shutdown\n");
}

int pipe_create(struct pipe **pipe,
                const char *name,
                size_t size,
                uint32_t flags)
{
    if (!pipe || size < PIPE_MIN_SIZE || size > PIPE_MAX_SIZE) {
        return IPC_ERROR_INVALID;
    }
    
    /* Ensure size is power of 2 */
    if ((size & (size - 1)) != 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate pipe structure */
    struct pipe *p = pipe_alloc();
    if (!p) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Create IPC handle */
    int ret = ipc_create(&p->ipc_handle, IPC_TYPE_PIPE, name, flags);
    if (ret != IPC_SUCCESS) {
        pipe_free(p);
        return ret;
    }
    
    /* Initialize SPSC ring */
    ret = spsc_ring_init(&p->ring, size);
    if (ret != IPC_SUCCESS) {
        ipc_destroy(p->ipc_handle);
        pipe_free(p);
        return ret;
    }
    
    /* Initialize pipe */
    p->flags = flags;
    p->numa_node = (flags & PIPE_FLAG_NUMA_LOCAL) ? 
                   numa_current_node() : -1;
    atomic_store_explicit(&p->producer_tid, 0, memory_order_relaxed);
    atomic_store_explicit(&p->consumer_tid, 0, memory_order_relaxed);
    atomic_store_explicit(&p->producer_blocked, false, memory_order_relaxed);
    atomic_store_explicit(&p->consumer_blocked, false, memory_order_relaxed);
    
    /* Add to global table */
    ret = pipe_add(p);
    if (ret != IPC_SUCCESS) {
        spsc_ring_destroy(&p->ring);
        ipc_destroy(p->ipc_handle);
        pipe_free(p);
        return ret;
    }
    
    /* Link IPC handle to pipe */
    p->ipc_handle->data = p;
    
    printf("[PIPE] Created pipe: size=%zu, numa_node=%d\n",
           size, p->numa_node);
    
    *pipe = p;
    return IPC_SUCCESS;
}

int pipe_destroy(struct pipe *pipe)
{
    if (!pipe) {
        return IPC_ERROR_INVALID;
    }
    
    /* Remove from global table */
    pipe_remove(pipe);
    
    /* Destroy SPSC ring */
    spsc_ring_destroy(&pipe->ring);
    
    /* Destroy IPC handle */
    if (pipe->ipc_handle) {
        ipc_destroy(pipe->ipc_handle);
    }
    
    /* Free pipe structure */
    pipe_free(pipe);
    
    return IPC_SUCCESS;
}

int pipe_open(struct pipe **pipe, const char *name)
{
    if (!pipe || !name) {
        return IPC_ERROR_INVALID;
    }
    
    /* Open IPC handle */
    struct ipc_handle *handle;
    int ret = ipc_open(&handle, name, 0);
    if (ret != IPC_SUCCESS) {
        return ret;
    }
    
    /* Get pipe from handle */
    struct pipe *p = (struct pipe *)handle->data;
    if (!p) {
        ipc_close(handle);
        return IPC_ERROR_INVALID;
    }
    
    *pipe = p;
    return IPC_SUCCESS;
}

int pipe_close(struct pipe *pipe)
{
    if (!pipe || !pipe->ipc_handle) {
        return IPC_ERROR_INVALID;
    }
    
    return ipc_close(pipe->ipc_handle);
}

ssize_t pipe_write(struct pipe *pipe, const void *data, size_t size)
{
    if (!pipe || !data || size == 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Write to SPSC ring */
    ssize_t written = spsc_ring_write(&pipe->ring, data, size);
    
    if (written > 0) {
        /* Update statistics */
        atomic_fetch_add_explicit(&pipe->total_writes, 1,
                                 memory_order_relaxed);
        atomic_fetch_add_explicit(&pipe->total_bytes_written, written,
                                 memory_order_relaxed);
        
        /* Wake up blocked consumer (TODO: integrate with scheduler) */
        if (atomic_load_explicit(&pipe->consumer_blocked,
                                memory_order_acquire)) {
            atomic_store_explicit(&pipe->consumer_blocked, false,
                                memory_order_release);
            /* TODO: Wake up consumer task */
        }
    } else if (written == IPC_ERROR_FULL) {
        /* Buffer full */
        if (pipe->flags & PIPE_FLAG_BLOCKING) {
            /* Block producer (TODO: integrate with scheduler) */
            atomic_store_explicit(&pipe->producer_blocked, true,
                                memory_order_release);
            atomic_fetch_add_explicit(&pipe->blocked_writes, 1,
                                     memory_order_relaxed);
            /* TODO: Block current task */
        }
    }
    
    return written;
}

ssize_t pipe_read(struct pipe *pipe, void *data, size_t size)
{
    if (!pipe || !data || size == 0) {
        return IPC_ERROR_INVALID;
    }
    
    /* Read from SPSC ring */
    ssize_t read_bytes = spsc_ring_read(&pipe->ring, data, size);
    
    if (read_bytes > 0) {
        /* Update statistics */
        atomic_fetch_add_explicit(&pipe->total_reads, 1,
                                 memory_order_relaxed);
        atomic_fetch_add_explicit(&pipe->total_bytes_read, read_bytes,
                                 memory_order_relaxed);
        
        /* Wake up blocked producer (TODO: integrate with scheduler) */
        if (atomic_load_explicit(&pipe->producer_blocked,
                                memory_order_acquire)) {
            atomic_store_explicit(&pipe->producer_blocked, false,
                                memory_order_release);
            /* TODO: Wake up producer task */
        }
    } else if (read_bytes == IPC_ERROR_EMPTY) {
        /* Buffer empty */
        if (pipe->flags & PIPE_FLAG_BLOCKING) {
            /* Block consumer (TODO: integrate with scheduler) */
            atomic_store_explicit(&pipe->consumer_blocked, true,
                                memory_order_release);
            atomic_fetch_add_explicit(&pipe->blocked_reads, 1,
                                     memory_order_relaxed);
            /* TODO: Block current task */
        }
    }
    
    return read_bytes;
}

size_t pipe_available(const struct pipe *pipe)
{
    return pipe ? spsc_ring_available(&pipe->ring) : 0;
}

size_t pipe_free_space(const struct pipe *pipe)
{
    return pipe ? spsc_ring_free_space(&pipe->ring) : 0;
}

bool pipe_is_empty(const struct pipe *pipe)
{
    return pipe && (spsc_ring_available(&pipe->ring) == 0);
}

bool pipe_is_full(const struct pipe *pipe)
{
    return pipe && (spsc_ring_free_space(&pipe->ring) == 0);
}

void pipe_print_stats(const struct pipe *pipe)
{
    if (pipe) {
        printf("[PIPE] Statistics for pipe %p:\n", (void *)pipe);
        printf("  Capacity: %zu bytes\n", pipe->ring.capacity);
        printf("  Available: %zu bytes\n", pipe_available(pipe));
        printf("  Free space: %zu bytes\n", pipe_free_space(pipe));
        printf("  Total writes: %lu\n",
               atomic_load_explicit(&pipe->total_writes, memory_order_relaxed));
        printf("  Total reads: %lu\n",
               atomic_load_explicit(&pipe->total_reads, memory_order_relaxed));
        printf("  Total bytes written: %lu\n",
               atomic_load_explicit(&pipe->total_bytes_written,
                                   memory_order_relaxed));
        printf("  Total bytes read: %lu\n",
               atomic_load_explicit(&pipe->total_bytes_read,
                                   memory_order_relaxed));
        printf("  Blocked writes: %lu\n",
               atomic_load_explicit(&pipe->blocked_writes, memory_order_relaxed));
        printf("  Blocked reads: %lu\n",
               atomic_load_explicit(&pipe->blocked_reads, memory_order_relaxed));
    } else {
        printf("[PIPE] Global statistics:\n");
        printf("  Total pipes: %u\n",
               atomic_load_explicit(&g_pipe_count, memory_order_relaxed));
    }
}
