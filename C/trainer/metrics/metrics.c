
#include "metrics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Helper function to get predicted class
static size_t argmax(const double* values, size_t size) {
    size_t max_idx = 0;
    double max_val = values[0];
    
    for (size_t i = 1; i < size; i++) {
        if (values[i] > max_val) {
            max_val = values[i];
            max_idx = i;
        }
    }
    
    return max_idx;
}

// Accuracy
double metric_accuracy(const double* predictions, const double* targets, 
                      size_t batch_size, size_t num_classes) {
    size_t correct = 0;
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * num_classes;
        const double* target = targets + b * num_classes;
        
        size_t pred_class = argmax(pred, num_classes);
        size_t target_class = argmax(target, num_classes);
        
        if (pred_class == target_class) {
            correct++;
        }
    }
    
    return (double)correct / batch_size;
}

double metric_binary_accuracy(const double* predictions, const double* targets, 
                              size_t size, double threshold) {
    size_t correct = 0;
    
    for (size_t i = 0; i < size; i++) {
        bool pred = predictions[i] >= threshold;
        bool target = targets[i] >= 0.5;
        
        if (pred == target) {
            correct++;
        }
    }
    
    return (double)correct / size;
}

// Binary classification metrics
ClassificationMetrics metric_binary_classification(const double* predictions, 
                                                   const double* targets, 
                                                   size_t size, double threshold) {
    ClassificationMetrics metrics = {0};
    
    for (size_t i = 0; i < size; i++) {
        bool pred = predictions[i] >= threshold;
        bool target = targets[i] >= 0.5;
        
        if (pred && target) {
            metrics.true_positives++;
        } else if (pred && !target) {
            metrics.false_positives++;
        } else if (!pred && target) {
            metrics.false_negatives++;
        } else {
            metrics.true_negatives++;
        }
    }
    
    // Calculate precision, recall, F1
    if (metrics.true_positives + metrics.false_positives > 0) {
        metrics.precision = (double)metrics.true_positives / 
                           (metrics.true_positives + metrics.false_positives);
    }
    
    if (metrics.true_positives + metrics.false_negatives > 0) {
        metrics.recall = (double)metrics.true_positives / 
                        (metrics.true_positives + metrics.false_negatives);
    }
    
    if (metrics.precision + metrics.recall > 0.0) {
        metrics.f1_score = 2.0 * metrics.precision * metrics.recall / 
                          (metrics.precision + metrics.recall);
    }
    
    return metrics;
}

// Multi-class metrics (one-vs-rest)
ClassificationMetrics* metric_multiclass_classification(const double* predictions, 
                                                        const double* targets,
                                                        size_t batch_size, 
                                                        size_t num_classes) {
    ClassificationMetrics* metrics = calloc(num_classes, sizeof(ClassificationMetrics));
    if (!metrics) return NULL;
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * num_classes;
        const double* target = targets + b * num_classes;
        
        size_t pred_class = argmax(pred, num_classes);
        size_t target_class = argmax(target, num_classes);
        
        for (size_t c = 0; c < num_classes; c++) {
            bool pred_c = (pred_class == c);
            bool target_c = (target_class == c);
            
            if (pred_c && target_c) {
                metrics[c].true_positives++;
            } else if (pred_c && !target_c) {
                metrics[c].false_positives++;
            } else if (!pred_c && target_c) {
                metrics[c].false_negatives++;
            } else {
                metrics[c].true_negatives++;
            }
        }
    }
    
    // Calculate precision, recall, F1 for each class
    for (size_t c = 0; c < num_classes; c++) {
        if (metrics[c].true_positives + metrics[c].false_positives > 0) {
            metrics[c].precision = (double)metrics[c].true_positives / 
                                  (metrics[c].true_positives + metrics[c].false_positives);
        }
        
        if (metrics[c].true_positives + metrics[c].false_negatives > 0) {
            metrics[c].recall = (double)metrics[c].true_positives / 
                               (metrics[c].true_positives + metrics[c].false_negatives);
        }
        
        if (metrics[c].precision + metrics[c].recall > 0.0) {
            metrics[c].f1_score = 2.0 * metrics[c].precision * metrics[c].recall / 
                                 (metrics[c].precision + metrics[c].recall);
        }
    }
    
    return metrics;
}

// Regression metrics
double metric_mae(const double* predictions, const double* targets, size_t size) {
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        sum += fabs(predictions[i] - targets[i]);
    }
    return sum / size;
}

