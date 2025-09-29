
/*
 * Master Memory Manager - Phase 4 Audit & Compliance
 * Comprehensive logging and compliance reporting
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_AUDIT_H
#define MMM_AUDIT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Audit event types
typedef enum {
    MMM_AUDIT_SYSTEM_START = 1,
    MMM_AUDIT_SYSTEM_STOP,
    MMM_AUDIT_USER_LOGIN,
    MMM_AUDIT_USER_LOGOUT,
    MMM_AUDIT_ACCESS_GRANTED,
    MMM_AUDIT_ACCESS_DENIED,
    MMM_AUDIT_CONFIGURATION_CHANGE,
    MMM_AUDIT_SECURITY_VIOLATION,
    MMM_AUDIT_ERROR_EVENT,
    MMM_AUDIT_PERFORMANCE_ALERT,
    MMM_AUDIT_CUSTOM
} mmm_audit_event_type_t;

// Compliance frameworks
typedef enum {
    MMM_COMPLIANCE_NONE = 0,
    MMM_COMPLIANCE_SOX,
    MMM_COMPLIANCE_HIPAA,
    MMM_COMPLIANCE_GDPR,
    MMM_COMPLIANCE_PCI_DSS,
    MMM_COMPLIANCE_ISO27001,
    MMM_COMPLIANCE_CUSTOM
} mmm_compliance_framework_t;

// Audit configuration
typedef struct {
    bool audit_enabled;
    uint32_t audit_level;
    char audit_log_path[256];
    uint32_t max_log_size_mb;
    uint32_t log_retention_days;
    bool real_time_monitoring;
    bool compliance_reporting;
    mmm_compliance_framework_t compliance_framework;
    bool encryption_enabled;
    bool tamper_protection;
} mmm_audit_config_t;

// Audit event
typedef struct {
    uint64_t event_id;
    mmm_audit_event_type_t event_type;
    uint32_t user_id;
    uint32_t resource_id;
    char description[256];
    char details[512];
    struct timespec timestamp;
    char source_ip[16];
    bool success;
    uint32_t error_code;
} mmm_audit_event_t;

// Compliance report
typedef struct {
    mmm_compliance_framework_t framework;
    struct timespec report_period_start;
    struct timespec report_period_end;
    uint32_t total_events;
    uint32_t security_violations;
    uint32_t access_denials;
    uint32_t configuration_changes;
    double compliance_score;
    char recommendations[1024];
    struct timespec generated_at;
} mmm_compliance_report_t;

// Function declarations

/**
 * Initialize audit system
 * @param config Audit configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_audit_init(mmm_audit_config_t *config);

/**
 * Log audit event
 * @param event_type Event type
 * @param description Event description
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_audit_log(uint32_t event_type, const char *description);

/**
 * Generate compliance report
 * @param report Output compliance report
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_generate_compliance_report(mmm_compliance_report_t *report);

/**
 * Cleanup audit system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_audit_cleanup(void);

#endif /* MMM_AUDIT_H */
