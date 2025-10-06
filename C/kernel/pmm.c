
/**
 * @file pmm.c
 * @brief Physical Memory Manager Implementation for BDI Kernel
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#include "pmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Global State
// ============================================================================

static pmm_state_t g_pmm_state = {0};

// ============================================================================
// Buddy Allocator Implementation
// ============================================================================

/**
 * @brief Get buddy page frame number
 */
static inline uint64_t get_buddy_pfn(uint64_t pfn, uint32_t order) {
    return pfn ^ (1ULL << order);
}

/**
 * @brief Check if two blocks are buddies
 */
static inline bool are_buddies(uint64_t pfn1, uint64_t pfn2, uint32_t order) {
    return get_buddy_pfn(pfn1, order) == pfn2;
}

/**
 * @brief Lock buddy allocator
 */
static inline void buddy_lock(buddy_allocator_t *buddy) {
    while (atomic_flag_test_and_set(&buddy->lock)) {
        // Spin wait
    }
}

/**
 * @brief Unlock buddy allocator
 */
static inline void buddy_unlock(buddy_allocator_t *buddy) {
    atomic_flag_clear(&buddy->lock);
}

int buddy_init(buddy_allocator_t *buddy, uint64_t base_pfn, uint64_t num_pages) {
    if (buddy == nullptr || num_pages == 0) {
        return -1;
    }
    
    memset(buddy, 0, sizeof(buddy_allocator_t));
    
    // Initialize free lists
    for (uint32_t order = 0; order < MAX_ORDER; order++) {
        buddy->free_lists[order] = nullptr;
        atomic_store(&buddy->free_count[order], 0);
    }
    
    atomic_store(&buddy->total_free, 0);
    atomic_flag_clear(&buddy->lock);
    
    // Add initial free blocks at maximum order
    uint64_t pfn = base_pfn;
    uint64_t remaining = num_pages;
    
    while (remaining > 0) {
        // Find largest order that fits
        uint32_t order = MAX_ORDER - 1;
        while (order > 0 && (1ULL << order) > remaining) {
            order--;
        }
        
        // Create free block
        buddy_block_t *block = malloc(sizeof(buddy_block_t));
        if (block == nullptr) {
            return -1;
        }
        
        block->order = order;
        block->pfn = pfn;
        block->next = buddy->free_lists[order];
        block->prev = nullptr;
        
        if (buddy->free_lists[order] != nullptr) {
            buddy->free_lists[order]->prev = block;
        }
        
        buddy->free_lists[order] = block;
        atomic_fetch_add(&buddy->free_count[order], 1);
        atomic_fetch_add(&buddy->total_free, 1ULL << order);
        
        pfn += (1ULL << order);
        remaining -= (1ULL << order);
    }
    
    printf("PMM: Buddy allocator initialized with %llu pages\n", num_pages);
    return 0;
}

uint64_t buddy_alloc(buddy_allocator_t *buddy, uint32_t order) {
    if (buddy == nullptr || order >= MAX_ORDER) {
        return 0;
    }
    
    buddy_lock(buddy);
    
    // Find smallest available order >= requested order
    uint32_t current_order = order;
    while (current_order < MAX_ORDER && buddy->free_lists[current_order] == nullptr) {
        current_order++;
    }
    
    if (current_order >= MAX_ORDER) {
        buddy_unlock(buddy);
        return 0;  // No memory available
    }
    
    // Remove block from free list
    buddy_block_t *block = buddy->free_lists[current_order];
    buddy->free_lists[current_order] = block->next;
    if (block->next != nullptr) {
        block->next->prev = nullptr;
    }
    
    atomic_fetch_sub(&buddy->free_count[current_order], 1);
    atomic_fetch_sub(&buddy->total_free, 1ULL << current_order);
    
    uint64_t pfn = block->pfn;
    
    // Split block if necessary
    while (current_order > order) {
        current_order--;
        atomic_fetch_add(&buddy->split_count, 1);
        
        // Create buddy block for upper half
        uint64_t buddy_pfn = pfn + (1ULL << current_order);
        buddy_block_t *buddy_block = malloc(sizeof(buddy_block_t));
        
        if (buddy_block != nullptr) {
            buddy_block->order = current_order;
            buddy_block->pfn = buddy_pfn;
            buddy_block->next = buddy->free_lists[current_order];
            buddy_block->prev = nullptr;
            
            if (buddy->free_lists[current_order] != nullptr) {
                buddy->free_lists[current_order]->prev = buddy_block;
            }
            
            buddy->free_lists[current_order] = buddy_block;
            atomic_fetch_add(&buddy->free_count[current_order], 1);
            atomic_fetch_add(&buddy->total_free, 1ULL << current_order);
        }
    }
    
    free(block);
    buddy_unlock(buddy);
    
    return pfn;
}

