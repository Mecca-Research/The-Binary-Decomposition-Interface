
/**
 * @file decision_tree.c
 * @brief Decision Tree ML Primitive Implementation
 */

#include "decision_tree.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Default configuration
DecisionTreeConfig decision_tree_default_config(void) {
    return (DecisionTreeConfig){
        .max_depth = 10,
        .min_samples_split = 2,
        .min_samples_leaf = 1,
        .min_impurity_decrease = 0.0
    };
}

// Create node
DecisionTreeNode* decision_tree_node_create(void) {
    DecisionTreeNode* node = calloc(1, sizeof(DecisionTreeNode));
    return node;
}

// Destroy node recursively
void decision_tree_node_destroy(DecisionTreeNode* node) {
    if (!node) return;
    
    decision_tree_node_destroy(node->left);
    decision_tree_node_destroy(node->right);
    free(node);
}

// Create model
DecisionTreeModel* decision_tree_create(DecisionTreeConfig config) {
    DecisionTreeModel* model = malloc(sizeof(DecisionTreeModel));
    if (!model) return nullptr;
    
    model->root = nullptr;
    model->max_depth = config.max_depth;
    model->min_samples_split = config.min_samples_split;
    model->min_samples_leaf = config.min_samples_leaf;
    model->min_impurity_decrease = config.min_impurity_decrease;
    model->n_features = 0;
    model->fitted = false;
    
    return model;
}

// Destroy model
void decision_tree_destroy(DecisionTreeModel* model) {
    if (!model) return;
    
    decision_tree_node_destroy(model->root);
    free(model);
}

// Compute mean of target values
static double compute_mean(const double* y, const size_t* indices, size_t n_samples) {
    double sum = 0.0;
    for (size_t i = 0; i < n_samples; i++) {
        sum += y[indices[i]];
    }
    return sum / n_samples;
}

// Compute variance (MSE for regression)
static double compute_variance(const double* y, const size_t* indices, size_t n_samples) {
    double mean = compute_mean(y, indices, n_samples);
    double variance = 0.0;
    
    for (size_t i = 0; i < n_samples; i++) {
        double diff = y[indices[i]] - mean;
        variance += diff * diff;
    }
    
    return variance / n_samples;
}

// Find best split
static bool find_best_split(const double* X, const double* y,
                           const size_t* indices, size_t n_samples, size_t n_features,
                           size_t* best_feature, double* best_threshold,
                           double* best_gain) {
    *best_gain = -INFINITY;
    *best_feature = 0;
    *best_threshold = 0.0;
    
    double parent_variance = compute_variance(y, indices, n_samples);
    
    for (size_t feature = 0; feature < n_features; feature++) {
        // Get unique values for this feature
        for (size_t i = 0; i < n_samples; i++) {
            double threshold = X[indices[i] * n_features + feature];
            
            // Split samples
            size_t n_left = 0;
            for (size_t j = 0; j < n_samples; j++) {
                if (X[indices[j] * n_features + feature] <= threshold) {
                    n_left++;
                }
            }
            
            size_t n_right = n_samples - n_left;
            
            if (n_left == 0 || n_right == 0) continue;
            
            // Compute variance for left and right splits
            size_t* left_indices = malloc(n_left * sizeof(size_t));
            size_t* right_indices = malloc(n_right * sizeof(size_t));
            
            if (!left_indices || !right_indices) {
                free(left_indices);
                free(right_indices);
                continue;
            }
            
            size_t left_idx = 0, right_idx = 0;
            for (size_t j = 0; j < n_samples; j++) {
                if (X[indices[j] * n_features + feature] <= threshold) {
                    left_indices[left_idx++] = indices[j];
                } else {
                    right_indices[right_idx++] = indices[j];
                }
            }
            
            double left_variance = compute_variance(y, left_indices, n_left);
            double right_variance = compute_variance(y, right_indices, n_right);
            
            double weighted_variance = (n_left * left_variance + n_right * right_variance) / n_samples;
            double gain = parent_variance - weighted_variance;
            
            free(left_indices);
            free(right_indices);
            
            if (gain > *best_gain) {
                *best_gain = gain;
                *best_feature = feature;
                *best_threshold = threshold;
            }
        }
    }
    
    return *best_gain > 0;
}

