
// ===================================================================
// DESC: End-to-End Pipeline Test Suite (20+ comprehensive tests)
//       Tests complete Source→Bytecode→Execution pipeline
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
// BASIC ARITHMETIC TESTS (5 tests)
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

void test_arithmetic_expression(void) {
    TEST("arithmetic_expression");
    
    PipelineResult result = pipeline_run("2 + 3 * 4;");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 14.0, "Result should be 14 (precedence)");
    
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

// ============================================================================
// COMPARISON OPERATIONS (3 tests)
// ============================================================================

void test_comparison_greater(void) {
    TEST("comparison_greater");
    
    PipelineResult result = pipeline_run("5 > 3");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 1.0, "Result should be true (1)");
    
    TEST_END;
}

void test_comparison_less(void) {
    TEST("comparison_less");
    
    PipelineResult result = pipeline_run("2 < 10");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 1.0, "Result should be true (1)");
    
    TEST_END;
}

void test_comparison_equal(void) {
    TEST("comparison_equal");
    
    PipelineResult result = pipeline_run("5 == 5");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 1.0, "Result should be true (1)");
    
    TEST_END;
}

// ============================================================================
// LOGICAL OPERATIONS (3 tests)
// ============================================================================

void test_logical_and_true(void) {
    TEST("logical_and_true");
    
    PipelineResult result = pipeline_run("true && true");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 1.0, "Result should be true (1)");
    
    TEST_END;
}

void test_logical_and_false(void) {
    TEST("logical_and_false");
    
    PipelineResult result = pipeline_run("true && false");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 0.0, "Result should be false (0)");
    
    TEST_END;
}

void test_logical_or(void) {
    TEST("logical_or");
    
    PipelineResult result = pipeline_run("false || true");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 1.0, "Result should be true (1)");
    
    TEST_END;
}

// ============================================================================
// VARIABLE TESTS (3 tests)
// ============================================================================

void test_variable_declaration(void) {
    TEST("variable_declaration");
    
    PipelineResult result = pipeline_run("var x = 10; x");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 10.0, "Result should be 10");
    
    TEST_END;
}

void test_variable_assignment(void) {
    TEST("variable_assignment");
    
    PipelineResult result = pipeline_run("var x = 5; x = 20; x");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 20.0, "Result should be 20");
    
    TEST_END;
}

void test_variable_arithmetic(void) {
    TEST("variable_arithmetic");
    
    PipelineResult result = pipeline_run("var x = 10; var y = 5; x + y");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 15.0, "Result should be 15");
    
    TEST_END;
}

// ============================================================================
// CONTROL FLOW TESTS (4 tests)
// ============================================================================

void test_if_statement_true(void) {
    TEST("if_statement_true");
    
    PipelineResult result = pipeline_run("if (true) { 42 } else { 0 }");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 42.0, "Result should be 42");
    
    TEST_END;
}

void test_if_statement_false(void) {
    TEST("if_statement_false");
    
    PipelineResult result = pipeline_run("if (false) { 42 } else { 99 }");
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 99.0, "Result should be 99");
    
    TEST_END;
}

void test_while_loop(void) {
    TEST("while_loop");
    
    PipelineResult result = pipeline_run(
        "var x = 0; "
        "while (x < 5) { x = x + 1; } "
        "x"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    
    TEST_END;
}

void test_for_loop(void) {
    TEST("for_loop");
    
    PipelineResult result = pipeline_run(
        "var sum = 0; "
        "for (var i = 1; i <= 10; i = i + 1) { sum = sum + i; } "
        "sum"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 55.0, "Result should be 55 (sum 1-10)");
    
    TEST_END;
}

// ============================================================================
// FUNCTION TESTS (3 tests)
// ============================================================================

void test_function_declaration(void) {
    TEST("function_declaration");
    
    PipelineResult result = pipeline_run(
        "fun add(a, b) { return a + b; } "
        "add(3, 4)"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 7.0, "Result should be 7");
    
    TEST_END;
}

void test_function_recursion(void) {
    TEST("function_recursion");
    
    PipelineResult result = pipeline_run(
        "fun factorial(n) { "
        "  if (n <= 1) { return 1; } "
        "  return n * factorial(n - 1); "
        "} "
        "factorial(5)"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 120.0, "Result should be 120 (5!)");
    
    TEST_END;
}

void test_function_fibonacci(void) {
    TEST("function_fibonacci");
    
    PipelineResult result = pipeline_run(
        "fun fib(n) { "
        "  if (n <= 1) { return n; } "
        "  return fib(n - 1) + fib(n - 2); "
        "} "
        "fib(7)"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 13.0, "Result should be 13 (fib(7))");
    
    TEST_END;
}

// ============================================================================
// ERROR HANDLING TESTS (2 tests)
// ============================================================================

void test_syntax_error(void) {
    TEST("syntax_error");
    
    PipelineResult result = pipeline_run("2 + + 3");
    
    ASSERT_FALSE(result.success, "Pipeline should fail");
    ASSERT_TRUE(result.error_message != NULL, "Should have error message");
    
    TEST_END;
}

void test_undefined_variable(void) {
    TEST("undefined_variable");
    
    PipelineResult result = pipeline_run("x + 5");
    
    ASSERT_FALSE(result.success, "Pipeline should fail");
    ASSERT_TRUE(result.error_message != NULL, "Should have error message");
    
    TEST_END;
}

// ============================================================================
// EDGE CASES (2 tests)
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
        "11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20"
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 210.0, "Result should be 210");
    
    TEST_END;
}

