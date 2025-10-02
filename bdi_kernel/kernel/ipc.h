
/**
 * @file ipc.h
 * @brief Core IPC framework with zero-copy support
 * 
 * Phase 4: Zero-Copy IPC & Communication
 * 
 * This header defines the core IPC framework that provides a unified
 * abstraction for all IPC mechanisms (shared memory, pipes, sockets).
 * 
 * Key Features:
 * - Zero-copy IPC operations
 * - Unified IPC handle abstraction
 * - C23 atomic state management
 * - [[nodiscard]] error handling
 * - Integration with scheduler for blocking operations
 */

#ifndef BDI_IPC_H
#define BDI_IPC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* IPC constants */
#define IPC_NAME_MAX            64      /* Maximum IPC object name length */
#define IPC_MAX_HANDLES         4096    /* Maximum number of IPC handles */
#define IPC_INVALID_ID          0       /* Invalid IPC ID */

/* IPC flags */
#define IPC_FLAG_BLOCKING       (1U << 0)  /* Blocking operations */
#define IPC_FLAG_NONBLOCKING    (1U << 1)  /* Non-blocking operations */
#define IPC_FLAG_NUMA_LOCAL     (1U << 2)  /* NUMA-local allocation */
#define IPC_FLAG_HUGE_PAGES     (1U << 3)  /* Use huge pages (2MB) */

/* IPC permissions */
#define IPC_PERM_READ           (1U << 0)  /* Read permission */
#define IPC_PERM_WRITE          (1U << 1)  /* Write permission */
#define IPC_PERM_EXEC           (1U << 2)  /* Execute permission */
#define IPC_PERM_OWNER          (1U << 3)  /* Owner permission */

/* IPC error codes */
#define IPC_SUCCESS             0       /* Success */
#define IPC_ERROR_INVALID       -1      /* Invalid parameter */
#define IPC_ERROR_NOMEM         -2      /* Out of memory */
#define IPC_ERROR_NOTFOUND      -3      /* IPC object not found */
#define IPC_ERROR_EXISTS        -4      /* IPC object already exists */
#define IPC_ERROR_PERM          -5      /* Permission denied */
#define IPC_ERROR_BUSY          -6      /* Resource busy */
#define IPC_ERROR_TIMEOUT       -7      /* Operation timed out */
#define IPC_ERROR_CLOSED        -8      /* IPC object closed */
#define IPC_ERROR_FULL          -9      /* Buffer full */
#define IPC_ERROR_EMPTY         -10     /* Buffer empty */

/**
 * @brief IPC types
 */
enum ipc_type {
    IPC_TYPE_INVALID = 0,   /* Invalid type */
    IPC_TYPE_SHM,           /* Shared memory */
    IPC_TYPE_PIPE,          /* Pipe (SPSC) */
    IPC_TYPE_SOCKET,        /* Socket/message queue (MPSC) */
    IPC_TYPE_MAX
};

/**
 * @brief IPC states
 * 
 * State transitions are atomic using compare-exchange operations.
 */
enum ipc_state {
    IPC_STATE_INVALID = 0,  /* Invalid state */
    IPC_STATE_CREATED,      /* Created but not initialized */
    IPC_STATE_READY,        /* Ready for operations */
    IPC_STATE_ACTIVE,       /* Active (has connections) */
    IPC_STATE_CLOSING,      /* Closing (cleanup in progress) */
    IPC_STATE_CLOSED        /* Closed */
};

/**
 * @brief IPC statistics
 * 
 * Per-IPC object statistics for monitoring and debugging.
 */
struct ipc_stats {
    _Atomic uint64_t total_sends;       /* Total send operations */
    _Atomic uint64_t total_recvs;       /* Total receive operations */
    _Atomic uint64_t total_bytes_sent;  /* Total bytes sent */
    _Atomic uint64_t total_bytes_recv;  /* Total bytes received */
    _Atomic uint64_t blocked_sends;     /* Blocked send operations */
    _Atomic uint64_t blocked_recvs;     /* Blocked receive operations */
    _Atomic uint64_t failed_sends;      /* Failed send operations */
    _Atomic uint64_t failed_recvs;      /* Failed receive operations */
};

/**
 * @brief IPC handle structure
 * 
 * Unified handle for all IPC types.
 * Uses C23 atomics for lock-free state management.
 */
struct ipc_handle {
    /* IPC ID (unique) */
    uint64_t id;
    
    /* IPC type */
    enum ipc_type type;
    
    /* IPC state (atomic) */
    _Atomic uint32_t state;
    
    /* IPC flags */
    uint32_t flags;
    
    /* IPC permissions */
    uint32_t permissions;
    
