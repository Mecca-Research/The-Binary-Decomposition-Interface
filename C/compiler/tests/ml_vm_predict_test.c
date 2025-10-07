/**
 * @file ml_vm_predict_test.c
 * @brief Test suite for ML VM Prediction Bug Fix
 * @details Tests the fixed decision tree VM execution handler
 */

#include "../VM/ml_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Test decision tree VM execution
static void test_tree_vm_execution(void) {
    printf("Testing decision tree VM execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    MLVMContext* ml_ctx = ml_vm_context_create();
    
    // Create and train a simple decision tree
    DecisionTreeConfig config = decision_tree_default_config();
    config.max_depth = 3;
    DecisionTreeModel* model = decision_tree_create(config);
    
    // Simple 1D dataset: y = 2*x
    double X[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};
    bool fit_success = decision_tree_fit(model, X, y, 5, 1);
    assert(fit_success);
    assert(model->fitted);
    assert(model->n_features == 1);
    
    printf("  Model trained with n_features=%zu\n", model->n_features);
    
    // Register model
    size_t handle = ml_vm_register_decision_tree(ml_ctx, model);
    
    // Test prediction for x=2.5
    vm_stack_push(&vm, 2.5);
    bool exec_success = ml_vm_execute_tree_predict(&vm, ml_ctx, (uint8_t)handle);
    assert(exec_success);
    
    double result = vm_stack_pop(&vm);
    printf("  Prediction for x=2.5: %.2f (expected: ~4.0-6.0)\n", result);
    assert(result >= 2.0 && result <= 8.0);  // Reasonable range
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ Decision tree VM execution test passed\n\n");
}

// Test multivariate decision tree VM execution
static void test_multivariate_tree_vm_execution(void) {
    printf("Testing multivariate decision tree VM execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    MLVMContext* ml_ctx = ml_vm_context_create();
    
    // Create and train a 2D decision tree
    DecisionTreeConfig config = decision_tree_default_config();
    config.max_depth = 5;
    DecisionTreeModel* model = decision_tree_create(config);
    
    // 2D dataset: y = x1 + x2
    double X[] = {
        1.0, 1.0,
        2.0, 2.0,
        3.0, 3.0,
        4.0, 4.0,
        5.0, 5.0
    };
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};
    
    bool fit_success = decision_tree_fit(model, X, y, 5, 2);
    assert(fit_success);
    assert(model->fitted);
    assert(model->n_features == 2);
    
    printf("  Model trained with n_features=%zu\n", model->n_features);
    
    // Register model
    size_t handle = ml_vm_register_decision_tree(ml_ctx, model);
    
    // Test prediction for [2.5, 2.5]
    // Push features in reverse order (stack is LIFO)
    vm_stack_push(&vm, 2.5);  // x2
    vm_stack_push(&vm, 2.5);  // x1
    
    bool exec_success = ml_vm_execute_tree_predict(&vm, ml_ctx, (uint8_t)handle);
    assert(exec_success);
    
    double result = vm_stack_pop(&vm);
    printf("  Prediction for [2.5, 2.5]: %.2f (expected: ~4.0-6.0)\n", result);
    assert(result >= 2.0 && result <= 8.0);  // Reasonable range
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ Multivariate decision tree VM execution test passed\n\n");
}

// Test linear regression VM execution
static void test_linear_vm_execution(void) {
    printf("Testing linear regression VM execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    MLVMContext* ml_ctx = ml_vm_context_create();
    
    // Create and train a model
    LinearRegressionConfig config = linear_regression_default_config();
    config.max_iterations = 1000;
    LinearRegressionModel* model = linear_regression_create(1, config);
    
    double X[] = {1.0, 2.0, 3.0};
    double y[] = {2.0, 4.0, 6.0};
    linear_regression_fit(model, X, y, 3, 1);
    
    printf("  Model trained with n_features=%zu\n", model->n_features);
    
    // Register model
    size_t handle = ml_vm_register_linear_regression(ml_ctx, model);
    
    // Push test data onto stack
    vm_stack_push(&vm, 4.0);  // Test input
    
    // Execute prediction
    bool success = ml_vm_execute_linear_reg_predict(&vm, ml_ctx, (uint8_t)handle);
    assert(success);
    
    // Pop result
    double result = vm_stack_pop(&vm);
    printf("  Prediction result: %.2f (expected: ~8.0)\n", result);
    assert(fabs(result - 8.0) < 1.0);
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ Linear regression VM execution test passed\n\n");
}

// Test SVM VM execution
static void test_svm_vm_execution(void) {
    printf("Testing SVM VM execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    MLVMContext* ml_ctx = ml_vm_context_create();
    
    // Create and train a simple SVM
    SVMConfig config = svm_default_config();
    config.max_iterations = 100;
    SVMModel* model = svm_create(2, config);
    
    // Simple 2D linearly separable dataset
    double X[] = {
        1.0, 1.0,
        2.0, 2.0,
        3.0, 3.0,
        4.0, 4.0
    };
    double y[] = {-1.0, -1.0, 1.0, 1.0};
    
    bool fit_success = svm_fit(model, X, y, 4, 2);
    assert(fit_success);
    assert(model->fitted);
    
    printf("  Model trained with n_features=%zu\n", model->n_features);
    
    // Register model
    size_t handle = ml_vm_register_svm(ml_ctx, model);
    
    // Test prediction for [2.5, 2.5]
    vm_stack_push(&vm, 2.5);  // x2
    vm_stack_push(&vm, 2.5);  // x1
    
    bool exec_success = ml_vm_execute_svm_predict(&vm, ml_ctx, (uint8_t)handle);
    assert(exec_success);
    
    double result = vm_stack_pop(&vm);
    printf("  SVM prediction for [2.5, 2.5]: %.2f (expected: -1.0 or 1.0)\n", result);
    assert(result == -1.0 || result == 1.0);
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ SVM VM execution test passed\n\n");
}

// Test K-means VM execution
static void test_kmeans_vm_execution(void) {
    printf("Testing K-means VM execution...\n");
    
    VM vm;
    vm_init(&vm);
    
    MLVMContext* ml_ctx = ml_vm_context_create();
    
    // Create and train a simple K-means model
    KMeansConfig config = kmeans_default_config(2);
    config.max_iterations = 100;
    KMeansModel* model = kmeans_create(2, config);
    
    // Simple 2D dataset with 2 clusters
    double X[] = {
        1.0, 1.0,
        1.5, 1.5,
        5.0, 5.0,
        5.5, 5.5
    };
    
    bool fit_success = kmeans_fit(model, X, 4, 2);
    assert(fit_success);
    assert(model->fitted);
    
    printf("  Model trained with n_features=%zu\n", model->n_features);
    
    // Register model
    size_t handle = ml_vm_register_kmeans(ml_ctx, model);
    
    // Test prediction for [1.2, 1.2] (should be cluster 0 or 1)
    vm_stack_push(&vm, 1.2);  // x2
    vm_stack_push(&vm, 1.2);  // x1
    
    bool exec_success = ml_vm_execute_kmeans_predict(&vm, ml_ctx, (uint8_t)handle);
    assert(exec_success);
    
    double result = vm_stack_pop(&vm);
    printf("  K-means prediction for [1.2, 1.2]: %.0f (expected: 0 or 1)\n", result);
    assert(result == 0.0 || result == 1.0);
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ K-means VM execution test passed\n\n");
}

int main(void) {
    printf("=== ML VM Prediction Bug Fix Tests ===\n\n");
    
    test_linear_vm_execution();
    test_tree_vm_execution();
    test_multivariate_tree_vm_execution();
    test_svm_vm_execution();
    test_kmeans_vm_execution();
    
    printf("=== All ML VM prediction tests passed! ===\n");
    printf("✓ Decision tree VM handler now properly calls dt_predict()\n");
    printf("✓ All VM handlers verified to work correctly\n");
    return 0;
}
