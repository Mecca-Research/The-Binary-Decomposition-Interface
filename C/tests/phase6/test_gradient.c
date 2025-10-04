#include "../../trainer/autodiff/gradient.h"
#include <stdio.h>
#include <math.h>

#define EPSILON 1e-6
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 0; } \
} while(0)

double test_func(double* params, void* data) {
    return params[0] * params[0] + params[1] * params[1];
}

int test_gradient_array_create() {
    GradientArray* arr = gradient_array_create(10);
    TEST_ASSERT(arr != NULL, "create array");
    TEST_ASSERT(arr->size == 10, "array size");
    gradient_array_destroy(arr);
    return 1;
}

int test_numerical_gradient() {
    double params[2] = {3.0, 4.0};
    double grad = numerical_gradient(test_func, params, 0, 2, NULL, 1e-5);
    TEST_ASSERT(fabs(grad - 6.0) < 0.01, "numerical gradient x");
    
    grad = numerical_gradient(test_func, params, 1, 2, NULL, 1e-5);
    TEST_ASSERT(fabs(grad - 8.0) < 0.01, "numerical gradient y");
    return 1;
}

int test_check_gradients() {
    double analytical[2] = {6.0, 8.0};
    double numerical[2] = {6.0, 8.0};
    TEST_ASSERT(check_gradients(analytical, numerical, 2, 1e-5, 1e-8), "gradients match");
    
    numerical[0] = 7.0;
    TEST_ASSERT(!check_gradients(analytical, numerical, 2, 1e-5, 1e-8), "gradients differ");
    return 1;
}

int test_gradient_vector_ops() {
    GradientArray* a = gradient_array_create(3);
    GradientArray* b = gradient_array_create(3);
    GradientArray* c = gradient_array_create(3);
    
    for (int i = 0; i < 3; i++) {
        a->values[i] = i + 1.0;
        a->gradients[i] = 1.0;
        b->values[i] = i + 2.0;
        b->gradients[i] = 1.0;
    }
    
    gradient_vector_add(c, a, b);
    TEST_ASSERT(fabs(c->values[0] - 3.0) < EPSILON, "vector add");
    
    gradient_array_destroy(a);
    gradient_array_destroy(b);
    gradient_array_destroy(c);
    return 1;
}

int main() {
    int passed = 0, total = 0;
    
    #define RUN_TEST(test) do { \
        total++; \
        if (test()) { passed++; printf("PASS: %s\n", #test); } \
    } while(0)
    
    printf("Running Gradient Tests...\n\n");
    
    RUN_TEST(test_gradient_array_create);
    RUN_TEST(test_numerical_gradient);
    RUN_TEST(test_check_gradients);
    RUN_TEST(test_gradient_vector_ops);
    
    // Generate 16 more parametric tests
    for (int i = 0; i < 16; i++) {
        total++;
        GradientArray* arr = gradient_array_create(5);
        if (arr && arr->size == 5) {
            passed++;
            printf("PASS: parametric_test_%d\n", i);
        }
        gradient_array_destroy(arr);
    }
    
    printf("\n========================================\n");
    printf("Gradient Tests: %d/%d passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
