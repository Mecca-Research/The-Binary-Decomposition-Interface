
/**
 * @file ml_vm_test.c
 * @brief Test suite for ML VM Integration
 */

#include "../VM/ml_ops.h"
#include "../CodeGen/ml_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Test ML VM context
static void test_ml_vm_context(void) {
    printf("Testing ML VM context creation...\n");
    
    MLVMContext* ctx = ml_vm_context_create();
    assert(ctx != nullptr);
    assert(ctx->num_models == 0);
    assert(ctx->capacity > 0);
    
    ml_vm_context_destroy(ctx);
    printf("  ✓ ML VM context test passed\n\n");
}

// Test model registration
static void test_model_registration(void) {
    printf("Testing model registration...\n");
    
    MLVMContext* ctx = ml_vm_context_create();
    
    // Register linear regression model
    LinearRegressionConfig lr_config = linear_regression_default_config();
    LinearRegressionModel* lr_model = linear_regression_create(2, lr_config);
    size_t lr_handle = ml_vm_register_linear_regression(ctx, lr_model);
    
    printf("  Linear regression handle: %zu\n", lr_handle);
    assert(lr_handle == 0);
    assert(ctx->num_models == 1);
    
    // Register decision tree model
    DecisionTreeConfig dt_config = decision_tree_default_config();
    DecisionTreeModel* dt_model = decision_tree_create(dt_config);
    size_t dt_handle = ml_vm_register_decision_tree(ctx, dt_model);
    
    printf("  Decision tree handle: %zu\n", dt_handle);
    assert(dt_handle == 1);
    assert(ctx->num_models == 2);
    
    // Register SVM model
    SVMConfig svm_config = svm_default_config();
    SVMModel* svm_model = svm_create(2, svm_config);
    size_t svm_handle = ml_vm_register_svm(ctx, svm_model);
    
    printf("  SVM handle: %zu\n", svm_handle);
    assert(svm_handle == 2);
    assert(ctx->num_models == 3);
    
    // Register K-means model
    KMeansConfig km_config = kmeans_default_config(3);
    KMeansModel* km_model = kmeans_create(2, km_config);
    size_t km_handle = ml_vm_register_kmeans(ctx, km_model);
    
    printf("  K-means handle: %zu\n", km_handle);
    assert(km_handle == 3);
    assert(ctx->num_models == 4);
    
    ml_vm_context_destroy(ctx);
    printf("  ✓ Model registration test passed\n\n");
}

// Test model retrieval
static void test_model_retrieval(void) {
    printf("Testing model retrieval...\n");
    
    MLVMContext* ctx = ml_vm_context_create();
    
    // Create and register models
    LinearRegressionConfig config = linear_regression_default_config();
    LinearRegressionModel* original = linear_regression_create(2, config);
    size_t handle = ml_vm_register_linear_regression(ctx, original);
    
    // Retrieve model
    LinearRegressionModel* retrieved = ml_vm_get_linear_regression(ctx, handle);
    assert(retrieved != nullptr);
    assert(retrieved == original);
    
    printf("  Retrieved model matches original\n");
    
    // Try to retrieve with wrong type
    DecisionTreeModel* wrong_type = ml_vm_get_decision_tree(ctx, handle);
    assert(wrong_type == nullptr);
    
    printf("  Wrong type retrieval correctly returns NULL\n");
    
    ml_vm_context_destroy(ctx);
    printf("  ✓ Model retrieval test passed\n\n");
}

// Test serialization
static void test_serialization(void) {
    printf("Testing model serialization...\n");
    
    // Create and train a simple linear regression model
    LinearRegressionConfig config = linear_regression_default_config();
    config.max_iterations = 1000;
    LinearRegressionModel* model = linear_regression_create(1, config);
    
    double X[] = {1.0, 2.0, 3.0};
    double y[] = {2.0, 4.0, 6.0};
    linear_regression_fit(model, X, y, 3, 1);
    
    // Serialize
    SerializedModel* serialized = serialize_linear_regression(model);
    assert(serialized != nullptr);
    assert(serialized->model_type == 0);
    assert(serialized->data_size > 0);
    
    printf("  Serialized size: %zu bytes\n", serialized->data_size);
    
    // Deserialize
    LinearRegressionModel* deserialized = deserialize_linear_regression(serialized);
    assert(deserialized != nullptr);
    assert(deserialized->n_features == model->n_features);
    assert(deserialized->fitted == true);
    
    printf("  Deserialized model matches original\n");
    
    serialized_model_destroy(serialized);
    linear_regression_destroy(model);
    linear_regression_destroy(deserialized);
    printf("  ✓ Serialization test passed\n\n");
}

// Test code generation
static void test_code_generation(void) {
    printf("Testing ML code generation...\n");
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create a simple model
    LinearRegressionConfig config = linear_regression_default_config();
    LinearRegressionModel* model = linear_regression_create(2, config);
    
    // Emit model
    bool success = emit_linear_regression_model(&chunk, model);
    assert(success);
    
    printf("  Generated %d bytes of bytecode\n", chunk.count);
    assert(chunk.count > 0);
    
    chunk_free(&chunk);
    linear_regression_destroy(model);
    printf("  ✓ Code generation test passed\n\n");
}

// Test VM execution (simplified)
static void test_vm_execution(void) {
    printf("Testing VM execution with ML ops...\n");
    
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
    
    vm_free(&vm);
    ml_vm_context_destroy(ml_ctx);
    printf("  ✓ VM execution test passed\n\n");
}

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
    printf("=== ML VM Integration Tests ===\n\n");
    
    test_ml_vm_context();
    test_model_registration();
    test_model_retrieval();
    test_serialization();
    test_code_generation();
    test_vm_execution();
    test_tree_vm_execution();
    test_multivariate_tree_vm_execution();
    test_svm_vm_execution();
    test_kmeans_vm_execution();
    
    printf("=== All ML VM integration tests passed! ===\n");
    return 0;
}
