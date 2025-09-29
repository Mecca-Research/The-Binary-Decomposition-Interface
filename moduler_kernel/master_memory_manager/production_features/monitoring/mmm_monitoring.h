
/**
 * @file mmm_monitoring.h
 * @brief Monitoring System for Master Memory Manager Phase 4
 */

#ifndef MMM_MONITORING_H
#define MMM_MONITORING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef struct {
    uint32_t sampling_interval_ms;
    uint32_t metrics_buffer_size;
    uint32_t alert_threshold_count;
    bool telemetry_enabled;
    bool real_time_enabled;
    bool aggregation_enabled;
    uint32_t retention_period_hours;
    char metrics_endpoint[256];
    char alert_endpoint[256];
} mmm_monitoring_config_t;

int mmm_monitoring_init(const mmm_monitoring_config_t* config);
int mmm_monitoring_start(void);
int mmm_monitoring_stop(void);
int mmm_monitoring_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_MONITORING_H
