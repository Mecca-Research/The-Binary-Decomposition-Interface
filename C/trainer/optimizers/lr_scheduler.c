
#include "lr_scheduler.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LRScheduler* lr_scheduler_create(LRSchedulerConfig config) {
    LRScheduler* scheduler = malloc(sizeof(LRScheduler));
    if (!scheduler) return NULL;
    
    scheduler->config = config;
    scheduler->current_step = 0;
    scheduler->current_lr = config.initial_lr;
    
    return scheduler;
}

void lr_scheduler_destroy(LRScheduler* scheduler) {
    free(scheduler);
}

void lr_scheduler_reset(LRScheduler* scheduler) {
    if (scheduler) {
        scheduler->current_step = 0;
        scheduler->current_lr = scheduler->config.initial_lr;
    }
}

double lr_scheduler_get_lr(LRScheduler* scheduler) {
    if (!scheduler) return 0.0;
    return scheduler->current_lr;
}

void lr_scheduler_step(LRScheduler* scheduler) {
    if (!scheduler) return;
    
    scheduler->current_step++;
    
    switch (scheduler->config.type) {
        case LR_CONSTANT:
            scheduler->current_lr = scheduler->config.initial_lr;
            break;
            
        case LR_STEP: {
            size_t num_drops = scheduler->current_step / scheduler->config.step_size;
            scheduler->current_lr = scheduler->config.initial_lr * 
                                   pow(scheduler->config.gamma, num_drops);
            break;
        }
            
        case LR_EXPONENTIAL:
            scheduler->current_lr = scheduler->config.initial_lr * 
                                   pow(scheduler->config.decay_rate, scheduler->current_step);
            break;
            
        case LR_COSINE: {
            double progress = (double)scheduler->current_step / scheduler->config.T_max;
            progress = fmin(progress, 1.0);
            scheduler->current_lr = scheduler->config.min_lr + 
                                   (scheduler->config.initial_lr - scheduler->config.min_lr) * 
                                   0.5 * (1.0 + cos(M_PI * progress));
            break;
        }
            
        case LR_COSINE_WARM_RESTARTS: {
            size_t T_cur = scheduler->current_step;
            size_t T_i = scheduler->config.T_0;
            size_t restart_count = 0;
            
            while (T_cur >= T_i) {
                T_cur -= T_i;
                T_i *= scheduler->config.T_mult;
                restart_count++;
            }
            
            double progress = (double)T_cur / T_i;
            scheduler->current_lr = scheduler->config.min_lr + 
                                   (scheduler->config.initial_lr - scheduler->config.min_lr) * 
                                   0.5 * (1.0 + cos(M_PI * progress));
            break;
        }
            
        case LR_LINEAR_WARMUP:
            if (scheduler->current_step < scheduler->config.warmup_steps) {
                double progress = (double)scheduler->current_step / scheduler->config.warmup_steps;
                scheduler->current_lr = scheduler->config.warmup_start_lr + 
                                       (scheduler->config.initial_lr - scheduler->config.warmup_start_lr) * 
                                       progress;
            } else {
                scheduler->current_lr = scheduler->config.initial_lr;
            }
            break;
    }
}

LRSchedulerConfig lr_constant_config(double lr) {
    return (LRSchedulerConfig){
        .type = LR_CONSTANT,
        .initial_lr = lr,
        .min_lr = lr
    };
}

LRSchedulerConfig lr_step_config(double initial_lr, size_t step_size, double gamma) {
    return (LRSchedulerConfig){
        .type = LR_STEP,
        .initial_lr = initial_lr,
        .step_size = step_size,
        .gamma = gamma
    };
}

LRSchedulerConfig lr_exponential_config(double initial_lr, double decay_rate) {
    return (LRSchedulerConfig){
        .type = LR_EXPONENTIAL,
        .initial_lr = initial_lr,
        .decay_rate = decay_rate
    };
}

LRSchedulerConfig lr_cosine_config(double initial_lr, size_t T_max, double min_lr) {
    return (LRSchedulerConfig){
        .type = LR_COSINE,
        .initial_lr = initial_lr,
        .T_max = T_max,
        .min_lr = min_lr
    };
}

LRSchedulerConfig lr_cosine_warm_restarts_config(double initial_lr, size_t T_0, size_t T_mult) {
    return (LRSchedulerConfig){
        .type = LR_COSINE_WARM_RESTARTS,
        .initial_lr = initial_lr,
        .T_0 = T_0,
        .T_mult = T_mult,
        .min_lr = 0.0
    };
}

LRSchedulerConfig lr_linear_warmup_config(double target_lr, size_t warmup_steps) {
    return (LRSchedulerConfig){
        .type = LR_LINEAR_WARMUP,
        .initial_lr = target_lr,
        .warmup_steps = warmup_steps,
        .warmup_start_lr = 0.0
    };
}
