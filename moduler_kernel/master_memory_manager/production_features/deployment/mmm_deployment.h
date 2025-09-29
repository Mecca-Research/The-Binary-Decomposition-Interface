
/**
 * @file mmm_deployment.h
 * @brief Deployment System for Master Memory Manager Phase 4
 */

#ifndef MMM_DEPLOYMENT_H
#define MMM_DEPLOYMENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_SYSTEM_FAILURE -3

typedef enum {
    MMM_DEPLOY_DEVELOPMENT,
    MMM_DEPLOY_STAGING,
    MMM_DEPLOY_PRODUCTION
} mmm_deployment_type_t;

typedef struct {
    mmm_deployment_type_t deployment_type;
    bool auto_configure;
    bool validation_enabled;
    bool rollback_enabled;
    uint32_t health_check_timeout;
    uint32_t deployment_timeout;
    uint32_t rollback_timeout;
    char config_path[256];
    char log_path[256];
    char backup_path[256];
} mmm_deployment_config_t;

int mmm_deployment_init(const mmm_deployment_config_t* config);
int mmm_deployment_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_DEPLOYMENT_H
