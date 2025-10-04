
#ifndef BDI_SGD_H
#define BDI_SGD_H

#include <stddef.h>
#include <stdbool.h>

// SGD optimizer configuration
typedef struct {
    double learning_rate;
    double momentum;
    double weight_decay;
    bool nesterov;
} SGDConfig;

// SGD optimizer state
typedef struct {
    SGDConfig config;
    double* velocity;
    size_t num_params;
    size_t step_count;
} SGDOptimizer;

// Optimizer management
SGDOptimizer* sgd_create(size_t num_params, SGDConfig config);
void sgd_destroy(SGDOptimizer* opt);
void sgd_reset(SGDOptimizer* opt);

// Optimization step
void sgd_step(SGDOptimizer* opt, double* params, double* gradients);
void sgd_zero_grad(double* gradients, size_t num_params);

// Configuration helpers
SGDConfig sgd_default_config(void);
SGDConfig sgd_config_with_momentum(double lr, double momentum);
SGDConfig sgd_config_with_weight_decay(double lr, double weight_decay);
SGDConfig sgd_config_nesterov(double lr, double momentum);

#endif // BDI_SGD_H
