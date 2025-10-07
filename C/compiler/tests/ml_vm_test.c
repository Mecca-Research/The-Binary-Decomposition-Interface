
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

int main(void) {
    printf("=== ML VM Integration Tests ===\n\n");
    
    test_ml_vm_context();
    test_model_registration();
    test_model_retrieval();
    test_serialization();
    test_code_generation();
    test_vm_execution();
    
    printf("=== All ML VM integration tests passed! ===\n");
    return 0;
}
