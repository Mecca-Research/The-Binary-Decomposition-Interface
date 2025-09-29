
/**
 * @file mmm_audit.h
 * @brief Audit System for Master Memory Manager Phase 4
 */

#ifndef MMM_AUDIT_H
#define MMM_AUDIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef enum {
    MMM_COMPLIANCE_ISO27001,
    MMM_COMPLIANCE_SOX,
    MMM_COMPLIANCE_GDPR
} mmm_compliance_framework_t;

typedef enum {
    MMM_AUDIT_SYSTEM_START,
    MMM_AUDIT_SYSTEM_STOP,
    MMM_AUDIT_ACCESS_GRANTED,
    MMM_AUDIT_ACCESS_DENIED
} mmm_audit_event_t;

typedef struct {
    bool audit_enabled;
    uint32_t audit_level;
    uint32_t max_log_size_mb;
    uint32_t log_retention_days;
    bool real_time_monitoring;
    bool compliance_reporting;
    mmm_compliance_framework_t compliance_framework;
    bool encryption_enabled;
    bool tamper_protection;
    char audit_log_path[256];
} mmm_audit_config_t;

int mmm_audit_init(const mmm_audit_config_t* config);
int mmm_audit_log(mmm_audit_event_t event, const char* message);
int mmm_audit_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_AUDIT_H
