
/**
 * @file per_cpu_arena.c
 * @brief Per-CPU NUMA-aware arena allocator implementation
 */

#define _GNU_SOURCE
#include "per_cpu_arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>
#include <errno.h>

// Global per-CPU arenas
static per_cpu_arena_t* g_arenas[NUMA_MAX_CPUS] = {0};
static bool g_initialized = false;

/**
 * @brief Get size class index for allocation size
 */
static inline int get_size_class(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    if (size <= 4096) return 8;
    return -1;  // Large allocation
}

/**
 * @brief Get actual size for size class
 */
static inline size_t get_class_size(int class_idx) {
    static const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    return sizes[class_idx];
}

/**
 * @brief Allocate memory from NUMA node
 */
static void* numa_alloc(size_t size, int node) {
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    
    // Try to bind to NUMA node (best effort)
    // In real kernel, would use mbind() or set_mempolicy()
    // For userspace, this is a hint
    
    return ptr;
}

/**
 * @brief Free NUMA-allocated memory
 */
static void numa_free(void* ptr, size_t size) {
    if (ptr) {
        munmap(ptr, size);
    }
}

int per_cpu_arena_init_cpu(uint32_t cpu_id, size_t size) {
    if (cpu_id >= NUMA_MAX_CPUS) {
        return -1;
    }
    
    if (g_arenas[cpu_id]) {
        return 0;  // Already initialized
    }
    
    // Get NUMA topology
    numa_topology_t* topo = numa_topology_get();
    if (!topo) {
        topo = numa_topology_init();
        if (!topo) {
            return -1;
        }
    }
    
    // Allocate arena structure
    per_cpu_arena_t* arena = calloc(1, sizeof(per_cpu_arena_t));
    if (!arena) {
        return -1;
    }
    
    arena->cpu_id = cpu_id;
    arena->numa_node = numa_topology_cpu_to_node(cpu_id);
    if (arena->numa_node < 0) {
        arena->numa_node = 0;  // Fallback to node 0
    }
    
    // Allocate arena memory from NUMA node
    if (size == 0) {
        size = PER_CPU_ARENA_DEFAULT_SIZE;
    }
    
    arena->total_size = size;
    arena->base_address = numa_alloc(size, arena->numa_node);
    if (!arena->base_address) {
        free(arena);
        return -1;
    }
    
    // Initialize free lists
    // Start with one large block
    free_block_t* initial_block = (free_block_t*)arena->base_address;
    initial_block->size = size;
    initial_block->next = NULL;
    arena->large_blocks = initial_block;
    
    arena->initialized = true;
    g_arenas[cpu_id] = arena;
    
    return 0;
}

int per_cpu_arena_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize NUMA topology
    numa_topology_t* topo = numa_topology_init();
    if (!topo) {
        return -1;
    }
    
    // Initialize arena for each CPU
    for (uint32_t i = 0; i < topo->num_cpus; i++) {
        if (per_cpu_arena_init_cpu(i, 0) < 0) {
            // Cleanup on failure
            per_cpu_arena_destroy();
            return -1;
        }
    }
    
    g_initialized = true;
    return 0;
}

/**
 * @brief Allocate from free list
 */
static void* alloc_from_free_list(per_cpu_arena_t* arena, int class_idx, size_t size) {
    free_block_t** list = &arena->free_lists[class_idx];
    
    if (*list) {
        free_block_t* block = *list;
        *list = block->next;
        
        arena->used_size += size;
        arena->stats.bytes_allocated += size;
        
        return (void*)block;
    }
    
    return NULL;
}

/**
 * @brief Allocate from large blocks
 */
