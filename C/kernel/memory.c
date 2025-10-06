
/**
 * @file memory.c
 * @brief Core Memory Management Implementation for BDI Kernel
 * @details Implements unified memory management integrating PMM, VMM, and HAM.
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#include "memory.h"
#include "pmm.h"
#include "vmm.h"
#include "ham/ham.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// Allocation Metadata Tracking
// ============================================================================

/**
 * @brief Allocation metadata header
 * @details Stored before each allocation to track size and type
 */
typedef struct {
    size_t size;                // Original allocation size
    uint32_t order;             // PMM order (if PMM-backed)
    page_frame_t *page;         // Page frame pointer (if PMM-backed)
    uint32_t magic;             // Magic number for validation
} alloc_metadata_t;

/**
 * @brief Arena allocation header
 * @details Stored before each arena allocation to track size
 * This enables safe reallocation by knowing the old allocation size
 */
typedef struct {
    size_t size;                // Original allocation size
    uint32_t magic;             // Magic number for validation
} arena_header_t;

#define ALLOC_MAGIC 0xDEADBEEF
#define ARENA_MAGIC 0xABCDEF01
#define METADATA_SIZE sizeof(alloc_metadata_t)
#define ARENA_HEADER_SIZE sizeof(arena_header_t)

// ============================================================================
// Global State
// ============================================================================

static numa_topology_t g_numa_topology = {0};
static memory_stats_t g_memory_stats = {0};
static percpu_arena_t g_percpu_arenas[MAX_CPUS] = {0};
static huge_page_pool_t g_huge_page_pool = {0};
static bool g_memory_initialized = false;

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Get current CPU ID
 */
static inline uint32_t get_current_cpu(void) {
    // In a real kernel, this would use CPU-specific instructions
    // For now, return 0 for single-CPU simulation
    return 0;
}

/**
 * @brief Get current timestamp
 */
static inline uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/**
 * @brief Align size to page boundary
 */
