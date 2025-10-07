
/**
 * @file svm.c
 * @brief Support Vector Machine ML Primitive Implementation
 */

#include "svm.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Default configuration
SVMConfig svm_default_config(void) {
    return (SVMConfig){
        .kernel_type = SVM_KERNEL_RBF,
        .C = 1.0,
        .gamma = 0.1,
        .tolerance = 1e-3,
        .max_iterations = 1000
    };
}

// Create model
SVMModel* svm_create(size_t n_features, SVMConfig config) {
    if (n_features == 0) return nullptr;
    
    SVMModel* model = malloc(sizeof(SVMModel));
    if (!model) return nullptr;
    
    model->support_vectors = nullptr;
    model->alphas = nullptr;
    model->support_labels = nullptr;
    model->n_support_vectors = 0;
    model->n_features = n_features;
    model->bias = 0.0;
    model->kernel_type = config.kernel_type;
    model->gamma = config.gamma;
    model->C = config.C;
    model->tolerance = config.tolerance;
    model->max_iterations = config.max_iterations;
    model->fitted = false;
    
    return model;
}

// Destroy model
void svm_destroy(SVMModel* model) {
    if (!model) return;
    
    free(model->support_vectors);
    free(model->alphas);
    free(model->support_labels);
    free(model);
}

// Linear kernel
double svm_kernel_linear(const double* x1, const double* x2, size_t n_features) {
    double result = 0.0;
    for (size_t i = 0; i < n_features; i++) {
        result += x1[i] * x2[i];
    }
    return result;
}

// RBF kernel
double svm_kernel_rbf(const double* x1, const double* x2, size_t n_features, double gamma) {
    double squared_distance = 0.0;
    for (size_t i = 0; i < n_features; i++) {
        double diff = x1[i] - x2[i];
        squared_distance += diff * diff;
    }
    return exp(-gamma * squared_distance);
}

// Compute kernel
double svm_kernel_compute(const SVMModel* model, const double* x1, const double* x2) {
    switch (model->kernel_type) {
        case SVM_KERNEL_LINEAR:
            return svm_kernel_linear(x1, x2, model->n_features);
        case SVM_KERNEL_RBF:
            return svm_kernel_rbf(x1, x2, model->n_features, model->gamma);
        default:
            return 0.0;
    }
}

// Decision function
double svm_decision_function(const SVMModel* model, const double* x) {
    if (!model || !x || !model->fitted) {
        return 0.0;
    }
    
    double result = model->bias;
    
    for (size_t i = 0; i < model->n_support_vectors; i++) {
        const double* sv = &model->support_vectors[i * model->n_features];
        double kernel_value = svm_kernel_compute(model, sv, x);
        result += model->alphas[i] * model->support_labels[i] * kernel_value;
    }
    
    return result;
}

// Predict single sample
double svm_predict_single(const SVMModel* model, const double* x) {
    double decision = svm_decision_function(model, x);
    return (decision >= 0.0) ? 1.0 : -1.0;
}

// Predict multiple samples
void svm_predict(const SVMModel* model,
                const double* X, double* predictions,
                size_t n_samples, size_t n_features) {
    if (!model || !X || !predictions || n_features != model->n_features) {
        return;
    }
    
    for (size_t i = 0; i < n_samples; i++) {
        predictions[i] = svm_predict_single(model, &X[i * n_features]);
    }
}

