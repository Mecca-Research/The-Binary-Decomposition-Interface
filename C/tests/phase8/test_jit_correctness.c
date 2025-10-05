

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../../vm/vm_jit_integration.h"
#include "../../vm/bci_chunk.h"
#include "../../compiler/codegen/codegen.h"

// Test framework
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running test: %s...", name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf(" FAILED\n"); \
            printf("  Assertion failed: %s\n", #condition); \
            printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_DOUBLE_EQ(a, b, epsilon) \
    ASSERT(fabs((a) - (b)) < (epsilon))

// Helper function to create arithmetic expression chunks
static Chunk* create_arithmetic_chunk(double a, double b, uint8_t op) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    int const1 = chunk_add_constant(chunk, a);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1, 1);
    int const2 = chunk_add_constant(chunk, b);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2, 1);
    chunk_write(chunk, op, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Helper function to create nested expression chunk
static Chunk* create_nested_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Expression: ((2 + 3) * 4) - (10 / 2)
    // = (5 * 4) - 5 = 20 - 5 = 15
    
    int const1_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1_idx, 1);
    int const2_idx = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    
    int const3_idx = chunk_add_constant(chunk, 4.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const3_idx, 1);
    chunk_write(chunk, OP_MULTIPLY, 1);
    
    int const4_idx = chunk_add_constant(chunk, 10.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const4_idx, 1);
    int const5_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const5_idx, 1);
    chunk_write(chunk, OP_DIVIDE, 1);
    
    chunk_write(chunk, OP_SUBTRACT, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Helper function to execute both JIT and interpreter and compare results
static void compare_jit_interpreter(const char* test_name, Chunk* chunk, double expected) {
    // Create JIT-enabled VM
    JITConfig jit_config = jit_vm_default_config();
    jit_config.jit_threshold = 1; // Force JIT compilation
    JITIntegratedVM* jit_vm = jit_vm_create_with_config(1024 * 1024, &jit_config);
    ASSERT(jit_vm != NULL);
    
    // Create interpreter-only VM
    JITConfig interp_config = jit_vm_default_config();
    interp_config.enable_jit = false;
    JITIntegratedVM* interp_vm = jit_vm_create_with_config(1024 * 1024, &interp_config);
    ASSERT(interp_vm != NULL);
    
    // Execute with interpreter
    JITVmResult interp_result = jit_vm_execute(interp_vm, chunk);
    ASSERT(interp_result.success == true);
    ASSERT_DOUBLE_EQ(interp_result.result_value, expected, 1e-10);
    
    // Execute with JIT (multiple times to trigger compilation)
    JITVmResult jit_result;
    for (int i = 0; i < 3; i++) {
        jit_result = jit_vm_execute(jit_vm, chunk);
        ASSERT(jit_result.success == true);
    }
    
    // Compare results
    ASSERT_DOUBLE_EQ(jit_result.result_value, interp_result.result_value, 1e-10);
    ASSERT_DOUBLE_EQ(jit_result.result_value, expected, 1e-10);
    
    jit_vm_destroy(jit_vm);
    jit_vm_destroy(interp_vm);
}

// Test 1: Addition Correctness
void test_jit_addition_correctness(void) {
    TEST("jit_addition_correctness");
    
    Chunk* chunk = create_arithmetic_chunk(7.5, 2.3, OP_ADD);
    compare_jit_interpreter("addition", chunk, 9.8);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 2: Subtraction Correctness
void test_jit_subtraction_correctness(void) {
    TEST("jit_subtraction_correctness");
    
    Chunk* chunk = create_arithmetic_chunk(10.0, 3.5, OP_SUBTRACT);
    compare_jit_interpreter("subtraction", chunk, 6.5);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 3: Multiplication Correctness
void test_jit_multiplication_correctness(void) {
    TEST("jit_multiplication_correctness");
    
    Chunk* chunk = create_arithmetic_chunk(4.5, 2.0, OP_MULTIPLY);
    compare_jit_interpreter("multiplication", chunk, 9.0);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 4: Division Correctness
void test_jit_division_correctness(void) {
    TEST("jit_division_correctness");
    
    Chunk* chunk = create_arithmetic_chunk(15.0, 3.0, OP_DIVIDE);
    compare_jit_interpreter("division", chunk, 5.0);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 5: Negation Correctness
void test_jit_negation_correctness(void) {
    TEST("jit_negation_correctness");
    
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    int const_idx = chunk_add_constant(chunk, 42.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_NEGATE, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    compare_jit_interpreter("negation", chunk, -42.0);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 6: Complex Expression Correctness
void test_jit_complex_expression_correctness(void) {
    TEST("jit_complex_expression_correctness");
    
    Chunk* chunk = create_nested_chunk();
    compare_jit_interpreter("complex_expression", chunk, 15.0);
    chunk_free(chunk);
    
    TEST_END;
}

// Test 7: Floating Point Precision
void test_jit_floating_point_precision(void) {
    TEST("jit_floating_point_precision");
    
    // Test with very small numbers
    Chunk* chunk1 = create_arithmetic_chunk(1e-10, 2e-10, OP_ADD);
    compare_jit_interpreter("small_numbers", chunk1, 3e-10);
    chunk_free(chunk1);
    
    // Test with very large numbers
    Chunk* chunk2 = create_arithmetic_chunk(1e10, 2e10, OP_ADD);
    compare_jit_interpreter("large_numbers", chunk2, 3e10);
    chunk_free(chunk2);
    
    TEST_END;
}

// Test 8: Edge Cases
void test_jit_edge_cases(void) {
    TEST("jit_edge_cases");
    
    // Test with zero
    Chunk* chunk1 = create_arithmetic_chunk(5.0, 0.0, OP_ADD);
    compare_jit_interpreter("add_zero", chunk1, 5.0);
    chunk_free(chunk1);
    
    // Test with negative numbers
    Chunk* chunk2 = create_arithmetic_chunk(-3.0, -7.0, OP_ADD);
    compare_jit_interpreter("negative_add", chunk2, -10.0);
    chunk_free(chunk2);
    
    // Test multiplication by zero
    Chunk* chunk3 = create_arithmetic_chunk(42.0, 0.0, OP_MULTIPLY);
    compare_jit_interpreter("multiply_zero", chunk3, 0.0);
    chunk_free(chunk3);
    
    TEST_END;
}

// Test 9: Multiple Executions Consistency
void test_jit_multiple_executions_consistency(void) {
    TEST("jit_multiple_executions_consistency");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 2;
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_nested_chunk();
    ASSERT(chunk != NULL);
    
    double first_result = 0.0;
    
    // Execute multiple times and ensure consistent results
    for (int i = 0; i < 10; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        
        if (i == 0) {
            first_result = result.result_value;
        } else {
            ASSERT_DOUBLE_EQ(result.result_value, first_result, 1e-10);
        }
    }
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 10: JIT vs Interpreter Result Verification
void test_jit_vs_interpreter_verification(void) {
    TEST("jit_vs_interpreter_verification");
    
    // Create multiple test expressions
    // Test different expression types
    Chunk* add_chunk = create_arithmetic_chunk(1.0, 2.0, OP_ADD);
    compare_jit_interpreter("simple_add", add_chunk, 3.0);
    chunk_free(add_chunk);
    
    Chunk* sub_chunk = create_arithmetic_chunk(5.0, 3.0, OP_SUBTRACT);
    compare_jit_interpreter("simple_sub", sub_chunk, 2.0);
    chunk_free(sub_chunk);
    
    Chunk* mul_chunk = create_arithmetic_chunk(3.0, 4.0, OP_MULTIPLY);
    compare_jit_interpreter("simple_mul", mul_chunk, 12.0);
    chunk_free(mul_chunk);
    
    Chunk* div_chunk = create_arithmetic_chunk(8.0, 2.0, OP_DIVIDE);
    compare_jit_interpreter("simple_div", div_chunk, 4.0);
    chunk_free(div_chunk);
    
    Chunk* complex_chunk = create_nested_chunk();
    compare_jit_interpreter("complex", complex_chunk, 15.0);
    chunk_free(complex_chunk);

    
    TEST_END;
}

int main(void) {
    printf("=== JIT Correctness Verification Tests ===\n");
    
    test_jit_addition_correctness();
    test_jit_subtraction_correctness();
    test_jit_multiplication_correctness();
    test_jit_division_correctness();
    test_jit_negation_correctness();
    test_jit_complex_expression_correctness();
    test_jit_floating_point_precision();
    test_jit_edge_cases();
    test_jit_multiple_executions_consistency();
    test_jit_vs_interpreter_verification();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}

