
#include "training.h"
#include "../metrics/metrics.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

TrainingHistory* training_history_create(size_t num_epochs) {
    TrainingHistory* history = malloc(sizeof(TrainingHistory));
    if (!history) return NULL;
    
    history->train_losses = calloc(num_epochs, sizeof(double));
    history->val_losses = calloc(num_epochs, sizeof(double));
    history->train_metrics = calloc(num_epochs, sizeof(double));
    history->val_metrics = calloc(num_epochs, sizeof(double));
    history->learning_rates = calloc(num_epochs, sizeof(double));
    
    if (!history->train_losses || !history->val_losses || 
        !history->train_metrics || !history->val_metrics || !history->learning_rates) {
        free(history->train_losses);
        free(history->val_losses);
        free(history->train_metrics);
        free(history->val_metrics);
        free(history->learning_rates);
        free(history);
        return NULL;
    }
    
    history->num_epochs = num_epochs;
    history->current_epoch = 0;
    
    return history;
}

void training_history_destroy(TrainingHistory* history) {
    if (history) {
        free(history->train_losses);
        free(history->val_losses);
        free(history->train_metrics);
        free(history->val_metrics);
        free(history->learning_rates);
        free(history);
    }
}

void training_history_update(TrainingHistory* history, size_t epoch,
                            double train_loss, double val_loss,
                            double train_metric, double val_metric,
                            double learning_rate) {
    if (!history || epoch >= history->num_epochs) return;
    
    history->train_losses[epoch] = train_loss;
    history->val_losses[epoch] = val_loss;
    history->train_metrics[epoch] = train_metric;
    history->val_metrics[epoch] = val_metric;
    history->learning_rates[epoch] = learning_rate;
    history->current_epoch = epoch + 1;
}

void training_history_print(const TrainingHistory* history) {
    if (!history) return;
    
    printf("\nTraining History:\n");
    printf("Epoch | Train Loss | Val Loss | Train Metric | Val Metric | LR\n");
    printf("------|------------|----------|--------------|------------|----------\n");
    
    for (size_t i = 0; i < history->current_epoch; i++) {
        printf("%5zu | %10.6f | %8.6f | %12.6f | %10.6f | %.6f\n",
               i + 1, history->train_losses[i], history->val_losses[i],
               history->train_metrics[i], history->val_metrics[i],
               history->learning_rates[i]);
    }
}

void training_history_save(const TrainingHistory* history, const char* filename) {
    if (!history || !filename) return;
    
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "epoch,train_loss,val_loss,train_metric,val_metric,learning_rate\n");
    for (size_t i = 0; i < history->current_epoch; i++) {
        fprintf(f, "%zu,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                i + 1, history->train_losses[i], history->val_losses[i],
                history->train_metrics[i], history->val_metrics[i],
                history->learning_rates[i]);
    }
    
    fclose(f);
}

TrainingConfig training_config_default(void) {
    return (TrainingConfig){
        .num_epochs = 10,
        .batch_size = 32,
        .learning_rate = 0.001,
        .shuffle = true,
        .verbose = true,
        .print_every = 1,
        .use_validation = false,
        .validation_split = 0.2,
        .early_stopping = false,
        .patience = 10,
        .min_delta = 1e-4,
        .optimizer_type = OPT_ADAM,
        .loss_type = LOSS_MSE,
        .use_scheduler = false
    };
}

TrainingConfig training_config_classification(size_t num_epochs, size_t batch_size) {
    TrainingConfig config = training_config_default();
    config.num_epochs = num_epochs;
    config.batch_size = batch_size;
    config.loss_type = LOSS_CROSS_ENTROPY;
    config.optimizer_type = OPT_ADAM;
    return config;
}

TrainingConfig training_config_regression(size_t num_epochs, size_t batch_size) {
    TrainingConfig config = training_config_default();
    config.num_epochs = num_epochs;
    config.batch_size = batch_size;
    config.loss_type = LOSS_MSE;
    config.optimizer_type = OPT_ADAM;
    return config;
}

