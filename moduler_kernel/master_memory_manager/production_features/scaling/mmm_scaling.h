
/**
 * @file mmm_scaling.h
 * @brief Scaling System for Master Memory Manager Phase 4
 */

#ifndef MMM_SCALING_H
#define MMM_SCALING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef struct {
    uint32_t min_instances;
    uint32_t max_instances;
    double scale_up_threshold;
    double scale_down_threshold;
    uint32_t scale_up_cooldown_ms;
    uint32_t scale_down_cooldown_ms;
    double scaling_factor;
    bool predictive_scaling_enabled;
    uint32_t evaluation_period_ms;
    uint32_t warmup_time_ms;
} mmm_scaling_config_t;

int mmm_scaling_init(const mmm_scaling_config_t* config);
int mmm_scaling_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_SCALING_H
