
/**
 * @file memory.h
 * @brief Core Memory Management Interface for BDI Kernel
 * @details Provides unified memory management interface integrating PMM, VMM, and HAM.
 *          Implements NUMA-aware allocation, per-CPU arenas, huge page support, and
 *          production-ready allocator entry points with comprehensive statistics.
 * 
 * Phase 1: Memory & HAM Readiness
 * - NUMA discovery and validation
 * - Per-CPU lock-free arenas
 * - Huge page support (2MB, 1GB)
 * - Production allocator hardening
 * - Fault handling and reclamation
 * - HAM lifecycle integration
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#ifndef BDI_KERNEL_MEMORY_H
#define BDI_KERNEL_MEMORY_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

// ============================================================================
// Memory Configuration Constants
// ============================================================================

#define PAGE_SIZE           4096UL
#define PAGE_SHIFT          12
#define HUGE_PAGE_2MB       (2UL * 1024 * 1024)
#define HUGE_PAGE_1GB       (1UL * 1024 * 1024 * 1024)
#define MAX_ORDER           11
#define MAX_NUMA_NODES      8
#define MAX_CPUS            256
#define CACHE_LINE_SIZE     64

// GFP (Get Free Pages) flags
#define GFP_KERNEL          0x01    // Normal kernel allocation
#define GFP_ATOMIC          0x02    // Cannot sleep, for interrupt context
#define GFP_DMA             0x04    // DMA-capable memory
#define GFP_ZERO            0x08    // Zero-initialized memory
#define GFP_NOWAIT          0x10    // Don't wait for memory
#define GFP_HIGHMEM         0x20    // High memory zone
#define GFP_NUMA_LOCAL      0x40    // Prefer local NUMA node

// Memory zones
typedef enum {
    ZONE_DMA,           // 0-16MB for ISA devices
    ZONE_NORMAL,        // 16MB-896MB directly mapped
    ZONE_HIGHMEM,       // >896MB requires temporary mapping
    ZONE_MOVABLE,       // For memory hotplug
    MAX_NR_ZONES
} memory_zone_t;

// Page flags
#define PG_LOCKED           (1UL << 0)
#define PG_REFERENCED       (1UL << 1)
#define PG_DIRTY            (1UL << 2)
#define PG_LRU              (1UL << 3)
#define PG_ACTIVE           (1UL << 4)
#define PG_SLAB             (1UL << 5)
#define PG_RESERVED         (1UL << 6)
#define PG_PRIVATE          (1UL << 7)
#define PG_WRITEBACK        (1UL << 8)
#define PG_HEAD             (1UL << 9)
#define PG_TAIL             (1UL << 10)
#define PG_COMPOUND         (1UL << 11)
#define PG_HUGE             (1UL << 12)

// ============================================================================
// NUMA Topology Structures
// ============================================================================

/**
 * @brief NUMA node distance matrix entry
 * @details Represents relative memory access latency between NUMA nodes
 */
typedef struct {
    uint8_t distance;       // Relative distance (10 = local, >10 = remote)
    uint8_t bandwidth;      // Relative bandwidth (255 = max)
} numa_distance_t;

/**
 * @brief NUMA node information
 * @details Complete topology and statistics for a NUMA node
 */
typedef struct {
    uint32_t node_id;
    uint64_t total_memory;
    uint64_t free_memory;
    uint64_t cached_memory;
    uint32_t cpu_mask[MAX_CPUS / 32];  // Bitmap of CPUs in this node
    numa_distance_t distances[MAX_NUMA_NODES];
    
    // Statistics
    atomic_uint_fast64_t alloc_count;
    atomic_uint_fast64_t free_count;
    atomic_uint_fast64_t remote_alloc_count;
    atomic_uint_fast64_t migration_count;
} numa_node_info_t;

/**
 * @brief Global NUMA topology
 */
typedef struct {
    uint32_t num_nodes;
    numa_node_info_t nodes[MAX_NUMA_NODES];
    bool initialized;
    
    // Validation status
    bool topology_valid;
    bool distances_valid;
    uint64_t discovery_timestamp;
} numa_topology_t;

// ============================================================================
// Page Frame Structures
// ============================================================================

/**
 * @brief Page frame descriptor
 * @details Represents a single physical page frame with metadata
 */
typedef struct page_frame {
    atomic_uint_fast64_t flags;         // Page state flags
    atomic_int refcount;                // Reference counter
    struct page_frame *next;            // Free list linkage
    struct page_frame *prev;            // LRU list linkage
    void *virtual_addr;                 // Virtual address mapping
    uint8_t order;                      // Buddy system order
    uint8_t zone;                       // Memory zone
    uint8_t numa_node;                  // NUMA node ID
    uint8_t reserved;                   // Padding
    
    // Statistics
    uint64_t last_access_time;
    uint32_t access_count;
    uint32_t fault_count;
} page_frame_t;

/**
 * @brief Free area for buddy allocator
 */
typedef struct {
    page_frame_t *free_list;
    atomic_uint_fast64_t nr_free;
} free_area_t;

