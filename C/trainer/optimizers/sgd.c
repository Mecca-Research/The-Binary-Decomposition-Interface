
#include "sgd.h"
#include <stdlib.h>
#include <string.h>

SGDOptimizer* sgd_create(size_t num_params, SGDConfig config) {
    SGDOptimizer* opt = malloc(sizeof(SGDOptimizer));
    if (!opt) return NULL;
    
    opt->velocity = calloc(num_params, sizeof(double));
    if (!opt->velocity) {
        free(opt);
        return NULL;
    }
    
    opt->config = config;
    opt->num_params = num_params;
    opt->step_count = 0;
    
    return opt;
}

void sgd_destroy(SGDOptimizer* opt) {
    if (opt) {
        free(opt->velocity);
        free(opt);
    }
}

void sgd_reset(SGDOptimizer* opt) {
    if (opt) {
        memset(opt->velocity, 0, opt->num_params * sizeof(double));
        opt->step_count = 0;
    }
}

void sgd_step(SGDOptimizer* opt, double* params, double* gradients) {
    if (!opt || !params || !gradients) return;
    
    opt->step_count++;
    
    for (size_t i = 0; i < opt->num_params; i++) {
        double grad = gradients[i];
        
        // Apply weight decay
        if (opt->config.weight_decay > 0.0) {
            grad += opt->config.weight_decay * params[i];
        }
        
        // Update velocity with momentum
        if (opt->config.momentum > 0.0) {
            opt->velocity[i] = opt->config.momentum * opt->velocity[i] + grad;
            
            // Nesterov momentum
            if (opt->config.nesterov) {
                grad = grad + opt->config.momentum * opt->velocity[i];
            } else {
                grad = opt->velocity[i];
            }
        }
        
        // Update parameters
        params[i] -= opt->config.learning_rate * grad;
    }
}

void sgd_zero_grad(double* gradients, size_t num_params) {
    if (gradients) {
        memset(gradients, 0, num_params * sizeof(double));
    }
}

SGDConfig sgd_default_config(void) {
    return (SGDConfig){
        .learning_rate = 0.01,
        .momentum = 0.0,
        .weight_decay = 0.0,
        .nesterov = false
    };
}

SGDConfig sgd_config_with_momentum(double lr, double momentum) {
    return (SGDConfig){
        .learning_rate = lr,
        .momentum = momentum,
        .weight_decay = 0.0,
        .nesterov = false
    };
}

SGDConfig sgd_config_with_weight_decay(double lr, double weight_decay) {
    return (SGDConfig){
        .learning_rate = lr,
        .momentum = 0.0,
        .weight_decay = weight_decay,
        .nesterov = false
    };
}

SGDConfig sgd_config_nesterov(double lr, double momentum) {
    return (SGDConfig){
        .learning_rate = lr,
        .momentum = momentum,
        .weight_decay = 0.0,
        .nesterov = true
    };
}
