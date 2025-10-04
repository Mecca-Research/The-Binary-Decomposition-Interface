
#include "rmsprop.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

RMSpropOptimizer* rmsprop_create(size_t num_params, RMSpropConfig config) {
    RMSpropOptimizer* opt = malloc(sizeof(RMSpropOptimizer));
    if (!opt) return NULL;
    
    opt->square_avg = calloc(num_params, sizeof(double));
    opt->grad_avg = config.centered ? calloc(num_params, sizeof(double)) : NULL;
    opt->momentum_buffer = (config.momentum > 0.0) ? calloc(num_params, sizeof(double)) : NULL;
    
    if (!opt->square_avg || (config.centered && !opt->grad_avg) || 
        (config.momentum > 0.0 && !opt->momentum_buffer)) {
        free(opt->square_avg);
        free(opt->grad_avg);
        free(opt->momentum_buffer);
        free(opt);
        return NULL;
    }
    
    opt->config = config;
    opt->num_params = num_params;
    opt->step_count = 0;
    
    return opt;
}

void rmsprop_destroy(RMSpropOptimizer* opt) {
    if (opt) {
        free(opt->square_avg);
        free(opt->grad_avg);
        free(opt->momentum_buffer);
        free(opt);
    }
}

void rmsprop_reset(RMSpropOptimizer* opt) {
    if (opt) {
        memset(opt->square_avg, 0, opt->num_params * sizeof(double));
        if (opt->grad_avg) {
            memset(opt->grad_avg, 0, opt->num_params * sizeof(double));
        }
        if (opt->momentum_buffer) {
            memset(opt->momentum_buffer, 0, opt->num_params * sizeof(double));
        }
        opt->step_count = 0;
    }
}

void rmsprop_step(RMSpropOptimizer* opt, double* params, double* gradients) {
    if (!opt || !params || !gradients) return;
    
    opt->step_count++;
    
    for (size_t i = 0; i < opt->num_params; i++) {
        double grad = gradients[i];
        
        // Apply weight decay
        if (opt->config.weight_decay > 0.0) {
            grad += opt->config.weight_decay * params[i];
        }
        
        // Update moving average of squared gradients
        opt->square_avg[i] = opt->config.alpha * opt->square_avg[i] + 
                            (1.0 - opt->config.alpha) * grad * grad;
        
        double avg;
        if (opt->config.centered) {
            // Update moving average of gradients
            opt->grad_avg[i] = opt->config.alpha * opt->grad_avg[i] + 
                              (1.0 - opt->config.alpha) * grad;
            
            // Centered RMSprop
            avg = sqrt(opt->square_avg[i] - opt->grad_avg[i] * opt->grad_avg[i] + 
                      opt->config.epsilon);
        } else {
            avg = sqrt(opt->square_avg[i] + opt->config.epsilon);
        }
        
        // Compute update
        double update = grad / avg;
        
        // Apply momentum if configured
        if (opt->config.momentum > 0.0) {
            opt->momentum_buffer[i] = opt->config.momentum * opt->momentum_buffer[i] + update;
            update = opt->momentum_buffer[i];
        }
        
        // Update parameters
        params[i] -= opt->config.learning_rate * update;
    }
}

RMSpropConfig rmsprop_default_config(void) {
    return (RMSpropConfig){
        .learning_rate = 0.01,
        .alpha = 0.99,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .momentum = 0.0,
        .centered = false
    };
}

RMSpropConfig rmsprop_config_centered(double lr) {
    return (RMSpropConfig){
        .learning_rate = lr,
        .alpha = 0.99,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .momentum = 0.0,
        .centered = true
    };
}

RMSpropConfig rmsprop_config_with_momentum(double lr, double momentum) {
    return (RMSpropConfig){
        .learning_rate = lr,
        .alpha = 0.99,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .momentum = momentum,
        .centered = false
    };
}
