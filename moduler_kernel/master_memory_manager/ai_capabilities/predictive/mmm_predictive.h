
/*
 * Master Memory Manager - Phase 4 Predictive Analytics
 * System behavior prediction and optimization
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_PREDICTIVE_H
#define MMM_PREDICTIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Prediction types
typedef enum {
    MMM_PREDICT_MEMORY_USAGE = 1,
    MMM_PREDICT_CPU_USAGE,
    MMM_PREDICT_SYSTEM_LOAD,
    MMM_PREDICT_ERROR_RATE,
    MMM_PREDICT_PERFORMANCE_DEGRADATION,
    MMM_PREDICT_RESOURCE_EXHAUSTION,
    MMM_PREDICT_SCALING_NEEDS,
    MMM_PREDICT_MAINTENANCE_NEEDS
} mmm_prediction_type_t;

// AI model types
typedef enum {
    MMM_AI_LINEAR_REGRESSION = 1,
    MMM_AI_NEURAL_NETWORK,
    MMM_AI_RANDOM_FOREST,
    MMM_AI_LSTM,
    MMM_AI_TRANSFORMER,
    MMM_AI_CUSTOM
} mmm_ai_model_type_t;

// AI configuration
typedef struct {
    uint32_t model_type;
    uint32_t learning_rate;
    uint32_t prediction_window;
    uint32_t anomaly_threshold;
    bool online_learning;
    bool feature_engineering;
    uint32_t training_data_size;
    uint32_t model_update_interval_ms;
} mmm_ai_config_t;

// Prediction result
typedef struct {
    mmm_prediction_type_t prediction_type;
    double predicted_value;
    double confidence_score;
    uint32_t prediction_horizon_ms;
    bool optimization_needed;
    char recommendation[256];
    struct timespec prediction_time;
} mmm_prediction_t;

// Function declarations

/**
 * Initialize AI capabilities
 * @param config AI configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_ai_init(mmm_ai_config_t *config);

/**
 * Predict system behavior
 * @param prediction Output prediction
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_predict_system_behavior(mmm_prediction_t *prediction);

/**
 * Cleanup AI capabilities
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_ai_cleanup(void);

#endif /* MMM_PREDICTIVE_H */
