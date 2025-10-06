
/**
 * @file pmm.h
 * @brief Physical Memory Manager for BDI Kernel
 * @details Implements buddy allocator, page frame management, and physical memory tracking.
 *          Provides NUMA-aware allocation and huge page support.
 * 
 * Phase 1: Memory & HAM Readiness
 * - Buddy allocator (orders 0-11)
 * - Page frame tracking
 * - NUMA-aware allocation
 * - Memory zone management
 * - Fragmentation tracking
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#ifndef BDI_KERNEL_PMM_H
#define BDI_KERNEL_PMM_H

#include "../c23_compat.h"
#include "memory.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

// ============================================================================
// PMM Configuration
// ============================================================================

#define PMM_MAX_PAGES           (1024 * 1024)  // 4GB worth of 4KB pages
#define PMM_BITMAP_SIZE         (PMM_MAX_PAGES / 8)
#define PMM_MIN_ALLOC_ORDER     0
#define PMM_MAX_ALLOC_ORDER     11

// ============================================================================
// Buddy Allocator Structures
// ============================================================================

/**
 * @brief Buddy allocator free block
 */
typedef struct buddy_block {
    struct buddy_block *next;
    struct buddy_block *prev;
    uint32_t order;
    uint32_t pfn;                       // Page frame number
} buddy_block_t;

/**
 * @brief Buddy allocator state
 */
typedef struct {
    buddy_block_t *free_lists[MAX_ORDER];
    atomic_uint_fast64_t free_count[MAX_ORDER];
    atomic_uint_fast64_t total_free;
    atomic_flag lock;
    
    // Fragmentation tracking
    uint32_t fragmentation_score;
    uint64_t last_defrag_time;
    atomic_uint_fast64_t coalesce_count;
    atomic_uint_fast64_t split_count;
} buddy_allocator_t;

/**
 * @brief Zone allocator
 */
typedef struct {
    zone_t zone;
    buddy_allocator_t buddy;
    
    // Zone-specific statistics
    atomic_uint_fast64_t alloc_attempts;
    atomic_uint_fast64_t alloc_success;
    atomic_uint_fast64_t alloc_failure;
    atomic_uint_fast64_t free_operations;
} zone_allocator_t;

/**
 * @brief PMM global state
 */
typedef struct {
    zone_allocator_t zones[MAX_NR_ZONES];
    page_frame_t *page_frames;
    uint64_t total_pages;
    uint64_t total_memory;
    
    // Bitmap for page allocation tracking
    uint8_t *allocation_bitmap;
    
    // NUMA-aware allocation
    uint32_t numa_node_count;
    uint64_t numa_pages[MAX_NUMA_NODES];
    
    // Statistics
    atomic_uint_fast64_t total_allocs;
    atomic_uint_fast64_t total_frees;
    atomic_uint_fast64_t oom_count;
    
    bool initialized;
} pmm_state_t;

// ============================================================================
// PMM Interface
// ============================================================================

/**
 * @brief Initialize physical memory manager
 * @return 0 on success, negative error code on failure
 */
NODISCARD int pmm_init(void);

/**
 * @brief Shutdown physical memory manager
 */
void pmm_shutdown(void);

/**
 * @brief Allocate physical pages
 * @param gfp_mask GFP allocation flags
 * @param order Allocation order (2^order pages)
 * @return Pointer to page frame or nullptr on failure
 */
NODISCARD page_frame_t* pmm_alloc_pages(uint32_t gfp_mask, uint32_t order);

/**
 * @brief Free physical pages
 * @param page Page frame pointer
 * @param order Allocation order
 */
void pmm_free_pages(page_frame_t *page, uint32_t order);

/**
 * @brief Allocate single page
 * @param gfp_mask GFP allocation flags
 * @return Pointer to page frame or nullptr on failure
 */
NODISCARD page_frame_t* pmm_alloc_page(uint32_t gfp_mask);

/**
 * @brief Free single page
 * @param page Page frame pointer
 */
void pmm_free_page(page_frame_t *page);

/**
 * @brief Allocate pages from specific NUMA node
 * @param node NUMA node ID
 * @param gfp_mask GFP allocation flags
 * @param order Allocation order
 * @return Pointer to page frame or nullptr on failure
 */
NODISCARD page_frame_t* pmm_alloc_pages_node(uint32_t node, uint32_t gfp_mask, uint32_t order);

/**
 * @brief Get page frame from physical address
 * @param paddr Physical address
 * @return Pointer to page frame or nullptr if invalid
 */
NODISCARD page_frame_t* pmm_pfn_to_page(uint64_t pfn);

/**
 * @brief Get physical address from page frame
 * @param page Page frame pointer
 * @return Physical address
 */
NODISCARD uint64_t pmm_page_to_pfn(const page_frame_t *page);

/**
 * @brief Check if zone can satisfy allocation
 * @param zone Zone to check
 * @param order Allocation order
 * @return true if allocation possible, false otherwise
 */
NODISCARD bool pmm_zone_watermark_ok(const zone_t *zone, uint32_t order);

/**
 * @brief Defragment memory
 * @param zone Zone to defragment (nullptr for all zones)
 * @return Number of pages coalesced
 */
NODISCARD uint64_t pmm_defragment(zone_t *zone);

/**
 * @brief Get fragmentation score
 * @param zone Zone to check (nullptr for overall)
 * @return Fragmentation score (0-100, higher = more fragmented)
 */
NODISCARD uint32_t pmm_get_fragmentation(const zone_t *zone);

/**
 * @brief Reserve pages for system use
 * @param pfn Starting page frame number
 * @param count Number of pages to reserve
 * @return 0 on success, negative error code on failure
 */
NODISCARD int pmm_reserve_pages(uint64_t pfn, uint64_t count);

/**
 * @brief Get PMM statistics
 * @param stats Output statistics structure
 */
void pmm_get_stats(memory_stats_t *stats);

/**
 * @brief Print PMM state
 */
void pmm_print_state(void);

/**
 * @brief Validate PMM consistency
 * @return true if consistent, false if corruption detected
 */
NODISCARD bool pmm_validate(void);

// ============================================================================
// Buddy Allocator Internal Interface
// ============================================================================

/**
 * @brief Initialize buddy allocator
 * @param buddy Buddy allocator structure
 * @param base_pfn Base page frame number
 * @param num_pages Number of pages
 * @return 0 on success, negative error code on failure
 */
NODISCARD int buddy_init(buddy_allocator_t *buddy, uint64_t base_pfn, uint64_t num_pages);

/**
 * @brief Allocate from buddy allocator
 * @param buddy Buddy allocator structure
 * @param order Allocation order
 * @return Page frame number or 0 on failure
 */
NODISCARD uint64_t buddy_alloc(buddy_allocator_t *buddy, uint32_t order);

/**
 * @brief Free to buddy allocator
 * @param buddy Buddy allocator structure
 * @param pfn Page frame number
 * @param order Allocation order
 */
void buddy_free(buddy_allocator_t *buddy, uint64_t pfn, uint32_t order);

/**
 * @brief Coalesce free blocks in buddy allocator
 * @param buddy Buddy allocator structure
 * @return Number of blocks coalesced
 */
NODISCARD uint64_t buddy_coalesce(buddy_allocator_t *buddy);

// ============================================================================
// Compile-time Assertions
// ============================================================================

static_assert(PMM_MAX_ALLOC_ORDER <= MAX_ORDER, "PMM max order exceeds system max");
static_assert(sizeof(buddy_block_t) <= 32, "Buddy block too large");

#endif // BDI_KERNEL_PMM_H
