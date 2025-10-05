
#include "../../framework/test_framework.h"
#include "../../../vm/gc/memory_pool.h"
#include "../../../vm/gc/mark_sweep.h"

// Test memory pool creation and initialization
static bool test_memory_pool_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024); // 1MB pool
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Test initial state
    TEST_ASSERT_EQ(0, memory_pool_allocated_bytes(pool), "Initial allocated bytes should be 0");
    TEST_ASSERT_EQ(1024 * 1024, memory_pool_total_bytes(pool), "Total bytes should match requested size");
    TEST_ASSERT_EQ(0, memory_pool_allocation_count(pool), "Initial allocation count should be 0");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory pool creation should not leak memory");
    
    return true;
}

// Test basic memory allocation
static bool test_memory_allocation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Allocate various sizes
    void* ptr1 = memory_pool_alloc(pool, 64);
    void* ptr2 = memory_pool_alloc(pool, 128);
    void* ptr3 = memory_pool_alloc(pool, 256);
    
    TEST_ASSERT_NOT_NULL(ptr1, "64-byte allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr2, "128-byte allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr3, "256-byte allocation should succeed");
    
    // Check that pointers are different
    TEST_ASSERT_NEQ(ptr1, ptr2, "Allocations should return different pointers");
    TEST_ASSERT_NEQ(ptr2, ptr3, "Allocations should return different pointers");
    TEST_ASSERT_NEQ(ptr1, ptr3, "Allocations should return different pointers");
    
    // Check allocation tracking
    TEST_ASSERT_EQ(3, memory_pool_allocation_count(pool), "Should have 3 allocations");
    TEST_ASSERT(memory_pool_allocated_bytes(pool) >= 64 + 128 + 256, "Allocated bytes should include all allocations");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory allocation should not leak memory");
    
    return true;
}

// Test memory alignment
static bool test_memory_alignment(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Test various alignment requirements
    void* ptr8 = memory_pool_alloc_aligned(pool, 100, 8);
    void* ptr16 = memory_pool_alloc_aligned(pool, 100, 16);
    void* ptr32 = memory_pool_alloc_aligned(pool, 100, 32);
    void* ptr64 = memory_pool_alloc_aligned(pool, 100, 64);
    
    TEST_ASSERT_NOT_NULL(ptr8, "8-byte aligned allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr16, "16-byte aligned allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr32, "32-byte aligned allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr64, "64-byte aligned allocation should succeed");
    
    // Check alignment
    TEST_ASSERT_EQ(0, (uintptr_t)ptr8 % 8, "8-byte alignment should be correct");
    TEST_ASSERT_EQ(0, (uintptr_t)ptr16 % 16, "16-byte alignment should be correct");
    TEST_ASSERT_EQ(0, (uintptr_t)ptr32 % 32, "32-byte alignment should be correct");
    TEST_ASSERT_EQ(0, (uintptr_t)ptr64 % 64, "64-byte alignment should be correct");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory alignment should not leak memory");
    
    return true;
}

// Test memory pool exhaustion
static bool test_memory_pool_exhaustion(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024); // Small pool
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Allocate until exhaustion
    void* ptrs[100];
    int successful_allocs = 0;
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = memory_pool_alloc(pool, 64);
        if (ptrs[i] != NULL) {
            successful_allocs++;
        } else {
            break;
        }
    }
    
    TEST_ASSERT(successful_allocs > 0, "Should have some successful allocations");
    TEST_ASSERT(successful_allocs < 100, "Should eventually fail due to exhaustion");
    
    // Try to allocate after exhaustion
    void* failed_ptr = memory_pool_alloc(pool, 64);
    TEST_ASSERT_NULL(failed_ptr, "Allocation should fail when pool is exhausted");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory pool exhaustion should not leak memory");
    
    return true;
}

// Test memory pool fragmentation handling
static bool test_memory_fragmentation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Allocate many small blocks
    void* small_ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        small_ptrs[i] = memory_pool_alloc(pool, 32);
        TEST_ASSERT_NOT_NULL(small_ptrs[i], "Small allocation should succeed");
    }
    
    // Free every other block to create fragmentation
    for (int i = 0; i < 1000; i += 2) {
        memory_pool_free(pool, small_ptrs[i]);
    }
    
    // Try to allocate a larger block
    void* large_ptr = memory_pool_alloc(pool, 1024);
    TEST_ASSERT_NOT_NULL(large_ptr, "Large allocation should succeed despite fragmentation");
    
    // Test defragmentation
    memory_pool_defragment(pool);
    
    // Verify pool is still functional after defragmentation
    void* test_ptr = memory_pool_alloc(pool, 256);
    TEST_ASSERT_NOT_NULL(test_ptr, "Allocation should work after defragmentation");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory fragmentation handling should not leak memory");
    
    return true;
}

