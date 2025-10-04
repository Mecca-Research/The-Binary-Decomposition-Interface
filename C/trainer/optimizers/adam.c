
#include "adam.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

AdamOptimizer* adam_create(size_t num_params, AdamConfig config) {
    AdamOptimizer* opt = malloc(sizeof(AdamOptimizer));
    if (!opt) return NULL;
    
    opt->m = calloc(num_params, sizeof(double));
    opt->v = calloc(num_params, sizeof(double));
    opt->v_max = config.amsgrad ? calloc(num_params, sizeof(double)) : NULL;
    
    if (!opt->m || !opt->v || (config.amsgrad && !opt->v_max)) {
        free(opt->m);
        free(opt->v);
        free(opt->v_max);
        free(opt);
        return NULL;
    }
    
    opt->config = config;
    opt->num_params = num_params;
    opt->step_count = 0;
    
    return opt;
}

void adam_destroy(AdamOptimizer* opt) {
    if (opt) {
        free(opt->m);
        free(opt->v);
        free(opt->v_max);
        free(opt);
    }
}

void adam_reset(AdamOptimizer* opt) {
    if (opt) {
        memset(opt->m, 0, opt->num_params * sizeof(double));
        memset(opt->v, 0, opt->num_params * sizeof(double));
        if (opt->v_max) {
            memset(opt->v_max, 0, opt->num_params * sizeof(double));
        }
        opt->step_count = 0;
    }
}

void adam_step(AdamOptimizer* opt, double* params, double* gradients) {
    if (!opt || !params || !gradients) return;
    
    opt->step_count++;
    
    // Bias correction terms
    double bias_correction1 = 1.0 - pow(opt->config.beta1, opt->step_count);
    double bias_correction2 = 1.0 - pow(opt->config.beta2, opt->step_count);
    
    for (size_t i = 0; i < opt->num_params; i++) {
        double grad = gradients[i];
        
        // Apply weight decay
        if (opt->config.weight_decay > 0.0) {
            grad += opt->config.weight_decay * params[i];
        }
        
        // Update biased first moment estimate
        opt->m[i] = opt->config.beta1 * opt->m[i] + (1.0 - opt->config.beta1) * grad;
        
        // Update biased second raw moment estimate
        opt->v[i] = opt->config.beta2 * opt->v[i] + (1.0 - opt->config.beta2) * grad * grad;
        
        // Compute bias-corrected first moment estimate
        double m_hat = opt->m[i] / bias_correction1;
        
        // Compute bias-corrected second raw moment estimate
        double v_hat = opt->v[i] / bias_correction2;
        
        // AMSGrad variant
        if (opt->config.amsgrad) {
            opt->v_max[i] = fmax(opt->v_max[i], v_hat);
            v_hat = opt->v_max[i];
        }
        
        // Update parameters
        params[i] -= opt->config.learning_rate * m_hat / (sqrt(v_hat) + opt->config.epsilon);
    }
}

AdamConfig adam_default_config(void) {
    return (AdamConfig){
        .learning_rate = 0.001,
        .beta1 = 0.9,
        .beta2 = 0.999,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .amsgrad = false
    };
}

AdamConfig adam_config_custom(double lr, double beta1, double beta2) {
    return (AdamConfig){
        .learning_rate = lr,
        .beta1 = beta1,
        .beta2 = beta2,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .amsgrad = false
    };
}

AdamConfig adam_config_amsgrad(double lr) {
    return (AdamConfig){
        .learning_rate = lr,
        .beta1 = 0.9,
        .beta2 = 0.999,
        .epsilon = 1e-8,
        .weight_decay = 0.0,
        .amsgrad = true
    };
}
