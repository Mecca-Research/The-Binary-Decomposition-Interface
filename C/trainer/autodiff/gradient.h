
#ifndef BDI_GRADIENT_H
#define BDI_GRADIENT_H

#include <stddef.h>
#include <stdbool.h>

// Gradient computation for arrays
typedef struct {
    double* values;
    double* gradients;
    size_t size;
} GradientArray;

// Array management
GradientArray* gradient_array_create(size_t size);
void gradient_array_destroy(GradientArray* arr);
void gradient_array_zero_grad(GradientArray* arr);

// Numerical gradient checking
double numerical_gradient(double (*func)(double*, void*), double* params, 
                         size_t param_idx, size_t num_params, void* user_data, 
                         double epsilon);

// Gradient checking utilities
bool check_gradients(double* analytical, double* numerical, size_t size, 
                    double rtol, double atol);

// Vector operations with gradients
void gradient_vector_add(GradientArray* result, GradientArray* a, GradientArray* b);
void gradient_vector_sub(GradientArray* result, GradientArray* a, GradientArray* b);
void gradient_vector_mul(GradientArray* result, GradientArray* a, GradientArray* b);
void gradient_vector_scale(GradientArray* result, GradientArray* a, double scalar);

// Dot product with gradient
double gradient_dot_product(GradientArray* a, GradientArray* b, 
                           double* grad_a, double* grad_b);

// Matrix-vector multiplication with gradient
void gradient_matvec(double* result, double* grad_result,
                    double* matrix, double* grad_matrix,
                    double* vector, double* grad_vector,
                    size_t rows, size_t cols);

#endif // BDI_GRADIENT_H