// Test memory pool statistics
static bool test_memory_pool_statistics(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Initial statistics
    MemoryPoolStats initial_stats = memory_pool_get_stats(pool);
    TEST_ASSERT_EQ(0, initial_stats.allocated_bytes, "Initial allocated bytes should be 0");
    TEST_ASSERT_EQ(0, initial_stats.allocation_count, "Initial allocation count should be 0");
    TEST_ASSERT_EQ(0, initial_stats.free_count, "Initial free count should be 0");
    
    // Perform some allocations
    void* ptr1 = memory_pool_alloc(pool, 100);
    void* ptr2 = memory_pool_alloc(pool, 200);
    void* ptr3 = memory_pool_alloc(pool, 300);
    
    MemoryPoolStats after_alloc_stats = memory_pool_get_stats(pool);
    TEST_ASSERT_EQ(3, after_alloc_stats.allocation_count, "Should have 3 allocations");
    TEST_ASSERT(after_alloc_stats.allocated_bytes >= 600, "Should have allocated at least 600 bytes");
    
    // Free some memory
    memory_pool_free(pool, ptr2);
    
    MemoryPoolStats after_free_stats = memory_pool_get_stats(pool);
    TEST_ASSERT_EQ(1, after_free_stats.free_count, "Should have 1 free operation");
    TEST_ASSERT(after_free_stats.allocated_bytes < after_alloc_stats.allocated_bytes, "Allocated bytes should decrease");
    
    // Test peak usage tracking
    TEST_ASSERT(after_alloc_stats.peak_allocated_bytes >= after_alloc_stats.allocated_bytes, "Peak should be at least current");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory pool statistics should not leak memory");
    
    return true;
}

// Test memory pool thread safety
static bool test_memory_pool_thread_safety(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Enable thread safety
    memory_pool_set_thread_safe(pool, true);
    TEST_ASSERT(memory_pool_is_thread_safe(pool), "Pool should be thread safe");
    
    // Test that allocations still work with thread safety enabled
    void* ptr1 = memory_pool_alloc(pool, 100);
    void* ptr2 = memory_pool_alloc(pool, 200);
    
    TEST_ASSERT_NOT_NULL(ptr1, "Thread-safe allocation should succeed");
    TEST_ASSERT_NOT_NULL(ptr2, "Thread-safe allocation should succeed");
    TEST_ASSERT_NEQ(ptr1, ptr2, "Thread-safe allocations should return different pointers");
    
    // Test concurrent access simulation (simplified)
    for (int i = 0; i < 100; i++) {
        void* ptr = memory_pool_alloc(pool, 50);
        TEST_ASSERT_NOT_NULL(ptr, "Concurrent allocation should succeed");
        memory_pool_free(pool, ptr);
    }
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory pool thread safety should not leak memory");
    
    return true;
}

// Test memory pool reallocation
static bool test_memory_reallocation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    MemoryPool* pool = memory_pool_create(1024 * 1024);
    TEST_ASSERT_NOT_NULL(pool, "Memory pool creation should succeed");
    
    // Initial allocation
    void* ptr = memory_pool_alloc(pool, 100);
    TEST_ASSERT_NOT_NULL(ptr, "Initial allocation should succeed");
    
    // Write some data
    memset(ptr, 0xAA, 100);
    
    // Reallocate to larger size
    void* new_ptr = memory_pool_realloc(pool, ptr, 200);
    TEST_ASSERT_NOT_NULL(new_ptr, "Reallocation to larger size should succeed");
    
    // Verify data is preserved
    bool data_preserved = true;
    for (int i = 0; i < 100; i++) {
        if (((uint8_t*)new_ptr)[i] != 0xAA) {
            data_preserved = false;
            break;
        }
    }
    TEST_ASSERT(data_preserved, "Data should be preserved during reallocation");
    
    // Reallocate to smaller size
    void* smaller_ptr = memory_pool_realloc(pool, new_ptr, 50);
    TEST_ASSERT_NOT_NULL(smaller_ptr, "Reallocation to smaller size should succeed");
    
    // Verify data is still preserved (first 50 bytes)
    data_preserved = true;
    for (int i = 0; i < 50; i++) {
        if (((uint8_t*)smaller_ptr)[i] != 0xAA) {
            data_preserved = false;
            break;
        }
    }
    TEST_ASSERT(data_preserved, "Data should be preserved during shrinking reallocation");
    
    memory_pool_destroy(pool);
    TEST_MEMORY_VERIFY("Memory reallocation should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t memory_pool_tests[] = {
    test_memory_pool_creation,
    test_memory_allocation,
    test_memory_alignment,
    test_memory_pool_exhaustion,
    test_memory_fragmentation,
    test_memory_pool_statistics,
    test_memory_pool_thread_safety,
    test_memory_reallocation
};

test_suite_t memory_test_suite = {
    .name = "Memory Pool Tests",
    .tests = memory_pool_tests,
    .test_count = sizeof(memory_pool_tests) / sizeof(memory_pool_tests[0])
};
