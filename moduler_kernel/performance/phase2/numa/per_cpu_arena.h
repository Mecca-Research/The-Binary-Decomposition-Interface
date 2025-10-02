
/**
 * @file per_cpu_arena.h
 * @brief Per-CPU NUMA-aware arena allocator
 * 
 * Each CPU owns a local arena allocated from its NUMA node.
 * Lock-free within single CPU context.
 * Integrates with Phase 1's shared_arena.
 */

#ifndef PHASE2_PER_CPU_ARENA_H
#define PHASE2_PER_CPU_ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "numa_topology.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default per-CPU arena size (4MB)
#define PER_CPU_ARENA_DEFAULT_SIZE (4 * 1024 * 1024)

// Size classes (16B to 4KB)
#define PER_CPU_ARENA_MIN_SIZE 16
#define PER_CPU_ARENA_MAX_SIZE 4096
#define PER_CPU_ARENA_NUM_SIZE_CLASSES 9

// Alignment
#define PER_CPU_ARENA_ALIGNMENT 16

/**
 * @brief Per-CPU arena statistics
 */
typedef struct {
    uint64_t total_allocs;
    uint64_t total_frees;
    uint64_t failed_allocs;
    uint64_t bytes_allocated;
    uint64_t bytes_freed;
    uint64_t current_usage;
    uint64_t peak_usage;
    uint64_t local_allocs;      // Allocations from local NUMA node
    uint64_t remote_allocs;     // Allocations from remote NUMA node
} per_cpu_arena_stats_t;

/**
 * @brief Free block in arena
 */
typedef struct free_block {
    size_t size;
    struct free_block* next;
} free_block_t;

/**
 * @brief Per-CPU arena structure
 */
typedef struct {
    uint32_t cpu_id;
    uint32_t numa_node;
    
    // Arena memory
    void* base_address;
    size_t total_size;
    size_t used_size;
    
    // Free lists (one per size class)
    free_block_t* free_lists[PER_CPU_ARENA_NUM_SIZE_CLASSES];
    
    // Large block free list (> max size class)
    free_block_t* large_blocks;
    
    // Statistics
    per_cpu_arena_stats_t stats;
    
    // Initialized flag
    bool initialized;
} per_cpu_arena_t;

/**
 * @brief Initialize per-CPU arena system
 * 
 * Creates arenas for all CPUs, allocating from local NUMA nodes.
 * 
 * @return 0 on success, -1 on failure
 */
int per_cpu_arena_init(void);

/**
 * @brief Initialize specific CPU arena
 * 
 * @param cpu_id CPU ID
 * @param size Arena size (0 = default)
 * @return 0 on success, -1 on failure
 */
int per_cpu_arena_init_cpu(uint32_t cpu_id, size_t size);

/**
 * @brief Allocate memory from current CPU's arena
 * 
 * Lock-free allocation from local arena.
 * Falls back to remote allocation if local arena is full.
 * 
 * @param size Size in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void* per_cpu_arena_alloc(size_t size);

/**
 * @brief Allocate memory from specific CPU's arena
 * 
 * @param cpu_id CPU ID
 * @param size Size in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void* per_cpu_arena_alloc_cpu(uint32_t cpu_id, size_t size);

/**
 * @brief Allocate aligned memory from current CPU's arena
 * 
 * Allocates memory with specified alignment. The returned pointer is guaranteed
 * to be aligned to the specified boundary. Must be freed with 
 * per_cpu_arena_free_aligned(), NOT per_cpu_arena_free().
 * 
 * Implementation: Stores original pointer and size metadata before the aligned
 * address to enable proper freeing without memory corruption.
 * 
 * @param size Size in bytes
 * @param alignment Alignment (must be power of 2)
 * @return Pointer to aligned memory, or NULL on failure
 */
void* per_cpu_arena_alloc_aligned(size_t size, size_t alignment);

/**
 * @brief Free aligned memory back to arena
 * 
 * Frees memory allocated with per_cpu_arena_alloc_aligned(). This function
 * retrieves the original pointer and size stored during allocation and
 * properly frees the underlying allocation.
 * 
 * WARNING: Only use this for pointers returned by per_cpu_arena_alloc_aligned().
 * Using this with regular allocations or using per_cpu_arena_free() with
 * aligned allocations will cause memory corruption.
 * 
 * @param ptr Pointer to free (must be from per_cpu_arena_alloc_aligned)
 */
void per_cpu_arena_free_aligned(void* ptr);

/**
 * @brief Free memory back to arena
 * 
 * Frees memory allocated with per_cpu_arena_alloc() or per_cpu_arena_alloc_cpu().
 * Do NOT use this for aligned allocations - use per_cpu_arena_free_aligned() instead.
 * 
 * @param ptr Pointer to free
 * @param size Size of allocation (must match original)
 */
void per_cpu_arena_free(void* ptr, size_t size);

/**
 * @brief Free memory back to specific CPU's arena
 * 
 * @param cpu_id CPU ID
 * @param ptr Pointer to free
 * @param size Size of allocation
 */
void per_cpu_arena_free_cpu(uint32_t cpu_id, void* ptr, size_t size);

/**
 * @brief Get arena for current CPU
 * 
 * @return Pointer to arena, or NULL if not initialized
 */
per_cpu_arena_t* per_cpu_arena_get_current(void);

/**
 * @brief Get arena for specific CPU
 * 
 * @param cpu_id CPU ID
 * @return Pointer to arena, or NULL if invalid
 */
per_cpu_arena_t* per_cpu_arena_get(uint32_t cpu_id);

/**
 * @brief Get statistics for current CPU's arena
 * 
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int per_cpu_arena_get_stats(per_cpu_arena_stats_t* stats);

/**
 * @brief Get statistics for specific CPU's arena
 * 
 * @param cpu_id CPU ID
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int per_cpu_arena_get_stats_cpu(uint32_t cpu_id, per_cpu_arena_stats_t* stats);

/**
 * @brief Reset arena statistics
 * 
 * @param cpu_id CPU ID
 */
void per_cpu_arena_reset_stats(uint32_t cpu_id);

/**
 * @brief Print arena statistics
 * 
 * @param cpu_id CPU ID
 */
void per_cpu_arena_print_stats(uint32_t cpu_id);

/**
 * @brief Destroy per-CPU arena system
 */
void per_cpu_arena_destroy(void);

/**
 * @brief Destroy specific CPU arena
 * 
 * @param cpu_id CPU ID
 */
void per_cpu_arena_destroy_cpu(uint32_t cpu_id);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_PER_CPU_ARENA_H
