
/**
 * Memory Stress Tests for BDI Kernel
 * 
 * Tests intensive memory allocation/deallocation patterns, leak detection,
 * heap exhaustion, fragmentation, and memory pressure scenarios.
 */

#include "../framework/test_framework.h"
#include "../test_utils.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include "../../vm/vm.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Configuration
#define STRESS_ITERATIONS_LOW 1000
#define STRESS_ITERATIONS_MEDIUM 10000
#define STRESS_ITERATIONS_HIGH 100000
#define LARGE_ALLOCATION_SIZE (10 * 1024 * 1024)  // 10 MB
#define SMALL_ALLOCATION_SIZE 64

// Test: Rapid allocation/deallocation cycles
static bool test_rapid_alloc_dealloc(void) {
    printf("Running rapid allocation/deallocation test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    for (int i = 0; i < STRESS_ITERATIONS_MEDIUM; i++) {
        // Allocate varying sizes
        size_t size = (i % 10 + 1) * 128;
        void* ptr = malloc(size);
        TEST_ASSERT_NOT_NULL(ptr, "Allocation should succeed");
        
        // Write pattern to verify memory
        memset(ptr, 0xAA, size);
        
        // Immediately free
        free(ptr);
    }
    
    TEST_MEMORY_VERIFY("Rapid alloc/dealloc should not leak memory");
    printf("✓ Rapid allocation/deallocation test passed\n");
    return true;
}

