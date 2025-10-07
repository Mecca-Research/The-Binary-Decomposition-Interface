
/**
 * @file decision_tree.h
 * @brief Decision Tree ML Primitive
 * @details Compiler-native decision tree implementation with CART algorithm
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides ML primitives as first-class compiler features.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_DECISION_TREE_H
#define BDI_DECISION_TREE_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Tree node structure
typedef struct DecisionTreeNode {
    bool is_leaf;
    double value;              // For leaf nodes: prediction value
    size_t feature_index;      // For internal nodes: feature to split on
    double threshold;          // For internal nodes: split threshold
    struct DecisionTreeNode* left;
    struct DecisionTreeNode* right;
    size_t depth;
} DecisionTreeNode;

// Decision tree model
typedef struct {
    DecisionTreeNode* root;
    size_t max_depth;
    size_t min_samples_split;
    size_t min_samples_leaf;
    double min_impurity_decrease;
    size_t n_features;     // Number of features (set during fit)
    bool fitted;
} DecisionTreeModel;

// Training configuration
typedef struct {
    size_t max_depth;
    size_t min_samples_split;
    size_t min_samples_leaf;
    double min_impurity_decrease;
} DecisionTreeConfig;

// Model lifecycle
DecisionTreeModel* decision_tree_create(DecisionTreeConfig config);
void decision_tree_destroy(DecisionTreeModel* model);
DecisionTreeConfig decision_tree_default_config(void);

// Training
bool decision_tree_fit(DecisionTreeModel* model,
                      const double* X, const double* y,
                      size_t n_samples, size_t n_features);

// Prediction
double decision_tree_predict_single(const DecisionTreeModel* model, const double* x);
void decision_tree_predict(const DecisionTreeModel* model,
                          const double* X, double* predictions,
                          size_t n_samples, size_t n_features);

// Impurity calculations
double decision_tree_gini_impurity(const double* y, size_t n_samples);
double decision_tree_information_gain(const double* y, size_t n_samples,
                                     const bool* left_mask, size_t n_left);

// Node operations
DecisionTreeNode* decision_tree_node_create(void);
void decision_tree_node_destroy(DecisionTreeNode* node);

// Compile-time invariants
static_assert(sizeof(double) == 8, "Decision tree requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "Decision tree requires at least 32-bit size_t");

#endif // BDI_DECISION_TREE_H
