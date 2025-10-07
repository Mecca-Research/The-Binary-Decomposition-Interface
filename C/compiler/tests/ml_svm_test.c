
/**
 * @file ml_svm_test.c
 * @brief Test suite for SVM ML Primitive
 */

#include "../AIBase/kernel/svm.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-4

// Test linear SVM
static void test_linear_svm(void) {
    printf("Testing linear SVM...\n");
    
    // Linearly separable data
    double X[] = {
        1.0, 1.0,
        2.0, 2.0,
        2.0, 1.0,
        6.0, 6.0,
        7.0, 7.0,
        7.0, 6.0
    };
    double y[] = {-1.0, -1.0, -1.0, 1.0, 1.0, 1.0};
    size_t n_samples = 6;
    size_t n_features = 2;
    
    SVMConfig config = svm_default_config();
    config.kernel_type = SVM_KERNEL_LINEAR;
    config.C = 1.0;
    
    SVMModel* model = svm_create(n_features, config);
    assert(model != nullptr);
    
    bool success = svm_fit(model, X, y, n_samples, n_features);
    assert(success);
    assert(model->fitted);
    
    printf("  Number of support vectors: %zu\n", model->n_support_vectors);
    assert(model->n_support_vectors > 0);
    
    // Test predictions
    double test_x1[] = {1.5, 1.5};
    double pred1 = svm_predict_single(model, test_x1);
    printf("  Prediction for [1.5, 1.5]: %.2f (expected: -1.0)\n", pred1);
    assert(pred1 == -1.0);
    
    double test_x2[] = {6.5, 6.5};
    double pred2 = svm_predict_single(model, test_x2);
    printf("  Prediction for [6.5, 6.5]: %.2f (expected: 1.0)\n", pred2);
    assert(pred2 == 1.0);
    
    svm_destroy(model);
    printf("  ✓ Linear SVM test passed\n\n");
}

// Test RBF kernel SVM
static void test_rbf_svm(void) {
    printf("Testing RBF kernel SVM...\n");
    
    // Non-linearly separable data (XOR-like)
    double X[] = {
        1.0, 1.0,
        1.0, -1.0,
        -1.0, 1.0,
        -1.0, -1.0
    };
    double y[] = {-1.0, 1.0, 1.0, -1.0};
    size_t n_samples = 4;
    size_t n_features = 2;
    
    SVMConfig config = svm_default_config();
    config.kernel_type = SVM_KERNEL_RBF;
    config.gamma = 0.5;
    config.C = 1.0;
    
    SVMModel* model = svm_create(n_features, config);
    assert(model != nullptr);
    
    bool success = svm_fit(model, X, y, n_samples, n_features);
    assert(success);
    
    printf("  Number of support vectors: %zu\n", model->n_support_vectors);
    
    // Test kernel computation
    double x1[] = {1.0, 1.0};
    double x2[] = {1.0, -1.0};
    double kernel_val = svm_kernel_compute(model, x1, x2);
    printf("  RBF kernel value: %.4f\n", kernel_val);
    assert(kernel_val >= 0.0 && kernel_val <= 1.0);
    
    svm_destroy(model);
    printf("  ✓ RBF SVM test passed\n\n");
}

// Test batch predictions
static void test_svm_batch_predictions(void) {
    printf("Testing SVM batch predictions...\n");
    
    double X_train[] = {
        1.0, 1.0,
        2.0, 2.0,
        6.0, 6.0,
        7.0, 7.0
    };
    double y_train[] = {-1.0, -1.0, 1.0, 1.0};
    size_t n_samples = 4;
    size_t n_features = 2;
    
    SVMConfig config = svm_default_config();
    config.kernel_type = SVM_KERNEL_LINEAR;
    
    SVMModel* model = svm_create(n_features, config);
    svm_fit(model, X_train, y_train, n_samples, n_features);
    
    // Batch prediction
    double X_test[] = {
        1.5, 1.5,
        6.5, 6.5
    };
    double predictions[2];
    size_t n_test = 2;
    
    svm_predict(model, X_test, predictions, n_test, n_features);
    
    printf("  Predictions: [%.2f, %.2f]\n", predictions[0], predictions[1]);
    assert(predictions[0] == -1.0);
    assert(predictions[1] == 1.0);
    
    svm_destroy(model);
    printf("  ✓ SVM batch predictions test passed\n\n");
}

// Test decision function
static void test_decision_function(void) {
    printf("Testing SVM decision function...\n");
    
    double X[] = {
        1.0, 1.0,
        2.0, 2.0,
        6.0, 6.0,
        7.0, 7.0
    };
    double y[] = {-1.0, -1.0, 1.0, 1.0};
    size_t n_samples = 4;
    size_t n_features = 2;
    
    SVMConfig config = svm_default_config();
    config.kernel_type = SVM_KERNEL_LINEAR;
    
    SVMModel* model = svm_create(n_features, config);
    svm_fit(model, X, y, n_samples, n_features);
    
    // Test decision function
    double test_x[] = {4.0, 4.0};
    double decision = svm_decision_function(model, test_x);
    printf("  Decision function value: %.4f\n", decision);
    
    svm_destroy(model);
    printf("  ✓ Decision function test passed\n\n");
}

// Test kernel functions
static void test_kernel_functions(void) {
    printf("Testing kernel functions...\n");
    
    double x1[] = {1.0, 2.0};
    double x2[] = {3.0, 4.0};
    size_t n_features = 2;
    
    // Linear kernel
    double linear = svm_kernel_linear(x1, x2, n_features);
    printf("  Linear kernel: %.4f (expected: 11.0)\n", linear);
    assert(fabs(linear - 11.0) < EPSILON);
    
    // RBF kernel
    double rbf = svm_kernel_rbf(x1, x2, n_features, 0.5);
    printf("  RBF kernel: %.4f\n", rbf);
    assert(rbf >= 0.0 && rbf <= 1.0);
    
    printf("  ✓ Kernel functions test passed\n\n");
}

int main(void) {
    printf("=== SVM ML Primitive Tests ===\n\n");
    
    test_linear_svm();
    test_rbf_svm();
    test_svm_batch_predictions();
    test_decision_function();
    test_kernel_functions();
    
    printf("=== All SVM tests passed! ===\n");
    return 0;
}
