
/**
 * @file test_per_cpu_arena.c
 * @brief Test per-CPU arena allocator
 */

#include "per_cpu_arena.h"
#include "numa_topology.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    printf("Testing per-CPU arena allocator...\n\n");
    
    // Initialize
    assert(numa_topology_init() != NULL);
    assert(per_cpu_arena_init() == 0);
    
    // Test allocation
    void* ptr1 = per_cpu_arena_alloc(64);
    assert(ptr1 != NULL);
    printf("Allocated 64 bytes: %p\n", ptr1);
    
    void* ptr2 = per_cpu_arena_alloc(128);
    assert(ptr2 != NULL);
    printf("Allocated 128 bytes: %p\n", ptr2);
    
    void* ptr3 = per_cpu_arena_alloc(256);
    assert(ptr3 != NULL);
    printf("Allocated 256 bytes: %p\n", ptr3);
    
    // Test aligned allocation
    void* ptr4 = per_cpu_arena_alloc_aligned(512, 64);
    assert(ptr4 != NULL);
    assert(((uintptr_t)ptr4 % 64) == 0);
    printf("Allocated 512 bytes (64-byte aligned): %p\n", ptr4);
    
    // Test free
    per_cpu_arena_free(ptr1, 64);
    per_cpu_arena_free(ptr2, 128);
    per_cpu_arena_free(ptr3, 256);
    per_cpu_arena_free(ptr4, 512);
    
    // Print statistics
    per_cpu_arena_stats_t stats;
    assert(per_cpu_arena_get_stats(&stats) == 0);
    
    printf("\nArena Statistics:\n");
    printf("  Total Allocations: %lu\n", stats.total_allocs);
    printf("  Total Frees: %lu\n", stats.total_frees);
    printf("  Failed Allocations: %lu\n", stats.failed_allocs);
    printf("  Local Allocations: %lu\n", stats.local_allocs);
    printf("  Remote Allocations: %lu\n", stats.remote_allocs);
    
    // Cleanup
    per_cpu_arena_destroy();
    numa_topology_destroy();
    
    printf("\nAll per-CPU arena tests passed!\n");
    return 0;
}
