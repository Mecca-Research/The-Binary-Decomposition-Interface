#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "../../bdi_pipeline.h"

#define EPSILON 1e-9

// Test helper to compare floating point values
static int double_equals(double a, double b) {
    return fabs(a - b) < EPSILON;
}

// Test helper to run expression with both GC enabled and disabled
static int test_gc_vs_no_gc(const char* expression, double expected, const char* test_name) {
    printf("Test: %s\n", test_name);
    printf("  Expression: %s\n", expression);
    printf("  Expected: %.6f\n", expected);
    
    // Test with GC enabled (enhanced VM)
    PipelineConfig gc_config = pipeline_default_config();
    gc_config.enable_gc = true;
    gc_config.enable_optimization = false;  // Disable optimization for stability
    gc_config.verbose = false;
    
    PipelineResult gc_result = pipeline_run_with_config(expression, gc_config);
    
    // Test with GC disabled (basic VM)
    PipelineConfig no_gc_config = pipeline_default_config();
    no_gc_config.enable_gc = false;
    no_gc_config.enable_optimization = false;  // Disable optimization for stability
    no_gc_config.verbose = false;
    
    PipelineResult no_gc_result = pipeline_run_with_config(expression, no_gc_config);
    
    printf("  GC enabled result: %.6f (success: %d)\n", gc_result.result_value, gc_result.success);
    printf("  GC disabled result: %.6f (success: %d)\n", no_gc_result.result_value, no_gc_result.success);
    
    // Both should succeed
    if (!gc_result.success || !no_gc_result.success) {
        printf("  ❌ FAILED: One or both executions failed\n");
        return 0;
    }
    
    // Both should match expected value
    if (!double_equals(gc_result.result_value, expected) || 
        !double_equals(no_gc_result.result_value, expected)) {
        printf("  ❌ FAILED: Results don't match expected value\n");
        return 0;
    }
    
    // GC and non-GC results should be identical
    if (!double_equals(gc_result.result_value, no_gc_result.result_value)) {
        printf("  ❌ FAILED: GC and non-GC results differ\n");
        return 0;
    }
    
    printf("  ✅ PASSED: All results match\n");
    return 1;
}

int main(void) {
    printf("=== GC Result Capture Verification Tests ===\n\n");
    
    int passed = 0;
    int total = 0;
    
    // Test 1: Simple literal
    total++;
    if (test_gc_vs_no_gc("42;", 42.0, "gc_result_simple")) {
        passed++;
    }
    printf("\n");
    
    // Test 2: Simple arithmetic
    total++;
    if (test_gc_vs_no_gc("2 + 3 * 4;", 14.0, "gc_result_arithmetic")) {
        passed++;
    }
    printf("\n");
    
    // Test 3: Complex expression
    total++;
    if (test_gc_vs_no_gc("(5 + 3) * 2 - 1;", 15.0, "gc_result_complex")) {
        passed++;
    }
    printf("\n");
    
    // Test 4: Float operations
    total++;
    if (test_gc_vs_no_gc("3.14 * 2;", 6.28, "gc_result_float")) {
        passed++;
    }
    printf("\n");
    
    // Test 5: Division
    total++;
    if (test_gc_vs_no_gc("15 / 3;", 5.0, "gc_result_division")) {
        passed++;
    }
    printf("\n");
    
    // Test 6: Nested parentheses
    total++;
    if (test_gc_vs_no_gc("((2 + 3) * (4 - 1));", 15.0, "gc_result_nested")) {
        passed++;
    }
    printf("\n");
    
    // Test 7: Zero result
    total++;
    if (test_gc_vs_no_gc("5 - 5;", 0.0, "gc_result_zero")) {
        passed++;
    }
    printf("\n");
    
    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", total - passed);
    
    if (passed == total) {
        printf("\n✅ All GC result capture tests passed!\n");
        printf("✅ GC and non-GC paths return identical results!\n");
        return 0;
    } else {
        printf("\n❌ Some GC result capture tests failed!\n");
        return 1;
    }
}
