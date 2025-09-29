/**
 * @file production_stack.h
 * @brief Production Stack Integration for Master Memory Manager Phase 4
 */

#ifndef MMM_PRODUCTION_STACK_H
#define MMM_PRODUCTION_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return codes
#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_NOT_INITIALIZED -2
#define MMM_ERROR_SYSTEM_FAILURE -3

// Production stack structure
typedef struct {
    bool deployment_enabled;
    bool monitoring_enabled;
    bool scaling_enabled;
    bool fault_tolerance_enabled;
    bool security_enabled;
    bool telemetry_enabled;
    bool ai_capabilities_enabled;
    bool performance_optimization_enabled;
} mmm_production_stack_t;

// Production readiness structure
typedef struct {
    double overall_readiness_score;
    char readiness_report[1024];
    bool deployment_ready;
    bool monitoring_ready;
    bool scaling_ready;
    bool fault_tolerance_ready;
    bool security_ready;
} mmm_production_readiness_t;

// Production dashboard structure
typedef struct {
    uint64_t total_requests;
    double success_rate;
    double system_health_score;
    uint64_t active_connections;
    double cpu_usage;
    double memory_usage;
    uint32_t error_count;
    time_t last_update;
} mmm_production_dashboard_t;

// Function declarations
int mmm_initialize_production_stack(mmm_production_stack_t* stack);
int mmm_check_production_readiness(mmm_production_readiness_t* readiness);
int mmm_get_production_dashboard(mmm_production_dashboard_t* dashboard);
int mmm_production_graceful_shutdown(uint32_t timeout_ms);
int mmm_production_stack_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_PRODUCTION_STACK_H
