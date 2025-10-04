
#ifndef BDI_LR_SCHEDULER_H
#define BDI_LR_SCHEDULER_H

#include <stddef.h>
#include <math.h>

// Learning rate scheduler types
typedef enum {
    LR_CONSTANT,
    LR_STEP,
    LR_EXPONENTIAL,
    LR_COSINE,
    LR_COSINE_WARM_RESTARTS,
    LR_LINEAR_WARMUP
} LRSchedulerType;

// Learning rate scheduler configuration
typedef struct {
    LRSchedulerType type;
    double initial_lr;
    double min_lr;
    
    // Step scheduler
    size_t step_size;
    double gamma;
    
    // Exponential scheduler
    double decay_rate;
    
    // Cosine scheduler
    size_t T_max;
    
    // Cosine warm restarts
    size_t T_0;
    size_t T_mult;
    
    // Linear warmup
    size_t warmup_steps;
    double warmup_start_lr;
} LRSchedulerConfig;

// Learning rate scheduler state
typedef struct {
    LRSchedulerConfig config;
    size_t current_step;
    double current_lr;
} LRScheduler;

// Scheduler management
LRScheduler* lr_scheduler_create(LRSchedulerConfig config);
void lr_scheduler_destroy(LRScheduler* scheduler);
void lr_scheduler_reset(LRScheduler* scheduler);

// Get current learning rate
double lr_scheduler_get_lr(LRScheduler* scheduler);

// Step the scheduler
void lr_scheduler_step(LRScheduler* scheduler);

// Configuration helpers
LRSchedulerConfig lr_constant_config(double lr);
LRSchedulerConfig lr_step_config(double initial_lr, size_t step_size, double gamma);
LRSchedulerConfig lr_exponential_config(double initial_lr, double decay_rate);
LRSchedulerConfig lr_cosine_config(double initial_lr, size_t T_max, double min_lr);
LRSchedulerConfig lr_cosine_warm_restarts_config(double initial_lr, size_t T_0, size_t T_mult);
LRSchedulerConfig lr_linear_warmup_config(double target_lr, size_t warmup_steps);

#endif // BDI_LR_SCHEDULER_H
