
// ===================================================================
// DESC: Result Capture Verification Tests
//       Tests that verify the VM correctly captures and returns
//       computed result values through the pipeline
// ===================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../../bdi_pipeline.h"

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Test %d: %s...", tests_run, name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        tests_failed++; \
        printf(" FAILED: %s\n", msg); \
        return; \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) TEST_FAIL(msg)

#define ASSERT_DOUBLE_EQ(a, b, msg) \
    if (fabs((a) - (b)) > 0.0001) { \
        char buf[256]; \
        snprintf(buf, sizeof(buf), "%s (expected %.6f, got %.6f)", msg, (double)(b), (double)(a)); \
        TEST_FAIL(buf); \
    }

// Helper to run pipeline with result capture enabled
static inline PipelineResult run_pipeline_helper(const char* source) {
    PipelineConfig config = pipeline_default_config();
    config.enable_gc = false;
    config.enable_optimization = false;
    return pipeline_run_with_config(source, config);
}

#define RUN_PIPELINE(source) run_pipeline_helper(source)

// ============================================================================
// RESULT CAPTURE TESTS
// ============================================================================

void test_result_simple_literal(void) {
    TEST("result_simple_literal");
    PipelineResult result = RUN_PIPELINE("42;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 42.0, "Result should be 42");
    TEST_END;
}

void test_result_addition(void) {
    TEST("result_addition");
    PipelineResult result = RUN_PIPELINE("2 + 3;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    TEST_END;
}

void test_result_multiplication(void) {
    TEST("result_multiplication");
    PipelineResult result = RUN_PIPELINE("4 * 5;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 20.0, "Result should be 20");
    TEST_END;
}

void test_result_precedence(void) {
    TEST("result_precedence");
    PipelineResult result = RUN_PIPELINE("2 + 3 * 4;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 14.0, "Result should be 14 (precedence: 2 + 12)");
    TEST_END;
}

void test_result_parentheses(void) {
    TEST("result_parentheses");
    PipelineResult result = RUN_PIPELINE("(2 + 3) * 4;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 20.0, "Result should be 20 (parentheses: 5 * 4)");
    TEST_END;
}

void test_result_complex_expression(void) {
    TEST("result_complex_expression");
    PipelineResult result = RUN_PIPELINE("10 - 2 * 3 + 4;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 8.0, "Result should be 8 (10 - 6 + 4)");
    TEST_END;
}

void test_result_division(void) {
    TEST("result_division");
    PipelineResult result = RUN_PIPELINE("20 / 4;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    TEST_END;
}

void test_result_subtraction(void) {
    TEST("result_subtraction");
    PipelineResult result = RUN_PIPELINE("10 - 3;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 7.0, "Result should be 7");
    TEST_END;
}

void test_result_nested_parentheses(void) {
    TEST("result_nested_parentheses");
    PipelineResult result = RUN_PIPELINE("((2 + 3) * 4) / 2;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 10.0, "Result should be 10 ((5 * 4) / 2)");
    TEST_END;
}

void test_result_negation(void) {
    TEST("result_negation");
    PipelineResult result = RUN_PIPELINE("-5 + 3;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, -2.0, "Result should be -2");
    TEST_END;
}

void test_result_zero(void) {
    TEST("result_zero");
    PipelineResult result = RUN_PIPELINE("5 - 5;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 0.0, "Result should be 0");
    TEST_END;
}

void test_result_floating_point(void) {
    TEST("result_floating_point");
    PipelineResult result = RUN_PIPELINE("3.14 * 2;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 6.28, "Result should be 6.28");
    TEST_END;
}

void test_result_large_expression(void) {
    TEST("result_large_expression");
    PipelineResult result = RUN_PIPELINE("1 + 2 + 3 + 4 + 5;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 15.0, "Result should be 15");
    TEST_END;
}

void test_result_mixed_operations(void) {
    TEST("result_mixed_operations");
    PipelineResult result = RUN_PIPELINE("100 / 5 + 3 * 2 - 4;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 22.0, "Result should be 22 (20 + 6 - 4)");
    TEST_END;
}

void test_result_single_number(void) {
    TEST("result_single_number");
    PipelineResult result = RUN_PIPELINE("123.456;");
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 123.456, "Result should be 123.456");
    TEST_END;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== Result Capture Verification Tests ===\n\n");
    
    // Run all tests
    test_result_simple_literal();
    test_result_addition();
    test_result_multiplication();
    test_result_precedence();
    test_result_parentheses();
    test_result_complex_expression();
    test_result_division();
    test_result_subtraction();
    test_result_nested_parentheses();
    test_result_negation();
    test_result_zero();
    test_result_floating_point();
    test_result_large_expression();
    test_result_mixed_operations();
    test_result_single_number();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}
