
#ifndef BDI_TRAINING_H
#define BDI_TRAINING_H

#include <stddef.h>
#include <stdbool.h>
#include "../optimizers/sgd.h"
#include "../optimizers/adam.h"
#include "../optimizers/rmsprop.h"
#include "../optimizers/lr_scheduler.h"
#include "../loss/loss.h"

// Training configuration
typedef struct {
    size_t num_epochs;
    size_t batch_size;
    double learning_rate;
    bool shuffle;
    bool verbose;
    size_t print_every;
    
    // Validation
    bool use_validation;
    double validation_split;
    
    // Early stopping
    bool early_stopping;
    size_t patience;
    double min_delta;
    
    // Optimizer type
    enum {
        OPT_SGD,
        OPT_ADAM,
        OPT_RMSPROP
    } optimizer_type;
    
    // Loss type
    LossType loss_type;
    
    // Learning rate scheduler
    bool use_scheduler;
    LRSchedulerConfig scheduler_config;
} TrainingConfig;

// Training history
typedef struct {
    double* train_losses;
    double* val_losses;
    double* train_metrics;
    double* val_metrics;
    double* learning_rates;
    size_t num_epochs;
    size_t current_epoch;
} TrainingHistory;

// Model interface (user must implement)
typedef struct {
    void* model_data;
    
    // Forward pass: compute predictions
    void (*forward)(void* model_data, const double* input, double* output, 
                   size_t batch_size, size_t input_size, size_t output_size);
    
    // Backward pass: compute gradients
    void (*backward)(void* model_data, const double* grad_output, double* grad_input,
                    size_t batch_size, size_t input_size, size_t output_size);
    
    // Get parameters
    double* (*get_params)(void* model_data, size_t* num_params);
    
    // Get gradients
    double* (*get_grads)(void* model_data, size_t* num_grads);
    
    // Zero gradients
    void (*zero_grad)(void* model_data);
} Model;

// Training history management
TrainingHistory* training_history_create(size_t num_epochs);
void training_history_destroy(TrainingHistory* history);
void training_history_update(TrainingHistory* history, size_t epoch,
                            double train_loss, double val_loss,
                            double train_metric, double val_metric,
                            double learning_rate);
void training_history_print(const TrainingHistory* history);
void training_history_save(const TrainingHistory* history, const char* filename);

// Training configuration helpers
TrainingConfig training_config_default(void);
TrainingConfig training_config_classification(size_t num_epochs, size_t batch_size);
TrainingConfig training_config_regression(size_t num_epochs, size_t batch_size);

// Main training function
TrainingHistory* train_model(Model* model,
                             const double* train_data, const double* train_labels,
                             size_t num_train_samples, size_t input_size, size_t output_size,
                             TrainingConfig config);

// Training with validation data
TrainingHistory* train_model_with_validation(Model* model,
                                             const double* train_data, const double* train_labels,
                                             const double* val_data, const double* val_labels,
                                             size_t num_train_samples, size_t num_val_samples,
                                             size_t input_size, size_t output_size,
                                             TrainingConfig config);

// Evaluation
double evaluate_model(Model* model,
                     const double* data, const double* labels,
                     size_t num_samples, size_t input_size, size_t output_size,
                     LossType loss_type);

#endif // BDI_TRAINING_H
