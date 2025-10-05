
/**
 * @file metrics.h
 * @brief Metrics API
 * @details This file provides the metrics functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_METRICS_H
#define BDI_METRICS_H

#include <stddef.h>
#include <stdbool.h>

// Classification metrics
double metric_accuracy(const double* predictions, const double* targets, 
                      size_t batch_size, size_t num_classes);
double metric_binary_accuracy(const double* predictions, const double* targets, 
                              size_t size, double threshold);

// Precision, Recall, F1
typedef struct {
    double precision;
    double recall;
    double f1_score;
    size_t true_positives;
    size_t false_positives;
    size_t false_negatives;
    size_t true_negatives;
} ClassificationMetrics;

ClassificationMetrics metric_binary_classification(const double* predictions, 
                                                   const double* targets, 
                                                   size_t size, double threshold);

// Multi-class metrics (one-vs-rest)
ClassificationMetrics* metric_multiclass_classification(const double* predictions, 
                                                        const double* targets,
                                                        size_t batch_size, 
                                                        size_t num_classes);

// Regression metrics
double metric_mae(const double* predictions, const double* targets, size_t size);
double metric_mse(const double* predictions, const double* targets, size_t size);
double metric_rmse(const double* predictions, const double* targets, size_t size);
double metric_r2_score(const double* predictions, const double* targets, size_t size);

// Top-k accuracy
double metric_top_k_accuracy(const double* predictions, const double* targets,
                             size_t batch_size, size_t num_classes, size_t k);

// Confusion matrix
typedef struct {
    size_t** matrix;
    size_t num_classes;
} ConfusionMatrix;

ConfusionMatrix* confusion_matrix_create(size_t num_classes);
void confusion_matrix_destroy(ConfusionMatrix* cm);
void confusion_matrix_update(ConfusionMatrix* cm, const double* predictions, 
                            const double* targets, size_t batch_size);
void confusion_matrix_print(const ConfusionMatrix* cm);

#endif // BDI_METRICS_H