static inline size_t align_to_page(size_t size) {
    return (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/**
 * @brief Check if address is page-aligned
 */
static inline bool is_page_aligned(const void *addr) {
    return ((uintptr_t)addr & (PAGE_SIZE - 1)) == 0;
}

/**
 * @brief Calculate order for given number of pages
 * @param num_pages Number of pages needed
 * @return Order value (2^order >= num_pages)
 */
static inline uint32_t calculate_order(size_t num_pages) {
    if (num_pages == 0) {
        return 0;
    }
    
    uint32_t order = 0;
    size_t pages = 1;
    
    while (pages < num_pages && order < MAX_ORDER) {
        order++;
        pages <<= 1;
    }
    
    return order;
}

// ============================================================================
// NUMA Topology Implementation
// ============================================================================

int numa_init(void) {
    printf("NUMA: Initializing topology discovery...\n");
    
    // Simulate NUMA topology discovery
    // In a real kernel, this would probe hardware
    g_numa_topology.num_nodes = 2;  // Simulate 2 NUMA nodes
    g_numa_topology.initialized = true;
    g_numa_topology.discovery_timestamp = get_timestamp();
    
    for (uint32_t i = 0; i < g_numa_topology.num_nodes; i++) {
        numa_node_info_t *node = &g_numa_topology.nodes[i];
        node->node_id = i;
        node->total_memory = 8ULL * 1024 * 1024 * 1024;  // 8GB per node
        node->free_memory = node->total_memory;
        node->cached_memory = 0;
        
        // Initialize distance matrix
        for (uint32_t j = 0; j < MAX_NUMA_NODES; j++) {
            if (i == j) {
                node->distances[j].distance = 10;   // Local access
                node->distances[j].bandwidth = 255; // Max bandwidth
            } else if (j < g_numa_topology.num_nodes) {
                node->distances[j].distance = 20;   // Remote access
                node->distances[j].bandwidth = 128; // Half bandwidth
            } else {
                node->distances[j].distance = 255;  // Unreachable
                node->distances[j].bandwidth = 0;
            }
        }
        
        // Initialize CPU mask (distribute CPUs across nodes)
        for (uint32_t cpu = 0; cpu < MAX_CPUS; cpu++) {
            if ((cpu % g_numa_topology.num_nodes) == i) {
                node->cpu_mask[cpu / 32] |= (1U << (cpu % 32));
            }
        }
        
        printf("NUMA: Node %u: %llu MB total, distance matrix initialized\n",
               i, node->total_memory / (1024 * 1024));
    }
    
    // Validate topology
    g_numa_topology.topology_valid = numa_validate_topology();
    g_numa_topology.distances_valid = g_numa_topology.topology_valid;
    
    printf("NUMA: Topology discovery complete. Nodes: %u, Valid: %s\n",
           g_numa_topology.num_nodes,
           g_numa_topology.topology_valid ? "YES" : "NO");
    
    return 0;
}

bool numa_validate_topology(void) {
    if (!g_numa_topology.initialized) {
        return false;
    }
    
    // Validate node count
    if (g_numa_topology.num_nodes == 0 || 
        g_numa_topology.num_nodes > MAX_NUMA_NODES) {
        printf("NUMA: Invalid node count: %u\n", g_numa_topology.num_nodes);
        return false;
    }
    
    // Validate distance matrix symmetry and local distances
    for (uint32_t i = 0; i < g_numa_topology.num_nodes; i++) {
        numa_node_info_t *node = &g_numa_topology.nodes[i];
        
        // Check local distance is 10
        if (node->distances[i].distance != 10) {
            printf("NUMA: Invalid local distance for node %u: %u\n",
                   i, node->distances[i].distance);
            return false;
        }
        
        // Check symmetry
        for (uint32_t j = 0; j < g_numa_topology.num_nodes; j++) {
            uint8_t dist_ij = node->distances[j].distance;
            uint8_t dist_ji = g_numa_topology.nodes[j].distances[i].distance;
            
            if (dist_ij != dist_ji) {
                printf("NUMA: Distance matrix not symmetric: "
                       "node %u->%u = %u, node %u->%u = %u\n",
                       i, j, dist_ij, j, i, dist_ji);
                return false;
            }
        }
    }
    
    printf("NUMA: Topology validation passed\n");
    return true;
}

const numa_topology_t* numa_get_topology(void) {
    return &g_numa_topology;
}

uint32_t numa_get_current_node(void) {
    uint32_t cpu = get_current_cpu();
    
    // Find which NUMA node this CPU belongs to
    for (uint32_t node = 0; node < g_numa_topology.num_nodes; node++) {
        uint32_t mask_idx = cpu / 32;
        uint32_t bit_idx = cpu % 32;
        
        if (g_numa_topology.nodes[node].cpu_mask[mask_idx] & (1U << bit_idx)) {
            return node;
        }
    }
    
    return 0;  // Default to node 0
}

uint8_t numa_get_distance(uint32_t from, uint32_t to) {
    if (from >= MAX_NUMA_NODES || to >= MAX_NUMA_NODES) {
        return 255;  // Invalid
    }
    
    return g_numa_topology.nodes[from].distances[to].distance;
}

// ============================================================================
// Per-CPU Arena Implementation
// ============================================================================

static int percpu_arena_init(uint32_t cpu_id) {
    percpu_arena_t *arena = &g_percpu_arenas[cpu_id];
    
    arena->cpu_id = cpu_id;
    arena->numa_node = numa_get_current_node();
    arena->arena_size = 1024 * 1024;  // 1MB per CPU
    arena->arena_base = malloc(arena->arena_size);
    
    if (arena->arena_base == nullptr) {
        return -1;
    }
    
    arena->arena_used = 0;
    arena->cache_count = 0;
    
    printf("Per-CPU Arena: Initialized arena for CPU %u (NUMA node %u)\n",
           cpu_id, arena->numa_node);
    
    return 0;
}

static void* percpu_arena_alloc(size_t size) {
    uint32_t cpu = get_current_cpu();
    percpu_arena_t *arena = &g_percpu_arenas[cpu];
    
    atomic_fetch_add(&arena->alloc_count, 1);
    
    // Try cache first for small allocations
    if (size <= 64 && arena->cache_count > 0) {
        void *cached_ptr = arena->cache[--arena->cache_count];
        
        // Check if cached block is large enough for the requested size
        arena_header_t *header = (arena_header_t*)((char*)cached_ptr - ARENA_HEADER_SIZE);
        
        // Verify magic and check if cached block size is sufficient
        if (header->magic == ARENA_MAGIC && header->size >= size) {
            // Cached block is large enough, reuse it
            // CRITICAL: Do NOT overwrite header->size!
            // The header must always reflect the actual allocated block capacity
            atomic_fetch_add(&arena->cache_hits, 1);
            return cached_ptr;
        } else {
            // Cached block too small or invalid, put it back and allocate fresh
            arena->cache[arena->cache_count++] = cached_ptr;
            // Fall through to fresh allocation below
        }
    }
    
    atomic_fetch_add(&arena->cache_misses, 1);
    
    // Allocate from arena with header
    // Total space needed: header + requested size, aligned to 16 bytes
    size_t total_size = ARENA_HEADER_SIZE + size;
    size_t aligned_size = (total_size + 15) & ~15UL;  // 16-byte alignment
    
    if (arena->arena_used + aligned_size <= arena->arena_size) {
        void *allocation = (char*)arena->arena_base + arena->arena_used;
        arena->arena_used += aligned_size;
        
        // Store header before the user pointer
        arena_header_t *header = (arena_header_t*)allocation;
        header->size = size;
        header->magic = ARENA_MAGIC;
        
        // Return pointer after header
        return (char*)allocation + ARENA_HEADER_SIZE;
    }
    
    // Arena full, fall back to slow path
    atomic_fetch_add(&arena->slow_path_count, 1);
    return nullptr;
}

// ============================================================================
// Huge Page Implementation
// ============================================================================

static int huge_page_pool_init(void) {
    printf("Huge Pages: Initializing pool...\n");
    
    // Allocate huge page arrays
    g_huge_page_pool.pages_2mb = calloc(256, sizeof(huge_page_t));
    g_huge_page_pool.pages_1gb = calloc(16, sizeof(huge_page_t));
    
    if (g_huge_page_pool.pages_2mb == nullptr || 
        g_huge_page_pool.pages_1gb == nullptr) {
        return -1;
    }
    
    atomic_store(&g_huge_page_pool.count_2mb, 0);
    atomic_store(&g_huge_page_pool.count_1gb, 0);
    
    printf("Huge Pages: Pool initialized (256x2MB, 16x1GB capacity)\n");
    return 0;
}

huge_page_t* alloc_huge_page(size_t size, uint32_t numa_node) {
    huge_page_t *page = nullptr;
    
    if (size == HUGE_PAGE_2MB) {
        uint32_t count = atomic_load(&g_huge_page_pool.count_2mb);
        if (count < 256) {
            page = &g_huge_page_pool.pages_2mb[count];
            atomic_fetch_add(&g_huge_page_pool.count_2mb, 1);
            atomic_fetch_add(&g_huge_page_pool.alloc_success_2mb, 1);
        } else {
            atomic_fetch_add(&g_huge_page_pool.alloc_failure_2mb, 1);
            return nullptr;
        }
    } else if (size == HUGE_PAGE_1GB) {
        uint32_t count = atomic_load(&g_huge_page_pool.count_1gb);
        if (count < 16) {
            page = &g_huge_page_pool.pages_1gb[count];
            atomic_fetch_add(&g_huge_page_pool.count_1gb, 1);
            atomic_fetch_add(&g_huge_page_pool.alloc_success_1gb, 1);
        } else {
            atomic_fetch_add(&g_huge_page_pool.alloc_failure_1gb, 1);
            return nullptr;
        }
    } else {
        return nullptr;
    }
    
    // Allocate backing memory
    page->base_addr = aligned_alloc(size, size);
    if (page->base_addr == nullptr) {
        return nullptr;
    }
    
    page->size = size;
    page->numa_node = numa_node;
    page->transparent = false;
    atomic_store(&page->refcount, 1);
    page->allocation_time = get_timestamp();
    page->split_count = 0;
    page->access_count = 0;
    
    atomic_fetch_add(&g_memory_stats.huge_pages_allocated, 1);
    
    printf("Huge Pages: Allocated %s page on NUMA node %u\n",
           size == HUGE_PAGE_2MB ? "2MB" : "1GB", numa_node);
    
    return page;
}

void free_huge_page(huge_page_t *page) {
    if (page == nullptr) {
        return;
    }
    
    if (atomic_fetch_sub(&page->refcount, 1) > 1) {
        return;  // Still has references
    }
    
    free(page->base_addr);
    page->base_addr = nullptr;
    
    atomic_fetch_add(&g_memory_stats.huge_pages_freed, 1);
    
    printf("Huge Pages: Freed %s page\n",
           page->size == HUGE_PAGE_2MB ? "2MB" : "1GB");
}

bool try_promote_huge_page(void *addr, size_t size) {
    if (!is_page_aligned(addr) || size < HUGE_PAGE_2MB) {
        return false;
    }
    
    // Check if region is suitable for promotion
    // In a real implementation, this would check:
    // - Contiguous physical pages
    // - Alignment requirements
    // - Access patterns
    
    atomic_fetch_add(&g_huge_page_pool.promotion_count, 1);
    atomic_fetch_add(&g_memory_stats.thp_promotions, 1);
    
    printf("Huge Pages: Promoted region at %p to huge page\n", addr);
    return true;
}

// ============================================================================
// Page Allocation Wrappers (Bug Fix #5)
// ============================================================================

/**
 * @brief Allocate pages - wrapper for pmm_alloc_pages
 * @param gfp_mask GFP allocation flags
 * @param order Allocation order
 * @return Page frame pointer or nullptr on failure
 */
page_frame_t* alloc_pages(uint32_t gfp_mask, uint32_t order) {
    return pmm_alloc_pages(gfp_mask, order);
}

/**
 * @brief Free pages - wrapper for pmm_free_pages
 * @param page Page frame pointer
 * @param order Allocation order
 */
void free_pages(page_frame_t *page, uint32_t order) {
    pmm_free_pages(page, order);
}

// ============================================================================
// Memory Allocator Implementation
// ============================================================================

int memory_init(void) {
    if (g_memory_initialized) {
        return 0;
    }
    
    printf("Memory: Initializing subsystem...\n");
    
    // Initialize NUMA topology
    if (numa_init() != 0) {
        printf("Memory: NUMA initialization failed\n");
        return -1;
    }
    
    // Initialize PMM
    if (pmm_init() != 0) {
        printf("Memory: PMM initialization failed\n");
        return -1;
    }
    
    // Initialize VMM
    if (vmm_init() != 0) {
        printf("Memory: VMM initialization failed\n");
        return -1;
    }
    
    // Initialize per-CPU arenas
    for (uint32_t cpu = 0; cpu < 4; cpu++) {  // Initialize first 4 CPUs
        if (percpu_arena_init(cpu) != 0) {
            printf("Memory: Per-CPU arena initialization failed for CPU %u\n", cpu);
            return -1;
        }
    }
    
    // Initialize huge page pool
    if (huge_page_pool_init() != 0) {
        printf("Memory: Huge page pool initialization failed\n");
        return -1;
    }
    
    // Initialize statistics
    atomic_store(&g_memory_stats.total_memory, 
                 16ULL * 1024 * 1024 * 1024);  // 16GB total
    atomic_store(&g_memory_stats.free_memory, 
                 atomic_load(&g_memory_stats.total_memory));
    
    g_memory_initialized = true;
    
    printf("Memory: Subsystem initialized successfully\n");
    printf("Memory: Total: %llu MB, NUMA nodes: %u\n",
           atomic_load(&g_memory_stats.total_memory) / (1024 * 1024),
           g_numa_topology.num_nodes);
    
    return 0;
}

void memory_shutdown(void) {
    if (!g_memory_initialized) {
        return;
    }
    
    printf("Memory: Shutting down subsystem...\n");
    
    // Free per-CPU arenas
    for (uint32_t cpu = 0; cpu < MAX_CPUS; cpu++) {
        if (g_percpu_arenas[cpu].arena_base != nullptr) {
            free(g_percpu_arenas[cpu].arena_base);
            g_percpu_arenas[cpu].arena_base = nullptr;
        }
    }
    
    // Free huge page pools
    free(g_huge_page_pool.pages_2mb);
    free(g_huge_page_pool.pages_1gb);
    
    vmm_shutdown();
    pmm_shutdown();
    
    g_memory_initialized = false;
    
    printf("Memory: Subsystem shutdown complete\n");
}

void* kmalloc(size_t size, uint32_t flags) {
    if (size == 0) {
        return nullptr;
    }
    
    atomic_fetch_add(&g_memory_stats.total_allocs, 1);
    
    void *ptr = nullptr;
    
    // Try per-CPU arena for small allocations
    if (size <= 1024 && !(flags & GFP_DMA)) {
        ptr = percpu_arena_alloc(size);
        if (ptr != nullptr) {
            if (flags & GFP_ZERO) {
                memset(ptr, 0, size);
            }
            
            atomic_fetch_add(&g_memory_stats.used_memory, size);
            atomic_fetch_sub(&g_memory_stats.free_memory, size);
            
            return ptr;
        }
    }
    
    // Fall back to PMM for larger allocations
    // Need space for metadata + actual allocation
    size_t total_size = size + METADATA_SIZE;
    size_t aligned_size = align_to_page(total_size);
    uint32_t order = calculate_order(aligned_size / PAGE_SIZE);
    
    page_frame_t *page = alloc_pages(flags, order);
    if (page == nullptr) {
        atomic_fetch_add(&g_memory_stats.failed_allocs, 1);
        return nullptr;
    }
    
    // Store metadata at the beginning
    alloc_metadata_t *metadata = (alloc_metadata_t*)page->virtual_addr;
    metadata->size = size;
    metadata->order = order;
    metadata->page = page;
    metadata->magic = ALLOC_MAGIC;
    
    // Return pointer after metadata
    ptr = (char*)page->virtual_addr + METADATA_SIZE;
    
    if (flags & GFP_ZERO) {
        memset(ptr, 0, size);
    }
    
    atomic_fetch_add(&g_memory_stats.used_memory, aligned_size);
    atomic_fetch_sub(&g_memory_stats.free_memory, aligned_size);
    
    return ptr;
}

void kfree(void *ptr) {
    if (ptr == nullptr) {
        return;
    }
    
    atomic_fetch_add(&g_memory_stats.total_frees, 1);
    
    // Check if pointer is in per-CPU arena
    uint32_t cpu = get_current_cpu();
    percpu_arena_t *arena = &g_percpu_arenas[cpu];
    
    if (ptr >= arena->arena_base && 
        ptr < (char*)arena->arena_base + arena->arena_size) {
        // Return to cache if space available
        if (arena->cache_count < 16) {
            arena->cache[arena->cache_count++] = ptr;
        }
        atomic_fetch_add(&arena->free_count, 1);
        return;
    }
    
    // Bug Fix #3: Properly free PMM allocations
    // Get metadata stored before the allocation
    alloc_metadata_t *metadata = (alloc_metadata_t*)((char*)ptr - METADATA_SIZE);
    
    // Validate metadata
    if (metadata->magic != ALLOC_MAGIC) {
        printf("kfree: Invalid metadata magic, possible corruption at %p\n", ptr);
        return;
    }
    
    // Free through PMM using stored page frame and order
    if (metadata->page != nullptr) {
        size_t freed_size = PAGE_SIZE << metadata->order;
        free_pages(metadata->page, metadata->order);
        
        atomic_fetch_add(&g_memory_stats.free_memory, freed_size);
        atomic_fetch_sub(&g_memory_stats.used_memory, freed_size);
    }
}

void* kzalloc(size_t size, uint32_t flags) {
    return kmalloc(size, flags | GFP_ZERO);
}

void* krealloc(void *ptr, size_t new_size, uint32_t flags) {
    if (ptr == nullptr) {
        return kmalloc(new_size, flags);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return nullptr;
    }
    
    // Bug Fix #6: Handle per-CPU arena allocations
    // Check if pointer is from per-CPU arena (same logic as kfree)
    uint32_t cpu = get_current_cpu();
    percpu_arena_t *arena = &g_percpu_arenas[cpu];
    
    if (ptr >= arena->arena_base && 
        ptr < (char*)arena->arena_base + arena->arena_size) {
        // Arena allocation - has arena header
        // Bug Fix #1: Read old size from arena header to prevent buffer overrun
        arena_header_t *header = (arena_header_t*)((char*)ptr - ARENA_HEADER_SIZE);
        
        // Validate arena header
        if (header->magic != ARENA_MAGIC) {
            printf("krealloc: Invalid arena header magic at %p, using fallback\n", ptr);
            // Fallback: use conservative copy size for safety
            size_t copy_size = (new_size < 1024) ? new_size : 1024;
            void *new_ptr = kmalloc(new_size, flags);
            if (new_ptr != nullptr) {
                memcpy(new_ptr, ptr, copy_size);
            }
            return new_ptr;
        }
        
        size_t old_size = header->size;
        
        // Allocate new memory with kmalloc
        void *new_ptr = kmalloc(new_size, flags);
        if (new_ptr == nullptr) {
            return nullptr;
        }
        
        // Copy only the minimum of old and new size to prevent buffer overrun
        size_t copy_size = (old_size < new_size) ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
        
        // Free old allocation back to arena cache
        if (arena->cache_count < 16) {
            arena->cache[arena->cache_count++] = ptr;
        }
        atomic_fetch_add(&arena->free_count, 1);
        
        return new_ptr;
    }
    
    // Non-arena allocation - has metadata header
    // Bug Fix #4: Use tracked allocation size to prevent buffer overrun
    // Get old allocation size from metadata
    alloc_metadata_t *metadata = (alloc_metadata_t*)((char*)ptr - METADATA_SIZE);
    
    // Validate metadata
    if (metadata->magic != ALLOC_MAGIC) {
        printf("krealloc: Invalid metadata magic, possible corruption at %p\n", ptr);
        return nullptr;
    }
    
    size_t old_size = metadata->size;
    
    void *new_ptr = kmalloc(new_size, flags);
    if (new_ptr == nullptr) {
        return nullptr;
    }
    
    // Copy only the minimum of old and new size to prevent buffer overrun
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);
    
    return new_ptr;
}

void* kmalloc_node(size_t size, uint32_t node, uint32_t flags) {
    if (node >= g_numa_topology.num_nodes) {
        return nullptr;
    }
    
    // Allocate with NUMA locality preference
    void *ptr = kmalloc(size, flags | GFP_NUMA_LOCAL);
    
    if (ptr != nullptr) {
        if (node == numa_get_current_node()) {
            atomic_fetch_add(&g_memory_stats.numa_local_allocs, 1);
        } else {
            atomic_fetch_add(&g_memory_stats.numa_remote_allocs, 1);
        }
    }
    
    return ptr;
}

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

const memory_stats_t* memory_get_stats(void) {
    return &g_memory_stats;
}

void memory_print_stats(void) {
    printf("\n=== Memory Statistics ===\n");
    printf("Total Memory:     %llu MB\n", 
           atomic_load(&g_memory_stats.total_memory) / (1024 * 1024));
    printf("Used Memory:      %llu MB\n", 
           atomic_load(&g_memory_stats.used_memory) / (1024 * 1024));
    printf("Free Memory:      %llu MB\n", 
           atomic_load(&g_memory_stats.free_memory) / (1024 * 1024));
    printf("Cached Memory:    %llu MB\n", 
           atomic_load(&g_memory_stats.cached_memory) / (1024 * 1024));
    printf("\nAllocations:\n");
    printf("  Total:          %llu\n", atomic_load(&g_memory_stats.total_allocs));
    printf("  Frees:          %llu\n", atomic_load(&g_memory_stats.total_frees));
    printf("  Failed:         %llu\n", atomic_load(&g_memory_stats.failed_allocs));
    printf("\nPage Faults:\n");
    printf("  Total:          %llu\n", atomic_load(&g_memory_stats.page_faults));
    printf("  Major:          %llu\n", atomic_load(&g_memory_stats.major_faults));
    printf("  Minor:          %llu\n", atomic_load(&g_memory_stats.minor_faults));
    printf("\nHuge Pages:\n");
    printf("  Allocated:      %llu\n", atomic_load(&g_memory_stats.huge_pages_allocated));
    printf("  Freed:          %llu\n", atomic_load(&g_memory_stats.huge_pages_freed));
    printf("  Promotions:     %llu\n", atomic_load(&g_memory_stats.thp_promotions));
    printf("\nNUMA:\n");
    printf("  Local Allocs:   %llu\n", atomic_load(&g_memory_stats.numa_local_allocs));
    printf("  Remote Allocs:  %llu\n", atomic_load(&g_memory_stats.numa_remote_allocs));
    printf("  Migrations:     %llu\n", atomic_load(&g_memory_stats.numa_migrations));
    printf("========================\n\n");
}

void memory_reset_stats(void) {
    // Reset counters but preserve memory totals
    uint64_t total = atomic_load(&g_memory_stats.total_memory);
    uint64_t used = atomic_load(&g_memory_stats.used_memory);
    uint64_t free = atomic_load(&g_memory_stats.free_memory);
    
    memset(&g_memory_stats, 0, sizeof(memory_stats_t));
    
    atomic_store(&g_memory_stats.total_memory, total);
    atomic_store(&g_memory_stats.used_memory, used);
    atomic_store(&g_memory_stats.free_memory, free);
    
    printf("Memory: Statistics reset\n");
}

void memory_dump_state(void) {
    printf("\n=== Memory State Dump ===\n");
    
    memory_print_stats();
    
    printf("NUMA Topology:\n");
    for (uint32_t i = 0; i < g_numa_topology.num_nodes; i++) {
        numa_node_info_t *node = &g_numa_topology.nodes[i];
        printf("  Node %u: %llu MB total, %llu MB free\n",
               i, node->total_memory / (1024 * 1024),
               node->free_memory / (1024 * 1024));
    }
    
    printf("\nPer-CPU Arenas:\n");
    for (uint32_t cpu = 0; cpu < 4; cpu++) {
        percpu_arena_t *arena = &g_percpu_arenas[cpu];
        if (arena->arena_base != nullptr) {
            printf("  CPU %u: %zu/%zu bytes used, %llu allocs, %llu cache hits\n",
                   cpu, arena->arena_used, arena->arena_size,
                   atomic_load(&arena->alloc_count),
                   atomic_load(&arena->cache_hits));
        }
    }
    
    printf("========================\n\n");
}

bool memory_check_consistency(void) {
    // Basic consistency checks
    uint64_t total = atomic_load(&g_memory_stats.total_memory);
    uint64_t used = atomic_load(&g_memory_stats.used_memory);
    uint64_t free = atomic_load(&g_memory_stats.free_memory);
    
    if (used + free > total) {
        printf("Memory: Consistency check FAILED - used+free > total\n");
        return false;
    }
    
    printf("Memory: Consistency check passed\n");
    return true;
}

uint32_t memory_get_pressure(void) {
    uint64_t total = atomic_load(&g_memory_stats.total_memory);
    uint64_t free = atomic_load(&g_memory_stats.free_memory);
    
    if (total == 0) {
        return 0;
    }
    
    uint64_t used_percent = ((total - free) * 100) / total;
    
    if (used_percent < 50) {
        return 0;  // No pressure
    } else if (used_percent < 75) {
        return 25;  // Low pressure
    } else if (used_percent < 90) {
        return 50;  // Medium pressure
    } else if (used_percent < 95) {
        return 75;  // High pressure
    } else {
        return 100;  // Critical pressure
    }
}
