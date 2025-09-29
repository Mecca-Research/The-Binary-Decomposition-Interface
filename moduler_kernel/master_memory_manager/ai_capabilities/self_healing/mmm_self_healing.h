
/**
 * @file mmm_self_healing.h
 * @brief Self-Healing AI Capabilities for Master Memory Manager Phase 4
 */

#ifndef MMM_SELF_HEALING_H
#define MMM_SELF_HEALING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef enum {
    MMM_AI_NEURAL_NETWORK,
    MMM_AI_DECISION_TREE,
    MMM_AI_ENSEMBLE
} mmm_ai_model_type_t;

typedef struct {
    mmm_ai_model_type_t model_type;
    uint32_t learning_rate;
    uint32_t prediction_window;
    uint32_t anomaly_threshold;
    bool online_learning;
    bool feature_engineering;
    uint32_t training_data_size;
    uint32_t model_update_interval_ms;
} mmm_ai_config_t;

int mmm_ai_init(const mmm_ai_config_t* config);
int mmm_ai_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_SELF_HEALING_H
