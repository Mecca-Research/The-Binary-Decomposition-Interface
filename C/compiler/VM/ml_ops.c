
/**
 * @file ml_ops.c
 * @brief ML Operations for VM Bytecode Execution Implementation
 */

#include "ml_ops.h"
#include <stdlib.h>
#include <string.h>

// Create ML VM context
MLVMContext* ml_vm_context_create(void) {
    MLVMContext* ctx = malloc(sizeof(MLVMContext));
    if (!ctx) return nullptr;
    
    ctx->capacity = 16;
    ctx->num_models = 0;
    ctx->models = calloc(ctx->capacity, sizeof(MLModelHandle));
    
    if (!ctx->models) {
        free(ctx);
        return nullptr;
    }
    
    return ctx;
}

// Destroy ML VM context
void ml_vm_context_destroy(MLVMContext* ctx) {
    if (!ctx) return;
    
    // Free all registered models
    for (size_t i = 0; i < ctx->num_models; i++) {
        if (ctx->models[i].model) {
            switch (ctx->models[i].model_type) {
                case 0:
                    linear_regression_destroy((LinearRegressionModel*)ctx->models[i].model);
                    break;
                case 1:
                    decision_tree_destroy((DecisionTreeModel*)ctx->models[i].model);
                    break;
                case 2:
                    svm_destroy((SVMModel*)ctx->models[i].model);
                    break;
                case 3:
                    kmeans_destroy((KMeansModel*)ctx->models[i].model);
                    break;
            }
        }
    }
    
    free(ctx->models);
    free(ctx);
}

// Register linear regression model
size_t ml_vm_register_linear_regression(MLVMContext* ctx, LinearRegressionModel* model) {
    if (!ctx || !model) return (size_t)-1;
    
    if (ctx->num_models >= ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        MLModelHandle* new_models = realloc(ctx->models, new_capacity * sizeof(MLModelHandle));
        if (!new_models) return (size_t)-1;
        
        ctx->models = new_models;
        ctx->capacity = new_capacity;
    }
    
    size_t handle = ctx->num_models;
    ctx->models[handle].model = model;
    ctx->models[handle].model_type = 0;
    ctx->num_models++;
    
    return handle;
}

// Register decision tree model
size_t ml_vm_register_decision_tree(MLVMContext* ctx, DecisionTreeModel* model) {
    if (!ctx || !model) return (size_t)-1;
    
    if (ctx->num_models >= ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        MLModelHandle* new_models = realloc(ctx->models, new_capacity * sizeof(MLModelHandle));
        if (!new_models) return (size_t)-1;
        
        ctx->models = new_models;
        ctx->capacity = new_capacity;
    }
    
    size_t handle = ctx->num_models;
    ctx->models[handle].model = model;
    ctx->models[handle].model_type = 1;
    ctx->num_models++;
    
    return handle;
}

// Register SVM model
size_t ml_vm_register_svm(MLVMContext* ctx, SVMModel* model) {
    if (!ctx || !model) return (size_t)-1;
    
    if (ctx->num_models >= ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        MLModelHandle* new_models = realloc(ctx->models, new_capacity * sizeof(MLModelHandle));
        if (!new_models) return (size_t)-1;
        
        ctx->models = new_models;
        ctx->capacity = new_capacity;
    }
    
    size_t handle = ctx->num_models;
    ctx->models[handle].model = model;
    ctx->models[handle].model_type = 2;
    ctx->num_models++;
    
    return handle;
}

// Register K-means model
size_t ml_vm_register_kmeans(MLVMContext* ctx, KMeansModel* model) {
    if (!ctx || !model) return (size_t)-1;
    
    if (ctx->num_models >= ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        MLModelHandle* new_models = realloc(ctx->models, new_capacity * sizeof(MLModelHandle));
        if (!new_models) return (size_t)-1;
        
        ctx->models = new_models;
        ctx->capacity = new_capacity;
    }
    
    size_t handle = ctx->num_models;
    ctx->models[handle].model = model;
    ctx->models[handle].model_type = 3;
    ctx->num_models++;
    
    return handle;
}