static void* alloc_from_large_blocks(per_cpu_arena_t* arena, size_t size) {
    free_block_t** list = &arena->large_blocks;
    free_block_t* prev = NULL;
    free_block_t* curr = *list;
    
    // First-fit allocation
    while (curr) {
        if (curr->size >= size) {
            // Found suitable block
            if (curr->size >= size + sizeof(free_block_t) + PER_CPU_ARENA_MIN_SIZE) {
                // Split block
                free_block_t* remainder = (free_block_t*)((char*)curr + size);
                remainder->size = curr->size - size;
                remainder->next = curr->next;
                
                if (prev) {
                    prev->next = remainder;
                } else {
                    *list = remainder;
                }
            } else {
                // Use entire block
                if (prev) {
                    prev->next = curr->next;
                } else {
                    *list = curr->next;
                }
            }
            
            arena->used_size += size;
            arena->stats.bytes_allocated += size;
            
            return (void*)curr;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return NULL;
}

void* per_cpu_arena_alloc_cpu(uint32_t cpu_id, size_t size) {
    if (cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return NULL;
    }
    
    per_cpu_arena_t* arena = g_arenas[cpu_id];
    
    // Align size
    size = (size + PER_CPU_ARENA_ALIGNMENT - 1) & ~(PER_CPU_ARENA_ALIGNMENT - 1);
    
    arena->stats.total_allocs++;
    
    void* ptr = NULL;
    int class_idx = get_size_class(size);
    
    if (class_idx >= 0) {
        // Small allocation - use size class
        size_t class_size = get_class_size(class_idx);
        ptr = alloc_from_free_list(arena, class_idx, class_size);
        
        if (!ptr) {
            // Allocate from large blocks
            ptr = alloc_from_large_blocks(arena, class_size);
        }
    } else {
        // Large allocation
        ptr = alloc_from_large_blocks(arena, size);
    }
    
    if (ptr) {
        // Track NUMA locality
        int current_node = numa_topology_current_node();
        if (current_node == arena->numa_node) {
            arena->stats.local_allocs++;
        } else {
            arena->stats.remote_allocs++;
        }
        
        if (arena->used_size > arena->stats.peak_usage) {
            arena->stats.peak_usage = arena->used_size;
        }
        arena->stats.current_usage = arena->used_size;
    } else {
        arena->stats.failed_allocs++;
    }
    
    return ptr;
}

void* per_cpu_arena_alloc(size_t size) {
    int cpu = sched_getcpu();
    if (cpu < 0) {
        return NULL;
    }
    
    return per_cpu_arena_alloc_cpu(cpu, size);
}

void* per_cpu_arena_alloc_aligned(size_t size, size_t alignment) {
    // For simplicity, allocate extra space and align
    size_t alloc_size = size + alignment;
    void* ptr = per_cpu_arena_alloc(alloc_size);
    
    if (!ptr) {
        return NULL;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    
    return (void*)aligned;
}

void per_cpu_arena_free_cpu(uint32_t cpu_id, void* ptr, size_t size) {
    if (!ptr || cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return;
    }
    
    per_cpu_arena_t* arena = g_arenas[cpu_id];
    
    // Align size
    size = (size + PER_CPU_ARENA_ALIGNMENT - 1) & ~(PER_CPU_ARENA_ALIGNMENT - 1);
    
    arena->stats.total_frees++;
    arena->stats.bytes_freed += size;
    arena->used_size -= size;
    arena->stats.current_usage = arena->used_size;
    
    int class_idx = get_size_class(size);
    
    if (class_idx >= 0) {
        // Return to size class free list
        size_t class_size = get_class_size(class_idx);
        free_block_t* block = (free_block_t*)ptr;
        block->size = class_size;
        block->next = arena->free_lists[class_idx];
        arena->free_lists[class_idx] = block;
    } else {
        // Return to large blocks
        free_block_t* block = (free_block_t*)ptr;
        block->size = size;
        block->next = arena->large_blocks;
        arena->large_blocks = block;
    }
}

void per_cpu_arena_free(void* ptr, size_t size) {
    int cpu = sched_getcpu();
    if (cpu < 0) {
        return;
    }
    
    per_cpu_arena_free_cpu(cpu, ptr, size);
}

per_cpu_arena_t* per_cpu_arena_get(uint32_t cpu_id) {
    if (cpu_id >= NUMA_MAX_CPUS) {
        return NULL;
    }
    return g_arenas[cpu_id];
}

per_cpu_arena_t* per_cpu_arena_get_current(void) {
    int cpu = sched_getcpu();
    if (cpu < 0) {
        return NULL;
    }
    return per_cpu_arena_get(cpu);
}

int per_cpu_arena_get_stats_cpu(uint32_t cpu_id, per_cpu_arena_stats_t* stats) {
    if (!stats || cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return -1;
    }
    
    *stats = g_arenas[cpu_id]->stats;
    return 0;
}

int per_cpu_arena_get_stats(per_cpu_arena_stats_t* stats) {
    int cpu = sched_getcpu();
    if (cpu < 0) {
        return -1;
    }
    return per_cpu_arena_get_stats_cpu(cpu, stats);
}

void per_cpu_arena_reset_stats(uint32_t cpu_id) {
    if (cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return;
    }
    
    memset(&g_arenas[cpu_id]->stats, 0, sizeof(per_cpu_arena_stats_t));
}

void per_cpu_arena_print_stats(uint32_t cpu_id) {
    if (cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return;
    }
    
    per_cpu_arena_t* arena = g_arenas[cpu_id];
    per_cpu_arena_stats_t* s = &arena->stats;
    
    printf("Per-CPU Arena Statistics (CPU %u, NUMA Node %u):\n", cpu_id, arena->numa_node);
    printf("  Total Allocations: %lu\n", s->total_allocs);
    printf("  Total Frees: %lu\n", s->total_frees);
    printf("  Failed Allocations: %lu\n", s->failed_allocs);
    printf("  Bytes Allocated: %lu\n", s->bytes_allocated);
    printf("  Bytes Freed: %lu\n", s->bytes_freed);
    printf("  Current Usage: %lu / %lu (%.1f%%)\n", 
           s->current_usage, arena->total_size,
           100.0 * s->current_usage / arena->total_size);
    printf("  Peak Usage: %lu (%.1f%%)\n",
           s->peak_usage, 100.0 * s->peak_usage / arena->total_size);
    printf("  Local Allocations: %lu (%.1f%%)\n",
           s->local_allocs, 100.0 * s->local_allocs / (s->total_allocs + 1));
    printf("  Remote Allocations: %lu (%.1f%%)\n",
           s->remote_allocs, 100.0 * s->remote_allocs / (s->total_allocs + 1));
}

void per_cpu_arena_destroy_cpu(uint32_t cpu_id) {
    if (cpu_id >= NUMA_MAX_CPUS || !g_arenas[cpu_id]) {
        return;
    }
    
    per_cpu_arena_t* arena = g_arenas[cpu_id];
    
    if (arena->base_address) {
        numa_free(arena->base_address, arena->total_size);
    }
    
    free(arena);
    g_arenas[cpu_id] = NULL;
}

void per_cpu_arena_destroy(void) {
    for (uint32_t i = 0; i < NUMA_MAX_CPUS; i++) {
        per_cpu_arena_destroy_cpu(i);
    }
    g_initialized = false;
}