void buddy_free(buddy_allocator_t *buddy, uint64_t pfn, uint32_t order) {
    if (buddy == nullptr || order >= MAX_ORDER) {
        return;
    }
    
    buddy_lock(buddy);
    
    // Try to coalesce with buddy
    while (order < MAX_ORDER - 1) {
        uint64_t buddy_pfn = get_buddy_pfn(pfn, order);
        
        // Search for buddy in free list
        buddy_block_t *buddy_block = buddy->free_lists[order];
        buddy_block_t *prev = nullptr;
        bool found = false;
        
        while (buddy_block != nullptr) {
            if (buddy_block->pfn == buddy_pfn) {
                found = true;
                break;
            }
            prev = buddy_block;
            buddy_block = buddy_block->next;
        }
        
        if (!found) {
            break;  // Buddy not free, can't coalesce
        }
        
        // Remove buddy from free list
        if (prev != nullptr) {
            prev->next = buddy_block->next;
        } else {
            buddy->free_lists[order] = buddy_block->next;
        }
        
        if (buddy_block->next != nullptr) {
            buddy_block->next->prev = prev;
        }
        
        atomic_fetch_sub(&buddy->free_count[order], 1);
        atomic_fetch_sub(&buddy->total_free, 1ULL << order);
        free(buddy_block);
        
        // Coalesce
        pfn = (pfn < buddy_pfn) ? pfn : buddy_pfn;
        order++;
        atomic_fetch_add(&buddy->coalesce_count, 1);
    }
    
    // Add coalesced block to free list
    buddy_block_t *block = malloc(sizeof(buddy_block_t));
    if (block != nullptr) {
        block->order = order;
        block->pfn = pfn;
        block->next = buddy->free_lists[order];
        block->prev = nullptr;
        
        if (buddy->free_lists[order] != nullptr) {
            buddy->free_lists[order]->prev = block;
        }
        
        buddy->free_lists[order] = block;
        atomic_fetch_add(&buddy->free_count[order], 1);
        atomic_fetch_add(&buddy->total_free, 1ULL << order);
    }
    
    buddy_unlock(buddy);
}

uint64_t buddy_coalesce(buddy_allocator_t *buddy) {
    if (buddy == nullptr) {
        return 0;
    }
    
    uint64_t coalesced = 0;
    
    buddy_lock(buddy);
    
    // Try to coalesce blocks at each order
    for (uint32_t order = 0; order < MAX_ORDER - 1; order++) {
        buddy_block_t *block = buddy->free_lists[order];
        
        while (block != nullptr) {
            uint64_t buddy_pfn = get_buddy_pfn(block->pfn, order);
            
            // Search for buddy
            buddy_block_t *buddy_block = buddy->free_lists[order];
            while (buddy_block != nullptr && buddy_block->pfn != buddy_pfn) {
                buddy_block = buddy_block->next;
            }
            
            if (buddy_block != nullptr) {
                // Found buddy, coalesce
                uint64_t new_pfn = (block->pfn < buddy_pfn) ? block->pfn : buddy_pfn;
                
                // Remove both blocks
                // (Simplified - real implementation would properly unlink)
                
                // Add coalesced block to next order
                buddy_block_t *new_block = malloc(sizeof(buddy_block_t));
                if (new_block != nullptr) {
                    new_block->order = order + 1;
                    new_block->pfn = new_pfn;
                    new_block->next = buddy->free_lists[order + 1];
                    new_block->prev = nullptr;
                    
                    if (buddy->free_lists[order + 1] != nullptr) {
                        buddy->free_lists[order + 1]->prev = new_block;
                    }
                    
                    buddy->free_lists[order + 1] = new_block;
                    coalesced++;
                }
            }
            
            block = block->next;
        }
    }
    
    buddy_unlock(buddy);
    
    return coalesced;
}

// ============================================================================
// PMM Implementation
// ============================================================================

