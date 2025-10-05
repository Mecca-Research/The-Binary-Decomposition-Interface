
/**
 * @file loss.h
 * @brief Loss API
 * @details This file provides the loss functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_LOSS_H
#define BDI_LOSS_H

#include <stddef.h>

// Loss function types
typedef enum {
    LOSS_MSE,
    LOSS_CROSS_ENTROPY,
    LOSS_BINARY_CROSS_ENTROPY,
    LOSS_MAE,
    LOSS_HUBER
} LossType;

// Loss computation result
typedef struct {
    double loss;
    double* gradients;
    size_t size;
} LossResult;

// Loss result management
LossResult* loss_result_create(size_t size);
void loss_result_destroy(LossResult* result);

// Mean Squared Error (MSE)
double loss_mse_forward(const double* predictions, const double* targets, size_t size);
void loss_mse_backward(const double* predictions, const double* targets, 
                       double* gradients, size_t size);
LossResult* loss_mse(const double* predictions, const double* targets, size_t size);

// Cross Entropy Loss
double loss_cross_entropy_forward(const double* predictions, const double* targets, 
                                  size_t batch_size, size_t num_classes);
void loss_cross_entropy_backward(const double* predictions, const double* targets, 
                                 double* gradients, size_t batch_size, size_t num_classes);
LossResult* loss_cross_entropy(const double* predictions, const double* targets, 
                               size_t batch_size, size_t num_classes);

// Binary Cross Entropy Loss
double loss_binary_cross_entropy_forward(const double* predictions, const double* targets, 
                                        size_t size);
void loss_binary_cross_entropy_backward(const double* predictions, const double* targets, 
                                       double* gradients, size_t size);
LossResult* loss_binary_cross_entropy(const double* predictions, const double* targets, 
                                      size_t size);

// Mean Absolute Error (MAE)
double loss_mae_forward(const double* predictions, const double* targets, size_t size);
void loss_mae_backward(const double* predictions, const double* targets, 
                      double* gradients, size_t size);
LossResult* loss_mae(const double* predictions, const double* targets, size_t size);

// Huber Loss (smooth L1)
double loss_huber_forward(const double* predictions, const double* targets, 
                         size_t size, double delta);
void loss_huber_backward(const double* predictions, const double* targets, 
                        double* gradients, size_t size, double delta);
LossResult* loss_huber(const double* predictions, const double* targets, 
                      size_t size, double delta);

// Utility functions
void softmax(const double* input, double* output, size_t size);
void log_softmax(const double* input, double* output, size_t size);

#endif // BDI_LOSS_H
