
// ===================================================================
// BDI Kernel - Memory Management Implementation (Phase 2)
// C23 Enhanced with NUMA Optimization and Per-CPU Arenas
// ===================================================================
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// ===================================================================
// Global State
// ===================================================================

static bool memory_initialized = false;
static MemoryStats global_stats = {0};

// Per-CPU arenas (thread-local for fast access)
static CpuArena arenas[MAX_ARENAS];
static _Atomic int num_arenas = 0;
_Thread_local CpuArena* current_arena = NULL;

// NUMA topology
static int numa_nodes = 1;
static bool numa_available = false;

// ===================================================================
// NUMA Detection and Initialization
// ===================================================================

NODISCARD int numa_init(void) {
    // Detect NUMA topology
    // In a real implementation, this would query /sys/devices/system/node/
    // or use libnuma. For now, we simulate basic detection.
    
    #ifdef __linux__
    // Check if NUMA is available
    if (access("/sys/devices/system/node/node1", F_OK) == 0) {
        numa_available = true;
        
        // Count NUMA nodes
        numa_nodes = 1;
        for (int i = 1; i < MAX_NUMA_NODES; i++) {
            char path[256];
            snprintf(path, sizeof(path), "/sys/devices/system/node/node%d", i);
            if (access(path, F_OK) == 0) {
                numa_nodes++;
            } else {
                break;
            }
        }
        
        printf("NUMA: Detected %d NUMA nodes\n", numa_nodes);
    } else {
        numa_available = false;
        numa_nodes = 1;
        printf("NUMA: Not available, using UMA mode\n");
    }
    #else
    numa_available = false;
    numa_nodes = 1;
    printf("NUMA: Not supported on this platform\n");
    #endif
    
    return 0;
}

NODISCARD int numa_num_nodes(void) {
    return numa_nodes;
}

NODISCARD int numa_current_node(void) {
    if (!numa_available) {
        return 0;
    }
    
    // In a real implementation, this would use getcpu() or similar
    // to determine the current CPU's NUMA node
    // For now, return node 0
    return 0;
}

NODISCARD int numa_cpu_to_node(int cpu) {
    if (!numa_available || cpu < 0) {
        return 0;
    }
    
    // Simple mapping: distribute CPUs across NUMA nodes
    // In a real implementation, read from /sys/devices/system/cpu/cpuX/node
    return cpu % numa_nodes;
}

NODISCARD void* numa_alloc_onnode(size_t size, int node) {
    if (size == 0) {
        return NULL;
    }
    
    if (node < 0 || node >= numa_nodes) {
        return NULL;
    }
    
    // In a real implementation, use mbind() or numa_alloc_onnode()
    // For now, use standard malloc
    void* ptr = malloc(size);
    
    if (ptr != NULL) {
        // Track NUMA allocation
        if (node == numa_current_node()) {
            ATOMIC_FETCH_ADD(&global_stats.numa_local_allocs, 1);
        } else {
            ATOMIC_FETCH_ADD(&global_stats.numa_remote_allocs, 1);
        }
    }
    
    return ptr;
}

NODISCARD void* numa_alloc_local(size_t size) {
    int node = numa_current_node();
    return numa_alloc_onnode(size, node);
}

NODISCARD int numa_free(void* ptr, size_t size) {
    if (ptr == NULL) {
        return -1;
    }
    
    free(ptr);
    return 0;
}

// ===================================================================
// Per-CPU Arena Implementation
// ===================================================================

NODISCARD int arena_init(void) {
    // Get number of CPUs
    int num_cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus <= 0 || num_cpus > MAX_ARENAS) {
        printf("Arena: Invalid CPU count %d\n", num_cpus);
        return -1;
    }
    
    printf("Arena: Initializing %d per-CPU arenas\n", num_cpus);
    
    // Initialize per-CPU arenas
    for (int i = 0; i < num_cpus; i++) {
        CpuArena* arena = &arenas[i];
        
        // Determine NUMA node for this CPU
        int node = numa_cpu_to_node(i);
        
        // Allocate arena on CPU's NUMA node
        arena->base = numa_alloc_onnode(ARENA_SIZE, node);
        if (arena->base == NULL) {
            printf("Arena: Failed to allocate arena for CPU %d\n", i);
            return -1;
        }
        
        arena->size = ARENA_SIZE;
        arena->used = 0;
        arena->numa_node = node;
        arena->cpu_id = i;
        ATOMIC_STORE(&arena->allocs, 0);
        ATOMIC_STORE(&arena->frees, 0);
        ATOMIC_STORE(&arena->bytes_allocated, 0);
        
        printf("Arena: CPU %d -> NUMA node %d (arena at %p)\n", 
               i, node, arena->base);
    }
    
    ATOMIC_STORE(&num_arenas, num_cpus);
    return 0;
}