// Simplified SMO algorithm for training
bool svm_fit(SVMModel* model,
            const double* X, const double* y,
            size_t n_samples, size_t n_features) {
    if (!model || !X || !y || n_features != model->n_features || n_samples == 0) {
        return false;
    }
    
    // Allocate temporary arrays for training
    double* alphas = calloc(n_samples, sizeof(double));
    double* errors = calloc(n_samples, sizeof(double));
    
    if (!alphas || !errors) {
        free(alphas);
        free(errors);
        return false;
    }
    
    double bias = 0.0;
    
    // Compute kernel matrix (simplified - in practice would be cached)
    double* kernel_matrix = malloc(n_samples * n_samples * sizeof(double));
    if (!kernel_matrix) {
        free(alphas);
        free(errors);
        return false;
    }
    
    for (size_t i = 0; i < n_samples; i++) {
        for (size_t j = 0; j < n_samples; j++) {
            kernel_matrix[i * n_samples + j] = 
                svm_kernel_compute(model, &X[i * n_features], &X[j * n_features]);
        }
    }
    
    // Simplified SMO iterations
    bool changed = false;
    size_t passes = 0;
    
    while (passes < 10) {  // Simplified: limited passes
        changed = false;
        
        for (size_t i = 0; i < n_samples; i++) {
            // Compute error for sample i
            double prediction = bias;
            for (size_t j = 0; j < n_samples; j++) {
                prediction += alphas[j] * y[j] * kernel_matrix[i * n_samples + j];
            }
            errors[i] = prediction - y[i];
            
            // Check KKT conditions (simplified)
            if ((y[i] * errors[i] < -model->tolerance && alphas[i] < model->C) ||
                (y[i] * errors[i] > model->tolerance && alphas[i] > 0)) {
                
                // Select second alpha (simplified: just pick next one)
                size_t j = (i + 1) % n_samples;
                
                double prediction_j = bias;
                for (size_t k = 0; k < n_samples; k++) {
                    prediction_j += alphas[k] * y[k] * kernel_matrix[j * n_samples + k];
                }
                errors[j] = prediction_j - y[j];
                
                // Save old alphas
                double alpha_i_old = alphas[i];
                double alpha_j_old = alphas[j];
                
                // Compute bounds
                double L, H;
                if (y[i] != y[j]) {
                    L = fmax(0, alphas[j] - alphas[i]);
                    H = fmin(model->C, model->C + alphas[j] - alphas[i]);
                } else {
                    L = fmax(0, alphas[i] + alphas[j] - model->C);
                    H = fmin(model->C, alphas[i] + alphas[j]);
                }
                
                if (L == H) continue;
                
                // Compute eta
                double eta = 2 * kernel_matrix[i * n_samples + j] - 
                           kernel_matrix[i * n_samples + i] - 
                           kernel_matrix[j * n_samples + j];
                
                if (eta >= 0) continue;
                
                // Update alpha j
                alphas[j] = alphas[j] - (y[j] * (errors[i] - errors[j])) / eta;
                alphas[j] = fmin(H, fmax(L, alphas[j]));
                
                if (fabs(alphas[j] - alpha_j_old) < 1e-5) continue;
                
                // Update alpha i
                alphas[i] = alphas[i] + y[i] * y[j] * (alpha_j_old - alphas[j]);
                
                // Update bias
                double b1 = bias - errors[i] - 
                          y[i] * (alphas[i] - alpha_i_old) * kernel_matrix[i * n_samples + i] -
                          y[j] * (alphas[j] - alpha_j_old) * kernel_matrix[i * n_samples + j];
                
                double b2 = bias - errors[j] -
                          y[i] * (alphas[i] - alpha_i_old) * kernel_matrix[i * n_samples + j] -
                          y[j] * (alphas[j] - alpha_j_old) * kernel_matrix[j * n_samples + j];
                
                if (alphas[i] > 0 && alphas[i] < model->C) {
                    bias = b1;
                } else if (alphas[j] > 0 && alphas[j] < model->C) {
                    bias = b2;
                } else {
                    bias = (b1 + b2) / 2.0;
                }
                
                changed = true;
            }
        }
        
        if (!changed) {
            passes++;
        } else {
            passes = 0;
        }
    }
    
    // Extract support vectors
    size_t n_sv = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (alphas[i] > 1e-5) {
            n_sv++;
        }
    }
    
    if (n_sv == 0) {
        free(alphas);
        free(errors);
        free(kernel_matrix);
        return false;
    }
    
    model->support_vectors = malloc(n_sv * n_features * sizeof(double));
    model->alphas = malloc(n_sv * sizeof(double));
    model->support_labels = malloc(n_sv * sizeof(double));
    
    if (!model->support_vectors || !model->alphas || !model->support_labels) {
        free(model->support_vectors);
        free(model->alphas);
        free(model->support_labels);
        free(alphas);
        free(errors);
        free(kernel_matrix);
        return false;
    }
    
    size_t sv_idx = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (alphas[i] > 1e-5) {
            memcpy(&model->support_vectors[sv_idx * n_features],
                   &X[i * n_features],
                   n_features * sizeof(double));
            model->alphas[sv_idx] = alphas[i];
            model->support_labels[sv_idx] = y[i];
            sv_idx++;
        }
    }
    
    model->n_support_vectors = n_sv;
    model->bias = bias;
    model->fitted = true;
    
    free(alphas);
    free(errors);
    free(kernel_matrix);
    
    return true;
}
