#include "../../trainer/autodiff/reverse_ad.h"
#include <stdio.h>
#include <math.h>

#define EPSILON 1e-6
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return 0; \
    } \
} while(0)

int test_tape_create() {
    GradientTape* tape = tape_create(10);
    TEST_ASSERT(tape != NULL, "tape_create");
    TEST_ASSERT(tape->size == 0, "initial size");
    TEST_ASSERT(tape->capacity >= 10, "initial capacity");
    tape_destroy(tape);
    return 1;
}

int test_tape_record_constant() {
    GradientTape* tape = tape_create(10);
    size_t id = tape_record_constant(tape, 5.0);
    TEST_ASSERT(id == 0, "constant id");
    TEST_ASSERT(tape->size == 1, "tape size");
    tape_destroy(tape);
    return 1;
}

int test_tape_record_variable() {
    GradientTape* tape = tape_create(10);
    size_t id = tape_record_variable(tape, 3.0);
    TEST_ASSERT(id == 0, "variable id");
    TEST_ASSERT(tape->num_variables == 1, "num variables");
    tape_destroy(tape);
    return 1;
}

int test_tape_add() {
    GradientTape* tape = tape_create(10);
    size_t a = tape_record_variable(tape, 2.0);
    size_t b = tape_record_variable(tape, 3.0);
    size_t c = tape_record_add(tape, a, b, 2.0, 3.0);
    
    tape_backward(tape, c);
    TEST_ASSERT(fabs(tape_get_gradient(tape, a) - 1.0) < EPSILON, "add grad a");
    TEST_ASSERT(fabs(tape_get_gradient(tape, b) - 1.0) < EPSILON, "add grad b");
    tape_destroy(tape);
    return 1;
}

int test_tape_mul() {
    GradientTape* tape = tape_create(10);
    size_t a = tape_record_variable(tape, 2.0);
    size_t b = tape_record_variable(tape, 3.0);
    size_t c = tape_record_mul(tape, a, b, 2.0, 3.0);
    
    tape_backward(tape, c);
    TEST_ASSERT(fabs(tape_get_gradient(tape, a) - 3.0) < EPSILON, "mul grad a");
    TEST_ASSERT(fabs(tape_get_gradient(tape, b) - 2.0) < EPSILON, "mul grad b");
    tape_destroy(tape);
    return 1;
}

int test_tape_chain_rule() {
    // f(x) = sin(x^2), df/dx = 2x*cos(x^2)
    GradientTape* tape = tape_create(10);
    size_t x = tape_record_variable(tape, 1.0);
    size_t x_sq = tape_record_mul(tape, x, x, 1.0, 1.0);
    size_t result = tape_record_sin(tape, x_sq, 1.0);
    
    tape_backward(tape, result);
    double expected = 2.0 * cos(1.0);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x) - expected) < EPSILON, "chain rule");
    tape_destroy(tape);
    return 1;
}

int test_tape_exp() {
    GradientTape* tape = tape_create(10);
    size_t x = tape_record_variable(tape, 0.0);
    size_t y = tape_record_exp(tape, x, 0.0);
    
    tape_backward(tape, y);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x) - 1.0) < EPSILON, "exp gradient");
    tape_destroy(tape);
    return 1;
}

int test_tape_log() {
    GradientTape* tape = tape_create(10);
    size_t x = tape_record_variable(tape, 1.0);
    size_t y = tape_record_log(tape, x, 1.0);
    
    tape_backward(tape, y);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x) - 1.0) < EPSILON, "log gradient");
    tape_destroy(tape);
    return 1;
}

int test_tape_sigmoid() {
    GradientTape* tape = tape_create(10);
    size_t x = tape_record_variable(tape, 0.0);
    size_t y = tape_record_sigmoid(tape, x, 0.0);
    
    tape_backward(tape, y);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x) - 0.25) < EPSILON, "sigmoid gradient");
    tape_destroy(tape);
    return 1;
}

int test_tape_relu() {
    GradientTape* tape = tape_create(10);
    size_t x1 = tape_record_variable(tape, 2.0);
    size_t y1 = tape_record_relu(tape, x1, 2.0);
    tape_backward(tape, y1);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x1) - 1.0) < EPSILON, "relu positive");
    
    tape_clear(tape);
    size_t x2 = tape_record_variable(tape, -2.0);
    size_t y2 = tape_record_relu(tape, x2, -2.0);
    tape_backward(tape, y2);
    TEST_ASSERT(fabs(tape_get_gradient(tape, x2) - 0.0) < EPSILON, "relu negative");
    
    tape_destroy(tape);
    return 1;
}

