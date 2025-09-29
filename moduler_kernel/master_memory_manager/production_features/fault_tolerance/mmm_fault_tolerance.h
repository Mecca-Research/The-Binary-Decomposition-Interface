
/**
 * @file mmm_fault_tolerance.h
 * @brief Fault Tolerance System for Master Memory Manager Phase 4
 */

#ifndef MMM_FAULT_TOLERANCE_H
#define MMM_FAULT_TOLERANCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef struct {
    bool error_detection_enabled;
    bool auto_recovery_enabled;
    bool circuit_breaker_enabled;
    bool retry_policy_enabled;
    uint32_t max_retry_attempts;
    uint32_t retry_backoff_ms;
    uint32_t circuit_breaker_threshold;
    uint32_t circuit_breaker_timeout_ms;
    double error_rate_threshold;
    uint32_t health_check_interval_ms;
    char log_path[256];
} mmm_fault_tolerance_config_t;

int mmm_fault_tolerance_init(const mmm_fault_tolerance_config_t* config);
int mmm_fault_tolerance_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_FAULT_TOLERANCE_H
