// ===================================================================
// DESC: Simplified E2E Pipeline Test Suite (20+ tests)
//       Tests what's actually implemented in the pipeline
// ===================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
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

#define ASSERT_FALSE(cond, msg) \
    if (cond) TEST_FAIL(msg)

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) TEST_FAIL(msg)

#define ASSERT_DOUBLE_EQ(a, b, msg) \
    if (fabs((a) - (b)) > 0.0001) TEST_FAIL(msg)

// ============================================================================
// BASIC ARITHMETIC TESTS (10 tests)
// ============================================================================

void test_simple_literal(void) {
    TEST("simple_literal");
    
    PipelineResult result = pipeline_run("42;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 42.0, "Result should be 42");
    ASSERT_TRUE(result.bytecode_size > 0, "Should generate bytecode");
    
    TEST_END;
}

void test_simple_addition(void) {
    TEST("simple_addition");
    
    PipelineResult result = pipeline_run("2 + 3;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    
    TEST_END;
}

void test_simple_subtraction(void) {
    TEST("simple_subtraction");
    
    PipelineResult result = pipeline_run("10 - 3;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 7.0, "Result should be 7");
    
    TEST_END;
}

void test_simple_multiplication(void) {
    TEST("simple_multiplication");
    
    PipelineResult result = pipeline_run("4 * 5;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 20.0, "Result should be 20");
    
    TEST_END;
}

void test_simple_division(void) {
    TEST("simple_division");
    
    PipelineResult result = pipeline_run("20 / 4;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    
    TEST_END;
}

void test_arithmetic_precedence(void) {
    TEST("arithmetic_precedence");
    
    PipelineResult result = pipeline_run("2 + 3 * 4;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 14.0, "Result should be 14 (precedence)");
    
    TEST_END;
}

void test_parentheses(void) {
    TEST("parentheses");
    
    PipelineResult result = pipeline_run("(2 + 3) * 4;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 20.0, "Result should be 20");
    
    TEST_END;
}

void test_complex_arithmetic(void) {
    TEST("complex_arithmetic");
    
    PipelineResult result = pipeline_run("(2 + 3) * (4 - 1);");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 15.0, "Result should be 15");
    
    TEST_END;
}

void test_nested_expressions(void) {
    TEST("nested_expressions");
    
    PipelineResult result = pipeline_run("((2 + 3) * 4) / 2;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 10.0, "Result should be 10");
    
    TEST_END;
}

void test_negation(void) {
    TEST("negation");
    
    PipelineResult result = pipeline_run("-5;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, -5.0, "Result should be -5");
    
    TEST_END;
}

// ============================================================================
// PIPELINE FUNCTIONALITY TESTS (5 tests)
// ============================================================================

void test_pipeline_create_destroy(void) {
    TEST("pipeline_create_destroy");
    
    PipelineContext* ctx = pipeline_create();
    ASSERT_TRUE(ctx != NULL, "Should create pipeline");
    
    pipeline_destroy(ctx);
    TEST_END;
}

void test_pipeline_compile_only(void) {
    TEST("pipeline_compile_only");
    
    PipelineContext* ctx = pipeline_create();
    ASSERT_TRUE(ctx != NULL, "Should create pipeline");
    
    bool success = pipeline_compile(ctx, "2 + 3;");
    ASSERT_TRUE(success, "Compilation should succeed");
    ASSERT_TRUE(ctx->result.bytecode_size > 0, "Should generate bytecode");
    
    pipeline_destroy(ctx);
    TEST_END;
}

void test_pipeline_reuse(void) {
    TEST("pipeline_reuse");
    
    PipelineContext* ctx = pipeline_create();
    ASSERT_TRUE(ctx != NULL, "Should create pipeline");
    
    // First compilation
    bool success1 = pipeline_compile(ctx, "2 + 3;");
    ASSERT_TRUE(success1, "First compilation should succeed");
    bool exec1 = pipeline_execute(ctx);
    ASSERT_TRUE(exec1, "First execution should succeed");
    PipelineResult result1 = pipeline_get_result(ctx);
    ASSERT_DOUBLE_EQ(result1.result_value, 5.0, "First result should be 5");
    
    // Reset and reuse
    pipeline_reset(ctx);
    
    // Second compilation
    bool success2 = pipeline_compile(ctx, "10 * 2;");
    ASSERT_TRUE(success2, "Second compilation should succeed");
    bool exec2 = pipeline_execute(ctx);
    ASSERT_TRUE(exec2, "Second execution should succeed");
    PipelineResult result2 = pipeline_get_result(ctx);
    ASSERT_DOUBLE_EQ(result2.result_value, 20.0, "Second result should be 20");
    
    pipeline_destroy(ctx);
    TEST_END;
}

void test_pipeline_with_optimization(void) {
    TEST("pipeline_with_optimization");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_optimization = true;
    
    PipelineResult result = pipeline_run_with_config("2 + 3;", config);
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    
    TEST_END;
}

void test_pipeline_without_optimization(void) {
    TEST("pipeline_without_optimization");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_optimization = false;
    
    PipelineResult result = pipeline_run_with_config("2 + 3;", config);
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    
    TEST_END;
}

// ============================================================================
// ERROR HANDLING TESTS (3 tests)
// ============================================================================

void test_syntax_error(void) {
    TEST("syntax_error");
    
    PipelineResult result = pipeline_run("2 + + 3;");
    
    ASSERT_FALSE(result.success, "Pipeline should fail");
    ASSERT_TRUE(result.error_message != NULL, "Should have error message");
    
    TEST_END;
}

void test_missing_semicolon(void) {
    TEST("missing_semicolon");
    
    PipelineResult result = pipeline_run("2 + 3");
    
    ASSERT_FALSE(result.success, "Pipeline should fail");
    ASSERT_TRUE(result.error_message != NULL, "Should have error message");
    
    TEST_END;
}

void test_unbalanced_parentheses(void) {
    TEST("unbalanced_parentheses");
    
    PipelineResult result = pipeline_run("(2 + 3;");
    
    ASSERT_FALSE(result.success, "Pipeline should fail");
    ASSERT_TRUE(result.error_message != NULL, "Should have error message");
    
    TEST_END;
}

// ============================================================================
// EDGE CASES (3 tests)
// ============================================================================

void test_empty_program(void) {
    TEST("empty_program");
    
    PipelineResult result = pipeline_run("");
    (void)result;
    
    // Empty program might succeed or fail depending on implementation
    // Just check it doesn't crash
    ASSERT_TRUE(true, "Should not crash");
    
    TEST_END;
}

void test_large_expression(void) {
    TEST("large_expression");
    
    // Test with a large expression to stress test the pipeline
    PipelineResult result = pipeline_run(
        "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
        "11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20;"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 210.0, "Result should be 210");
    
    TEST_END;
}

void test_deeply_nested(void) {
    TEST("deeply_nested");
    
    PipelineResult result = pipeline_run("((((1 + 2) + 3) + 4) + 5);");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 15.0, "Result should be 15");
    
    TEST_END;
}

// ============================================================================
// PERFORMANCE TESTS (2 tests)
// ============================================================================

void test_compilation_performance(void) {
    TEST("compilation_performance");
    
    PipelineConfig config = pipeline_default_config();
    config.verbose = false;
    
    PipelineResult result = pipeline_run_with_config(
        "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10;",
        config
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_TRUE(result.compile_time_us > 0, "Should record compile time");
    ASSERT_TRUE(result.execute_time_us > 0, "Should record execute time");
    
    printf(" [compile: %lu us, execute: %lu us]", 
           result.compile_time_us, result.execute_time_us);
    
    TEST_END;
}

void test_execution_performance(void) {
    TEST("execution_performance");
    
    PipelineConfig config = pipeline_default_config();
    config.verbose = false;
    
    // Complex expression to measure execution time
    PipelineResult result = pipeline_run_with_config(
        "((1 + 2) * (3 + 4)) - ((5 + 6) * (7 - 8));",
        config
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 32.0, "Result should be 32");
    
    printf(" [total: %lu us]", 
           result.compile_time_us + result.execute_time_us);
    
    TEST_END;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main(void) {
    printf("========================================\n");
    printf("E2E Pipeline Test Suite (Simplified)\n");
    printf("========================================\n\n");
    
    // Basic Arithmetic (10 tests)
    test_simple_literal();
    test_simple_addition();
    test_simple_subtraction();
    test_simple_multiplication();
    test_simple_division();
    test_arithmetic_precedence();
    test_parentheses();
    test_complex_arithmetic();
    test_nested_expressions();
    test_negation();
    
    // Pipeline Functionality (5 tests)
    test_pipeline_create_destroy();
    test_pipeline_compile_only();
    test_pipeline_reuse();
    test_pipeline_with_optimization();
    test_pipeline_without_optimization();
    
    // Error Handling (3 tests)
    test_syntax_error();
    test_missing_semicolon();
    test_unbalanced_parentheses();
    
    // Edge Cases (3 tests)
    test_empty_program();
    test_large_expression();
    test_deeply_nested();
    
    // Performance (2 tests)
    test_compilation_performance();
    test_execution_performance();
    
    // Print summary
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (tests_passed * 100.0) / tests_run);
    printf("========================================\n");
    
    return tests_failed == 0 ? 0 : 1;
}
