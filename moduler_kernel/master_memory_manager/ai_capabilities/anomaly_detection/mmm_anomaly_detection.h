
/**
 * @file mmm_anomaly_detection.h
 * @brief Anomaly Detection AI Capabilities for Master Memory Manager Phase 4
 */

#ifndef MMM_ANOMALY_DETECTION_H
#define MMM_ANOMALY_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef struct {
    double anomaly_score;
    char description[256];
    bool auto_response_triggered;
    char recommended_action[256];
} mmm_anomaly_report_t;

int mmm_detect_anomalies(mmm_anomaly_report_t* report);

#ifdef __cplusplus
}
#endif

#endif // MMM_ANOMALY_DETECTION_H
