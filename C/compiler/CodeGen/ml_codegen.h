
/**
 * @file ml_codegen.h
 * @brief ML Primitives Code Generation
 * @details Generates IR and bytecode for ML algorithms
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides code generation for ML primitives.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_ML_CODEGEN_H
#define BDI_ML_CODEGEN_H

#include "../../c23_compat.h"
#include "../../vm/bci_chunk.h"
#include "../AIBase/linear/linear_regression.h"
#include "../AIBase/tree/decision_tree.h"
#include "../AIBase/kernel/svm.h"
#include "../AIBase/clustering/kmeans.h"
#include <stddef.h>
#include <stdbool.h>

// ML-specific opcodes (extending base VM opcodes)
typedef enum {
    // Linear Regression opcodes
    OP_ML_LINEAR_REG_PREDICT = 100,
    OP_ML_LINEAR_REG_TRAIN,
    OP_ML_LINEAR_REG_LOSS,
    
    // Decision Tree opcodes
    OP_ML_TREE_PREDICT = 110,
    OP_ML_TREE_TRAIN,
    OP_ML_TREE_SPLIT,
    
    // SVM opcodes
    OP_ML_SVM_PREDICT = 120,
    OP_ML_SVM_TRAIN,
    OP_ML_SVM_KERNEL,
    
    // K-means opcodes
    OP_ML_KMEANS_PREDICT = 130,
    OP_ML_KMEANS_TRAIN,
    OP_ML_KMEANS_ASSIGN,
    OP_ML_KMEANS_UPDATE_CENTROIDS
} MLOpCode;

// Model serialization structures
typedef struct {
    uint8_t model_type;  // 0=linear, 1=tree, 2=svm, 3=kmeans
    size_t data_size;
    uint8_t* data;
} SerializedModel;

// Code generation functions

// Linear Regression
bool emit_linear_regression_model(Chunk* chunk, const LinearRegressionModel* model);
bool emit_linear_regression_predict(Chunk* chunk, size_t model_constant_idx);
bool emit_linear_regression_train(Chunk* chunk, size_t data_idx, size_t labels_idx);

// Decision Tree
bool emit_decision_tree_model(Chunk* chunk, const DecisionTreeModel* model);
bool emit_decision_tree_predict(Chunk* chunk, size_t model_constant_idx);
bool emit_decision_tree_train(Chunk* chunk, size_t data_idx, size_t labels_idx);

// SVM
bool emit_svm_model(Chunk* chunk, const SVMModel* model);
bool emit_svm_predict(Chunk* chunk, size_t model_constant_idx);
bool emit_svm_train(Chunk* chunk, size_t data_idx, size_t labels_idx);

// K-means
bool emit_kmeans_model(Chunk* chunk, const KMeansModel* model);
bool emit_kmeans_predict(Chunk* chunk, size_t model_constant_idx);
bool emit_kmeans_train(Chunk* chunk, size_t data_idx);

// Model serialization
SerializedModel* serialize_linear_regression(const LinearRegressionModel* model);
SerializedModel* serialize_decision_tree(const DecisionTreeModel* model);
SerializedModel* serialize_svm(const SVMModel* model);
SerializedModel* serialize_kmeans(const KMeansModel* model);
void serialized_model_destroy(SerializedModel* model);

// Model deserialization
LinearRegressionModel* deserialize_linear_regression(const SerializedModel* data);
DecisionTreeModel* deserialize_decision_tree(const SerializedModel* data);
SVMModel* deserialize_svm(const SerializedModel* data);
KMeansModel* deserialize_kmeans(const SerializedModel* data);

// Compile-time invariants
static_assert(sizeof(MLOpCode) == sizeof(int), "MLOpCode must be int-sized");

#endif // BDI_ML_CODEGEN_H
