
#include "../../trainer/autodiff/forward_ad.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-6
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return 0; \
    } \
} while(0)

#define TEST_DUAL_EQ(d, v, deriv, msg) do { \
    TEST_ASSERT(fabs((d).value - (v)) < EPSILON, msg " - value"); \
    TEST_ASSERT(fabs((d).derivative - (deriv)) < EPSILON, msg " - derivative"); \
} while(0)

// Test dual number creation
int test_dual_create() {
    Dual d = dual_create(3.0, 2.0);
    TEST_DUAL_EQ(d, 3.0, 2.0, "dual_create");
    return 1;
}

int test_dual_constant() {
    Dual d = dual_constant(5.0);
    TEST_DUAL_EQ(d, 5.0, 0.0, "dual_constant");
    return 1;
}

int test_dual_variable() {
    Dual d = dual_variable(7.0);
    TEST_DUAL_EQ(d, 7.0, 1.0, "dual_variable");
    return 1;
}

// Test arithmetic operations
int test_dual_add() {
    Dual a = dual_create(3.0, 1.0);
    Dual b = dual_create(2.0, 1.0);
    Dual c = dual_add(a, b);
    TEST_DUAL_EQ(c, 5.0, 2.0, "dual_add");
    return 1;
}

int test_dual_sub() {
    Dual a = dual_create(5.0, 1.0);
    Dual b = dual_create(2.0, 1.0);
    Dual c = dual_sub(a, b);
    TEST_DUAL_EQ(c, 3.0, 0.0, "dual_sub");
    return 1;
}

int test_dual_mul() {
    Dual a = dual_create(3.0, 1.0);
    Dual b = dual_create(2.0, 0.0);
    Dual c = dual_mul(a, b);
    TEST_DUAL_EQ(c, 6.0, 2.0, "dual_mul");
    return 1;
}

int test_dual_div() {
    Dual a = dual_create(6.0, 1.0);
    Dual b = dual_create(2.0, 0.0);
    Dual c = dual_div(a, b);
    TEST_DUAL_EQ(c, 3.0, 0.5, "dual_div");
    return 1;
}

int test_dual_neg() {
    Dual a = dual_create(3.0, 2.0);
    Dual b = dual_neg(a);
    TEST_DUAL_EQ(b, -3.0, -2.0, "dual_neg");
    return 1;
}

// Test mathematical functions
int test_dual_sin() {
    Dual x = dual_variable(0.0);
    Dual y = dual_sin(x);
    TEST_DUAL_EQ(y, 0.0, 1.0, "dual_sin at 0");
    return 1;
}

int test_dual_cos() {
    Dual x = dual_variable(0.0);
    Dual y = dual_cos(x);
    TEST_DUAL_EQ(y, 1.0, 0.0, "dual_cos at 0");
    return 1;
}

int test_dual_exp() {
    Dual x = dual_variable(0.0);
    Dual y = dual_exp(x);
    TEST_DUAL_EQ(y, 1.0, 1.0, "dual_exp at 0");
    return 1;
}

int test_dual_log() {
    Dual x = dual_variable(1.0);
    Dual y = dual_log(x);
    TEST_DUAL_EQ(y, 0.0, 1.0, "dual_log at 1");
    return 1;
}

int test_dual_sqrt() {
    Dual x = dual_variable(4.0);
    Dual y = dual_sqrt(x);
    TEST_DUAL_EQ(y, 2.0, 0.25, "dual_sqrt of 4");
    return 1;
}

int test_dual_pow() {
    Dual x = dual_variable(2.0);
    Dual y = dual_pow(x, 3.0);
    TEST_DUAL_EQ(y, 8.0, 12.0, "dual_pow x^3 at x=2");
    return 1;
}

int test_dual_tanh() {
    Dual x = dual_variable(0.0);
    Dual y = dual_tanh(x);
    TEST_DUAL_EQ(y, 0.0, 1.0, "dual_tanh at 0");
    return 1;
}

int test_dual_sigmoid() {
    Dual x = dual_variable(0.0);
    Dual y = dual_sigmoid(x);
    TEST_DUAL_EQ(y, 0.5, 0.25, "dual_sigmoid at 0");
    return 1;
}

int test_dual_relu() {
    Dual x1 = dual_variable(2.0);
    Dual y1 = dual_relu(x1);
    TEST_DUAL_EQ(y1, 2.0, 1.0, "dual_relu positive");
    
    Dual x2 = dual_variable(-2.0);
    Dual y2 = dual_relu(x2);
    TEST_DUAL_EQ(y2, 0.0, 0.0, "dual_relu negative");
    return 1;
}

// Test composite functions
int test_composite_polynomial() {
    // f(x) = x^2 + 2x + 1, f'(x) = 2x + 2
    Dual x = dual_variable(3.0);
    Dual x_sq = dual_mul(x, x);
    Dual two_x = dual_mul(dual_constant(2.0), x);
    Dual result = dual_add(dual_add(x_sq, two_x), dual_constant(1.0));
    TEST_DUAL_EQ(result, 16.0, 8.0, "polynomial x^2+2x+1 at x=3");
    return 1;
}

