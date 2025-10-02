
/**
 * @file shared_arena.h
 * @brief Shared memory arena allocator for zero-copy IPC
 * 
 * Provides fast allocation from shared memory regions.
 * Uses segregated free lists for different size classes.
 * DMA-aligned allocations for hardware compatibility.
 */

#ifndef PHASE1_SHARED_ARENA_H
#define PHASE1_SHARED_ARENA_H

#include "arena_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct shared_arena shared_arena_t;
typedef struct free_block free_block_t;

/**
 * @brief Free block structure
 */
struct free_block {
    size_t size;
    free_block_t* next;
};

/**
 * @brief Shared memory arena structure
 */
struct shared_arena {
    // Arena metadata
    void* base_address;
    size_t total_size;
    size_t used_size;
    
    // Free lists (one per size class)
    free_block_t* free_lists[ARENA_NUM_SIZE_CLASSES];
    
    // Large block free list (> max size class)
    free_block_t* large_blocks;
    
    // Allocation bitmap (for fast lookup)
    uint64_t* allocation_bitmap;
    size_t bitmap_size;
    
    // Statistics
    arena_stats_t stats;
    
    // MMM integration (placeholder for now)
    void* mmm_handle;
};

/**
 * @brief Create a new shared arena
 * 
 * @param size Total arena size in bytes
 * @return Pointer to arena, or NULL on failure
 */
shared_arena_t* shared_arena_create(size_t size);

/**
 * @brief Destroy shared arena
 * 
 * @param arena Arena to destroy
 */
void shared_arena_destroy(shared_arena_t* arena);

/**
 * @brief Allocate memory from arena
 * 
 * @param arena Arena
 * @param size Size in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void* shared_arena_alloc(shared_arena_t* arena, size_t size);

/**
 * @brief Allocate DMA-aligned memory from arena
 * 
 * @param arena Arena
 * @param size Size in bytes
 * @return Pointer to allocated memory (4KB aligned), or NULL on failure
 */
void* shared_arena_alloc_dma(shared_arena_t* arena, size_t size);

/**
 * @brief Free memory back to arena
 * 
 * @param arena Arena
 * @param ptr Pointer to free
 * @param size Size of allocation (must match original allocation)
 */
void shared_arena_free(shared_arena_t* arena, void* ptr, size_t size);

/**
 * @brief Get arena statistics
 * 
 * @param arena Arena
 * @param stats Output parameter for statistics
 */
void shared_arena_get_stats(const shared_arena_t* arena, arena_stats_t* stats);

/**
 * @brief Reset arena statistics
 * 
 * @param arena Arena
 */
void shared_arena_reset_stats(shared_arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_SHARED_ARENA_H
