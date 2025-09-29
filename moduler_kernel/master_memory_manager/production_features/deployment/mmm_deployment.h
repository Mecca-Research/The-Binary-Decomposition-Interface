
/*
 * Master Memory Manager - Phase 4 Production Deployment
 * Complete deployment automation and configuration
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_DEPLOYMENT_H
#define MMM_DEPLOYMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Deployment types
typedef enum {
    MMM_DEPLOY_DEVELOPMENT = 1,
    MMM_DEPLOY_STAGING,
    MMM_DEPLOY_PRODUCTION,
    MMM_DEPLOY_TESTING
} mmm_deployment_type_t;

// Deployment states
typedef enum {
    MMM_DEPLOY_PENDING = 0,
    MMM_DEPLOY_PREPARING,
    MMM_DEPLOY_DEPLOYING,
    MMM_DEPLOY_SUCCESS,
    MMM_DEPLOY_FAILED,
    MMM_DEPLOY_ROLLING_BACK,
    MMM_DEPLOY_ROLLED_BACK
} mmm_deployment_state_t;

// Health check status
typedef enum {
    MMM_HEALTH_UNKNOWN = 0,
    MMM_HEALTH_HEALTHY,
    MMM_HEALTH_DEGRADED,
    MMM_HEALTH_UNHEALTHY,
    MMM_HEALTH_CRITICAL
} mmm_health_status_t;

// Deployment configuration
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

// Configuration validation result
typedef struct {
    bool is_valid;
    uint32_t error_count;
    uint32_t warning_count;
    char errors[10][256];
    char warnings[10][256];
    double validation_score;
} mmm_config_validation_t;

// Deployment plan
typedef struct {
    uint32_t plan_id;
    uint32_t steps_count;
    struct {
        uint32_t step_id;
        char description[128];
        uint32_t estimated_duration_ms;
        bool critical;
        bool rollback_supported;
    } steps[32];
    uint32_t total_estimated_duration_ms;
    struct timespec created_at;
} mmm_deployment_plan_t;

// Deployment status
typedef struct {
    uint32_t deployment_id;
    mmm_deployment_state_t state;
    uint32_t current_step;
    uint32_t completed_steps;
    uint32_t failed_steps;
    uint32_t progress_percent;
    uint32_t elapsed_time_ms;
    uint32_t remaining_time_ms;
    char current_operation[128];
    char last_error[256];
    struct timespec started_at;
    struct timespec updated_at;
} mmm_deployment_status_t;

// Health check result
typedef struct {
    mmm_health_status_t overall_status;
    uint32_t checks_performed;
    uint32_t checks_passed;
    uint32_t checks_failed;
    struct {
        char component[64];
        mmm_health_status_t status;
        char message[128];
        uint32_t response_time_ms;
    } component_health[16];
    double health_score;
    struct timespec checked_at;
} mmm_health_check_result_t;

// Function declarations

/**
 * Initialize deployment system
 * @param config Deployment configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_deployment_init(mmm_deployment_config_t *config);

/**
 * Validate deployment configuration
 * @param config Configuration to validate
 * @param validation Output validation result
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_validate_deployment_config(mmm_deployment_config_t *config, 
                                  mmm_config_validation_t *validation);

/**
 * Prepare deployment plan
 * @param config Deployment configuration
 * @param plan Output deployment plan
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_prepare_deployment(mmm_deployment_config_t *config, mmm_deployment_plan_t *plan);

/**
 * Execute deployment
 * @param plan Deployment plan to execute
 * @param status Output deployment status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_execute_deployment(mmm_deployment_plan_t *plan, mmm_deployment_status_t *status);

/**
 * Get deployment status
 * @param deployment_id Deployment ID
 * @param status Output deployment status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_deployment_status(uint32_t deployment_id, mmm_deployment_status_t *status);

/**
 * Cancel deployment
 * @param deployment_id Deployment ID to cancel
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_cancel_deployment(uint32_t deployment_id);

/**
 * Rollback deployment
 * @param deployment_id Deployment ID to rollback
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_rollback_deployment(uint32_t deployment_id);

/**
 * Perform health check
 * @param result Output health check result
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_perform_health_check(mmm_health_check_result_t *result);

/**
 * Configure auto-deployment
 * @param enabled Enable/disable auto-deployment
 * @param trigger_conditions Conditions that trigger deployment
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_auto_deployment(bool enabled, uint32_t trigger_conditions);

/**
 * Get deployment history
 * @param history Output deployment history
 * @param max_entries Maximum number of entries to return
 * @return Number of entries returned, negative on error
 */
int mmm_get_deployment_history(mmm_deployment_status_t *history, uint32_t max_entries);

/**
 * Cleanup deployment system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_deployment_cleanup(void);

#endif /* MMM_DEPLOYMENT_H */
