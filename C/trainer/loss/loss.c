
#include "loss.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

LossResult* loss_result_create(size_t size) {
    LossResult* result = malloc(sizeof(LossResult));
    if (!result) return NULL;
    
    result->gradients = calloc(size, sizeof(double));
    if (!result->gradients) {
        free(result);
        return NULL;
    }
    
    result->size = size;
    result->loss = 0.0;
    
    return result;
}

void loss_result_destroy(LossResult* result) {
    if (result) {
        free(result->gradients);
        free(result);
    }
}

// MSE Loss
double loss_mse_forward(const double* predictions, const double* targets, size_t size) {
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        double diff = predictions[i] - targets[i];
        sum += diff * diff;
    }
    return sum / size;
}

void loss_mse_backward(const double* predictions, const double* targets, 
                       double* gradients, size_t size) {
    for (size_t i = 0; i < size; i++) {
        gradients[i] = 2.0 * (predictions[i] - targets[i]) / size;
    }
}

LossResult* loss_mse(const double* predictions, const double* targets, size_t size) {
    LossResult* result = loss_result_create(size);
    if (!result) return NULL;
    
    result->loss = loss_mse_forward(predictions, targets, size);
    loss_mse_backward(predictions, targets, result->gradients, size);
    
    return result;
}

// Cross Entropy Loss
void softmax(const double* input, double* output, size_t size) {
    double max_val = input[0];
    for (size_t i = 1; i < size; i++) {
        if (input[i] > max_val) max_val = input[i];
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        output[i] = exp(input[i] - max_val);
        sum += output[i];
    }
    
    for (size_t i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

void log_softmax(const double* input, double* output, size_t size) {
    double max_val = input[0];
    for (size_t i = 1; i < size; i++) {
        if (input[i] > max_val) max_val = input[i];
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        sum += exp(input[i] - max_val);
    }
    
    double log_sum = log(sum);
    for (size_t i = 0; i < size; i++) {
        output[i] = input[i] - max_val - log_sum;
    }
}

double loss_cross_entropy_forward(const double* predictions, const double* targets, 
                                  size_t batch_size, size_t num_classes) {
    double total_loss = 0.0;
    double* log_probs = malloc(num_classes * sizeof(double));
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * num_classes;
        const double* target = targets + b * num_classes;
        
        log_softmax(pred, log_probs, num_classes);
        
        for (size_t c = 0; c < num_classes; c++) {
            total_loss -= target[c] * log_probs[c];
        }
    }
    
    free(log_probs);
    return total_loss / batch_size;
}

void loss_cross_entropy_backward(const double* predictions, const double* targets, 
                                 double* gradients, size_t batch_size, size_t num_classes) {
    double* probs = malloc(num_classes * sizeof(double));
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * num_classes;
        const double* target = targets + b * num_classes;
        double* grad = gradients + b * num_classes;
        
        softmax(pred, probs, num_classes);
        
        for (size_t c = 0; c < num_classes; c++) {
            grad[c] = (probs[c] - target[c]) / batch_size;
        }
    }
    
    free(probs);
}

LossResult* loss_cross_entropy(const double* predictions, const double* targets, 
                               size_t batch_size, size_t num_classes) {
    size_t total_size = batch_size * num_classes;
    LossResult* result = loss_result_create(total_size);
    if (!result) return NULL;
    
    result->loss = loss_cross_entropy_forward(predictions, targets, batch_size, num_classes);
    loss_cross_entropy_backward(predictions, targets, result->gradients, batch_size, num_classes);
    
    return result;
}

// Binary Cross Entropy Loss
double loss_binary_cross_entropy_forward(const double* predictions, const double* targets, 
                                        size_t size) {
    double sum = 0.0;
    const double epsilon = 1e-7;
    
    for (size_t i = 0; i < size; i++) {
        double p = fmax(fmin(predictions[i], 1.0 - epsilon), epsilon);
        sum -= targets[i] * log(p) + (1.0 - targets[i]) * log(1.0 - p);
    }
    
    return sum / size;
}

void loss_binary_cross_entropy_backward(const double* predictions, const double* targets, 
                                       double* gradients, size_t size) {
    const double epsilon = 1e-7;
    
    for (size_t i = 0; i < size; i++) {
        double p = fmax(fmin(predictions[i], 1.0 - epsilon), epsilon);
        gradients[i] = (p - targets[i]) / (p * (1.0 - p) * size);
    }
}

LossResult* loss_binary_cross_entropy(const double* predictions, const double* targets, 
                                      size_t size) {
    LossResult* result = loss_result_create(size);
    if (!result) return NULL;
    
    result->loss = loss_binary_cross_entropy_forward(predictions, targets, size);
    loss_binary_cross_entropy_backward(predictions, targets, result->gradients, size);
    
    return result;
}

// MAE Loss
double loss_mae_forward(const double* predictions, const double* targets, size_t size) {
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        sum += fabs(predictions[i] - targets[i]);
    }
    return sum / size;
}

void loss_mae_backward(const double* predictions, const double* targets, 
                      double* gradients, size_t size) {
    for (size_t i = 0; i < size; i++) {
        double diff = predictions[i] - targets[i];
        gradients[i] = (diff > 0.0 ? 1.0 : -1.0) / size;
    }
}

LossResult* loss_mae(const double* predictions, const double* targets, size_t size) {
    LossResult* result = loss_result_create(size);
    if (!result) return NULL;
    
    result->loss = loss_mae_forward(predictions, targets, size);
    loss_mae_backward(predictions, targets, result->gradients, size);
    
    return result;
}

// Huber Loss
double loss_huber_forward(const double* predictions, const double* targets, 
                         size_t size, double delta) {
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        double diff = fabs(predictions[i] - targets[i]);
        if (diff <= delta) {
            sum += 0.5 * diff * diff;
        } else {
            sum += delta * (diff - 0.5 * delta);
        }
    }
    return sum / size;
}

void loss_huber_backward(const double* predictions, const double* targets, 
                        double* gradients, size_t size, double delta) {
    for (size_t i = 0; i < size; i++) {
        double diff = predictions[i] - targets[i];
        double abs_diff = fabs(diff);
        
        if (abs_diff <= delta) {
            gradients[i] = diff / size;
        } else {
            gradients[i] = delta * (diff > 0.0 ? 1.0 : -1.0) / size;
        }
    }
}

LossResult* loss_huber(const double* predictions, const double* targets, 
                      size_t size, double delta) {
    LossResult* result = loss_result_create(size);
    if (!result) return NULL;
    
    result->loss = loss_huber_forward(predictions, targets, size, delta);
    loss_huber_backward(predictions, targets, result->gradients, size, delta);
    
    return result;
}
