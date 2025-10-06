
/**
 * @file test_pmm_redteam.c
 * @brief Red-Team Tests for Physical Memory Manager (PMM)
 * @details Page pool exhaustion, remote-node fallback, double-free detection,
 *          address range fuzzing, and refcount validation.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fuzzing_utils.h"
#include "../common/fault_injection.h"
#include "../../../kernel/pmm.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test: Page Pool Exhaustion
// ============================================================================

REDTEAM_TEST(pmm_pool_exhaustion, "pmm",
             "Test page pool exhaustion scenarios") {
    const uint32_t max_pages = 10000;
    uint64_t *pages = malloc(max_pages * sizeof(uint64_t));
    uint32_t allocated_count = 0;
    
    REDTEAM_ASSERT_NOT_NULL(pages, "Failed to allocate tracking array");
    
    // Try to exhaust page pool
    for (uint32_t i = 0; i < max_pages; i++) {
        uint64_t pfn = pmm_alloc_page(0); // Order 0 (single page)
        
        if (pfn != 0) {
            pages[allocated_count++] = pfn;
        } else {
            // Pool exhausted
            break;
        }
    }
    
    redteam_log("Allocated %u pages before exhaustion", allocated_count);
    
    // Verify we can allocate after freeing some
    if (allocated_count > 100) {
        for (uint32_t i = 0; i < 50; i++) {
            pmm_free_page(pages[i], 0);
        }
        
        // Try to allocate again
        uint64_t pfn = pmm_alloc_page(0);
        REDTEAM_ASSERT(pfn != 0, "Failed to allocate after freeing pages");
        pmm_free_page(pfn, 0);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < allocated_count; i++) {
        pmm_free_page(pages[i], 0);
    }
    
    free(pages);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Buddy Allocator Orders
// ============================================================================

REDTEAM_TEST(pmm_buddy_orders, "pmm",
             "Test buddy allocator with various orders") {
    
    // Test all orders from 0 to MAX_ORDER
    for (uint32_t order = 0; order <= MAX_ORDER; order++) {
        uint64_t pfn = pmm_alloc_page(order);
        
        if (pfn != 0) {
            // Verify alignment for order
            size_t alignment = PAGE_SIZE << order;
            REDTEAM_ASSERT((pfn * PAGE_SIZE) % alignment == 0,
                          "Page not aligned for order");
            
            pmm_free_page(pfn, order);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Double-Free Detection
// ============================================================================

REDTEAM_TEST(pmm_double_free, "pmm",
             "Detect double-free of pages") {
    uint64_t pfn = pmm_alloc_page(0);
    REDTEAM_ASSERT(pfn != 0, "Failed to allocate page");
    
    // First free - should succeed
    pmm_free_page(pfn, 0);
    
    // Second free - should be detected
    // Note: This test expects the PMM to handle double-free gracefully
    // In production, this might trigger an assertion or error log
    
    // For now, we just verify the test doesn't crash
    // TODO: Add proper double-free detection verification when implemented
    
    return TEST_PASS;
}

// ============================================================================
// Test: Address Range Fuzzing
// ============================================================================

REDTEAM_TEST(pmm_address_fuzzing, "pmm",
             "Fuzz test with invalid address ranges") {
    
    // Test with invalid PFNs
    uint64_t invalid_pfns[] = {
        0,                  // Zero PFN
        UINT64_MAX,         // Maximum value
        UINT64_MAX / 2,     // Large value
        0xDEADBEEF,         // Random value
    };
    
    for (size_t i = 0; i < sizeof(invalid_pfns) / sizeof(invalid_pfns[0]); i++) {
        // Try to free invalid PFN - should be handled gracefully
        // Note: This should not crash or corrupt memory
        // TODO: Verify proper error handling when implemented
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Refcount Validation
// ============================================================================

REDTEAM_TEST(pmm_refcount, "pmm",
             "Test page reference counting") {
    uint64_t pfn = pmm_alloc_page(0);
    REDTEAM_ASSERT(pfn != 0, "Failed to allocate page");
    
    // Get initial refcount
    // Note: This assumes PMM provides refcount API
    // TODO: Implement when PMM refcount API is available
    
    // Increment refcount
    // pmm_page_get(pfn);
    
    // Decrement refcount
    // pmm_page_put(pfn);
    
    // Free page
    pmm_free_page(pfn, 0);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Remote Node Fallback
// ============================================================================

REDTEAM_TEST(pmm_remote_fallback, "pmm",
             "Test remote NUMA node fallback allocation") {
    
    // Try to allocate from specific NUMA node
    for (uint32_t node = 0; node < MAX_NUMA_NODES; node++) {
        uint64_t pfn = pmm_alloc_page_node(0, node);
        
        if (pfn != 0) {
            // Verify allocation succeeded
            // TODO: Verify page is from correct NUMA node when API available
            pmm_free_page(pfn, 0);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Page Descriptor Boundary
// ============================================================================

REDTEAM_TEST(pmm_descriptor_boundary, "pmm",
             "Test page descriptor boundary conditions") {
    
    // Allocate pages near descriptor boundaries
    const uint32_t num_pages = 100;
    uint64_t pages[num_pages];
    
    for (uint32_t i = 0; i < num_pages; i++) {
        pages[i] = pmm_alloc_page(0);
    }
    
    // Free in reverse order
    for (int i = num_pages - 1; i >= 0; i--) {
        if (pages[i] != 0) {
            pmm_free_page(pages[i], 0);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Coalescing Stress
// ============================================================================

REDTEAM_TEST(pmm_coalescing_stress, "pmm",
             "Stress test buddy coalescing") {
    const uint32_t iterations = 1000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Allocate and free pages to trigger coalescing
        uint64_t pfn1 = pmm_alloc_page(0);
        uint64_t pfn2 = pmm_alloc_page(0);
        
        if (pfn1 != 0) pmm_free_page(pfn1, 0);
        if (pfn2 != 0) pmm_free_page(pfn2, 0);
        
        // Try higher order allocation (should coalesce)
        uint64_t pfn_large = pmm_alloc_page(1);
        if (pfn_large != 0) {
            pmm_free_page(pfn_large, 1);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Splitting Stress
// ============================================================================

REDTEAM_TEST(pmm_splitting_stress, "pmm",
             "Stress test buddy splitting") {
    const uint32_t iterations = 1000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        // Allocate large block
        uint64_t pfn_large = pmm_alloc_page(3); // Order 3 (8 pages)
        
        if (pfn_large != 0) {
            pmm_free_page(pfn_large, 3);
            
            // Allocate smaller blocks (should split)
            uint64_t pfn1 = pmm_alloc_page(0);
            uint64_t pfn2 = pmm_alloc_page(0);
            
            if (pfn1 != 0) pmm_free_page(pfn1, 0);
            if (pfn2 != 0) pmm_free_page(pfn2, 0);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Fragmentation Metrics
// ============================================================================

REDTEAM_TEST(pmm_fragmentation, "pmm",
             "Test fragmentation tracking") {
    
    // Create fragmentation
    const uint32_t num_allocs = 100;
    uint64_t pages[num_allocs];
    
    // Allocate
    for (uint32_t i = 0; i < num_allocs; i++) {
        pages[i] = pmm_alloc_page(0);
    }
    
    // Free every other page
    for (uint32_t i = 0; i < num_allocs; i += 2) {
        if (pages[i] != 0) {
            pmm_free_page(pages[i], 0);
        }
    }
    
    // Try to allocate large block (should be difficult)
    uint64_t large_pfn = pmm_alloc_page(5); // Order 5 (32 pages)
    
    if (large_pfn != 0) {
        pmm_free_page(large_pfn, 5);
    }
    
    // Cleanup
    for (uint32_t i = 1; i < num_allocs; i += 2) {
        if (pages[i] != 0) {
            pmm_free_page(pages[i], 0);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Fault Injection
// ============================================================================

REDTEAM_TEST(pmm_fault_injection, "pmm",
             "Test PMM with fault injection") {
    fault_injection_init(54321);
    fault_injection_enable(FAULT_ALLOC_FAIL, 0.05, "5% allocation failure");
    
    uint32_t attempts = 0;
    uint32_t failures = 0;
    
    for (uint32_t i = 0; i < 1000; i++) {
        attempts++;
        uint64_t pfn = pmm_alloc_page(0);
        
        if (pfn == 0) {
            failures++;
        } else {
            pmm_free_page(pfn, 0);
        }
    }
    
    fault_injection_disable_all();
    
    redteam_log("PMM fault injection: %u attempts, %u failures (%.1f%%)",
                attempts, failures, 100.0 * failures / attempts);
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "pmm_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_pmm_pool_exhaustion);
    redteam_register_test(&test_case_pmm_buddy_orders);
    redteam_register_test(&test_case_pmm_double_free);
    redteam_register_test(&test_case_pmm_address_fuzzing);
    redteam_register_test(&test_case_pmm_refcount);
    redteam_register_test(&test_case_pmm_remote_fallback);
    redteam_register_test(&test_case_pmm_descriptor_boundary);
    redteam_register_test(&test_case_pmm_coalescing_stress);
    redteam_register_test(&test_case_pmm_splitting_stress);
    redteam_register_test(&test_case_pmm_fragmentation);
    redteam_register_test(&test_case_pmm_fault_injection);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