int pmm_init(void) {
    if (g_pmm_state.initialized) {
        return 0;
    }
    
    printf("PMM: Initializing physical memory manager...\n");
    
    // Simulate 4GB of physical memory
    g_pmm_state.total_pages = 1024 * 1024;  // 4GB / 4KB
    g_pmm_state.total_memory = g_pmm_state.total_pages * PAGE_SIZE;
    
    // Allocate page frame array
    g_pmm_state.page_frames = calloc(g_pmm_state.total_pages, sizeof(page_frame_t));
    if (g_pmm_state.page_frames == nullptr) {
        printf("PMM: Failed to allocate page frame array\n");
        return -1;
    }
    
    // Allocate bitmap
    g_pmm_state.allocation_bitmap = calloc(PMM_BITMAP_SIZE, 1);
    if (g_pmm_state.allocation_bitmap == nullptr) {
        free(g_pmm_state.page_frames);
        printf("PMM: Failed to allocate bitmap\n");
        return -1;
    }
    
    // Initialize page frames
    for (uint64_t i = 0; i < g_pmm_state.total_pages; i++) {
        page_frame_t *page = &g_pmm_state.page_frames[i];
        atomic_store(&page->flags, 0);
        atomic_store(&page->refcount, 0);
        page->next = nullptr;
        page->prev = nullptr;
        page->virtual_addr = nullptr;
        page->order = 0;
        page->numa_node = i % 2;  // Distribute across 2 NUMA nodes
        
        // Assign zones based on address
        if (i < (16 * 1024 * 1024 / PAGE_SIZE)) {
            page->zone = ZONE_DMA;
        } else if (i < (896 * 1024 * 1024 / PAGE_SIZE)) {
            page->zone = ZONE_NORMAL;
        } else {
            page->zone = ZONE_HIGHMEM;
        }
    }
    
    // Initialize zones
    const char *zone_names[] = {"DMA", "Normal", "HighMem", "Movable"};
    uint64_t zone_sizes[] = {
        16 * 1024 * 1024 / PAGE_SIZE,      // DMA: 16MB
        880 * 1024 * 1024 / PAGE_SIZE,     // Normal: 880MB
        3200 * 1024 * 1024 / PAGE_SIZE,    // HighMem: 3200MB
        0                                   // Movable: 0
    };
    
    uint64_t base_pfn = 0;
    for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
        zone_allocator_t *zone_alloc = &g_pmm_state.zones[i];
        zone_t *zone = &zone_alloc->zone;
        
        zone->name = zone_names[i];
        zone->base_pfn = base_pfn;
        zone->spanned_pages = zone_sizes[i];
        atomic_store(&zone->free_pages, zone_sizes[i]);
        
        // Set watermarks
        zone->watermark_min = zone_sizes[i] / 100;      // 1%
        zone->watermark_low = zone_sizes[i] / 50;       // 2%
        zone->watermark_high = zone_sizes[i] / 20;      // 5%
        
        atomic_flag_clear(&zone->lock);
        
        // Initialize buddy allocator for this zone
        if (zone_sizes[i] > 0) {
            if (buddy_init(&zone_alloc->buddy, base_pfn, zone_sizes[i]) != 0) {
                printf("PMM: Failed to initialize buddy allocator for zone %s\n", 
                       zone_names[i]);
                return -1;
            }
        }
        
        printf("PMM: Zone %s: %llu pages (%llu MB)\n",
               zone_names[i], zone_sizes[i], 
               (zone_sizes[i] * PAGE_SIZE) / (1024 * 1024));
        
        base_pfn += zone_sizes[i];
    }
    
    g_pmm_state.numa_node_count = 2;
    g_pmm_state.initialized = true;
    
    printf("PMM: Initialization complete. Total: %llu MB\n",
           g_pmm_state.total_memory / (1024 * 1024));
    
    return 0;
}

void pmm_shutdown(void) {
    if (!g_pmm_state.initialized) {
        return;
    }
    
    printf("PMM: Shutting down...\n");
    
    // Free buddy allocator blocks
    for (uint32_t zone_idx = 0; zone_idx < MAX_NR_ZONES; zone_idx++) {
        buddy_allocator_t *buddy = &g_pmm_state.zones[zone_idx].buddy;
        
        for (uint32_t order = 0; order < MAX_ORDER; order++) {
            buddy_block_t *block = buddy->free_lists[order];
            while (block != nullptr) {
                buddy_block_t *next = block->next;
                free(block);
                block = next;
            }
        }
    }
    
    free(g_pmm_state.page_frames);
    free(g_pmm_state.allocation_bitmap);
    
    g_pmm_state.initialized = false;
    
    printf("PMM: Shutdown complete\n");
}

