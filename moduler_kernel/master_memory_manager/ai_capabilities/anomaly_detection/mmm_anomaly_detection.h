
/*
 * Master Memory Manager - Phase 4 Anomaly Detection
 * Real-time anomaly detection and response
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_ANOMALY_DETECTION_H
#define MMM_ANOMALY_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Anomaly types
typedef enum {
    MMM_ANOMALY_MEMORY_LEAK = 1,
    MMM_ANOMALY_CPU_SPIKE,
    MMM_ANOMALY_UNUSUAL_PATTERN,
    MMM_ANOMALY_PERFORMANCE_DEGRADATION,
    MMM_ANOMALY_SECURITY_THREAT,
    MMM_ANOMALY_RESOURCE_EXHAUSTION,
    MMM_ANOMALY_NETWORK_ISSUE,
    MMM_ANOMALY_UNKNOWN
} mmm_anomaly_type_t;

// Anomaly severity
typedef enum {
    MMM_ANOMALY_SEVERITY_LOW = 1,
    MMM_ANOMALY_SEVERITY_MEDIUM,
    MMM_ANOMALY_SEVERITY_HIGH,
    MMM_ANOMALY_SEVERITY_CRITICAL
} mmm_anomaly_severity_t;

// Anomaly report
typedef struct {
    uint32_t anomaly_id;
    mmm_anomaly_type_t anomaly_type;
    mmm_anomaly_severity_t severity;
    double anomaly_score;
    char description[256];
    char affected_components[512];
    struct timespec detected_at;
    bool auto_response_triggered;
    char recommended_action[256];
} mmm_anomaly_report_t;

// Function declarations

/**
 * Detect anomalies
 * @param report Output anomaly report
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_detect_anomalies(mmm_anomaly_report_t *report);

#endif /* MMM_ANOMALY_DETECTION_H */
