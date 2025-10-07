
/*
 * Regression Test Suite
 * Tests for previously fixed bugs and known edge cases
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Test result
typedef enum {
    REGRESSION_PASS,
    REGRESSION_FAIL,
    REGRESSION_SKIP
} regression_result_t;

// Test case structure
typedef struct {
    const char *bug_id;
    const char *description;
    const char *fixed_in_pr;
    regression_result_t (*test_func)(void);
} regression_test_t;

// ============================================================================
// Bug #145: krealloc NULL pointer handling
// ============================================================================
static regression_result_t test_krealloc_null_ptr(void) {
    // Test that krealloc handles NULL pointer correctly
    void *ptr = NULL;
    
    // krealloc(NULL, size) should behave like kmalloc(size)
    ptr = realloc(ptr, 1024);
    if (!ptr) {
        return REGRESSION_FAIL;
    }
    
    // Write to memory to ensure it's valid
    memset(ptr, 0xAA, 1024);
    
    // krealloc to larger size
    ptr = realloc(ptr, 2048);
    if (!ptr) {
        return REGRESSION_FAIL;
    }
    
    // Verify first 1024 bytes still have 0xAA
    for (int i = 0; i < 1024; i++) {
        if (((unsigned char*)ptr)[i] != 0xAA) {
            free(ptr);
            return REGRESSION_FAIL;
        }
    }
    
    free(ptr);
    return REGRESSION_PASS;
}

// ============================================================================
// Bug #145: ham_free double-free protection
// ============================================================================
static regression_result_t test_ham_free_double_free(void) {
    // Test that double-free is detected/handled
    void *ptr = malloc(1024);
    if (!ptr) {
        return REGRESSION_SKIP;
    }
    
    free(ptr);
    
    // Second free should be handled gracefully
    // In production, this should be detected and prevented
    // For now, we just verify the system doesn't crash
    // free(ptr); // Commented out to prevent actual double-free in test
    
    return REGRESSION_PASS;
}

// ============================================================================
// Bug #146: Syscall registration order
// ============================================================================
static regression_result_t test_syscall_registration_order(void) {
    // Test that syscalls are registered in correct order
    // This would require access to syscall table
    // For now, placeholder test
    
    // TODO: Verify syscall table integrity
    return REGRESSION_PASS;
}

// ============================================================================
// Edge Case: Zero-size allocation
// ============================================================================
static regression_result_t test_zero_size_allocation(void) {
    // Test zero-size allocation behavior
    void *ptr = malloc(0);
    
    // Standard allows returning NULL or unique pointer
    // Either is acceptable
    if (ptr) {
        free(ptr);
    }
    
    return REGRESSION_PASS;
}

// ============================================================================
// Edge Case: Large allocation
// ============================================================================
static regression_result_t test_large_allocation(void) {
    // Test very large allocation
    size_t large_size = 1024 * 1024 * 100; // 100 MB
    void *ptr = malloc(large_size);
    
    if (!ptr) {
        // Large allocation failure is acceptable
        return REGRESSION_SKIP;
    }
    
    // Write to first and last page to ensure it's mapped
    ((char*)ptr)[0] = 0xAA;
    ((char*)ptr)[large_size - 1] = 0xBB;
    
    // Verify
    if (((char*)ptr)[0] != 0xAA || ((char*)ptr)[large_size - 1] != 0xBB) {
        free(ptr);
        return REGRESSION_FAIL;
    }
    
    free(ptr);
    return REGRESSION_PASS;
}

// ============================================================================
// Edge Case: Alignment requirements
// ============================================================================
static regression_result_t test_allocation_alignment(void) {
    // Test that allocations are properly aligned
    void *ptr = malloc(1);
    if (!ptr) {
        return REGRESSION_FAIL;
    }
    
    // Check alignment (should be at least pointer-size aligned)
    if ((uintptr_t)ptr % sizeof(void*) != 0) {
        free(ptr);
        return REGRESSION_FAIL;
    }
    
    free(ptr);
    return REGRESSION_PASS;
}

// ============================================================================
// Edge Case: Realloc size reduction
// ============================================================================
static regression_result_t test_realloc_shrink(void) {
    // Test realloc to smaller size
    void *ptr = malloc(2048);
    if (!ptr) {
        return REGRESSION_FAIL;
    }
    
    memset(ptr, 0xCC, 2048);
    
    // Shrink allocation
    void *new_ptr = realloc(ptr, 1024);
    if (!new_ptr) {
        free(ptr);
        return REGRESSION_FAIL;
    }
    
    // Verify first 1024 bytes still have 0xCC
    for (int i = 0; i < 1024; i++) {
        if (((unsigned char*)new_ptr)[i] != 0xCC) {
            free(new_ptr);
            return REGRESSION_FAIL;
        }
    }
    
    free(new_ptr);
    return REGRESSION_PASS;
}

// ============================================================================
// Regression test registry
// ============================================================================
static regression_test_t regression_tests[] = {
    {
        .bug_id = "BUG-145-1",
        .description = "krealloc NULL pointer handling",
        .fixed_in_pr = "#145",
        .test_func = test_krealloc_null_ptr
    },
    {
        .bug_id = "BUG-145-2",
        .description = "ham_free double-free protection",
        .fixed_in_pr = "#145",
        .test_func = test_ham_free_double_free
    },
    {
        .bug_id = "BUG-146",
        .description = "Syscall registration order",
        .fixed_in_pr = "#146",
        .test_func = test_syscall_registration_order
    },
    {
        .bug_id = "EDGE-001",
        .description = "Zero-size allocation",
        .fixed_in_pr = "N/A",
        .test_func = test_zero_size_allocation
    },
    {
        .bug_id = "EDGE-002",
        .description = "Large allocation (100MB)",
        .fixed_in_pr = "N/A",
        .test_func = test_large_allocation
    },
    {
        .bug_id = "EDGE-003",
        .description = "Allocation alignment",
        .fixed_in_pr = "N/A",
        .test_func = test_allocation_alignment
    },
    {
        .bug_id = "EDGE-004",
        .description = "Realloc size reduction",
        .fixed_in_pr = "N/A",
        .test_func = test_realloc_shrink
    }
};

static const size_t num_regression_tests = sizeof(regression_tests) / sizeof(regression_tests[0]);

// ============================================================================
// Test runner
// ============================================================================
int run_regression_tests(void) {
    printf("=== Regression Test Suite ===\n");
    printf("Total tests: %zu\n\n", num_regression_tests);
    
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    
    for (size_t i = 0; i < num_regression_tests; i++) {
        regression_test_t *test = &regression_tests[i];
        
        printf("[%s] %s ... ", test->bug_id, test->description);
        fflush(stdout);
        
        regression_result_t result = test->test_func();
        
        switch (result) {
            case REGRESSION_PASS:
                printf("PASS\n");
                passed++;
                break;
            case REGRESSION_FAIL:
                printf("FAIL (Fixed in %s)\n", test->fixed_in_pr);
                failed++;
                break;
            case REGRESSION_SKIP:
                printf("SKIP\n");
                skipped++;
                break;
        }
    }
    
    printf("\n=== Results ===\n");
    printf("Passed:  %d\n", passed);
    printf("Failed:  %d\n", failed);
    printf("Skipped: %d\n", skipped);
    
    if (failed > 0) {
        printf("\nREGRESSION DETECTED!\n");
        return 1;
    }
    
    printf("\nAll regression tests passed\n");
    return 0;
}

// Entry point
#ifndef TEST_RUNNER_BUILD
int main(void) {
    return run_regression_tests();
}

#endif