/**
 * @brief Memory zone descriptor
 */
typedef struct {
    const char *name;
    uint64_t base_pfn;                  // Base page frame number
    uint64_t spanned_pages;             // Total pages in zone
    atomic_uint_fast64_t free_pages;    // Currently free pages
    free_area_t free_area[MAX_ORDER];   // Buddy allocator free lists
    
    // Watermarks for memory pressure
    uint64_t watermark_min;
    uint64_t watermark_low;
    uint64_t watermark_high;
    
    // Statistics
    atomic_uint_fast64_t alloc_success;
    atomic_uint_fast64_t alloc_failure;
    atomic_uint_fast64_t reclaim_count;
    
    // Lock for zone operations
    atomic_flag lock;
} zone_t;

// ============================================================================
// Per-CPU Arena Structures
// ============================================================================

/**
 * @brief Per-CPU memory arena for lock-free allocation
 * @details Each CPU maintains its own arena to avoid lock contention
 */
typedef struct {
    uint32_t cpu_id;
    uint32_t numa_node;
    
    // Fast path cache
    void *cache[16];                    // Small object cache
    size_t cache_sizes[16];             // Sizes of cached allocations (for safe krealloc)
    uint8_t cache_count;
    
    // Arena memory pool
    void *arena_base;
    size_t arena_size;
    size_t arena_used;
    
    // Statistics
    atomic_uint_fast64_t alloc_count;
    atomic_uint_fast64_t free_count;
    atomic_uint_fast64_t cache_hits;
    atomic_uint_fast64_t cache_misses;
    atomic_uint_fast64_t slow_path_count;
    
    // Padding to cache line
    uint8_t padding[CACHE_LINE_SIZE - 
                    (sizeof(uint32_t) * 2 + 
                     sizeof(void*) * 17 + 
                     sizeof(size_t) * 17 +
                     sizeof(uint8_t) + 
                     sizeof(size_t) * 2 + 
                     sizeof(atomic_uint_fast64_t) * 5) % CACHE_LINE_SIZE];
} NODISCARD percpu_arena_t;

static_assert(sizeof(percpu_arena_t) % CACHE_LINE_SIZE == 0, 
              "Per-CPU arena must be cache-line aligned");

// ============================================================================
// Huge Page Structures
// ============================================================================

/**
 * @brief Huge page descriptor
 */
typedef struct {
    void *base_addr;
    size_t size;                        // 2MB or 1GB
    uint8_t numa_node;
    bool transparent;                   // Transparent huge page
    atomic_int refcount;
    
    // Statistics
    uint64_t allocation_time;
    uint32_t split_count;               // Times split into smaller pages
    uint32_t access_count;
} huge_page_t;

/**
 * @brief Huge page pool
 */
typedef struct {
    huge_page_t *pages_2mb;
    huge_page_t *pages_1gb;
    atomic_uint_fast32_t count_2mb;
    atomic_uint_fast32_t count_1gb;
    atomic_uint_fast32_t reserved_2mb;
    atomic_uint_fast32_t reserved_1gb;
    
    // Statistics
    atomic_uint_fast64_t alloc_success_2mb;
    atomic_uint_fast64_t alloc_success_1gb;
    atomic_uint_fast64_t alloc_failure_2mb;
    atomic_uint_fast64_t alloc_failure_1gb;
    atomic_uint_fast64_t promotion_count;
    atomic_uint_fast64_t demotion_count;
} huge_page_pool_t;

// ============================================================================
// Memory Statistics
// ============================================================================

/**
 * @brief Global memory statistics
 */
typedef struct {
    // Overall statistics
    atomic_uint_fast64_t total_memory;
    atomic_uint_fast64_t used_memory;
    atomic_uint_fast64_t free_memory;
    atomic_uint_fast64_t cached_memory;
    
    // Allocation statistics
    atomic_uint_fast64_t total_allocs;
    atomic_uint_fast64_t total_frees;
    atomic_uint_fast64_t failed_allocs;
    atomic_uint_fast64_t oom_kills;
    
    // Page fault statistics
    atomic_uint_fast64_t page_faults;
    atomic_uint_fast64_t major_faults;
    atomic_uint_fast64_t minor_faults;
    
    // Reclamation statistics
    atomic_uint_fast64_t pages_reclaimed;
    atomic_uint_fast64_t pages_scanned;
    atomic_uint_fast64_t swap_out;
    atomic_uint_fast64_t swap_in;
    
    // Huge page statistics
    atomic_uint_fast64_t huge_pages_allocated;
    atomic_uint_fast64_t huge_pages_freed;
    atomic_uint_fast64_t thp_promotions;
    atomic_uint_fast64_t thp_demotions;
    
    // NUMA statistics
    atomic_uint_fast64_t numa_local_allocs;
    atomic_uint_fast64_t numa_remote_allocs;
    atomic_uint_fast64_t numa_migrations;
} memory_stats_t;

// ============================================================================
// Memory Allocator Interface
// ============================================================================

