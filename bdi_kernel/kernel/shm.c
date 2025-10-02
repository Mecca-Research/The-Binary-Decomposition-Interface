
/**
 * @file shm.c
 * @brief Shared memory implementation with huge pages
 * 
 * Phase 4: Zero-Copy IPC & Communication
 */

#include "shm.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global SHM state */
static struct shm_region *g_shm_regions[SHM_MAX_REGIONS] = {0};
static _Atomic uint32_t g_shm_count = 0;

/* ===================================================================
 * Internal Helper Functions
 * =================================================================== */

/**
 * @brief Allocate shared memory region structure
 */
static struct shm_region *shm_alloc_region(void)
{
    struct shm_region *region = alloc_memory(sizeof(struct shm_region), 64);
    if (!region) {
        return NULL;
    }
    
    memset(region, 0, sizeof(struct shm_region));
    atomic_store_explicit(&region->ref_count, 1, memory_order_relaxed);
    atomic_store_explicit(&region->num_mappings, 0, memory_order_relaxed);
    
    return region;
}

/**
 * @brief Free shared memory region structure
 */
static void shm_free_region(struct shm_region *region)
{
    if (!region) {
        return;
    }
    
    free_memory(region, sizeof(struct shm_region));
}

/**
 * @brief Allocate shared memory with huge pages
 */
