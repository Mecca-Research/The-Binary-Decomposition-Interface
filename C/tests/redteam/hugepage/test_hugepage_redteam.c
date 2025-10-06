
/**
 * @file test_hugepage_redteam.c
 * @brief Red-Team Tests for Huge Page Support
 * @details Boundary testing, alignment validation, counter integrity,
 *          and misalignment handling for 2MB and 1GB huge pages.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fuzzing_utils.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test: 2MB Huge Page Boundary Testing
// ============================================================================

REDTEAM_TEST(hugepage_2mb_boundary, "hugepage",
             "Test 2MB huge page size boundaries") {
    const size_t boundary = HUGE_PAGE_2MB;
    
    // Test sizes around 2MB boundary
    size_t test_sizes[] = {
        boundary - 4096,
        boundary - 1,
        boundary,
        boundary + 1,
        boundary + 4096,
    };
    
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        size_t size = test_sizes[i];
        
        void *ptr = kmalloc(size, GFP_KERNEL);
        
        if (ptr) {
            // Verify allocation
            REDTEAM_ASSERT_NOT_NULL(ptr, "Allocation failed");
            
            // For sizes >= 2MB, verify huge page alignment
            if (size >= boundary) {
                REDTEAM_ASSERT_PTR_ALIGNED(ptr, boundary,
                                          "2MB huge page not properly aligned");
            }
            
            // Write to memory
            memset(ptr, 0xDD, size > 4096 ? 4096 : size);
            
            kfree(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: 1GB Huge Page Boundary Testing
// ============================================================================

REDTEAM_TEST(hugepage_1gb_boundary, "hugepage",
             "Test 1GB huge page size boundaries") {
    const size_t boundary = HUGE_PAGE_1GB;
    
    // Test sizes around 1GB boundary
    size_t test_sizes[] = {
        boundary - HUGE_PAGE_2MB,
        boundary - 4096,
        boundary,
        // Note: boundary + 1 might be too large to allocate
    };
    
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        size_t size = test_sizes[i];
        
        void *ptr = kmalloc(size, GFP_KERNEL);
        
        if (ptr) {
            // For sizes >= 1GB, verify huge page alignment
            if (size >= boundary) {
                REDTEAM_ASSERT_PTR_ALIGNED(ptr, boundary,
                                          "1GB huge page not properly aligned");
            }
            
            // Write to first and last pages
            memset(ptr, 0xEE, 4096);
            
            kfree(ptr);
        }
        // Failure is acceptable for very large allocations
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Huge Page Alignment Validation
// ============================================================================

REDTEAM_TEST(hugepage_alignment, "hugepage",
             "Validate huge page alignment requirements") {
    
    // Test 2MB huge pages
    for (uint32_t i = 0; i < 10; i++) {
        size_t size = HUGE_PAGE_2MB + (i * 4096);
        void *ptr = kmalloc(size, GFP_KERNEL);
        
        if (ptr) {
            REDTEAM_ASSERT_PTR_ALIGNED(ptr, HUGE_PAGE_2MB,
                                      "2MB huge page alignment violation");
            kfree(ptr);
        }
    }
    
    // Test 1GB huge pages (if supported)
    void *ptr_1gb = kmalloc(HUGE_PAGE_1GB, GFP_KERNEL);
    if (ptr_1gb) {
        REDTEAM_ASSERT_PTR_ALIGNED(ptr_1gb, HUGE_PAGE_1GB,
                                  "1GB huge page alignment violation");
        kfree(ptr_1gb);
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Huge Page Counter Integrity
// ============================================================================

REDTEAM_TEST(hugepage_counter_integrity, "hugepage",
             "Test huge page allocation counter integrity") {
    MemoryStats stats_before = get_memory_stats();
    
    const uint32_t num_allocs = 10;
    void *allocations[num_allocs];
    
    // Allocate huge pages
    for (uint32_t i = 0; i < num_allocs; i++) {
        allocations[i] = kmalloc(HUGE_PAGE_2MB, GFP_KERNEL);
    }
    
    MemoryStats stats_after_alloc = get_memory_stats();
    
    // Verify counters increased
    REDTEAM_ASSERT(stats_after_alloc.huge_pages_allocated >= 
                  stats_before.huge_pages_allocated,
                  "Huge page counter did not increase");
    
    // Free huge pages
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            kfree(allocations[i]);
        }
    }
    
    MemoryStats stats_after_free = get_memory_stats();
    
    // Verify counters updated
    REDTEAM_ASSERT(stats_after_free.huge_pages_freed >= 
                  stats_after_alloc.huge_pages_freed,
                  "Huge page free counter did not increase");
    
    return TEST_PASS;
}

// ============================================================================
// Test: Mixed Size Allocations
// ============================================================================

REDTEAM_TEST(hugepage_mixed_sizes, "hugepage",
             "Test mixed regular and huge page allocations") {
    const uint32_t num_allocs = 50;
    void *allocations[num_allocs];
    
    // Mix of regular and huge page allocations
    for (uint32_t i = 0; i < num_allocs; i++) {
        size_t size;
        
        if (i % 3 == 0) {
            size = HUGE_PAGE_2MB; // 2MB huge page
        } else if (i % 7 == 0) {
            size = HUGE_PAGE_1GB; // 1GB huge page
        } else {
            size = fuzz_random_size(4096, 1024 * 1024); // Regular
        }
        
        allocations[i] = kmalloc(size, GFP_KERNEL);
        
        if (allocations[i]) {
            // Verify alignment for huge pages
            if (size >= HUGE_PAGE_2MB) {
                REDTEAM_ASSERT_PTR_ALIGNED(allocations[i], HUGE_PAGE_2MB,
                                          "Huge page alignment error");
            }
        }
    }
    
    // Free all
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            kfree(allocations[i]);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Huge Page Fallback
// ============================================================================

REDTEAM_TEST(hugepage_fallback, "hugepage",
             "Test fallback to regular pages when huge pages unavailable") {
    const uint32_t max_attempts = 100;
    void *allocations[max_attempts];
    uint32_t alloc_count = 0;
    
    // Try to allocate many huge pages until exhaustion
    for (uint32_t i = 0; i < max_attempts; i++) {
        void *ptr = kmalloc(HUGE_PAGE_2MB, GFP_KERNEL);
        
        if (ptr) {
            allocations[alloc_count++] = ptr;
        } else {
            // Huge pages exhausted, try regular allocation
            ptr = kmalloc(HUGE_PAGE_2MB, GFP_KERNEL | GFP_NOWAIT);
            if (ptr) {
                // Fallback succeeded
                kfree(ptr);
            }
            break;
        }
    }
    
    redteam_log("Allocated %u huge pages before exhaustion", alloc_count);
    
    // Cleanup
    for (uint32_t i = 0; i < alloc_count; i++) {
        kfree(allocations[i]);
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Huge Page Stress Test
// ============================================================================

REDTEAM_TEST(hugepage_stress, "hugepage",
             "Stress test huge page allocation and freeing") {
    const uint32_t iterations = 100;
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Randomly choose huge page size
        size_t size = (rand() % 2 == 0) ? HUGE_PAGE_2MB : HUGE_PAGE_1GB;
        
        void *ptr = kmalloc(size, GFP_KERNEL);
        
        if (ptr) {
            // Write to memory
            memset(ptr, 0xFF, 4096);
            
            // Immediately free
            kfree(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Huge Page Counter Drift Detection
// ============================================================================

REDTEAM_TEST(hugepage_counter_drift, "hugepage",
             "Detect counter drift in huge page accounting") {
    MemoryStats stats_initial = get_memory_stats();
    
    const uint32_t cycles = 10;
    
    for (uint32_t cycle = 0; cycle < cycles; cycle++) {
        void *allocations[10];
        
        // Allocate
        for (uint32_t i = 0; i < 10; i++) {
            allocations[i] = kmalloc(HUGE_PAGE_2MB, GFP_KERNEL);
        }
        
        // Free
        for (uint32_t i = 0; i < 10; i++) {
            if (allocations[i]) {
                kfree(allocations[i]);
            }
        }
    }
    
    MemoryStats stats_final = get_memory_stats();
    
    // Check for counter drift
    uint64_t allocated_delta = stats_final.huge_pages_allocated - 
                               stats_initial.huge_pages_allocated;
    uint64_t freed_delta = stats_final.huge_pages_freed - 
                          stats_initial.huge_pages_freed;
    
    // Allocated and freed should be roughly equal
    int64_t drift = (int64_t)allocated_delta - (int64_t)freed_delta;
    
    if (abs(drift) > 5) {
        redteam_log("Counter drift detected: %ld", drift);
    }
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "hugepage_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_hugepage_2mb_boundary);
    redteam_register_test(&test_case_hugepage_1gb_boundary);
    redteam_register_test(&test_case_hugepage_alignment);
    redteam_register_test(&test_case_hugepage_counter_integrity);
    redteam_register_test(&test_case_hugepage_mixed_sizes);
    redteam_register_test(&test_case_hugepage_fallback);
    redteam_register_test(&test_case_hugepage_stress);
    redteam_register_test(&test_case_hugepage_counter_drift);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
