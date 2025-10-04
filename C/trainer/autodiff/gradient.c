
#include "gradient.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

GradientArray* gradient_array_create(size_t size) {
    GradientArray* arr = malloc(sizeof(GradientArray));
    if (!arr) return NULL;
    
    arr->values = calloc(size, sizeof(double));
    arr->gradients = calloc(size, sizeof(double));
    
    if (!arr->values || !arr->gradients) {
        free(arr->values);
        free(arr->gradients);
        free(arr);
        return NULL;
    }
    
    arr->size = size;
    return arr;
}

void gradient_array_destroy(GradientArray* arr) {
    if (arr) {
        free(arr->values);
        free(arr->gradients);
        free(arr);
    }
}

void gradient_array_zero_grad(GradientArray* arr) {
    if (arr && arr->gradients) {
        memset(arr->gradients, 0, arr->size * sizeof(double));
    }
}

double numerical_gradient(double (*func)(double*, void*), double* params, 
                         size_t param_idx, size_t num_params, void* user_data, 
                         double epsilon) {
    double* params_plus = malloc(num_params * sizeof(double));
    double* params_minus = malloc(num_params * sizeof(double));
    
    memcpy(params_plus, params, num_params * sizeof(double));
    memcpy(params_minus, params, num_params * sizeof(double));
    
    params_plus[param_idx] += epsilon;
    params_minus[param_idx] -= epsilon;
    
    double f_plus = func(params_plus, user_data);
    double f_minus = func(params_minus, user_data);
    
    free(params_plus);
    free(params_minus);
    
    return (f_plus - f_minus) / (2.0 * epsilon);
}

bool check_gradients(double* analytical, double* numerical, size_t size, 
                    double rtol, double atol) {
    for (size_t i = 0; i < size; i++) {
        double diff = fabs(analytical[i] - numerical[i]);
        double threshold = atol + rtol * fabs(numerical[i]);
        
        if (diff > threshold) {
            return false;
        }
    }
    return true;
}

void gradient_vector_add(GradientArray* result, GradientArray* a, GradientArray* b) {
    if (!result || !a || !b || result->size != a->size || result->size != b->size) {
        return;
    }
    
    for (size_t i = 0; i < result->size; i++) {
        result->values[i] = a->values[i] + b->values[i];
        result->gradients[i] = a->gradients[i] + b->gradients[i];
    }
}

void gradient_vector_sub(GradientArray* result, GradientArray* a, GradientArray* b) {
    if (!result || !a || !b || result->size != a->size || result->size != b->size) {
        return;
    }
    
    for (size_t i = 0; i < result->size; i++) {
        result->values[i] = a->values[i] - b->values[i];
        result->gradients[i] = a->gradients[i] - b->gradients[i];
    }
}

void gradient_vector_mul(GradientArray* result, GradientArray* a, GradientArray* b) {
    if (!result || !a || !b || result->size != a->size || result->size != b->size) {
        return;
    }
    
    for (size_t i = 0; i < result->size; i++) {
        result->values[i] = a->values[i] * b->values[i];
        result->gradients[i] = a->gradients[i] * b->values[i] + a->values[i] * b->gradients[i];
    }
}

void gradient_vector_scale(GradientArray* result, GradientArray* a, double scalar) {
    if (!result || !a || result->size != a->size) {
        return;
    }
    
    for (size_t i = 0; i < result->size; i++) {
        result->values[i] = a->values[i] * scalar;
        result->gradients[i] = a->gradients[i] * scalar;
    }
}

double gradient_dot_product(GradientArray* a, GradientArray* b, 
                           double* grad_a, double* grad_b) {
    if (!a || !b || a->size != b->size) {
        return 0.0;
    }
    
    double result = 0.0;
    
    for (size_t i = 0; i < a->size; i++) {
        result += a->values[i] * b->values[i];
        
        if (grad_a) {
            grad_a[i] += b->values[i];
        }
        if (grad_b) {
            grad_b[i] += a->values[i];
        }
    }
    
    return result;
}

void gradient_matvec(double* result, double* grad_result,
                    double* matrix, double* grad_matrix,
                    double* vector, double* grad_vector,
                    size_t rows, size_t cols) {
    if (!result || !matrix || !vector) {
        return;
    }
    
    // Forward pass: result = matrix * vector
    for (size_t i = 0; i < rows; i++) {
        result[i] = 0.0;
        for (size_t j = 0; j < cols; j++) {
            result[i] += matrix[i * cols + j] * vector[j];
        }
    }
    
    // Backward pass if gradients are provided
    if (grad_result && grad_matrix && grad_vector) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                grad_matrix[i * cols + j] += grad_result[i] * vector[j];
                grad_vector[j] += grad_result[i] * matrix[i * cols + j];
            }
        }
    }
}