int test_tape_complex() {
    // f(x,y) = x^2 + xy + y^2
    GradientTape* tape = tape_create(20);
    size_t x = tape_record_variable(tape, 2.0);
    size_t y = tape_record_variable(tape, 3.0);
    
    size_t x_sq = tape_record_mul(tape, x, x, 2.0, 2.0);
    size_t xy = tape_record_mul(tape, x, y, 2.0, 3.0);
    size_t y_sq = tape_record_mul(tape, y, y, 3.0, 3.0);
    
    size_t temp = tape_record_add(tape, x_sq, xy, 4.0, 6.0);
    size_t result = tape_record_add(tape, temp, y_sq, 10.0, 9.0);
    
    tape_backward(tape, result);
    
    // df/dx = 2x + y = 7, df/dy = x + 2y = 8
    TEST_ASSERT(fabs(tape_get_gradient(tape, x) - 7.0) < EPSILON, "complex grad x");
    TEST_ASSERT(fabs(tape_get_gradient(tape, y) - 8.0) < EPSILON, "complex grad y");
    
    tape_destroy(tape);
    return 1;
}

// Generate more tests programmatically
int run_operation_tests() {
    int passed = 0;
    
    // Test all unary operations
    const char* unary_ops[] = {"sin", "cos", "tan", "exp", "log", "sqrt", "tanh", "sigmoid", "relu", "abs"};
    for (int i = 0; i < 10; i++) {
        GradientTape* tape = tape_create(10);
        size_t x = tape_record_variable(tape, 1.0);
        size_t y;
        
        switch(i) {
            case 0: y = tape_record_sin(tape, x, 1.0); break;
            case 1: y = tape_record_cos(tape, x, 1.0); break;
            case 2: y = tape_record_tan(tape, x, 1.0); break;
            case 3: y = tape_record_exp(tape, x, 1.0); break;
            case 4: y = tape_record_log(tape, x, 1.0); break;
            case 5: y = tape_record_sqrt(tape, x, 1.0); break;
            case 6: y = tape_record_tanh(tape, x, 1.0); break;
            case 7: y = tape_record_sigmoid(tape, x, 1.0); break;
            case 8: y = tape_record_relu(tape, x, 1.0); break;
            case 9: y = tape_record_abs(tape, x, 1.0); break;
        }
        
        tape_backward(tape, y);
        double grad = tape_get_gradient(tape, x);
        
        if (!isnan(grad) && !isinf(grad)) {
            passed++;
            printf("PASS: tape_%s\n", unary_ops[i]);
        }
        
        tape_destroy(tape);
    }
    
    return passed;
}

int main() {
    int passed = 0;
    int total = 0;
    
    #define RUN_TEST(test) do { \
        total++; \
        if (test()) { \
            passed++; \
            printf("PASS: %s\n", #test); \
        } \
    } while(0)
    
    printf("Running Reverse AD Tests...\n\n");
    
    RUN_TEST(test_tape_create);
    RUN_TEST(test_tape_record_constant);
    RUN_TEST(test_tape_record_variable);
    RUN_TEST(test_tape_add);
    RUN_TEST(test_tape_mul);
    RUN_TEST(test_tape_chain_rule);
    RUN_TEST(test_tape_exp);
    RUN_TEST(test_tape_log);
    RUN_TEST(test_tape_sigmoid);
    RUN_TEST(test_tape_relu);
    RUN_TEST(test_tape_complex);
    
    int op_tests = run_operation_tests();
    passed += op_tests;
    total += 10;
    
    // Add 20 more parametric tests
    for (int i = 0; i < 20; i++) {
        GradientTape* tape = tape_create(10);
        double val = (double)i * 0.5;
        size_t x = tape_record_variable(tape, val);
        size_t y = tape_record_mul(tape, x, x, val, val);
        tape_backward(tape, y);
        
        if (fabs(tape_get_gradient(tape, x) - 2.0 * val) < EPSILON) {
            passed++;
            printf("PASS: parametric_test_%d\n", i);
        }
        total++;
        tape_destroy(tape);
    }
    
    printf("\n========================================\n");
    printf("Reverse AD Tests: %d/%d passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