/**
 * @brief Initialize memory subsystem
 * @return 0 on success, negative error code on failure
 */
NODISCARD int memory_init(void);

/**
 * @brief Shutdown memory subsystem
 */
void memory_shutdown(void);

/**
 * @brief Allocate memory with specified flags
 * @param size Size in bytes
 * @param flags GFP flags
 * @return Pointer to allocated memory or nullptr on failure
 */
NODISCARD void* kmalloc(size_t size, uint32_t flags);

/**
 * @brief Free previously allocated memory
 * @param ptr Pointer to memory
 */
void kfree(void *ptr);

/**
 * @brief Allocate zeroed memory
 * @param size Size in bytes
 * @param flags GFP flags
 * @return Pointer to zeroed memory or nullptr on failure
 */
NODISCARD void* kzalloc(size_t size, uint32_t flags);

/**
 * @brief Reallocate memory
 * @param ptr Original pointer
 * @param new_size New size in bytes
 * @param flags GFP flags
 * @return Pointer to reallocated memory or nullptr on failure
 */
NODISCARD void* krealloc(void *ptr, size_t new_size, uint32_t flags);

/**
 * @brief Allocate NUMA-local memory
 * @param size Size in bytes
 * @param node NUMA node ID
 * @param flags GFP flags
 * @return Pointer to allocated memory or nullptr on failure
 */
NODISCARD void* kmalloc_node(size_t size, uint32_t node, uint32_t flags);

// ============================================================================
// Page Allocator Interface
// ============================================================================

/**
 * @brief Allocate pages
 * @param gfp_mask GFP flags
 * @param order Allocation order (2^order pages)
 * @return Pointer to page frame or nullptr on failure
 */
NODISCARD page_frame_t* alloc_pages(uint32_t gfp_mask, uint32_t order);

/**
 * @brief Free pages
 * @param page Page frame pointer
 * @param order Allocation order
 */
void free_pages(page_frame_t *page, uint32_t order);

/**
 * @brief Get single free page
 * @param gfp_mask GFP flags
 * @return Pointer to page frame or nullptr on failure
 */
NODISCARD page_frame_t* get_free_page(uint32_t gfp_mask);

// ============================================================================
// Huge Page Interface
// ============================================================================

/**
 * @brief Allocate huge page
 * @param size Huge page size (HUGE_PAGE_2MB or HUGE_PAGE_1GB)
 * @param numa_node NUMA node preference
 * @return Pointer to huge page or nullptr on failure
 */
NODISCARD huge_page_t* alloc_huge_page(size_t size, uint32_t numa_node);

/**
 * @brief Free huge page
 * @param page Huge page pointer
 */
void free_huge_page(huge_page_t *page);

/**
 * @brief Try to promote pages to huge page
 * @param addr Base address
 * @param size Size to promote
 * @return true if promoted, false otherwise
 */
bool try_promote_huge_page(void *addr, size_t size);

// ============================================================================
// NUMA Interface
// ============================================================================

/**
 * @brief Initialize NUMA topology
 * @return 0 on success, negative error code on failure
 */
NODISCARD int numa_init(void);

/**
 * @brief Get NUMA topology
 * @return Pointer to NUMA topology structure
 */
NODISCARD const numa_topology_t* numa_get_topology(void);

/**
 * @brief Validate NUMA configuration
 * @return true if valid, false otherwise
 */
NODISCARD bool numa_validate_topology(void);

/**
 * @brief Get current CPU's NUMA node
 * @return NUMA node ID
 */
NODISCARD uint32_t numa_get_current_node(void);

/**
 * @brief Get distance between NUMA nodes
 * @param from Source node
 * @param to Destination node
 * @return Distance value
 */
NODISCARD uint8_t numa_get_distance(uint32_t from, uint32_t to);

// ============================================================================
// Memory Statistics Interface
// ============================================================================

/**
 * @brief Get global memory statistics
 * @return Pointer to statistics structure
 */
NODISCARD const memory_stats_t* memory_get_stats(void);

/**
 * @brief Print memory statistics
 */
void memory_print_stats(void);

/**
 * @brief Reset memory statistics
 */
void memory_reset_stats(void);

// ============================================================================
// Diagnostic Interface
// ============================================================================

/**
 * @brief Dump memory state for debugging
 */
void memory_dump_state(void);

/**
 * @brief Check memory consistency
 * @return true if consistent, false if corruption detected
 */
NODISCARD bool memory_check_consistency(void);

/**
 * @brief Get memory pressure level
 * @return Pressure level (0=none, 100=critical)
 */
NODISCARD uint32_t memory_get_pressure(void);

// ============================================================================
// Compile-time Assertions
// ============================================================================

static_assert(PAGE_SIZE == 4096, "Page size must be 4KB");
static_assert(MAX_ORDER <= 11, "Maximum order must not exceed 11");
static_assert(sizeof(page_frame_t) <= 128, "Page frame descriptor too large");

#endif // BDI_KERNEL_MEMORY_H
