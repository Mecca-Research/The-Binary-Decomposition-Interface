
/**
 * @file test_per_cpu_arena.c
 * @brief Test per-CPU arena allocator
 */

#include "per_cpu_arena.h"
#include "numa_topology.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Test basic aligned allocation and freeing
 */
void test_aligned_allocation_basic(void) {
    printf("Test 1: Basic aligned allocation...\n");
    
    // Test aligned allocation with different alignments
    void* ptr1 = per_cpu_arena_alloc_aligned(512, 64);
    assert(ptr1 != NULL);
    assert(((uintptr_t)ptr1 % 64) == 0);
    printf("  ✓ Allocated 512 bytes (64-byte aligned): %p\n", ptr1);
    
    void* ptr2 = per_cpu_arena_alloc_aligned(256, 128);
    assert(ptr2 != NULL);
    assert(((uintptr_t)ptr2 % 128) == 0);
    printf("  ✓ Allocated 256 bytes (128-byte aligned): %p\n", ptr2);
    
    void* ptr3 = per_cpu_arena_alloc_aligned(1024, 256);
    assert(ptr3 != NULL);
    assert(((uintptr_t)ptr3 % 256) == 0);
    printf("  ✓ Allocated 1024 bytes (256-byte aligned): %p\n", ptr3);
    
    // BUG FIX: Use per_cpu_arena_free_aligned() instead of per_cpu_arena_free()
    per_cpu_arena_free_aligned(ptr1);
    per_cpu_arena_free_aligned(ptr2);
    per_cpu_arena_free_aligned(ptr3);
    printf("  ✓ All aligned allocations freed successfully\n");
}

/**
 * @brief Test multiple aligned allocations and frees in random order
 */
void test_aligned_allocation_random_order(void) {
    printf("\nTest 2: Multiple aligned allocations with random free order...\n");
    
    #define NUM_ALLOCS 10
    void* ptrs[NUM_ALLOCS];
    size_t sizes[] = {64, 128, 256, 512, 1024, 64, 128, 256, 512, 1024};
    size_t alignments[] = {16, 32, 64, 128, 256, 64, 128, 256, 512, 1024};
    
    // Allocate multiple blocks
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = per_cpu_arena_alloc_aligned(sizes[i], alignments[i]);
        assert(ptrs[i] != NULL);
        assert(((uintptr_t)ptrs[i] % alignments[i]) == 0);
        
        // Write pattern to verify no corruption
        memset(ptrs[i], 0xAA + i, sizes[i]);
    }
    printf("  ✓ Allocated %d aligned blocks\n", NUM_ALLOCS);
    
    // Verify patterns are intact
    for (int i = 0; i < NUM_ALLOCS; i++) {
        unsigned char* data = (unsigned char*)ptrs[i];
        for (size_t j = 0; j < sizes[i]; j++) {
            assert(data[j] == (unsigned char)(0xAA + i));
        }
    }
    printf("  ✓ All data patterns verified (no corruption)\n");
    
    // Free in reverse order
    for (int i = NUM_ALLOCS - 1; i >= 0; i--) {
        per_cpu_arena_free_aligned(ptrs[i]);
    }
    printf("  ✓ All blocks freed in reverse order\n");
    
    // Allocate again to ensure no corruption
    void* ptr_test = per_cpu_arena_alloc_aligned(512, 64);
    assert(ptr_test != NULL);
    assert(((uintptr_t)ptr_test % 64) == 0);
    per_cpu_arena_free_aligned(ptr_test);
    printf("  ✓ Re-allocation after free works correctly\n");
}

/**
 * @brief Test mixing regular and aligned allocations
 */