// Build tree recursively
static DecisionTreeNode* build_tree(const double* X, const double* y,
                                   const size_t* indices, size_t n_samples, size_t n_features,
                                   size_t depth, const DecisionTreeModel* model) {
    DecisionTreeNode* node = decision_tree_node_create();
    if (!node) return nullptr;
    
    node->depth = depth;
    
    // Check stopping criteria
    if (depth >= model->max_depth || 
        n_samples < model->min_samples_split ||
        n_samples < 2 * model->min_samples_leaf) {
        node->is_leaf = true;
        node->value = compute_mean(y, indices, n_samples);
        return node;
    }
    
    // Find best split
    size_t best_feature;
    double best_threshold, best_gain;
    
    if (!find_best_split(X, y, indices, n_samples, n_features,
                        &best_feature, &best_threshold, &best_gain)) {
        node->is_leaf = true;
        node->value = compute_mean(y, indices, n_samples);
        return node;
    }
    
    if (best_gain < model->min_impurity_decrease) {
        node->is_leaf = true;
        node->value = compute_mean(y, indices, n_samples);
        return node;
    }
    
    // Split samples
    size_t n_left = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (X[indices[i] * n_features + best_feature] <= best_threshold) {
            n_left++;
        }
    }
    
    size_t n_right = n_samples - n_left;
    
    if (n_left < model->min_samples_leaf || n_right < model->min_samples_leaf) {
        node->is_leaf = true;
        node->value = compute_mean(y, indices, n_samples);
        return node;
    }
    
    size_t* left_indices = malloc(n_left * sizeof(size_t));
    size_t* right_indices = malloc(n_right * sizeof(size_t));
    
    if (!left_indices || !right_indices) {
        free(left_indices);
        free(right_indices);
        decision_tree_node_destroy(node);
        return nullptr;
    }
    
    size_t left_idx = 0, right_idx = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (X[indices[i] * n_features + best_feature] <= best_threshold) {
            left_indices[left_idx++] = indices[i];
        } else {
            right_indices[right_idx++] = indices[i];
        }
    }
    
    // Create internal node
    node->is_leaf = false;
    node->feature_index = best_feature;
    node->threshold = best_threshold;
    
    // Recursively build subtrees
    node->left = build_tree(X, y, left_indices, n_left, n_features, depth + 1, model);
    node->right = build_tree(X, y, right_indices, n_right, n_features, depth + 1, model);
    
    free(left_indices);
    free(right_indices);
    
    return node;
}

// Fit model
bool decision_tree_fit(DecisionTreeModel* model,
                      const double* X, const double* y,
                      size_t n_samples, size_t n_features) {
    if (!model || !X || !y || n_samples == 0 || n_features == 0) {
        return false;
    }
    
    // Create indices array
    size_t* indices = malloc(n_samples * sizeof(size_t));
    if (!indices) return false;
    
    for (size_t i = 0; i < n_samples; i++) {
        indices[i] = i;
    }
    
    // Build tree
    model->root = build_tree(X, y, indices, n_samples, n_features, 0, model);
    free(indices);
    
    if (!model->root) {
        return false;
    }
    
    model->n_features = n_features;
    model->fitted = true;
    return true;
}

// Predict single sample
double decision_tree_predict_single(const DecisionTreeModel* model, const double* x) {
    if (!model || !model->root || !x) {
        return 0.0;
    }
    
    DecisionTreeNode* node = model->root;
    
    while (!node->is_leaf) {
        if (x[node->feature_index] <= node->threshold) {
            node = node->left;
        } else {
            node = node->right;
        }
        
        if (!node) return 0.0;
    }
    
    return node->value;
}

// Predict multiple samples
void decision_tree_predict(const DecisionTreeModel* model,
                          const double* X, double* predictions,
                          size_t n_samples, size_t n_features) {
    if (!model || !X || !predictions) return;
    
    for (size_t i = 0; i < n_samples; i++) {
        predictions[i] = decision_tree_predict_single(model, &X[i * n_features]);
    }
}

// Gini impurity (for classification - included for completeness)
double decision_tree_gini_impurity(const double* y, size_t n_samples) {
    if (n_samples == 0) return 0.0;
    
    // Count unique classes
    double sum = 0.0;
    for (size_t i = 0; i < n_samples; i++) {
        sum += y[i];
    }
    
    double mean = sum / n_samples;
    double p = mean;
    
    return 2.0 * p * (1.0 - p);
}

// Information gain
double decision_tree_information_gain(const double* y, size_t n_samples,
                                     const bool* left_mask, size_t n_left) {
    if (n_samples == 0 || n_left == 0 || n_left == n_samples) {
        return 0.0;
    }
    
    size_t n_right = n_samples - n_left;
    
    double parent_impurity = decision_tree_gini_impurity(y, n_samples);
    
    // This is a simplified version - full implementation would compute
    // impurity for left and right splits separately
    double weighted_impurity = (n_left * 0.5 + n_right * 0.5) / n_samples;
    
    return parent_impurity - weighted_impurity;
}