    /* Reference count (atomic) */
    _Atomic uint32_t ref_count;
    
    /* Owner process ID */
    uint64_t owner_pid;
    
    /* NUMA node */
    int numa_node;
    
    /* IPC name */
    char name[IPC_NAME_MAX];
    
    /* Type-specific data pointer */
    void *data;
    
    /* Statistics */
    struct ipc_stats stats;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(uint64_t) * 2 + 
                          sizeof(enum ipc_type) + 
                          sizeof(_Atomic uint32_t) * 2 + 
                          sizeof(uint32_t) * 2 + 
                          sizeof(int) + 
                          sizeof(char) * IPC_NAME_MAX + 
                          sizeof(void *) + 
                          sizeof(struct ipc_stats)) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Global IPC state
 * 
 * Tracks all IPC objects in the system.
 */
struct ipc_global_state {
    /* Total IPC objects (atomic) */
    _Atomic uint32_t total_objects;
    
    /* Active IPC objects (atomic) */
    _Atomic uint32_t active_objects;
    
    /* Next IPC ID (atomic) */
    _Atomic uint64_t next_id;
    
    /* Global statistics */
    struct ipc_stats global_stats;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(_Atomic uint32_t) * 2 + 
                          sizeof(_Atomic uint64_t) + 
                          sizeof(struct ipc_stats)) % 64)];
} __attribute__((aligned(64)));

/* ===================================================================
 * IPC Framework Functions
 * =================================================================== */

/**
 * @brief Initialize IPC subsystem
 * 
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int ipc_init(void);

/**
 * @brief Shutdown IPC subsystem
 */
void ipc_shutdown(void);

/**
 * @brief Create IPC handle
 * 
 * @param handle Pointer to store created handle
 * @param type IPC type
 * @param name IPC object name (optional, can be NULL)
 * @param flags IPC flags
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int ipc_create(struct ipc_handle **handle, 
                             enum ipc_type type,
                             const char *name,
                             uint32_t flags);

/**
 * @brief Destroy IPC handle
 * 
 * @param handle IPC handle to destroy
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int ipc_destroy(struct ipc_handle *handle);

/**
 * @brief Open existing IPC object by name
 * 
 * @param handle Pointer to store opened handle
 * @param name IPC object name
 * @param flags IPC flags
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int ipc_open(struct ipc_handle **handle,
                           const char *name,
                           uint32_t flags);

/**
 * @brief Close IPC handle
 * 
 * @param handle IPC handle to close
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int ipc_close(struct ipc_handle *handle);

/**
 * @brief Increment reference count
 * 
 * @param handle IPC handle
 * @return New reference count
 */
uint32_t ipc_ref(struct ipc_handle *handle);

/**
 * @brief Decrement reference count
 * 
 * @param handle IPC handle
 * @return New reference count
 */
uint32_t ipc_unref(struct ipc_handle *handle);

/**
 * @brief Get IPC state
 * 
 * @param handle IPC handle
 * @return Current IPC state
 */
enum ipc_state ipc_get_state(const struct ipc_handle *handle);

/**
 * @brief Set IPC state (atomic)
 * 
 * @param handle IPC handle
 * @param new_state New state
 * @return true if state changed, false otherwise
 */
bool ipc_set_state(struct ipc_handle *handle, enum ipc_state new_state);

/**
 * @brief Compare and exchange IPC state (atomic)
 * 
 * @param handle IPC handle
 * @param expected Expected current state
 * @param desired Desired new state
 * @return true if state changed, false otherwise
 */
bool ipc_cas_state(struct ipc_handle *handle,
                   enum ipc_state expected,
                   enum ipc_state desired);

/**
 * @brief Get IPC statistics
 * 
 * @param handle IPC handle
 * @param stats Pointer to store statistics
 */
void ipc_get_stats(const struct ipc_handle *handle, struct ipc_stats *stats);

/**
 * @brief Get global IPC statistics
 * 
 * @param stats Pointer to store statistics
 */
void ipc_get_global_stats(struct ipc_stats *stats);

/**
 * @brief Print IPC statistics
 * 
 * @param handle IPC handle (NULL for global stats)
 */
void ipc_print_stats(const struct ipc_handle *handle);

/**
 * @brief Get IPC type name
 * 
 * @param type IPC type
 * @return Type name string
 */
const char *ipc_type_name(enum ipc_type type);

/**
 * @brief Get IPC state name
 * 
 * @param state IPC state
 * @return State name string
 */
const char *ipc_state_name(enum ipc_state state);

/**
 * @brief Get error string
 * 
 * @param error Error code
 * @return Error string
 */
const char *ipc_error_string(int error);

#endif /* BDI_IPC_H */
