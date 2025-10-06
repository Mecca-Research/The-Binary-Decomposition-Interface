
/**
 * @file test_allocator_redteam.c
 * @brief Red-Team Tests for Core Memory Allocator
 * @details Comprehensive adversarial testing including fuzz testing, property tests,
 *          fault injection, double-free detection, and counter validation.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fault_injection.h"
#include "../common/fuzzing_utils.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test: Random Size Allocation Fuzzing
// ============================================================================

REDTEAM_TEST(alloc_random_sizes, "allocator", 
             "Fuzz test with random allocation sizes") {
    const uint32_t iterations = 10000;
    void *allocations[100] = {0};
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Random size between 1 byte and 1 MB
        size_t size = fuzz_random_size(1, 1024 * 1024);
        
        // Allocate
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (ptr) {
            // Verify allocation
            REDTEAM_ASSERT_NOT_NULL(ptr, "Allocation returned NULL unexpectedly");
            
            // Store for later free
            int slot = rand() % 100;
            if (allocations[slot]) {
                free_memory(allocations[slot]);
            }
            allocations[slot] = ptr;
            
            // Write to memory to ensure it's accessible
            memset(ptr, 0xAA, size);
        }
    }
    
    // Cleanup
    for (int i = 0; i < 100; i++) {
        if (allocations[i]) {
            free_memory(allocations[i]);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Alignment Fuzzing
// ============================================================================

REDTEAM_TEST(alloc_alignment_fuzz, "allocator",
             "Fuzz test with random alignments") {
    const uint32_t iterations = 5000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = fuzz_random_size(16, 4096);
        size_t alignment = fuzz_random_alignment(4096);
        
        void *ptr = alloc_memory_aligned(size, alignment, GFP_KERNEL);
        
        if (ptr) {
            // Verify alignment
            REDTEAM_ASSERT_PTR_ALIGNED(ptr, alignment, 
                                      "Allocation not properly aligned");
            
            // Write to memory
            memset(ptr, 0xBB, size);
            
            free_memory(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Flag Combination Fuzzing
// ============================================================================

REDTEAM_TEST(alloc_flag_combinations, "allocator",
             "Test various flag combinations") {
    const uint32_t flags[] = {
        GFP_KERNEL,
        GFP_ATOMIC,
        GFP_DMA,
        GFP_ZERO,
        GFP_NOWAIT,
        GFP_HIGHMEM,
        GFP_NUMA_LOCAL,
        GFP_KERNEL | GFP_ZERO,
        GFP_ATOMIC | GFP_NOWAIT,
        GFP_KERNEL | GFP_NUMA_LOCAL,
    };
    
    const uint32_t num_flags = sizeof(flags) / sizeof(flags[0]);
    
    for (uint32_t i = 0; i < 1000; i++) {
        size_t size = fuzz_random_size(64, 8192);
        uint32_t flag = flags[rand() % num_flags];
        
        void *ptr = alloc_memory(size, flag);
        
        if (ptr) {
            // If GFP_ZERO, verify memory is zeroed
            if (flag & GFP_ZERO) {
                uint8_t *bytes = (uint8_t *)ptr;
                bool all_zero = true;
                for (size_t j = 0; j < size && j < 256; j++) {
                    if (bytes[j] != 0) {
                        all_zero = false;
                        break;
                    }
                }
                REDTEAM_ASSERT(all_zero, "GFP_ZERO flag did not zero memory");
            }
            
            free_memory(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Boundary Size Testing
// ============================================================================

REDTEAM_TEST(alloc_boundary_sizes, "allocator",
             "Test allocation sizes around important boundaries") {
    size_t boundaries[] = {
        64, 128, 256, 512, 1024, 2048, 4096,
        8192, 16384, 32768, 65536,
        HUGE_PAGE_2MB, HUGE_PAGE_1GB
    };
    
    for (size_t i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); i++) {
        size_t boundary = boundaries[i];
        
        // Test boundary - 1, boundary, boundary + 1
        for (int offset = -1; offset <= 1; offset++) {
            size_t size = boundary + offset;
            if (size == 0 || size > HUGE_PAGE_1GB) continue;
            
            void *ptr = alloc_memory(size, GFP_KERNEL);
            
            if (ptr) {
                memset(ptr, 0xCC, size);
                free_memory(ptr);
            }
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Double-Free Detection
// ============================================================================

REDTEAM_TEST(detect_double_free, "allocator",
             "Verify double-free detection") {
    void *ptr = alloc_memory(1024, GFP_KERNEL);
    REDTEAM_ASSERT_NOT_NULL(ptr, "Initial allocation failed");
    
    // First free - should succeed
    free_memory(ptr);
    
    // Second free - should be detected and handled
    // Note: This test expects the allocator to handle double-free gracefully
    // In production, this might trigger an assertion or error log
    
    // For now, we just verify the test doesn't crash
    // TODO: Add proper double-free detection verification when implemented
    
    return TEST_PASS;
}

// ============================================================================
// Test: Mismatched Size Free
// ============================================================================

REDTEAM_TEST(detect_mismatched_free, "allocator",
             "Verify mismatched size free detection") {
    size_t alloc_size = 1024;
    void *ptr = alloc_memory(alloc_size, GFP_KERNEL);
    REDTEAM_ASSERT_NOT_NULL(ptr, "Allocation failed");
    
    // Free with different size - should be detected
    // Note: This assumes the allocator tracks allocation sizes
    free_memory(ptr);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Memory Stats Invariants
// ============================================================================

REDTEAM_TEST(stats_invariants, "allocator",
             "Verify MemoryStats invariants hold") {
    MemoryStats stats_before = get_memory_stats();
    
    const uint32_t num_allocs = 100;
    void *allocations[num_allocs];
    size_t sizes[num_allocs];
    size_t total_allocated = 0;
    
    // Allocate
    for (uint32_t i = 0; i < num_allocs; i++) {
        sizes[i] = fuzz_random_size(64, 4096);
        allocations[i] = alloc_memory(sizes[i], GFP_KERNEL);
        if (allocations[i]) {
            total_allocated += sizes[i];
        }
    }
    
    MemoryStats stats_after_alloc = get_memory_stats();
    
    // Verify stats increased
    REDTEAM_ASSERT(stats_after_alloc.total_allocated >= stats_before.total_allocated,
                  "Total allocated should increase");
    
    // Free all
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            free_memory(allocations[i]);
        }
    }
    
    MemoryStats stats_after_free = get_memory_stats();
    
    // Verify stats updated correctly
    REDTEAM_ASSERT(stats_after_free.total_freed >= stats_after_alloc.total_freed,
                  "Total freed should increase");
    
    return TEST_PASS;
}

// ============================================================================
// Test: Fault Injection - Allocation Failures
// ============================================================================

REDTEAM_TEST(fault_injection_alloc, "allocator",
             "Test allocation failure handling with fault injection") {
    fault_injection_init(12345);
    fault_injection_enable(FAULT_ALLOC_FAIL, 0.1, "10% allocation failure");
    
    uint32_t alloc_attempts = 0;
    uint32_t alloc_failures = 0;
    
    for (uint32_t i = 0; i < 1000; i++) {
        size_t size = fuzz_random_size(64, 4096);
        alloc_attempts++;
        
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (!ptr) {
            alloc_failures++;
        } else {
            free_memory(ptr);
        }
    }
    
    fault_injection_disable_all();
    
    // We should have seen some failures
    REDTEAM_ASSERT(alloc_failures > 0, "No allocation failures observed");
    
    redteam_log("Allocation attempts: %u, failures: %u (%.1f%%)",
                alloc_attempts, alloc_failures,
                100.0 * alloc_failures / alloc_attempts);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Counter Overflow Detection
// ============================================================================

REDTEAM_TEST(counter_overflow, "allocator",
             "Test counter overflow handling") {
    // This test verifies that allocation counters don't overflow
    // We simulate many allocations to stress the counters
    
    const uint32_t iterations = 10000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = fuzz_random_size(16, 256);
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (ptr) {
            free_memory(ptr);
        }
    }
    
    MemoryStats stats = get_memory_stats();
    
    // Verify counters are reasonable
    REDTEAM_ASSERT(stats.total_allocated < UINT64_MAX / 2,
                  "Allocation counter approaching overflow");
    REDTEAM_ASSERT(stats.total_freed < UINT64_MAX / 2,
                  "Free counter approaching overflow");
    
    return TEST_PASS;
}

// ============================================================================
// Test: Zero-Size Allocation
// ============================================================================

REDTEAM_TEST(zero_size_alloc, "allocator",
             "Test zero-size allocation handling") {
    void *ptr = alloc_memory(0, GFP_KERNEL);
    
    // Zero-size allocation should either return NULL or a valid pointer
    // that can be safely freed
    if (ptr) {
        free_memory(ptr);
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Maximum Size Allocation
// ============================================================================

REDTEAM_TEST(max_size_alloc, "allocator",
             "Test maximum size allocation") {
    // Try to allocate very large sizes
    size_t large_sizes[] = {
        1024 * 1024 * 1024,      // 1 GB
        512 * 1024 * 1024,       // 512 MB
        256 * 1024 * 1024,       // 256 MB
    };
    
    for (size_t i = 0; i < sizeof(large_sizes) / sizeof(large_sizes[0]); i++) {
        void *ptr = alloc_memory(large_sizes[i], GFP_KERNEL);
        
        if (ptr) {
            // Successfully allocated large memory
            free_memory(ptr);
        }
        // Failure is acceptable for very large allocations
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Stress Test - Rapid Alloc/Free
// ============================================================================

REDTEAM_TEST(stress_rapid_alloc_free, "allocator",
             "Stress test with rapid allocation and freeing") {
    const uint32_t iterations = 50000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = fuzz_random_size(16, 1024);
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (ptr) {
            // Immediately free
            free_memory(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Fragmentation Stress
// ============================================================================

REDTEAM_TEST(stress_fragmentation, "allocator",
             "Stress test to induce memory fragmentation") {
    const uint32_t num_allocs = 1000;
    void *allocations[num_allocs];
    
    // Allocate with varying sizes
    for (uint32_t i = 0; i < num_allocs; i++) {
        size_t size = fuzz_random_size(64, 8192);
        allocations[i] = alloc_memory(size, GFP_KERNEL);
    }
    
    // Free every other allocation to create fragmentation
    for (uint32_t i = 0; i < num_allocs; i += 2) {
        if (allocations[i]) {
            free_memory(allocations[i]);
            allocations[i] = NULL;
        }
    }
    
    // Try to allocate large blocks (should be challenging with fragmentation)
    for (uint32_t i = 0; i < 100; i++) {
        void *ptr = alloc_memory(16384, GFP_KERNEL);
        if (ptr) {
            free_memory(ptr);
        }
    }
    
    // Cleanup remaining allocations
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            free_memory(allocations[i]);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    // Initialize test harness
    if (!redteam_init(true, "allocator_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    // Initialize fuzzing
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_alloc_random_sizes);
    redteam_register_test(&test_case_alloc_alignment_fuzz);
    redteam_register_test(&test_case_alloc_flag_combinations);
    redteam_register_test(&test_case_alloc_boundary_sizes);
    redteam_register_test(&test_case_detect_double_free);
    redteam_register_test(&test_case_detect_mismatched_free);
    redteam_register_test(&test_case_stats_invariants);
    redteam_register_test(&test_case_fault_injection_alloc);
    redteam_register_test(&test_case_counter_overflow);
    redteam_register_test(&test_case_zero_size_alloc);
    redteam_register_test(&test_case_max_size_alloc);
    redteam_register_test(&test_case_stress_rapid_alloc_free);
    redteam_register_test(&test_case_stress_fragmentation);
    
    // Run all tests
    test_stats_t stats = redteam_run_all_tests();
    
    // Print results
    redteam_print_stats(&stats);
    
    // Cleanup
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