// Get linear regression model
LinearRegressionModel* ml_vm_get_linear_regression(MLVMContext* ctx, size_t handle) {
    if (!ctx || handle >= ctx->num_models || ctx->models[handle].model_type != 0) {
        return nullptr;
    }
    return (LinearRegressionModel*)ctx->models[handle].model;
}

// Get decision tree model
DecisionTreeModel* ml_vm_get_decision_tree(MLVMContext* ctx, size_t handle) {
    if (!ctx || handle >= ctx->num_models || ctx->models[handle].model_type != 1) {
        return nullptr;
    }
    return (DecisionTreeModel*)ctx->models[handle].model;
}

// Get SVM model
SVMModel* ml_vm_get_svm(MLVMContext* ctx, size_t handle) {
    if (!ctx || handle >= ctx->num_models || ctx->models[handle].model_type != 2) {
        return nullptr;
    }
    return (SVMModel*)ctx->models[handle].model;
}

// Get K-means model
KMeansModel* ml_vm_get_kmeans(MLVMContext* ctx, size_t handle) {
    if (!ctx || handle >= ctx->num_models || ctx->models[handle].model_type != 3) {
        return nullptr;
    }
    return (KMeansModel*)ctx->models[handle].model;
}

// Execute linear regression predict
bool ml_vm_execute_linear_reg_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    LinearRegressionModel* model = ml_vm_get_linear_regression(ml_ctx, model_idx);
    if (!model) return false;
    
    // Pop feature values from stack
    double* features = malloc(model->n_features * sizeof(double));
    if (!features) return false;
    
    for (size_t i = model->n_features; i > 0; i--) {
        features[i - 1] = vm_stack_pop(vm);
    }
    
    // Predict
    double prediction = linear_regression_predict_single(model, features);
    
    // Push result
    vm_stack_push(vm, prediction);
    
    free(features);
    return true;
}

// Execute decision tree predict
bool ml_vm_execute_tree_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    DecisionTreeModel* model = ml_vm_get_decision_tree(ml_ctx, model_idx);
    if (!model || !model->fitted) return false;
    
    // Pop feature values from stack (in reverse order)
    double* features = malloc(model->n_features * sizeof(double));
    if (!features) return false;
    
    for (size_t i = model->n_features; i > 0; i--) {
        features[i - 1] = vm_stack_pop(vm);
    }
    
    // Predict using the decision tree model
    double prediction = decision_tree_predict_single(model, features);
    
    // Clean up and push result
    free(features);
    vm_stack_push(vm, prediction);
    
    return true;
}

// Execute SVM predict
bool ml_vm_execute_svm_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    SVMModel* model = ml_vm_get_svm(ml_ctx, model_idx);
    if (!model) return false;
    
    // Pop feature values from stack
    double* features = malloc(model->n_features * sizeof(double));
    if (!features) return false;
    
    for (size_t i = model->n_features; i > 0; i--) {
        features[i - 1] = vm_stack_pop(vm);
    }
    
    // Predict
    double prediction = svm_predict_single(model, features);
    
    // Push result
    vm_stack_push(vm, prediction);
    
    free(features);
    return true;
}

// Execute K-means predict
bool ml_vm_execute_kmeans_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    KMeansModel* model = ml_vm_get_kmeans(ml_ctx, model_idx);
    if (!model) return false;
    
    // Pop feature values from stack
    double* features = malloc(model->n_features * sizeof(double));
    if (!features) return false;
    
    for (size_t i = model->n_features; i > 0; i--) {
        features[i - 1] = vm_stack_pop(vm);
    }
    
    // Predict
    size_t cluster = kmeans_predict_single(model, features);
    
    // Push result
    vm_stack_push(vm, (double)cluster);
    
    free(features);
    return true;
}

// Training operations (simplified - would need data loading from VM memory)
bool ml_vm_execute_linear_reg_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    // Placeholder - full implementation would load training data from VM memory
    return true;
}

bool ml_vm_execute_tree_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    // Placeholder
    return true;
}

bool ml_vm_execute_svm_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    // Placeholder
    return true;
}

bool ml_vm_execute_kmeans_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    // Placeholder
    return true;
}