double metric_mse(const double* predictions, const double* targets, size_t size) {
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        double diff = predictions[i] - targets[i];
        sum += diff * diff;
    }
    return sum / size;
}

double metric_rmse(const double* predictions, const double* targets, size_t size) {
    return sqrt(metric_mse(predictions, targets, size));
}

double metric_r2_score(const double* predictions, const double* targets, size_t size) {
    // Calculate mean of targets
    double mean = 0.0;
    for (size_t i = 0; i < size; i++) {
        mean += targets[i];
    }
    mean /= size;
    
    // Calculate total sum of squares and residual sum of squares
    double ss_tot = 0.0;
    double ss_res = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        double diff_mean = targets[i] - mean;
        double diff_pred = targets[i] - predictions[i];
        ss_tot += diff_mean * diff_mean;
        ss_res += diff_pred * diff_pred;
    }
    
    if (ss_tot == 0.0) return 0.0;
    
    return 1.0 - (ss_res / ss_tot);
}

// Top-k accuracy
double metric_top_k_accuracy(const double* predictions, const double* targets,
                             size_t batch_size, size_t num_classes, size_t k) {
    if (k > num_classes) k = num_classes;
    
    size_t correct = 0;
    size_t* top_k_indices = malloc(k * sizeof(size_t));
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * num_classes;
        const double* target = targets + b * num_classes;
        
        size_t target_class = argmax(target, num_classes);
        
        // Find top-k predictions
        for (size_t i = 0; i < k; i++) {
            size_t max_idx = 0;
            double max_val = -INFINITY;
            
            for (size_t j = 0; j < num_classes; j++) {
                bool already_selected = false;
                for (size_t m = 0; m < i; m++) {
                    if (top_k_indices[m] == j) {
                        already_selected = true;
                        break;
                    }
                }
                
                if (!already_selected && pred[j] > max_val) {
                    max_val = pred[j];
                    max_idx = j;
                }
            }
            
            top_k_indices[i] = max_idx;
        }
        
        // Check if target is in top-k
        for (size_t i = 0; i < k; i++) {
            if (top_k_indices[i] == target_class) {
                correct++;
                break;
            }
        }
    }
    
    free(top_k_indices);
    return (double)correct / batch_size;
}

// Confusion matrix
ConfusionMatrix* confusion_matrix_create(size_t num_classes) {
    ConfusionMatrix* cm = malloc(sizeof(ConfusionMatrix));
    if (!cm) return NULL;
    
    cm->matrix = malloc(num_classes * sizeof(size_t*));
    if (!cm->matrix) {
        free(cm);
        return NULL;
    }
    
    for (size_t i = 0; i < num_classes; i++) {
        cm->matrix[i] = calloc(num_classes, sizeof(size_t));
        if (!cm->matrix[i]) {
            for (size_t j = 0; j < i; j++) {
                free(cm->matrix[j]);
            }
            free(cm->matrix);
            free(cm);
            return NULL;
        }
    }
    
    cm->num_classes = num_classes;
    return cm;
}

void confusion_matrix_destroy(ConfusionMatrix* cm) {
    if (cm) {
        for (size_t i = 0; i < cm->num_classes; i++) {
            free(cm->matrix[i]);
        }
        free(cm->matrix);
        free(cm);
    }
}

void confusion_matrix_update(ConfusionMatrix* cm, const double* predictions, 
                            const double* targets, size_t batch_size) {
    if (!cm) return;
    
    for (size_t b = 0; b < batch_size; b++) {
        const double* pred = predictions + b * cm->num_classes;
        const double* target = targets + b * cm->num_classes;
        
        size_t pred_class = argmax(pred, cm->num_classes);
        size_t target_class = argmax(target, cm->num_classes);
        
        cm->matrix[target_class][pred_class]++;
    }
}

void confusion_matrix_print(const ConfusionMatrix* cm) {
    if (!cm) return;
    
    printf("\nConfusion Matrix (%zu classes):\n", cm->num_classes);
    printf("     ");
    for (size_t i = 0; i < cm->num_classes; i++) {
        printf("%6zu ", i);
    }
    printf("\n");
    
    for (size_t i = 0; i < cm->num_classes; i++) {
        printf("%4zu ", i);
        for (size_t j = 0; j < cm->num_classes; j++) {
            printf("%6zu ", cm->matrix[i][j]);
        }
        printf("\n");
    }
}
