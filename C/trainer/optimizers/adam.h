
/**
 * @file adam.h
 * @brief Adam API
 * @details This file provides the adam functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_ADAM_H
#define BDI_ADAM_H

#include <stddef.h>
#include <stdbool.h>

// Adam optimizer configuration
typedef struct {
    double learning_rate;
    double beta1;           // Exponential decay rate for first moment
    double beta2;           // Exponential decay rate for second moment
    double epsilon;         // Small constant for numerical stability
    double weight_decay;
    bool amsgrad;          // Use AMSGrad variant
} AdamConfig;

// Adam optimizer state
typedef struct {
    AdamConfig config;
    double* m;             // First moment estimate
    double* v;             // Second moment estimate
    double* v_max;         // Max of second moment (for AMSGrad)
    size_t num_params;
    size_t step_count;
} AdamOptimizer;

// Optimizer management
AdamOptimizer* adam_create(size_t num_params, AdamConfig config);
void adam_destroy(AdamOptimizer* opt);
void adam_reset(AdamOptimizer* opt);

// Optimization step
void adam_step(AdamOptimizer* opt, double* params, double* gradients);

// Configuration helpers
AdamConfig adam_default_config(void);
AdamConfig adam_config_custom(double lr, double beta1, double beta2);
AdamConfig adam_config_amsgrad(double lr);

#endif // BDI_ADAM_H
