
/**
 * @file ml_ops.h
 * @brief ML Operations for VM Bytecode Execution
 * @details VM execution handlers for ML primitives
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides VM execution support for ML primitives.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_ML_OPS_H
#define BDI_ML_OPS_H

#include "../../c23_compat.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include "../AIBase/linear/linear_regression.h"
#include "../AIBase/tree/decision_tree.h"
#include "../AIBase/kernel/svm.h"
#include "../AIBase/clustering/kmeans.h"
#include <stddef.h>
#include <stdbool.h>

// ML model storage in VM
typedef struct {
    void* model;
    uint8_t model_type; // 0=linear, 1=tree, 2=svm, 3=kmeans
} MLModelHandle;

// VM ML context
typedef struct {
    MLModelHandle* models;
    size_t num_models;
    size_t capacity;
} MLVMContext;

// Context management
MLVMContext* ml_vm_context_create(void);
void ml_vm_context_destroy(MLVMContext* ctx);

// Model registration
size_t ml_vm_register_linear_regression(MLVMContext* ctx, LinearRegressionModel* model);
size_t ml_vm_register_decision_tree(MLVMContext* ctx, DecisionTreeModel* model);
size_t ml_vm_register_svm(MLVMContext* ctx, SVMModel* model);
size_t ml_vm_register_kmeans(MLVMContext* ctx, KMeansModel* model);

// Model retrieval
LinearRegressionModel* ml_vm_get_linear_regression(MLVMContext* ctx, size_t handle);
DecisionTreeModel* ml_vm_get_decision_tree(MLVMContext* ctx, size_t handle);
SVMModel* ml_vm_get_svm(MLVMContext* ctx, size_t handle);
KMeansModel* ml_vm_get_kmeans(MLVMContext* ctx, size_t handle);

// Opcode execution handlers
bool ml_vm_execute_linear_reg_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_linear_reg_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_tree_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_tree_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_svm_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_svm_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_kmeans_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);
bool ml_vm_execute_kmeans_train(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx);

// Compile-time invariants
static_assert(sizeof(MLModelHandle) <= 16, "MLModelHandle should be compact");

#endif // BDI_ML_OPS_H
