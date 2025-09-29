
/**
 * @file mmm_security.h
 * @brief Security Framework for Master Memory Manager Phase 4
 */

#ifndef MMM_SECURITY_H
#define MMM_SECURITY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef struct {
    uint32_t encryption_level;
    uint32_t access_control_flags;
    uint32_t audit_level;
    bool authentication_required;
    bool authorization_required;
    bool encryption_at_rest;
    bool encryption_in_transit;
    uint32_t session_timeout_ms;
    uint32_t max_failed_attempts;
    char security_policy[256];
} mmm_security_config_t;

int mmm_security_init(const mmm_security_config_t* config);
int mmm_security_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_SECURITY_H
