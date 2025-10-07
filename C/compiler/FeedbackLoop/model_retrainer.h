
#ifndef BDI_MODEL_RETRAINER_H
#define BDI_MODEL_RETRAINER_H

#include "feedback_collector.h"
#include "../ModelFormat/bdi_model.h"
#include <stdbool.h>

// Retraining configuration
typedef struct {
    int min_samples;
    double improvement_threshold;
    int max_iterations;
    bool enable_online_learning;
} RetrainingConfig;

// Initialize model retrainer
bool model_retrainer_init(void);

// Cleanup model retrainer
void model_retrainer_cleanup(void);

// Retrain model with feedback data
bool model_retrainer_retrain(BDIModel *model, const FeedbackDatabase *feedback,
                            const RetrainingConfig *config);

// Evaluate model performance
double model_retrainer_evaluate(const BDIModel *model, const FeedbackDatabase *test_data);

// Trigger automatic retraining
bool model_retrainer_auto_retrain(BDIModel *model, const FeedbackDatabase *feedback);

#endif // BDI_MODEL_RETRAINER_H