page_frame_t* pmm_alloc_pages(uint32_t gfp_mask, uint32_t order) {
    if (!g_pmm_state.initialized || order >= MAX_ORDER) {
        return nullptr;
    }
    
    atomic_fetch_add(&g_pmm_state.total_allocs, 1);
    
    // Determine zone based on GFP flags
    memory_zone_t zone_type = ZONE_NORMAL;
    if (gfp_mask & GFP_DMA) {
        zone_type = ZONE_DMA;
    } else if (gfp_mask & GFP_HIGHMEM) {
        zone_type = ZONE_HIGHMEM;
    }
    
    zone_allocator_t *zone_alloc = &g_pmm_state.zones[zone_type];
    
    // Check watermarks
    if (!pmm_zone_watermark_ok(&zone_alloc->zone, order)) {
        if (!(gfp_mask & GFP_ATOMIC)) {
            // Try to reclaim memory
            printf("PMM: Low memory in zone %s, attempting reclamation\n",
                   zone_alloc->zone.name);
        }
    }
    
    // Allocate from buddy allocator
    uint64_t pfn = buddy_alloc(&zone_alloc->buddy, order);
    if (pfn == 0) {
        atomic_fetch_add(&zone_alloc->alloc_failure, 1);
        return nullptr;
    }
    
    atomic_fetch_add(&zone_alloc->alloc_success, 1);
    
    // Get page frame
    page_frame_t *page = pmm_pfn_to_page(pfn);
    if (page == nullptr) {
        buddy_free(&zone_alloc->buddy, pfn, order);
        return nullptr;
    }
    
    // Initialize page
    atomic_store(&page->refcount, 1);
    page->order = order;
    
    // Allocate virtual address
    size_t alloc_size = PAGE_SIZE << order;
    page->virtual_addr = aligned_alloc(PAGE_SIZE, alloc_size);
    
    if (page->virtual_addr == nullptr) {
        buddy_free(&zone_alloc->buddy, pfn, order);
        return nullptr;
    }
    
    // Zero memory if requested
    if (gfp_mask & GFP_ZERO) {
        memset(page->virtual_addr, 0, alloc_size);
    }
    
    atomic_fetch_sub(&zone_alloc->zone.free_pages, 1ULL << order);
    
    return page;
}

void pmm_free_pages(page_frame_t *page, uint32_t order) {
    if (page == nullptr || !g_pmm_state.initialized) {
        return;
    }
    
    atomic_fetch_add(&g_pmm_state.total_frees, 1);
    
    // Decrement reference count
    if (atomic_fetch_sub(&page->refcount, 1) > 1) {
        return;  // Still has references
    }
    
    // Free virtual address
    if (page->virtual_addr != nullptr) {
        free(page->virtual_addr);
        page->virtual_addr = nullptr;
    }
    
    // Get zone
    zone_allocator_t *zone_alloc = &g_pmm_state.zones[page->zone];
    
    // Return to buddy allocator
    uint64_t pfn = pmm_page_to_pfn(page);
    buddy_free(&zone_alloc->buddy, pfn, order);
    
    atomic_fetch_add(&zone_alloc->zone.free_pages, 1ULL << order);
    atomic_fetch_add(&zone_alloc->free_operations, 1);
}

page_frame_t* pmm_alloc_page(uint32_t gfp_mask) {
    return pmm_alloc_pages(gfp_mask, 0);
}

void pmm_free_page(page_frame_t *page) {
    pmm_free_pages(page, 0);
}

page_frame_t* pmm_alloc_pages_node(uint32_t node, uint32_t gfp_mask, uint32_t order) {
    if (node >= g_pmm_state.numa_node_count) {
        return nullptr;
    }
    
    // For now, just use regular allocation
    // Real implementation would prefer pages from specified node
    return pmm_alloc_pages(gfp_mask | GFP_NUMA_LOCAL, order);
}

page_frame_t* pmm_pfn_to_page(uint64_t pfn) {
    if (pfn >= g_pmm_state.total_pages) {
        return nullptr;
    }
    
    return &g_pmm_state.page_frames[pfn];
}

uint64_t pmm_page_to_pfn(const page_frame_t *page) {
    if (page == nullptr || page < g_pmm_state.page_frames ||
        page >= g_pmm_state.page_frames + g_pmm_state.total_pages) {
        return 0;
    }
    
    return page - g_pmm_state.page_frames;
}

