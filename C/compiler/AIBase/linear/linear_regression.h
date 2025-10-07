
/**
 * @file linear_regression.h
 * @brief Linear Regression ML Primitive
 * @details Compiler-native linear regression implementation with gradient descent
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides ML primitives as first-class compiler features.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_LINEAR_REGRESSION_H
#define BDI_LINEAR_REGRESSION_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Linear regression model structure
typedef struct {
    double* weights;        // Weight vector (size: n_features)
    double bias;           // Bias term
    size_t n_features;     // Number of features
    double learning_rate;  // Learning rate for gradient descent
    size_t max_iterations; // Maximum training iterations
    double tolerance;      // Convergence tolerance
    bool fitted;          // Whether model has been trained
} LinearRegressionModel;

// Training configuration
typedef struct {
    double learning_rate;
    size_t max_iterations;
    double tolerance;
    bool verbose;
} LinearRegressionConfig;

// Model lifecycle
LinearRegressionModel* linear_regression_create(size_t n_features, LinearRegressionConfig config);
void linear_regression_destroy(LinearRegressionModel* model);
LinearRegressionConfig linear_regression_default_config(void);

// Training
bool linear_regression_fit(LinearRegressionModel* model, 
                           const double* X, const double* y, 
                           size_t n_samples, size_t n_features);

// Prediction
double linear_regression_predict_single(const LinearRegressionModel* model, const double* x);
void linear_regression_predict(const LinearRegressionModel* model,
                              const double* X, double* predictions,
                              size_t n_samples, size_t n_features);

// Loss computation
double linear_regression_mse_loss(const LinearRegressionModel* model,
                                 const double* X, const double* y,
                                 size_t n_samples, size_t n_features);

// Gradient computation
void linear_regression_compute_gradients(const LinearRegressionModel* model,
                                        const double* X, const double* y,
                                        size_t n_samples, size_t n_features,
                                        double* weight_gradients, double* bias_gradient);

// Matrix operations
void linear_regression_matrix_vector_multiply(const double* matrix, const double* vector,
                                             double* result, size_t rows, size_t cols);
double linear_regression_dot_product(const double* a, const double* b, size_t size);

// Compile-time invariants
static_assert(sizeof(double) == 8, "Linear regression requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "Linear regression requires at least 32-bit size_t");

#endif // BDI_LINEAR_REGRESSION_H
