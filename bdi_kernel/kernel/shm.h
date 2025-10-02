
/**
 * @file shm.h
 * @brief Shared memory with huge pages support
 * 
 * Phase 4: Zero-Copy IPC & Communication
 * 
 * This header defines shared memory regions with huge page support (2MB pages)
 * for zero-copy IPC. Integrates with Phase 2's NUMA-aware memory management.
 * 
 * Key Features:
 * - 2MB huge pages for reduced TLB misses
 * - NUMA-aware allocation
 * - Atomic reference counting
 * - Zero-copy mapping
 * - Fine-grained permissions
 */

#ifndef BDI_SHM_H
#define BDI_SHM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "ipc.h"

/* Shared memory constants */
#define SHM_NAME_MAX            64      /* Maximum SHM name length */
#define SHM_MIN_SIZE            4096    /* Minimum SHM size (4KB) */
#define SHM_MAX_SIZE            (1ULL << 40)  /* Maximum SHM size (1TB) */
#define SHM_HUGE_PAGE_SIZE      (2 * 1024 * 1024)  /* 2MB huge pages */
#define SHM_MAX_REGIONS         1024    /* Maximum number of SHM regions */

/* Shared memory flags */
#define SHM_FLAG_HUGE_PAGES     (1U << 0)  /* Use huge pages */
#define SHM_FLAG_NUMA_LOCAL     (1U << 1)  /* NUMA-local allocation */
#define SHM_FLAG_ZERO           (1U << 2)  /* Zero-initialize memory */
#define SHM_FLAG_LOCKED         (1U << 3)  /* Lock pages in memory */

/* Shared memory permissions (same as IPC) */
#define SHM_PERM_READ           IPC_PERM_READ
#define SHM_PERM_WRITE          IPC_PERM_WRITE
#define SHM_PERM_EXEC           IPC_PERM_EXEC
#define SHM_PERM_OWNER          IPC_PERM_OWNER

/**
 * @brief Shared memory region structure
 * 
 * Represents a shared memory region with huge page support.
 * Uses C23 atomics for reference counting.
 */
struct shm_region {
    /* IPC handle */
    struct ipc_handle *ipc_handle;
    
    /* Base address */
    void *base;
    
    /* Size in bytes */
    size_t size;
    
    /* Actual allocated size (may be larger due to huge pages) */
    size_t allocated_size;
    
    /* Reference count (atomic) */
    _Atomic uint32_t ref_count;
    
    /* Number of mappings (atomic) */
    _Atomic uint32_t num_mappings;
    
    /* Flags */
    uint32_t flags;
    
    /* Permissions */
    uint32_t permissions;
    
    /* NUMA node */
    int numa_node;
    
    /* Owner process ID */
    uint64_t owner_pid;
    
    /* Creation timestamp */
    uint64_t created_at;
    
    /* Last access timestamp (atomic) */
    _Atomic uint64_t last_access;
    
    /* Statistics */
    _Atomic uint64_t total_reads;
    _Atomic uint64_t total_writes;
    _Atomic uint64_t total_bytes_read;
    _Atomic uint64_t total_bytes_written;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(struct ipc_handle *) + 
                          sizeof(void *) + 
                          sizeof(size_t) * 2 + 
                          sizeof(_Atomic uint32_t) * 2 + 
                          sizeof(uint32_t) * 2 + 
                          sizeof(int) + 
                          sizeof(uint64_t) * 2 + 
                          sizeof(_Atomic uint64_t) * 5) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Shared memory mapping structure
 * 
 * Represents a process's mapping of a shared memory region.
 */
struct shm_mapping {
    /* SHM region */
    struct shm_region *region;
    
    /* Mapped address in process */
    void *mapped_addr;
    
    /* Mapped size */
    size_t mapped_size;
    
    /* Mapping flags */
    uint32_t flags;
    
    /* Mapping permissions */
    uint32_t permissions;
    
    /* Process ID */
    uint64_t pid;
};

/* ===================================================================
 * Shared Memory Functions
 * =================================================================== */

/**
 * @brief Initialize shared memory subsystem
 * 
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_init(void);

/**
 * @brief Shutdown shared memory subsystem
 */
void shm_shutdown(void);

/**
 * @brief Create shared memory region
 * 
 * @param region Pointer to store created region
 * @param name SHM name (optional, can be NULL)
 * @param size Size in bytes
 * @param flags SHM flags
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_create(struct shm_region **region,
                             const char *name,
                             size_t size,
                             uint32_t flags);

/**
 * @brief Destroy shared memory region
 * 
 * @param region SHM region to destroy
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_destroy(struct shm_region *region);

/**
 * @brief Open existing shared memory region by name
 * 
 * @param region Pointer to store opened region
 * @param name SHM name
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_open(struct shm_region **region, const char *name);

/**
 * @brief Attach (map) shared memory region into process
 * 
 * @param region SHM region
 * @param mapping Pointer to store mapping info
 * @param permissions Mapping permissions
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_attach(struct shm_region *region,
                             struct shm_mapping **mapping,
                             uint32_t permissions);

/**
 * @brief Detach (unmap) shared memory region from process
 * 
 * @param mapping SHM mapping
 * @return IPC_SUCCESS on success, error code otherwise
 */
[[nodiscard]] int shm_detach(struct shm_mapping *mapping);

/**
 * @brief Increment reference count
 * 
 * @param region SHM region
 * @return New reference count
 */
uint32_t shm_ref(struct shm_region *region);

/**
 * @brief Decrement reference count
 * 
 * @param region SHM region
 * @return New reference count
 */
uint32_t shm_unref(struct shm_region *region);

/**
 * @brief Get shared memory base address
 * 
 * @param region SHM region
 * @return Base address
 */
void *shm_get_base(const struct shm_region *region);

/**
 * @brief Get shared memory size
 * 
 * @param region SHM region
 * @return Size in bytes
 */
size_t shm_get_size(const struct shm_region *region);

/**
 * @brief Check if using huge pages
 * 
 * @param region SHM region
 * @return true if using huge pages, false otherwise
 */
bool shm_is_huge_pages(const struct shm_region *region);

/**
 * @brief Get NUMA node
 * 
 * @param region SHM region
 * @return NUMA node ID
 */
int shm_get_numa_node(const struct shm_region *region);

/**
 * @brief Print shared memory statistics
 * 
 * @param region SHM region (NULL for global stats)
 */
void shm_print_stats(const struct shm_region *region);

#endif /* BDI_SHM_H */