static void *shm_alloc_memory(size_t size, uint32_t flags, int numa_node)
{
    void *ptr = NULL;
    
    if (flags & SHM_FLAG_HUGE_PAGES) {
        /* Align size to huge page boundary */
        size_t aligned_size = (size + SHM_HUGE_PAGE_SIZE - 1) & 
                             ~(SHM_HUGE_PAGE_SIZE - 1);
        
        /* Allocate using NUMA-aware allocator with huge pages */
        if (flags & SHM_FLAG_NUMA_LOCAL) {
            ptr = numa_alloc_onnode(aligned_size, numa_node);
        } else {
            /* Use regular allocation with alignment for huge pages */
            ptr = alloc_memory(aligned_size, SHM_HUGE_PAGE_SIZE);
        }
    } else {
        /* Regular allocation */
        if (flags & SHM_FLAG_NUMA_LOCAL) {
            ptr = numa_alloc_onnode(size, numa_node);
        } else {
            ptr = alloc_memory(size, 64);
        }
    }
    
    /* Zero-initialize if requested */
    if (ptr && (flags & SHM_FLAG_ZERO)) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

/**
 * @brief Free shared memory
 */
static void shm_free_memory(void *ptr, size_t size, uint32_t flags)
{
    if (!ptr) {
        return;
    }
    
    if (flags & SHM_FLAG_NUMA_LOCAL) {
        numa_free(ptr, size);
    } else {
        free_memory(ptr, size);
    }
}

/**
 * @brief Add region to global table
 */
static int shm_add_region(struct shm_region *region)
{
    uint32_t count = atomic_load_explicit(&g_shm_count, memory_order_acquire);
    
    if (count >= SHM_MAX_REGIONS) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Find empty slot */
    for (uint32_t i = 0; i < SHM_MAX_REGIONS; i++) {
        if (g_shm_regions[i] == NULL) {
            g_shm_regions[i] = region;
            atomic_fetch_add_explicit(&g_shm_count, 1, memory_order_release);
            return IPC_SUCCESS;
        }
    }
    
    return IPC_ERROR_NOMEM;
}

/**
 * @brief Remove region from global table
 */
static void shm_remove_region(struct shm_region *region)
{
    if (!region) {
        return;
    }
    
    for (uint32_t i = 0; i < SHM_MAX_REGIONS; i++) {
        if (g_shm_regions[i] == region) {
            g_shm_regions[i] = NULL;
            atomic_fetch_sub_explicit(&g_shm_count, 1, memory_order_release);
            break;
        }
    }
}

/* ===================================================================
 * Public API Implementation
 * =================================================================== */

int shm_init(void)
{
    /* Clear region table */
    memset(g_shm_regions, 0, sizeof(g_shm_regions));
    atomic_store_explicit(&g_shm_count, 0, memory_order_relaxed);
    
    printf("[SHM] Shared memory subsystem initialized\n");
    printf("[SHM] Huge page size: %zu bytes (2MB)\n", SHM_HUGE_PAGE_SIZE);
    return IPC_SUCCESS;
}

void shm_shutdown(void)
{
    /* Destroy all regions */
    uint32_t count = atomic_load_explicit(&g_shm_count, memory_order_acquire);
    
    for (uint32_t i = 0; i < SHM_MAX_REGIONS && count > 0; i++) {
        struct shm_region *region = g_shm_regions[i];
        if (region) {
            shm_destroy(region);
            count--;
        }
    }
    
    printf("[SHM] Shared memory subsystem shutdown\n");
}

int shm_create(struct shm_region **region,
               const char *name,
               size_t size,
               uint32_t flags)
{
    if (!region || size < SHM_MIN_SIZE || size > SHM_MAX_SIZE) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate region structure */
    struct shm_region *r = shm_alloc_region();
    if (!r) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Create IPC handle */
    int ret = ipc_create(&r->ipc_handle, IPC_TYPE_SHM, name, flags);
    if (ret != IPC_SUCCESS) {
        shm_free_region(r);
        return ret;
    }
    
    /* Get NUMA node */
    r->numa_node = (flags & SHM_FLAG_NUMA_LOCAL) ? 
                   numa_current_node() : -1;
    
    /* Allocate shared memory */
    r->base = shm_alloc_memory(size, flags, r->numa_node);
    if (!r->base) {
        ipc_destroy(r->ipc_handle);
        shm_free_region(r);
        return IPC_ERROR_NOMEM;
    }
    
    /* Initialize region */
    r->size = size;
    r->allocated_size = (flags & SHM_FLAG_HUGE_PAGES) ?
                       ((size + SHM_HUGE_PAGE_SIZE - 1) & 
                        ~(SHM_HUGE_PAGE_SIZE - 1)) : size;
    r->flags = flags;
    r->permissions = SHM_PERM_READ | SHM_PERM_WRITE | SHM_PERM_OWNER;
    r->owner_pid = 0; /* TODO: Get current process ID */
    r->created_at = 0; /* TODO: Get timestamp */
    
    /* Add to global table */
    ret = shm_add_region(r);
    if (ret != IPC_SUCCESS) {
        shm_free_memory(r->base, r->allocated_size, r->flags);
        ipc_destroy(r->ipc_handle);
        shm_free_region(r);
        return ret;
    }
    
    /* Link IPC handle to region */
    r->ipc_handle->data = r;
    
    printf("[SHM] Created region: size=%zu, huge_pages=%s, numa_node=%d\n",
           size, (flags & SHM_FLAG_HUGE_PAGES) ? "yes" : "no", r->numa_node);
    
    *region = r;
    return IPC_SUCCESS;
}

int shm_destroy(struct shm_region *region)
{
    if (!region) {
        return IPC_ERROR_INVALID;
    }
    
    /* Wait for all mappings to be detached */
    while (atomic_load_explicit(&region->num_mappings,
                               memory_order_acquire) > 0) {
        /* TODO: Add timeout or force cleanup */
    }
    
    /* Remove from global table */
    shm_remove_region(region);
    
    /* Free shared memory */
    shm_free_memory(region->base, region->allocated_size, region->flags);
    
    /* Destroy IPC handle */
    if (region->ipc_handle) {
        ipc_destroy(region->ipc_handle);
    }
    
    /* Free region structure */
    shm_free_region(region);
    
    return IPC_SUCCESS;
}

int shm_open(struct shm_region **region, const char *name)
{
    if (!region || !name) {
        return IPC_ERROR_INVALID;
    }
    
    /* Open IPC handle */
    struct ipc_handle *handle;
    int ret = ipc_open(&handle, name, 0);
    if (ret != IPC_SUCCESS) {
        return ret;
    }
    
    /* Get region from handle */
    struct shm_region *r = (struct shm_region *)handle->data;
    if (!r) {
        ipc_close(handle);
        return IPC_ERROR_INVALID;
    }
    
    /* Increment reference count */
    shm_ref(r);
    
    *region = r;
    return IPC_SUCCESS;
}

int shm_attach(struct shm_region *region,
               struct shm_mapping **mapping,
               uint32_t permissions)
{
    if (!region || !mapping) {
        return IPC_ERROR_INVALID;
    }
    
    /* Allocate mapping structure */
    struct shm_mapping *m = alloc_memory(sizeof(struct shm_mapping), 64);
    if (!m) {
        return IPC_ERROR_NOMEM;
    }
    
    /* Initialize mapping */
    m->region = region;
    m->mapped_addr = region->base;  /* Direct mapping (zero-copy) */
    m->mapped_size = region->size;
    m->flags = region->flags;
    m->permissions = permissions;
    m->pid = 0; /* TODO: Get current process ID */
    
    /* Increment mapping count */
    atomic_fetch_add_explicit(&region->num_mappings, 1, memory_order_release);
    
    /* Increment reference count */
    shm_ref(region);
    
    *mapping = m;
    return IPC_SUCCESS;
}

int shm_detach(struct shm_mapping *mapping)
{
    if (!mapping || !mapping->region) {
        return IPC_ERROR_INVALID;
    }
    
    /* Decrement mapping count */
    atomic_fetch_sub_explicit(&mapping->region->num_mappings, 1,
                             memory_order_release);
    
    /* Decrement reference count */
    shm_unref(mapping->region);
    
    /* Free mapping structure */
    free_memory(mapping, sizeof(struct shm_mapping));
    
    return IPC_SUCCESS;
}

uint32_t shm_ref(struct shm_region *region)
{
    if (!region) {
        return 0;
    }
    
    return atomic_fetch_add_explicit(&region->ref_count, 1,
                                    memory_order_relaxed) + 1;
}

uint32_t shm_unref(struct shm_region *region)
{
    if (!region) {
        return 0;
    }
    
    uint32_t old = atomic_fetch_sub_explicit(&region->ref_count, 1,
                                             memory_order_release);
    if (old > 0) {
        return old - 1;
    }
    return 0;
}

void *shm_get_base(const struct shm_region *region)
{
    return region ? region->base : NULL;
}

size_t shm_get_size(const struct shm_region *region)
{
    return region ? region->size : 0;
}

bool shm_is_huge_pages(const struct shm_region *region)
{
    return region && (region->flags & SHM_FLAG_HUGE_PAGES);
}

int shm_get_numa_node(const struct shm_region *region)
{
    return region ? region->numa_node : -1;
}

void shm_print_stats(const struct shm_region *region)
{
    if (region) {
        printf("[SHM] Statistics for region %p:\n", (void *)region);
        printf("  Size: %zu bytes\n", region->size);
        printf("  Allocated size: %zu bytes\n", region->allocated_size);
        printf("  Huge pages: %s\n", 
               (region->flags & SHM_FLAG_HUGE_PAGES) ? "yes" : "no");
        printf("  NUMA node: %d\n", region->numa_node);
        printf("  Reference count: %u\n",
               atomic_load_explicit(&region->ref_count, memory_order_relaxed));
        printf("  Mappings: %u\n",
               atomic_load_explicit(&region->num_mappings, memory_order_relaxed));
        printf("  Total reads: %lu\n",
               atomic_load_explicit(&region->total_reads, memory_order_relaxed));
        printf("  Total writes: %lu\n",
               atomic_load_explicit(&region->total_writes, memory_order_relaxed));
    } else {
        printf("[SHM] Global statistics:\n");
        printf("  Total regions: %u\n",
               atomic_load_explicit(&g_shm_count, memory_order_relaxed));
    }
}