void test_mixed_allocations(void) {
    printf("\nTest 3: Mixed regular and aligned allocations...\n");
    
    // Regular allocations
    void* reg1 = per_cpu_arena_alloc(64);
    void* reg2 = per_cpu_arena_alloc(128);
    assert(reg1 != NULL && reg2 != NULL);
    printf("  ✓ Regular allocations: %p, %p\n", reg1, reg2);
    
    // Aligned allocations
    void* align1 = per_cpu_arena_alloc_aligned(256, 64);
    void* align2 = per_cpu_arena_alloc_aligned(512, 128);
    assert(align1 != NULL && align2 != NULL);
    assert(((uintptr_t)align1 % 64) == 0);
    assert(((uintptr_t)align2 % 128) == 0);
    printf("  ✓ Aligned allocations: %p, %p\n", align1, align2);
    
    // More regular allocations
    void* reg3 = per_cpu_arena_alloc(256);
    assert(reg3 != NULL);
    printf("  ✓ Another regular allocation: %p\n", reg3);
    
    // Free in mixed order using correct free functions
    per_cpu_arena_free(reg1, 64);
    per_cpu_arena_free_aligned(align1);
    per_cpu_arena_free(reg2, 128);
    per_cpu_arena_free_aligned(align2);
    per_cpu_arena_free(reg3, 256);
    printf("  ✓ All mixed allocations freed with correct functions\n");
}

/**
 * @brief Test edge cases
 */
void test_edge_cases(void) {
    printf("\nTest 4: Edge cases...\n");
    
    // Small alignment
    void* ptr1 = per_cpu_arena_alloc_aligned(32, 8);
    assert(ptr1 != NULL);
    assert(((uintptr_t)ptr1 % 8) == 0);
    per_cpu_arena_free_aligned(ptr1);
    printf("  ✓ Small alignment (8 bytes) works\n");
    
    // Large alignment
    void* ptr2 = per_cpu_arena_alloc_aligned(128, 1024);
    assert(ptr2 != NULL);
    assert(((uintptr_t)ptr2 % 1024) == 0);
    per_cpu_arena_free_aligned(ptr2);
    printf("  ✓ Large alignment (1024 bytes) works\n");
    
    // Free NULL pointer (should not crash)
    per_cpu_arena_free_aligned(NULL);
    printf("  ✓ Freeing NULL pointer handled safely\n");
}

int main(void) {
    printf("Testing per-CPU arena allocator...\n");
    printf("===========================================\n\n");
    
    // Initialize
    assert(numa_topology_init() != NULL);
    assert(per_cpu_arena_init() == 0);
    printf("✓ Initialization successful\n\n");
    
    // Test basic allocation (original test)
    printf("Test 0: Basic allocations...\n");
    void* ptr1 = per_cpu_arena_alloc(64);
    assert(ptr1 != NULL);
    printf("  ✓ Allocated 64 bytes: %p\n", ptr1);
    
    void* ptr2 = per_cpu_arena_alloc(128);
    assert(ptr2 != NULL);
    printf("  ✓ Allocated 128 bytes: %p\n", ptr2);
    
    void* ptr3 = per_cpu_arena_alloc(256);
    assert(ptr3 != NULL);
    printf("  ✓ Allocated 256 bytes: %p\n", ptr3);
    
    per_cpu_arena_free(ptr1, 64);
    per_cpu_arena_free(ptr2, 128);
    per_cpu_arena_free(ptr3, 256);
    printf("  ✓ All basic allocations freed\n");
    
    // Run aligned allocation tests
    test_aligned_allocation_basic();
    test_aligned_allocation_random_order();
    test_mixed_allocations();
    test_edge_cases();
    
    // Print final statistics
    per_cpu_arena_stats_t stats;
    assert(per_cpu_arena_get_stats(&stats) == 0);
    
    printf("\n===========================================\n");
    printf("Final Arena Statistics:\n");
    printf("  Total Allocations: %lu\n", stats.total_allocs);
    printf("  Total Frees: %lu\n", stats.total_frees);
    printf("  Failed Allocations: %lu\n", stats.failed_allocs);
    printf("  Bytes Allocated: %lu\n", stats.bytes_allocated);
    printf("  Bytes Freed: %lu\n", stats.bytes_freed);
    printf("  Current Usage: %lu\n", stats.current_usage);
    printf("  Peak Usage: %lu\n", stats.peak_usage);
    printf("  Local Allocations: %lu\n", stats.local_allocs);
    printf("  Remote Allocations: %lu\n", stats.remote_allocs);
    
    // Cleanup
    per_cpu_arena_destroy();
    numa_topology_destroy();
    
    printf("\n===========================================\n");
    printf("✅ All per-CPU arena tests passed!\n");
    printf("✅ Bug fix validated: Aligned allocations work correctly\n");
    return 0;
}