// Test: Memory fragmentation stress
static bool test_memory_fragmentation(void) {
    printf("Running memory fragmentation test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    // Allocate many small blocks
    void* blocks[1000];
    for (int i = 0; i < 1000; i++) {
        blocks[i] = malloc(SMALL_ALLOCATION_SIZE);
        TEST_ASSERT_NOT_NULL(blocks[i], "Small allocation should succeed");
    }
    
    // Free every other block to create fragmentation
    for (int i = 0; i < 1000; i += 2) {
        free(blocks[i]);
        blocks[i] = NULL;
    }
    
    // Try to allocate larger blocks in the gaps
    for (int i = 0; i < 1000; i += 2) {
        blocks[i] = malloc(SMALL_ALLOCATION_SIZE * 2);
        // May fail due to fragmentation, which is expected
        if (blocks[i] == NULL) {
            printf("  Note: Allocation failed due to fragmentation (expected)\n");
        }
    }
    
    // Clean up
    for (int i = 0; i < 1000; i++) {
        if (blocks[i] != NULL) {
            free(blocks[i]);
        }
    }
    
    TEST_MEMORY_VERIFY("Fragmentation test should not leak memory");
    printf("✓ Memory fragmentation test passed\n");
    return true;
}

// Test: Large allocation stress
static bool test_large_allocations(void) {
    printf("Running large allocation test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    // Allocate and free large blocks
    for (int i = 0; i < 10; i++) {
        void* ptr = malloc(LARGE_ALLOCATION_SIZE);
        if (ptr == NULL) {
            printf("  Note: Large allocation failed (may be expected on constrained systems)\n");
            continue;
        }
        
        // Write pattern to verify memory
        memset(ptr, 0xBB, LARGE_ALLOCATION_SIZE);
        
        // Verify pattern
        unsigned char* bytes = (unsigned char*)ptr;
        for (size_t j = 0; j < LARGE_ALLOCATION_SIZE; j += 4096) {
            TEST_ASSERT_EQ(0xBB, bytes[j], "Memory pattern should be preserved");
        }
        
        free(ptr);
    }
    
    TEST_MEMORY_VERIFY("Large allocation test should not leak memory");
    printf("✓ Large allocation test passed\n");
    return true;
}

// Test: VM memory stress
static bool test_vm_memory_stress(void) {
    printf("Running VM memory stress test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    // Create and destroy many VMs
    for (int i = 0; i < 100; i++) {
        VM vm;
        vm_init(&vm);
        
        // Create chunk with many constants
        Chunk chunk;
        chunk_init(&chunk);
        
        for (int j = 0; j < 100; j++) {
            chunk_add_constant(&chunk, (double)j);
        }
        
        // Write some bytecode
        for (int j = 0; j < 100; j++) {
            chunk_write(&chunk, OP_CONSTANT, 1);
            chunk_write(&chunk, j % 100, 1);
        }
        chunk_write(&chunk, OP_RETURN, 1);
        
        // Execute
        vm_interpret(&vm, &chunk);
        
        // Cleanup
        chunk_free(&chunk);
        vm_free(&vm);
    }
    
    TEST_MEMORY_VERIFY("VM memory stress should not leak memory");
    printf("✓ VM memory stress test passed\n");
    return true;
}

// Test: Constant pool growth stress
static bool test_constant_pool_growth(void) {
    printf("Running constant pool growth test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Add many constants to stress dynamic array growth
    for (int i = 0; i < 10000; i++) {
        int idx = chunk_add_constant(&chunk, (double)i);
        TEST_ASSERT_EQ(i, idx, "Constant index should match");
    }
    
    // Verify constants
    for (int i = 0; i < 10000; i++) {
        TEST_ASSERT_EQ((double)i, chunk.constants.data[i], "Constant value should be preserved");
    }
    
    chunk_free(&chunk);
    
    TEST_MEMORY_VERIFY("Constant pool growth should not leak memory");
    printf("✓ Constant pool growth test passed\n");
    return true;
}

// Test: Memory pressure simulation
static bool test_memory_pressure(void) {
    printf("Running memory pressure test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    // Allocate memory until we approach system limits
    void* allocations[1000];
    int alloc_count = 0;
    size_t total_allocated = 0;
    
    for (int i = 0; i < 1000; i++) {
        size_t size = 1024 * 1024;  // 1 MB
        allocations[i] = malloc(size);
        
        if (allocations[i] == NULL) {
            printf("  Reached memory limit after %d MB\n", alloc_count);
            break;
        }
        
        alloc_count++;
        total_allocated += size;
        
        // Stop at 100 MB to avoid system issues
        if (total_allocated >= 100 * 1024 * 1024) {
            break;
        }
    }
    
    // Free all allocations
    for (int i = 0; i < alloc_count; i++) {
        free(allocations[i]);
    }
    
    TEST_MEMORY_VERIFY("Memory pressure test should not leak memory");
    printf("✓ Memory pressure test passed (allocated %zu MB)\n", total_allocated / (1024 * 1024));
    return true;
}

// Test: Sustained memory load
static bool test_sustained_memory_load(void) {
    printf("Running sustained memory load test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    // Maintain sustained memory usage
    void* blocks[100];
    for (int iteration = 0; iteration < 100; iteration++) {
        // Allocate
        for (int i = 0; i < 100; i++) {
            blocks[i] = malloc(10240);  // 10 KB
            TEST_ASSERT_NOT_NULL(blocks[i], "Allocation should succeed");
        }
        
        // Use the memory
        for (int i = 0; i < 100; i++) {
            memset(blocks[i], iteration & 0xFF, 10240);
        }
        
        // Free
        for (int i = 0; i < 100; i++) {
            free(blocks[i]);
        }
    }
    
    TEST_MEMORY_VERIFY("Sustained memory load should not leak memory");
    printf("✓ Sustained memory load test passed\n");
    return true;
}

// Test: Mixed allocation patterns
static bool test_mixed_allocation_patterns(void) {
    printf("Running mixed allocation patterns test...\n");
    
    TEST_MEMORY_CHECKPOINT();
    
    for (int i = 0; i < 1000; i++) {
        // Mix of small, medium, and large allocations
        void* small = malloc(64);
        void* medium = malloc(4096);
        void* large = malloc(65536);
        
        TEST_ASSERT_NOT_NULL(small, "Small allocation should succeed");
        TEST_ASSERT_NOT_NULL(medium, "Medium allocation should succeed");
        
        if (large != NULL) {
            memset(large, 0xCC, 65536);
            free(large);
        }
        
        free(medium);
        free(small);
    }
    
    TEST_MEMORY_VERIFY("Mixed allocation patterns should not leak memory");
    printf("✓ Mixed allocation patterns test passed\n");
    return true;
}

// Main test runner
int main(void) {
    printf("\n=== Memory Stress Tests ===\n\n");
    
    test_framework_init();
    
    bool all_passed = true;
    
    all_passed &= test_rapid_alloc_dealloc();
    all_passed &= test_memory_fragmentation();
    all_passed &= test_large_allocations();
    all_passed &= test_vm_memory_stress();
    all_passed &= test_constant_pool_growth();
    all_passed &= test_memory_pressure();
    all_passed &= test_sustained_memory_load();
    all_passed &= test_mixed_allocation_patterns();
    
    test_framework_print_summary();
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