void arena_shutdown(void) {
    int count = ATOMIC_LOAD(&num_arenas);
    
    for (int i = 0; i < count; i++) {
        CpuArena* arena = &arenas[i];
        if (arena->base != NULL) {
            numa_free(arena->base, arena->size);
            arena->base = NULL;
        }
    }
    
    ATOMIC_STORE(&num_arenas, 0);
}

NODISCARD CpuArena* arena_get_current(void) {
    if (current_arena != NULL) {
        return current_arena;
    }
    
    // Get current CPU (simplified - in real kernel use sched_getcpu())
    int cpu = 0;
    #ifdef __linux__
    cpu = sched_getcpu();
    if (cpu < 0) cpu = 0;
    #endif
    
    int count = ATOMIC_LOAD(&num_arenas);
    if (cpu >= 0 && cpu < count) {
        current_arena = &arenas[cpu];
        return current_arena;
    }
    
    return NULL;
}

NODISCARD void* arena_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Get current CPU's arena
    CpuArena* arena = arena_get_current();
    if (arena == NULL) {
        // Fallback to global allocator
        return alloc_memory(size, MIN_ALIGNMENT);
    }
    
    // Align size to minimum alignment
    size = (size + MIN_ALIGNMENT - 1) & ~(MIN_ALIGNMENT - 1);
    
    // Fast path: allocate from arena
    if (arena->used + size <= arena->size) {
        void* ptr = (char*)arena->base + arena->used;
        arena->used += size;
        ATOMIC_FETCH_ADD(&arena->allocs, 1);
        ATOMIC_FETCH_ADD(&arena->bytes_allocated, size);
        ATOMIC_FETCH_ADD(&global_stats.arena_allocs, 1);
        return ptr;
    }
    
    // Slow path: arena full, use global allocator
    return alloc_memory(size, MIN_ALIGNMENT);
}

NODISCARD int arena_free(void* ptr) {
    if (ptr == NULL) {
        return -1;
    }
    
    // Arena allocations are not individually freed
    // They are freed when the arena is reset or destroyed
    CpuArena* arena = arena_get_current();
    if (arena != NULL) {
        ATOMIC_FETCH_ADD(&arena->frees, 1);
    }
    
    return 0;
}

// ===================================================================
// Huge Page Support
// ===================================================================

NODISCARD void* alloc_huge_page(size_t size, int numa_node) {
    if (size == 0) {
        return NULL;
    }
    
    // Determine huge page size
    size_t page_size = (size <= HUGE_PAGE_2MB) ? HUGE_PAGE_2MB : HUGE_PAGE_1GB;
    
    // Round up to huge page size
    size = (size + page_size - 1) & ~(page_size - 1);
    
    // Allocate on NUMA node
    void* ptr = numa_alloc_onnode(size, numa_node);
    if (ptr == NULL) {
        return NULL;
    }
    
    // Verify alignment
    if (!is_huge_page_aligned(ptr, page_size)) {
        numa_free(ptr, size);
        return NULL;
    }
    
    ATOMIC_FETCH_ADD(&global_stats.huge_page_allocs, 1);
    return ptr;
}

NODISCARD int free_huge_page(void* ptr, size_t size) {
    if (ptr == NULL) {
        return -1;
    }
    
    return numa_free(ptr, size);
}

NODISCARD bool is_huge_page_aligned(void* ptr, size_t page_size) {
    return ((uintptr_t)ptr & (page_size - 1)) == 0;
}

// ===================================================================
// Core Memory Allocation
// ===================================================================

NODISCARD int memory_init(void) {
    if (memory_initialized) {
        return 0;
    }
    
    printf("Memory: Initializing Phase 2 memory subsystem\n");
    
    // Initialize NUMA
    if (numa_init() != 0) {
        printf("Memory: NUMA initialization failed\n");
        return -1;
    }
    
    // Initialize per-CPU arenas
    if (arena_init() != 0) {
        printf("Memory: Arena initialization failed\n");
        return -1;
    }
    
    // Initialize statistics
    ATOMIC_STORE(&global_stats.total_allocated, 0);
    ATOMIC_STORE(&global_stats.total_freed, 0);
    ATOMIC_STORE(&global_stats.current_usage, 0);
    ATOMIC_STORE(&global_stats.peak_usage, 0);
    ATOMIC_STORE(&global_stats.numa_local_allocs, 0);
    ATOMIC_STORE(&global_stats.numa_remote_allocs, 0);
    ATOMIC_STORE(&global_stats.huge_page_allocs, 0);
    ATOMIC_STORE(&global_stats.arena_allocs, 0);
    
    memory_initialized = true;
    printf("Memory: Phase 2 initialization complete\n");
    return 0;
}

void memory_shutdown(void) {
    if (!memory_initialized) {
        return;
    }
    
    arena_shutdown();
    memory_initialized = false;
}