bool pmm_zone_watermark_ok(const zone_t *zone, uint32_t order) {
    if (zone == nullptr) {
        return false;
    }
    
    uint64_t free = atomic_load(&zone->free_pages);
    uint64_t required = zone->watermark_low + (1ULL << order);
    
    return free >= required;
}

uint64_t pmm_defragment(zone_t *zone) {
    uint64_t total_coalesced = 0;
    
    if (zone != nullptr) {
        // Defragment specific zone
        for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
            if (&g_pmm_state.zones[i].zone == zone) {
                total_coalesced = buddy_coalesce(&g_pmm_state.zones[i].buddy);
                break;
            }
        }
    } else {
        // Defragment all zones
        for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
            total_coalesced += buddy_coalesce(&g_pmm_state.zones[i].buddy);
        }
    }
    
    printf("PMM: Defragmentation coalesced %llu blocks\n", total_coalesced);
    return total_coalesced;
}

uint32_t pmm_get_fragmentation(const zone_t *zone) {
    // Calculate fragmentation score based on free block distribution
    // Higher score = more fragmented
    
    if (zone != nullptr) {
        // Calculate for specific zone
        for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
            if (&g_pmm_state.zones[i].zone == zone) {
                buddy_allocator_t *buddy = &g_pmm_state.zones[i].buddy;
                
                uint64_t small_blocks = 0;
                uint64_t large_blocks = 0;
                
                for (uint32_t order = 0; order < MAX_ORDER / 2; order++) {
                    small_blocks += atomic_load(&buddy->free_count[order]);
                }
                
                for (uint32_t order = MAX_ORDER / 2; order < MAX_ORDER; order++) {
                    large_blocks += atomic_load(&buddy->free_count[order]);
                }
                
                if (small_blocks + large_blocks == 0) {
                    return 0;
                }
                
                return (uint32_t)((small_blocks * 100) / (small_blocks + large_blocks));
            }
        }
    }
    
    return 0;
}

int pmm_reserve_pages(uint64_t pfn, uint64_t count) {
    if (pfn + count > g_pmm_state.total_pages) {
        return -1;
    }
    
    for (uint64_t i = 0; i < count; i++) {
        page_frame_t *page = pmm_pfn_to_page(pfn + i);
        if (page != nullptr) {
            atomic_fetch_or(&page->flags, PG_RESERVED);
        }
    }
    
    printf("PMM: Reserved %llu pages starting at PFN %llu\n", count, pfn);
    return 0;
}

void pmm_get_stats(memory_stats_t *stats) {
    if (stats == nullptr) {
        return;
    }
    
    atomic_store(&stats->total_allocs, atomic_load(&g_pmm_state.total_allocs));
    atomic_store(&stats->total_frees, atomic_load(&g_pmm_state.total_frees));
}

void pmm_print_state(void) {
    printf("\n=== PMM State ===\n");
    printf("Total Pages: %llu (%llu MB)\n",
           g_pmm_state.total_pages,
           g_pmm_state.total_memory / (1024 * 1024));
    printf("Total Allocs: %llu\n", atomic_load(&g_pmm_state.total_allocs));
    printf("Total Frees: %llu\n", atomic_load(&g_pmm_state.total_frees));
    
    printf("\nZones:\n");
    for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
        zone_allocator_t *zone_alloc = &g_pmm_state.zones[i];
        zone_t *zone = &zone_alloc->zone;
        
        if (zone->spanned_pages == 0) {
            continue;
        }
        
        printf("  %s: %llu/%llu pages free, fragmentation: %u%%\n",
               zone->name,
               atomic_load(&zone->free_pages),
               zone->spanned_pages,
               pmm_get_fragmentation(zone));
    }
    printf("=================\n\n");
}

bool pmm_validate(void) {
    if (!g_pmm_state.initialized) {
        return false;
    }
    
    // Validate page frame array
    if (g_pmm_state.page_frames == nullptr) {
        printf("PMM: Validation failed - null page frame array\n");
        return false;
    }
    
    // Validate zones
    for (uint32_t i = 0; i < MAX_NR_ZONES; i++) {
        zone_t *zone = &g_pmm_state.zones[i].zone;
        
        if (zone->spanned_pages > 0) {
            uint64_t free = atomic_load(&zone->free_pages);
            if (free > zone->spanned_pages) {
                printf("PMM: Validation failed - zone %s has more free than total\n",
                       zone->name);
                return false;
            }
        }
    }
    
    printf("PMM: Validation passed\n");
    return true;
}