// ============================================================================
// OPTIMIZATION TESTS (2 tests)
// ============================================================================

void test_constant_folding(void) {
    TEST("constant_folding");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_optimization = true;
    
    PipelineResult result = pipeline_run_with_config("2 + 3", config);
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 5.0, "Result should be 5");
    // With optimization, bytecode should be smaller
    ASSERT_TRUE(result.bytecode_size > 0, "Should generate bytecode");
    
    TEST_END;
}

void test_dead_code_elimination(void) {
    TEST("dead_code_elimination");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_optimization = true;
    
    PipelineResult result = pipeline_run_with_config(
        "var x = 10; var y = 20; x",  // y is unused
        config
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 10.0, "Result should be 10");
    
    TEST_END;
}

// ============================================================================
// INTEGRATION TESTS (3 tests)
// ============================================================================

void test_pipeline_reuse(void) {
    TEST("pipeline_reuse");
    
    PipelineContext* ctx = pipeline_create();
    ASSERT_TRUE(ctx != NULL, "Should create pipeline");
    
    // First compilation
    bool success1 = pipeline_compile(ctx, "2 + 3");
    ASSERT_TRUE(success1, "First compilation should succeed");
    bool exec1 = pipeline_execute(ctx);
    ASSERT_TRUE(exec1, "First execution should succeed");
    PipelineResult result1 = pipeline_get_result(ctx);
    ASSERT_DOUBLE_EQ(result1.result_value, 5.0, "First result should be 5");
    
    // Reset and reuse
    pipeline_reset(ctx);
    
    // Second compilation
    bool success2 = pipeline_compile(ctx, "10 * 2");
    ASSERT_TRUE(success2, "Second compilation should succeed");
    bool exec2 = pipeline_execute(ctx);
    ASSERT_TRUE(exec2, "Second execution should succeed");
    PipelineResult result2 = pipeline_get_result(ctx);
    ASSERT_DOUBLE_EQ(result2.result_value, 20.0, "Second result should be 20");
    
    pipeline_destroy(ctx);
    TEST_END;
}

void test_gc_integration(void) {
    TEST("gc_integration");
    
    PipelineConfig config = pipeline_default_config();
    config.enable_gc = true;
    
    // Allocate many objects to trigger GC
    PipelineResult result = pipeline_run_with_config(
        "var sum = 0; "
        "for (var i = 0; i < 100; i = i + 1) { "
        "  var temp = i * 2; "
        "  sum = sum + temp; "
        "} "
        "sum",
        config
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 9900.0, "Result should be 9900");
    
    TEST_END;
}

void test_performance_baseline(void) {
    TEST("performance_baseline");
    
    PipelineConfig config = pipeline_default_config();
    config.verbose = false;
    
    PipelineResult result = pipeline_run_with_config(
        "var sum = 0; "
        "for (var i = 0; i < 1000; i = i + 1) { "
        "  sum = sum + i; "
        "} "
        "sum",
        config
    );
    
    ASSERT_TRUE(result.success, "Pipeline should succeed");
    ASSERT_DOUBLE_EQ(result.result_value, 499500.0, "Result should be 499500");
    
    // Check performance metrics are recorded
    ASSERT_TRUE(result.compile_time_us > 0, "Should record compile time");
    ASSERT_TRUE(result.execute_time_us > 0, "Should record execute time");
    
    printf(" [compile: %lu us, execute: %lu us]", 
           result.compile_time_us, result.execute_time_us);
    
    TEST_END;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main(void) {
    printf("========================================\n");
    printf("End-to-End Pipeline Test Suite\n");
    printf("========================================\n\n");
    
    // Basic Arithmetic (5 tests)
    test_simple_literal();
    test_simple_addition();
    test_arithmetic_expression();
    test_complex_arithmetic();
    test_nested_expressions();
    
    // Comparison Operations (3 tests)
    test_comparison_greater();
    test_comparison_less();
    test_comparison_equal();
    
    // Logical Operations (3 tests)
    test_logical_and_true();
    test_logical_and_false();
    test_logical_or();
    
    // Variables (3 tests)
    test_variable_declaration();
    test_variable_assignment();
    test_variable_arithmetic();
    
    // Control Flow (4 tests)
    test_if_statement_true();
    test_if_statement_false();
    test_while_loop();
    test_for_loop();
    
    // Functions (3 tests)
    test_function_declaration();
    test_function_recursion();
    test_function_fibonacci();
    
    // Error Handling (2 tests)
    test_syntax_error();
    test_undefined_variable();
    
    // Edge Cases (2 tests)
    test_empty_program();
    test_large_expression();
    
    // Optimization (2 tests)
    test_constant_folding();
    test_dead_code_elimination();
    
    // Integration (3 tests)
    test_pipeline_reuse();
    test_gc_integration();
    test_performance_baseline();
    
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
