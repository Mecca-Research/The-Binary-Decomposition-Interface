
/**
 * @file socket.h
 * @brief Zero-copy sockets with lock-free MPSC rings
 * 
 * Phase 4: Zero-Copy IPC & Communication
 * 
 * This header defines zero-copy sockets/message queues using lock-free MPSC
 * (Multi-Producer Single-Consumer) ring buffers for efficient many-to-one
 * communication.
 * 
 * Key Features:
 * - Lock-free MPSC ring buffer
 * - Message-based communication
 * - Priority support
 * - Blocking and non-blocking modes
 * - Integration with Phase 3 scheduler
 */

#ifndef BDI_SOCKET_H
#define BDI_SOCKET_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "ipc.h"

/* Socket constants */
#define SOCKET_NAME_MAX         64      /* Maximum socket name length */
#define SOCKET_MIN_CAPACITY     16      /* Minimum message capacity */
#define SOCKET_DEFAULT_CAPACITY 256     /* Default message capacity */
#define SOCKET_MAX_CAPACITY     4096    /* Maximum message capacity */
#define SOCKET_MAX_MSG_SIZE     (64 * 1024)  /* Maximum message size (64KB) */
#define SOCKET_MAX_SOCKETS      1024    /* Maximum number of sockets */

/* Socket flags */
#define SOCKET_FLAG_BLOCKING    IPC_FLAG_BLOCKING
#define SOCKET_FLAG_NONBLOCKING IPC_FLAG_NONBLOCKING
#define SOCKET_FLAG_NUMA_LOCAL  IPC_FLAG_NUMA_LOCAL
#define SOCKET_FLAG_PRIORITY    (1U << 4)  /* Priority queue support */

/* Message priorities */
#define MSG_PRIORITY_LOW        0       /* Low priority */
#define MSG_PRIORITY_NORMAL     1       /* Normal priority */
#define MSG_PRIORITY_HIGH       2       /* High priority */
#define MSG_PRIORITY_URGENT     3       /* Urgent priority */

/**
 * @brief Message structure
 * 
 * Represents a message in the socket queue.
 */
struct socket_message {
    /* Message data pointer */
    void *data;
    
    /* Message size */
    size_t size;
    
    /* Message priority */
    uint8_t priority;
    
    /* Padding for alignment */
    uint8_t _pad1[7];
    
    /* Sender task ID */
    uint64_t sender_tid;
    
    /* Timestamp */
    uint64_t timestamp;
} __attribute__((aligned(32)));

/**
 * @brief Lock-free MPSC ring buffer
 * 
 * Multi-Producer Single-Consumer ring buffer with cache-line alignment.
 */
struct mpsc_ring {
    /* Producer head (cache-line aligned, atomic for multiple producers) */
    _Atomic size_t head __attribute__((aligned(64)));
    
    /* Consumer tail (cache-line aligned) */
    _Atomic size_t tail __attribute__((aligned(64)));
    
    /* Ring capacity (power of 2) */
    size_t capacity;
    
    /* Ring mask (capacity - 1) */
    size_t mask;
    
    /* Message array */
    struct socket_message *messages;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(size_t) * 2 + 
                          sizeof(struct socket_message *)) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Socket structure
 * 
 * Represents a zero-copy socket with MPSC ring buffer.
 */
struct socket {
    /* IPC handle */
    struct ipc_handle *ipc_handle;
    
    /* MPSC ring buffer */
    struct mpsc_ring ring;
    
    /* Flags */
    uint32_t flags;
    
    /* NUMA node */
    int numa_node;
    
    /* Consumer task ID (atomic) */
    _Atomic uint64_t consumer_tid;
    
    /* Number of producers (atomic) */
    _Atomic uint32_t num_producers;
    
    /* Blocked consumer flag (atomic) */
    _Atomic bool consumer_blocked;
    
    /* Statistics */
    _Atomic uint64_t total_sends;
    _Atomic uint64_t total_recvs;
    _Atomic uint64_t total_bytes_sent;
    _Atomic uint64_t total_bytes_recv;
    _Atomic uint64_t blocked_sends;
    _Atomic uint64_t blocked_recvs;
    _Atomic uint64_t dropped_messages;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(struct ipc_handle *) + 
                          sizeof(struct mpsc_ring) + 
                          sizeof(uint32_t) + 
                          sizeof(int) + 
                          sizeof(_Atomic uint64_t) * 7 + 
                          sizeof(_Atomic uint32_t) + 
                          sizeof(_Atomic bool)) % 64)];
} __attribute__((aligned(64)));

/* ===================================================================
 * Socket Functions
 * =================================================================== */

/**
 * @brief Initialize socket subsystem
 * 
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_init(void);

/**
 * @brief Shutdown socket subsystem
 */
void socket_shutdown(void);

/**
 * @brief Create socket
 * 
 * @param socket Pointer to store created socket
 * @param name Socket name (optional, can be NULL)
 * @param capacity Message capacity (must be power of 2)
 * @param flags Socket flags
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_create(struct socket **socket,
                                const char *name,
                                size_t capacity,
                                uint32_t flags);

/**
 * @brief Destroy socket
 * 
 * @param socket Socket to destroy
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_destroy(struct socket *socket);

/**
 * @brief Open existing socket by name
 * 
 * @param socket Pointer to store opened socket
 * @param name Socket name
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_open(struct socket **socket, const char *name);

/**
 * @brief Close socket
 * 
 * @param socket Socket to close
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_close(struct socket *socket);

/**
 * @brief Send message to socket (zero-copy)
 * 
 * @param socket Socket
 * @param data Message data
 * @param size Message size
 * @param priority Message priority
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_send(struct socket *socket,
                              const void *data,
                              size_t size,
                              uint8_t priority);

/**
 * @brief Receive message from socket (zero-copy)
 * 
 * @param socket Socket
 * @param msg Pointer to store received message
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int socket_recv(struct socket *socket,
                              struct socket_message *msg);

/**
 * @brief Get number of pending messages
 * 
 * @param socket Socket
 * @return Number of pending messages
 */
size_t socket_pending(const struct socket *socket);

/**
 * @brief Check if socket is empty
 * 
 * @param socket Socket
 * @return true if empty, false otherwise
 */
bool socket_is_empty(const struct socket *socket);

/**
 * @brief Check if socket is full
 * 
 * @param socket Socket
 * @return true if full, false otherwise
 */
bool socket_is_full(const struct socket *socket);

/**
 * @brief Print socket statistics
 * 
 * @param socket Socket (NULL for global stats)
 */
void socket_print_stats(const struct socket *socket);

#endif /* BDI_SOCKET_H */
