
/**
 * @file linear_regression.c
 * @brief Linear Regression ML Primitive Implementation
 */

#include "linear_regression.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Default configuration
LinearRegressionConfig linear_regression_default_config(void) {
    return (LinearRegressionConfig){
        .learning_rate = 0.01,
        .max_iterations = 1000,
        .tolerance = 1e-6,
        .verbose = false
    };
}

// Create model
LinearRegressionModel* linear_regression_create(size_t n_features, LinearRegressionConfig config) {
    if (n_features == 0) {
        return nullptr;
    }
    
    LinearRegressionModel* model = malloc(sizeof(LinearRegressionModel));
    if (!model) {
        return nullptr;
    }
    
    model->weights = calloc(n_features, sizeof(double));
    if (!model->weights) {
        free(model);
        return nullptr;
    }
    
    model->bias = 0.0;
    model->n_features = n_features;
    model->learning_rate = config.learning_rate;
    model->max_iterations = config.max_iterations;
    model->tolerance = config.tolerance;
    model->fitted = false;
    
    return model;
}

// Destroy model
void linear_regression_destroy(LinearRegressionModel* model) {
    if (model) {
        free(model->weights);
        free(model);
    }
}

// Dot product
double linear_regression_dot_product(const double* a, const double* b, size_t size) {
    double result = 0.0;
    for (size_t i = 0; i < size; i++) {
        result += a[i] * b[i];
    }
    return result;
}

// Matrix-vector multiplication
void linear_regression_matrix_vector_multiply(const double* matrix, const double* vector,
                                             double* result, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; i++) {
        result[i] = linear_regression_dot_product(&matrix[i * cols], vector, cols);
    }
}

// Predict single sample
double linear_regression_predict_single(const LinearRegressionModel* model, const double* x) {
    if (!model || !x) {
        return 0.0;
    }
    
    double prediction = model->bias;
    prediction += linear_regression_dot_product(model->weights, x, model->n_features);
    return prediction;
}

// Predict multiple samples
void linear_regression_predict(const LinearRegressionModel* model,
                              const double* X, double* predictions,
                              size_t n_samples, size_t n_features) {
    if (!model || !X || !predictions || n_features != model->n_features) {
        return;
    }
    
    for (size_t i = 0; i < n_samples; i++) {
        predictions[i] = linear_regression_predict_single(model, &X[i * n_features]);
    }
}

// Compute MSE loss
double linear_regression_mse_loss(const LinearRegressionModel* model,
                                 const double* X, const double* y,
                                 size_t n_samples, size_t n_features) {
    if (!model || !X || !y || n_features != model->n_features) {
        return INFINITY;
    }
    
    double total_loss = 0.0;
    for (size_t i = 0; i < n_samples; i++) {
        double prediction = linear_regression_predict_single(model, &X[i * n_features]);
        double error = prediction - y[i];
        total_loss += error * error;
    }
    
    return total_loss / (2.0 * n_samples);
}

// Compute gradients
void linear_regression_compute_gradients(const LinearRegressionModel* model,
                                        const double* X, const double* y,
                                        size_t n_samples, size_t n_features,
                                        double* weight_gradients, double* bias_gradient) {
    if (!model || !X || !y || !weight_gradients || !bias_gradient) {
        return;
    }
    
    // Zero gradients
    memset(weight_gradients, 0, n_features * sizeof(double));
    *bias_gradient = 0.0;
    
    // Compute gradients
    for (size_t i = 0; i < n_samples; i++) {
        double prediction = linear_regression_predict_single(model, &X[i * n_features]);
        double error = prediction - y[i];
        
        // Weight gradients
        for (size_t j = 0; j < n_features; j++) {
            weight_gradients[j] += error * X[i * n_features + j];
        }
        
        // Bias gradient
        *bias_gradient += error;
    }
    
    // Average gradients
    for (size_t j = 0; j < n_features; j++) {
        weight_gradients[j] /= n_samples;
    }
    *bias_gradient /= n_samples;
}

// Fit model using gradient descent
bool linear_regression_fit(LinearRegressionModel* model,
                          const double* X, const double* y,
                          size_t n_samples, size_t n_features) {
    if (!model || !X || !y || n_features != model->n_features || n_samples == 0) {
        return false;
    }
    
    double* weight_gradients = calloc(n_features, sizeof(double));
    if (!weight_gradients) {
        return false;
    }
    
    double prev_loss = INFINITY;
    
    for (size_t iter = 0; iter < model->max_iterations; iter++) {
        // Compute gradients
        double bias_gradient = 0.0;
        linear_regression_compute_gradients(model, X, y, n_samples, n_features,
                                           weight_gradients, &bias_gradient);
        
        // Update weights
        for (size_t j = 0; j < n_features; j++) {
            model->weights[j] -= model->learning_rate * weight_gradients[j];
        }
        model->bias -= model->learning_rate * bias_gradient;
        
        // Check convergence
        if (iter % 100 == 0) {
            double current_loss = linear_regression_mse_loss(model, X, y, n_samples, n_features);
            
            if (model->learning_rate > 0 && fabs(prev_loss - current_loss) < model->tolerance) {
                if (model->learning_rate > 0) {
                    free(weight_gradients);
                    model->fitted = true;
                    return true;
                }
            }
            
            prev_loss = current_loss;
        }
    }
    
    free(weight_gradients);
    model->fitted = true;
    return true;
}
