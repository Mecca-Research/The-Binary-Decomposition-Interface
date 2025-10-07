
/**
 * @file svm.h
 * @brief Support Vector Machine ML Primitive
 * @details Compiler-native SVM implementation with kernel methods
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides ML primitives as first-class compiler features.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_SVM_H
#define BDI_SVM_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Kernel types
typedef enum {
    SVM_KERNEL_LINEAR,
    SVM_KERNEL_RBF,
    SVM_KERNEL_POLYNOMIAL
} SVMKernelType;

// SVM model structure
typedef struct {
    double* support_vectors;    // Support vectors (n_sv * n_features)
    double* alphas;            // Lagrange multipliers
    double* support_labels;    // Labels for support vectors
    size_t n_support_vectors;
    size_t n_features;
    double bias;
    SVMKernelType kernel_type;
    double gamma;              // RBF kernel parameter
    double C;                  // Regularization parameter
    double tolerance;          // Training tolerance
    size_t max_iterations;     // Maximum training iterations
    bool fitted;
} SVMModel;

// Training configuration
typedef struct {
    SVMKernelType kernel_type;
    double C;
    double gamma;
    double tolerance;
    size_t max_iterations;
} SVMConfig;

// Model lifecycle
SVMModel* svm_create(size_t n_features, SVMConfig config);
void svm_destroy(SVMModel* model);
SVMConfig svm_default_config(void);

// Training (simplified SMO algorithm)
bool svm_fit(SVMModel* model,
            const double* X, const double* y,
            size_t n_samples, size_t n_features);

// Prediction
double svm_predict_single(const SVMModel* model, const double* x);
void svm_predict(const SVMModel* model,
                const double* X, double* predictions,
                size_t n_samples, size_t n_features);

// Kernel functions
double svm_kernel_linear(const double* x1, const double* x2, size_t n_features);
double svm_kernel_rbf(const double* x1, const double* x2, size_t n_features, double gamma);
double svm_kernel_compute(const SVMModel* model, const double* x1, const double* x2);

// Decision function
double svm_decision_function(const SVMModel* model, const double* x);

// Compile-time invariants
static_assert(sizeof(double) == 8, "SVM requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "SVM requires at least 32-bit size_t");

#endif // BDI_SVM_H