int test_composite_chain_rule() {
    // f(x) = sin(x^2), f'(x) = 2x*cos(x^2)
    Dual x = dual_variable(1.0);
    Dual x_sq = dual_mul(x, x);
    Dual result = dual_sin(x_sq);
    double expected_deriv = 2.0 * 1.0 * cos(1.0);
    TEST_DUAL_EQ(result, sin(1.0), expected_deriv, "chain rule sin(x^2)");
    return 1;
}

int test_composite_product_rule() {
    // f(x) = x * sin(x), f'(x) = sin(x) + x*cos(x)
    Dual x = dual_variable(1.0);
    Dual sin_x = dual_sin(x);
    Dual result = dual_mul(x, sin_x);
    double expected_deriv = sin(1.0) + 1.0 * cos(1.0);
    TEST_DUAL_EQ(result, sin(1.0), expected_deriv, "product rule x*sin(x)");
    return 1;
}

int test_composite_quotient_rule() {
    // f(x) = x / (x+1), f'(x) = 1/(x+1)^2
    Dual x = dual_variable(2.0);
    Dual x_plus_1 = dual_add(x, dual_constant(1.0));
    Dual result = dual_div(x, x_plus_1);
    double expected_deriv = 1.0 / 9.0;
    TEST_DUAL_EQ(result, 2.0/3.0, expected_deriv, "quotient rule x/(x+1)");
    return 1;
}

// Test edge cases
int test_edge_zero() {
    Dual zero = dual_variable(0.0);
    Dual result = dual_mul(zero, dual_constant(5.0));
    TEST_DUAL_EQ(result, 0.0, 5.0, "multiply by zero");
    return 1;
}

int test_edge_one() {
    Dual one = dual_variable(1.0);
    Dual result = dual_mul(one, dual_constant(5.0));
    TEST_DUAL_EQ(result, 5.0, 5.0, "multiply by one");
    return 1;
}

int test_edge_negative() {
    Dual x = dual_variable(-2.0);
    Dual result = dual_mul(x, x);
    TEST_DUAL_EQ(result, 4.0, -4.0, "negative squared");
    return 1;
}

// Test comparison operations
int test_dual_eq() {
    Dual a = dual_create(3.0, 2.0);
    Dual b = dual_create(3.0, 2.0);
    TEST_ASSERT(dual_eq(a, b, EPSILON), "dual_eq equal");
    
    Dual c = dual_create(3.1, 2.0);
    TEST_ASSERT(!dual_eq(a, c, EPSILON), "dual_eq not equal");
    return 1;
}

int test_dual_lt() {
    Dual a = dual_create(2.0, 1.0);
    Dual b = dual_create(3.0, 1.0);
    TEST_ASSERT(dual_lt(a, b), "dual_lt true");
    TEST_ASSERT(!dual_lt(b, a), "dual_lt false");
    return 1;
}

int test_dual_gt() {
    Dual a = dual_create(3.0, 1.0);
    Dual b = dual_create(2.0, 1.0);
    TEST_ASSERT(dual_gt(a, b), "dual_gt true");
    TEST_ASSERT(!dual_gt(b, a), "dual_gt false");
    return 1;
}

// Additional mathematical function tests
int test_dual_tan() {
    Dual x = dual_variable(0.0);
    Dual y = dual_tan(x);
    TEST_DUAL_EQ(y, 0.0, 1.0, "dual_tan at 0");
    return 1;
}

int test_dual_abs_positive() {
    Dual x = dual_variable(3.0);
    Dual y = dual_abs(x);
    TEST_DUAL_EQ(y, 3.0, 1.0, "dual_abs positive");
    return 1;
}

int test_dual_abs_negative() {
    Dual x = dual_variable(-3.0);
    Dual y = dual_abs(x);
    TEST_DUAL_EQ(y, 3.0, -1.0, "dual_abs negative");
    return 1;
}

// More composite tests
int test_composite_exp_polynomial() {
    // f(x) = exp(x^2), f'(x) = 2x*exp(x^2)
    Dual x = dual_variable(1.0);
    Dual x_sq = dual_mul(x, x);
    Dual result = dual_exp(x_sq);
    double expected_deriv = 2.0 * exp(1.0);
    TEST_DUAL_EQ(result, exp(1.0), expected_deriv, "exp(x^2)");
    return 1;
}

int test_composite_log_polynomial() {
    // f(x) = log(x^2), f'(x) = 2/x
    Dual x = dual_variable(2.0);
    Dual x_sq = dual_mul(x, x);
    Dual result = dual_log(x_sq);
    double expected_deriv = 1.0;
    TEST_DUAL_EQ(result, log(4.0), expected_deriv, "log(x^2)");
    return 1;
}

