
/**
 * @file pipe.h
 * @brief Zero-copy pipes with lock-free SPSC rings
 * 
 * Phase 4: Zero-Copy IPC & Communication
 * 
 * This header defines zero-copy pipes using lock-free SPSC (Single-Producer
 * Single-Consumer) ring buffers for efficient one-to-one communication.
 * 
 * Key Features:
 * - Lock-free SPSC ring buffer
 * - Zero-copy operations
 * - Blocking and non-blocking modes
 * - Integration with Phase 3 scheduler
 * - Direct buffer access
 */

#ifndef BDI_PIPE_H
#define BDI_PIPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <sys/types.h>
#include "ipc.h"

/* Pipe constants */
#define PIPE_NAME_MAX           64      /* Maximum pipe name length */
#define PIPE_MIN_SIZE           4096    /* Minimum pipe buffer size (4KB) */
#define PIPE_DEFAULT_SIZE       65536   /* Default pipe buffer size (64KB) */
#define PIPE_MAX_SIZE           (16 * 1024 * 1024)  /* Maximum size (16MB) */
#define PIPE_MAX_PIPES          1024    /* Maximum number of pipes */

/* Pipe flags */
#define PIPE_FLAG_BLOCKING      IPC_FLAG_BLOCKING
#define PIPE_FLAG_NONBLOCKING   IPC_FLAG_NONBLOCKING
#define PIPE_FLAG_NUMA_LOCAL    IPC_FLAG_NUMA_LOCAL

/**
 * @brief Lock-free SPSC ring buffer
 * 
 * Single-Producer Single-Consumer ring buffer with cache-line alignment
 * to prevent false sharing between producer and consumer.
 */
struct spsc_ring {
    /* Producer head (cache-line aligned) */
    _Atomic size_t head __attribute__((aligned(64)));
    
    /* Consumer tail (cache-line aligned) */
    _Atomic size_t tail __attribute__((aligned(64)));
    
    /* Buffer capacity (power of 2) */
    size_t capacity;
    
    /* Buffer mask (capacity - 1) */
    size_t mask;
    
    /* Buffer data */
    uint8_t *buffer;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(size_t) * 2 + sizeof(uint8_t *)) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Pipe structure
 * 
 * Represents a zero-copy pipe with SPSC ring buffer.
 */
struct pipe {
    /* IPC handle */
    struct ipc_handle *ipc_handle;
    
    /* SPSC ring buffer */
    struct spsc_ring ring;
    
    /* Flags */
    uint32_t flags;
    
    /* NUMA node */
    int numa_node;
    
    /* Producer task ID (atomic) */
    _Atomic uint64_t producer_tid;
    
    /* Consumer task ID (atomic) */
    _Atomic uint64_t consumer_tid;
    
    /* Blocked producer flag (atomic) */
    _Atomic bool producer_blocked;
    
    /* Blocked consumer flag (atomic) */
    _Atomic bool consumer_blocked;
    
    /* Statistics */
    _Atomic uint64_t total_writes;
    _Atomic uint64_t total_reads;
    _Atomic uint64_t total_bytes_written;
    _Atomic uint64_t total_bytes_read;
    _Atomic uint64_t blocked_writes;
    _Atomic uint64_t blocked_reads;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(struct ipc_handle *) + 
                          sizeof(struct spsc_ring) + 
                          sizeof(uint32_t) + 
                          sizeof(int) + 
                          sizeof(_Atomic uint64_t) * 6 + 
                          sizeof(_Atomic bool) * 2) % 64)];
} __attribute__((aligned(64)));

/* ===================================================================
 * Pipe Functions
 * =================================================================== */

/**
 * @brief Initialize pipe subsystem
 * 
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int pipe_init(void);

/**
 * @brief Shutdown pipe subsystem
 */
void pipe_shutdown(void);

/**
 * @brief Create pipe
 * 
 * @param pipe Pointer to store created pipe
 * @param name Pipe name (optional, can be NULL)
 * @param size Buffer size (must be power of 2)
 * @param flags Pipe flags
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int pipe_create(struct pipe **pipe,
                              const char *name,
                              size_t size,
                              uint32_t flags);

/**
 * @brief Destroy pipe
 * 
 * @param pipe Pipe to destroy
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int pipe_destroy(struct pipe *pipe);

/**
 * @brief Open existing pipe by name
 * 
 * @param pipe Pointer to store opened pipe
 * @param name Pipe name
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int pipe_open(struct pipe **pipe, const char *name);

/**
 * @brief Close pipe
 * 
 * @param pipe Pipe to close
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int pipe_close(struct pipe *pipe);

/**
 * @brief Write data to pipe (zero-copy)
 * 
 * @param pipe Pipe
 * @param data Data to write
 * @param size Size in bytes
 * @return Number of bytes written, or error code (< 0)
 */
[[nodiscard]] ssize_t pipe_write(struct pipe *pipe,
                                 const void *data,
                                 size_t size);

/**
 * @brief Read data from pipe (zero-copy)
 * 
 * @param pipe Pipe
 * @param data Buffer to read into
 * @param size Maximum size to read
 * @return Number of bytes read, or error code (< 0)
 */
[[nodiscard]] ssize_t pipe_read(struct pipe *pipe,
                                void *data,
                                size_t size);

/**
 * @brief Get available bytes for reading
 * 
 * @param pipe Pipe
 * @return Number of bytes available
 */
size_t pipe_available(const struct pipe *pipe);

/**
 * @brief Get free space for writing
 * 
 * @param pipe Pipe
 * @return Number of bytes free
 */
size_t pipe_free_space(const struct pipe *pipe);

/**
 * @brief Check if pipe is empty
 * 
 * @param pipe Pipe
 * @return true if empty, false otherwise
 */
bool pipe_is_empty(const struct pipe *pipe);

/**
 * @brief Check if pipe is full
 * 
 * @param pipe Pipe
 * @return true if full, false otherwise
 */
bool pipe_is_full(const struct pipe *pipe);

/**
 * @brief Print pipe statistics
 * 
 * @param pipe Pipe (NULL for global stats)
 */
void pipe_print_stats(const struct pipe *pipe);

#endif /* BDI_PIPE_H */