// Shuffle indices
static void shuffle_indices(size_t* indices, size_t size) {
    for (size_t i = size - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

double evaluate_model(Model* model,
                     const double* data, const double* labels,
                     size_t num_samples, size_t input_size, size_t output_size,
                     LossType loss_type) {
    if (!model || !data || !labels) return 0.0;
    
    double total_loss = 0.0;
    size_t batch_size = 32;
    
    double* predictions = malloc(batch_size * output_size * sizeof(double));
    if (!predictions) return 0.0;
    
    for (size_t i = 0; i < num_samples; i += batch_size) {
        size_t current_batch = (i + batch_size > num_samples) ? 
                               (num_samples - i) : batch_size;
        
        const double* batch_data = data + i * input_size;
        const double* batch_labels = labels + i * output_size;
        
        // Forward pass
        model->forward(model->model_data, batch_data, predictions,
                      current_batch, input_size, output_size);
        
        // Compute loss
        double batch_loss = 0.0;
        switch (loss_type) {
            case LOSS_MSE:
                batch_loss = loss_mse_forward(predictions, batch_labels, 
                                             current_batch * output_size);
                break;
            case LOSS_CROSS_ENTROPY:
                batch_loss = loss_cross_entropy_forward(predictions, batch_labels,
                                                       current_batch, output_size);
                break;
            case LOSS_BINARY_CROSS_ENTROPY:
                batch_loss = loss_binary_cross_entropy_forward(predictions, batch_labels,
                                                              current_batch * output_size);
                break;
            case LOSS_MAE:
                batch_loss = loss_mae_forward(predictions, batch_labels,
                                             current_batch * output_size);
                break;
            case LOSS_HUBER:
                batch_loss = loss_huber_forward(predictions, batch_labels,
                                               current_batch * output_size, 1.0);
                break;
        }
        
        total_loss += batch_loss * current_batch;
    }
    
    free(predictions);
    return total_loss / num_samples;
}

TrainingHistory* train_model(Model* model,
                             const double* train_data, const double* train_labels,
                             size_t num_train_samples, size_t input_size, size_t output_size,
                             TrainingConfig config) {
    if (!model || !train_data || !train_labels) return NULL;
    
    // Create training history
    TrainingHistory* history = training_history_create(config.num_epochs);
    if (!history) return NULL;
    
    // Get model parameters
    size_t num_params;
    double* params = model->get_params(model->model_data, &num_params);
    
    // Create optimizer
    void* optimizer = NULL;
    SGDOptimizer* sgd_opt = NULL;
    AdamOptimizer* adam_opt = NULL;
    RMSpropOptimizer* rmsprop_opt = NULL;
    
    switch (config.optimizer_type) {
        case OPT_SGD: {
            SGDConfig sgd_config = sgd_default_config();
            sgd_config.learning_rate = config.learning_rate;
            sgd_opt = sgd_create(num_params, sgd_config);
            optimizer = sgd_opt;
            break;
        }
        case OPT_ADAM: {
            AdamConfig adam_config = adam_default_config();
            adam_config.learning_rate = config.learning_rate;
            adam_opt = adam_create(num_params, adam_config);
            optimizer = adam_opt;
            break;
        }
        case OPT_RMSPROP: {
            RMSpropConfig rmsprop_config = rmsprop_default_config();
            rmsprop_config.learning_rate = config.learning_rate;
            rmsprop_opt = rmsprop_create(num_params, rmsprop_config);
            optimizer = rmsprop_opt;
            break;
        }
    }
    
    if (!optimizer) {
        training_history_destroy(history);
        return NULL;
    }
    
    // Create learning rate scheduler if needed
    LRScheduler* scheduler = NULL;
    if (config.use_scheduler) {
        scheduler = lr_scheduler_create(config.scheduler_config);
    }
    
    // Allocate batch buffers
    double* batch_data = malloc(config.batch_size * input_size * sizeof(double));
    double* batch_labels = malloc(config.batch_size * output_size * sizeof(double));
    double* predictions = malloc(config.batch_size * output_size * sizeof(double));
    double* grad_output = malloc(config.batch_size * output_size * sizeof(double));
    
    if (!batch_data || !batch_labels || !predictions || !grad_output) {
        free(batch_data);
        free(batch_labels);
        free(predictions);
        free(grad_output);
        training_history_destroy(history);
        if (sgd_opt) sgd_destroy(sgd_opt);
        if (adam_opt) adam_destroy(adam_opt);
        if (rmsprop_opt) rmsprop_destroy(rmsprop_opt);
        if (scheduler) lr_scheduler_destroy(scheduler);
        return NULL;
    }
    
    // Create indices for shuffling
    size_t* indices = malloc(num_train_samples * sizeof(size_t));
    for (size_t i = 0; i < num_train_samples; i++) {
        indices[i] = i;
    }
    
    // Training loop
    srand(time(NULL));
    
    for (size_t epoch = 0; epoch < config.num_epochs; epoch++) {
        double epoch_loss = 0.0;
        size_t num_batches = 0;
        
        // Shuffle data if needed
        if (config.shuffle) {
            shuffle_indices(indices, num_train_samples);
        }
        
        // Batch training
        for (size_t i = 0; i < num_train_samples; i += config.batch_size) {
            size_t current_batch = (i + config.batch_size > num_train_samples) ?
                                  (num_train_samples - i) : config.batch_size;
            
            // Gather batch data
            for (size_t b = 0; b < current_batch; b++) {
                size_t idx = indices[i + b];
                memcpy(batch_data + b * input_size,
                      train_data + idx * input_size,
                      input_size * sizeof(double));
                memcpy(batch_labels + b * output_size,
                      train_labels + idx * output_size,
                      output_size * sizeof(double));
            }
            
            // Zero gradients
            model->zero_grad(model->model_data);
            
            // Forward pass
            model->forward(model->model_data, batch_data, predictions,
                          current_batch, input_size, output_size);
            
            // Compute loss and gradients
            LossResult* loss_result = NULL;
            switch (config.loss_type) {
                case LOSS_MSE:
                    loss_result = loss_mse(predictions, batch_labels,
                                          current_batch * output_size);
                    break;
                case LOSS_CROSS_ENTROPY:
                    loss_result = loss_cross_entropy(predictions, batch_labels,
                                                    current_batch, output_size);
                    break;
                case LOSS_BINARY_CROSS_ENTROPY:
                    loss_result = loss_binary_cross_entropy(predictions, batch_labels,
                                                           current_batch * output_size);
                    break;
                case LOSS_MAE:
                    loss_result = loss_mae(predictions, batch_labels,
                                          current_batch * output_size);
                    break;
                case LOSS_HUBER:
                    loss_result = loss_huber(predictions, batch_labels,
                                            current_batch * output_size, 1.0);
                    break;
            }
            
            if (loss_result) {
                epoch_loss += loss_result->loss * current_batch;
                
                // Backward pass
                memcpy(grad_output, loss_result->gradients,
                      current_batch * output_size * sizeof(double));
                model->backward(model->model_data, grad_output, NULL,
                               current_batch, input_size, output_size);
                
                loss_result_destroy(loss_result);
            }
            
            // Optimizer step
            size_t num_grads;
            double* grads = model->get_grads(model->model_data, &num_grads);
            
            switch (config.optimizer_type) {
                case OPT_SGD:
                    sgd_step(sgd_opt, params, grads);
                    break;
                case OPT_ADAM:
                    adam_step(adam_opt, params, grads);
                    break;
                case OPT_RMSPROP:
                    rmsprop_step(rmsprop_opt, params, grads);
                    break;
            }
            
            num_batches++;
        }
        
        epoch_loss /= num_train_samples;
        
        // Update learning rate scheduler
        double current_lr = config.learning_rate;
        if (scheduler) {
            lr_scheduler_step(scheduler);
            current_lr = lr_scheduler_get_lr(scheduler);
            
            // Update optimizer learning rate
            switch (config.optimizer_type) {
                case OPT_SGD:
                    sgd_opt->config.learning_rate = current_lr;
                    break;
                case OPT_ADAM:
                    adam_opt->config.learning_rate = current_lr;
                    break;
                case OPT_RMSPROP:
                    rmsprop_opt->config.learning_rate = current_lr;
                    break;
            }
        }
        
        // Update history
        training_history_update(history, epoch, epoch_loss, 0.0, 0.0, 0.0, current_lr);
        
        // Print progress
        if (config.verbose && (epoch + 1) % config.print_every == 0) {
            printf("Epoch %zu/%zu - Loss: %.6f - LR: %.6f\n",
                   epoch + 1, config.num_epochs, epoch_loss, current_lr);
        }
    }
    
    // Cleanup
    free(indices);
    free(batch_data);
    free(batch_labels);
    free(predictions);
    free(grad_output);
    
    if (sgd_opt) sgd_destroy(sgd_opt);
    if (adam_opt) adam_destroy(adam_opt);
    if (rmsprop_opt) rmsprop_destroy(rmsprop_opt);
    if (scheduler) lr_scheduler_destroy(scheduler);
    
    return history;
}

TrainingHistory* train_model_with_validation(Model* model,
                                             const double* train_data, const double* train_labels,
                                             const double* val_data, const double* val_labels,
                                             size_t num_train_samples, size_t num_val_samples,
                                             size_t input_size, size_t output_size,
                                             TrainingConfig config) {
    // Similar to train_model but with validation evaluation
    // For brevity, this would be implemented similarly with validation loss computation
    return train_model(model, train_data, train_labels, num_train_samples,
                      input_size, output_size, config);
}