int test_composite_nested() {
    // f(x) = sin(cos(x)), f'(x) = -sin(x)*cos(cos(x))
    Dual x = dual_variable(0.5);
    Dual cos_x = dual_cos(x);
    Dual result = dual_sin(cos_x);
    double expected_deriv = -sin(0.5) * cos(cos(0.5));
    TEST_DUAL_EQ(result, sin(cos(0.5)), expected_deriv, "sin(cos(x))");
    return 1;
}

int test_composite_sqrt_sum() {
    // f(x) = sqrt(x + 1), f'(x) = 1/(2*sqrt(x+1))
    Dual x = dual_variable(3.0);
    Dual x_plus_1 = dual_add(x, dual_constant(1.0));
    Dual result = dual_sqrt(x_plus_1);
    double expected_deriv = 0.25;
    TEST_DUAL_EQ(result, 2.0, expected_deriv, "sqrt(x+1)");
    return 1;
}

int test_composite_sigmoid_linear() {
    // f(x) = sigmoid(2x), f'(x) = 2*sigmoid(2x)*(1-sigmoid(2x))
    Dual x = dual_variable(0.0);
    Dual two_x = dual_mul(dual_constant(2.0), x);
    Dual result = dual_sigmoid(two_x);
    double expected_deriv = 0.5;
    TEST_DUAL_EQ(result, 0.5, expected_deriv, "sigmoid(2x)");
    return 1;
}

int test_composite_tanh_squared() {
    // f(x) = tanh(x)^2, f'(x) = 2*tanh(x)*(1-tanh(x)^2)
    Dual x = dual_variable(0.0);
    Dual tanh_x = dual_tanh(x);
    Dual result = dual_mul(tanh_x, tanh_x);
    TEST_DUAL_EQ(result, 0.0, 0.0, "tanh(x)^2 at 0");
    return 1;
}

// Test multiple variables (partial derivatives)
int test_partial_derivatives_sum() {
    // f(x,y) = x + y, ∂f/∂x = 1, ∂f/∂y = 1
    Dual x = dual_variable(2.0);
    Dual y = dual_constant(3.0);
    Dual result = dual_add(x, y);
    TEST_DUAL_EQ(result, 5.0, 1.0, "partial derivative sum");
    return 1;
}

int test_partial_derivatives_product() {
    // f(x,y) = x*y, ∂f/∂x = y
    Dual x = dual_variable(2.0);
    Dual y = dual_constant(3.0);
    Dual result = dual_mul(x, y);
    TEST_DUAL_EQ(result, 6.0, 3.0, "partial derivative product");
    return 1;
}

int test_partial_derivatives_complex() {
    // f(x,y) = x^2 + xy + y^2, ∂f/∂x = 2x + y
    Dual x = dual_variable(2.0);
    Dual y = dual_constant(3.0);
    Dual x_sq = dual_mul(x, x);
    Dual xy = dual_mul(x, y);
    Dual y_sq = dual_constant(9.0);
    Dual result = dual_add(dual_add(x_sq, xy), y_sq);
    TEST_DUAL_EQ(result, 19.0, 7.0, "partial derivative complex");
    return 1;
}

// Run all tests
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
    
    printf("Running Forward AD Tests...\n\n");
    
    RUN_TEST(test_dual_create);
    RUN_TEST(test_dual_constant);
    RUN_TEST(test_dual_variable);
    RUN_TEST(test_dual_add);
    RUN_TEST(test_dual_sub);
    RUN_TEST(test_dual_mul);
    RUN_TEST(test_dual_div);
    RUN_TEST(test_dual_neg);
    RUN_TEST(test_dual_sin);
    RUN_TEST(test_dual_cos);
    RUN_TEST(test_dual_exp);
    RUN_TEST(test_dual_log);
    RUN_TEST(test_dual_sqrt);
    RUN_TEST(test_dual_pow);
    RUN_TEST(test_dual_tanh);
    RUN_TEST(test_dual_sigmoid);
    RUN_TEST(test_dual_relu);
    RUN_TEST(test_composite_polynomial);
    RUN_TEST(test_composite_chain_rule);
    RUN_TEST(test_composite_product_rule);
    RUN_TEST(test_composite_quotient_rule);
    RUN_TEST(test_edge_zero);
    RUN_TEST(test_edge_one);
    RUN_TEST(test_edge_negative);
    RUN_TEST(test_dual_eq);
    RUN_TEST(test_dual_lt);
    RUN_TEST(test_dual_gt);
    RUN_TEST(test_dual_tan);
    RUN_TEST(test_dual_abs_positive);
    RUN_TEST(test_dual_abs_negative);
    RUN_TEST(test_composite_exp_polynomial);
    RUN_TEST(test_composite_log_polynomial);
    RUN_TEST(test_composite_nested);
    RUN_TEST(test_composite_sqrt_sum);
    RUN_TEST(test_composite_sigmoid_linear);
    RUN_TEST(test_composite_tanh_squared);
    RUN_TEST(test_partial_derivatives_sum);
    RUN_TEST(test_partial_derivatives_product);
    RUN_TEST(test_partial_derivatives_complex);
    
    printf("\n========================================\n");
    printf("Forward AD Tests: %d/%d passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