NODISCARD void* alloc_memory(size_t size, size_t alignment) {
    if (size == 0) {
        return NULL;
    }
    
    if (alignment < MIN_ALIGNMENT) {
        alignment = MIN_ALIGNMENT;
    }
    
    if (alignment > MAX_ALIGNMENT) {
        return NULL;
    }
    
    // Use NUMA-aware allocation
    void* ptr = numa_alloc_local(size);
    if (ptr == NULL) {
        return NULL;
    }
    
    // Update statistics
    ATOMIC_FETCH_ADD(&global_stats.total_allocated, size);
    size_t current = ATOMIC_FETCH_ADD(&global_stats.current_usage, size) + size;
    
    // Update peak usage
    size_t peak = ATOMIC_LOAD(&global_stats.peak_usage);
    while (current > peak) {
        if (__atomic_compare_exchange_n(&global_stats.peak_usage, &peak, current,
                                       false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            break;
        }
    }
    
    return ptr;
}

NODISCARD void* alloc_memory_flags(size_t size, size_t alignment, uint32_t flags) {
    if (size == 0) {
        return NULL;
    }
    
    void* ptr = NULL;
    
    // Check for huge page allocation
    if (flags & MEM_FLAG_HUGE) {
        int node = (flags & MEM_FLAG_NUMA) ? numa_current_node() : 0;
        ptr = alloc_huge_page(size, node);
    } else {
        ptr = alloc_memory(size, alignment);
    }
    
    if (ptr == NULL) {
        return NULL;
    }
    
    // Zero memory if requested
    if (flags & MEM_FLAG_ZERO) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

NODISCARD void* alloc_memory_numa(size_t size, size_t alignment, int node) {
    if (size == 0) {
        return NULL;
    }
    
    if (node < 0 || node >= numa_nodes) {
        return NULL;
    }
    
    void* ptr = numa_alloc_onnode(size, node);
    if (ptr == NULL) {
        return NULL;
    }
    
    // Update statistics
    ATOMIC_FETCH_ADD(&global_stats.total_allocated, size);
    ATOMIC_FETCH_ADD(&global_stats.current_usage, size);
    
    return ptr;
}

NODISCARD void* alloc_memory_local(size_t size, size_t alignment) {
    int node = numa_current_node();
    return alloc_memory_numa(size, alignment, node);
}

NODISCARD int free_memory(void* ptr, size_t size) {
    if (ptr == NULL) {
        return -1;
    }
    
    numa_free(ptr, size);
    
    // Update statistics
    ATOMIC_FETCH_ADD(&global_stats.total_freed, size);
    ATOMIC_FETCH_SUB(&global_stats.current_usage, size);
    
    return 0;
}

// ===================================================================
// Memory Statistics
// ===================================================================

NODISCARD MemoryStats memory_get_stats(void) {
    MemoryStats stats;
    stats.total_allocated = ATOMIC_LOAD(&global_stats.total_allocated);
    stats.total_freed = ATOMIC_LOAD(&global_stats.total_freed);
    stats.current_usage = ATOMIC_LOAD(&global_stats.current_usage);
    stats.peak_usage = ATOMIC_LOAD(&global_stats.peak_usage);
    stats.numa_local_allocs = ATOMIC_LOAD(&global_stats.numa_local_allocs);
    stats.numa_remote_allocs = ATOMIC_LOAD(&global_stats.numa_remote_allocs);
    stats.huge_page_allocs = ATOMIC_LOAD(&global_stats.huge_page_allocs);
    stats.arena_allocs = ATOMIC_LOAD(&global_stats.arena_allocs);
    return stats;
}

void memory_print_stats(void) {
    MemoryStats stats = memory_get_stats();
    
    printf("\n=== Memory Statistics (Phase 2) ===\n");
    printf("Total Allocated:     %zu bytes\n", stats.total_allocated);
    printf("Total Freed:         %zu bytes\n", stats.total_freed);
    printf("Current Usage:       %zu bytes\n", stats.current_usage);
    printf("Peak Usage:          %zu bytes\n", stats.peak_usage);
    printf("NUMA Local Allocs:   %zu\n", stats.numa_local_allocs);
    printf("NUMA Remote Allocs:  %zu\n", stats.numa_remote_allocs);
    printf("Huge Page Allocs:    %zu\n", stats.huge_page_allocs);
    printf("Arena Allocs:        %zu\n", stats.arena_allocs);
    
    // Print per-CPU arena statistics
    int count = ATOMIC_LOAD(&num_arenas);
    printf("\n=== Per-CPU Arena Statistics ===\n");
    for (int i = 0; i < count; i++) {
        CpuArena* arena = &arenas[i];
        printf("CPU %d (NUMA %d): %zu/%zu bytes, %zu allocs, %zu frees\n",
               arena->cpu_id, arena->numa_node,
               arena->used, arena->size,
               ATOMIC_LOAD(&arena->allocs),
               ATOMIC_LOAD(&arena->frees));
    }
    printf("===================================\n\n");
}
