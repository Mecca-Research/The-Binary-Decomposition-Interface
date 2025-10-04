
#ifndef BDI_RMSPROP_H
#define BDI_RMSPROP_H

#include <stddef.h>
#include <stdbool.h>

// RMSprop optimizer configuration
typedef struct {
    double learning_rate;
    double alpha;          // Smoothing constant
    double epsilon;        // Small constant for numerical stability
    double weight_decay;
    double momentum;
    bool centered;         // Use centered RMSprop variant
} RMSpropConfig;

// RMSprop optimizer state
typedef struct {
    RMSpropConfig config;
    double* square_avg;    // Moving average of squared gradients
    double* grad_avg;      // Moving average of gradients (for centered variant)
    double* momentum_buffer;
    size_t num_params;
    size_t step_count;
} RMSpropOptimizer;

// Optimizer management
RMSpropOptimizer* rmsprop_create(size_t num_params, RMSpropConfig config);
void rmsprop_destroy(RMSpropOptimizer* opt);
void rmsprop_reset(RMSpropOptimizer* opt);

// Optimization step
void rmsprop_step(RMSpropOptimizer* opt, double* params, double* gradients);

// Configuration helpers
RMSpropConfig rmsprop_default_config(void);
RMSpropConfig rmsprop_config_centered(double lr);
RMSpropConfig rmsprop_config_with_momentum(double lr, double momentum);

#endif // BDI_RMSPROP_H
