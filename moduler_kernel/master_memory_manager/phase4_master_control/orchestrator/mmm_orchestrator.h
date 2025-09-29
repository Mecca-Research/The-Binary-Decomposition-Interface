
/*
 * Master Memory Manager - Phase 4 Orchestrator
 * Central orchestration and system-wide control
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_ORCHESTRATOR_H
#define MMM_ORCHESTRATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// Forward declarations
typedef struct mmm_master_control mmm_master_control_t;
typedef struct mmm_component_info mmm_component_info_t;
typedef struct mmm_system_status mmm_system_status_t;

// Return codes
#define MMM_SUCCESS                 0
#define MMM_ERROR_INVALID_PARAM    -1
#define MMM_ERROR_OUT_OF_MEMORY    -2
#define MMM_ERROR_SYSTEM_FAILURE   -3
#define MMM_ERROR_ACCESS_DENIED    -4
#define MMM_ERROR_NOT_INITIALIZED  -5
#define MMM_ERROR_TIMEOUT          -6

// Component types
typedef enum {
    MMM_COMPONENT_MEMORY_POOL = 1,
    MMM_COMPONENT_CACHE,
    MMM_COMPONENT_SCHEDULER,
    MMM_COMPONENT_AI_ENGINE,
    MMM_COMPONENT_SECURITY,
    MMM_COMPONENT_TELEMETRY,
    MMM_COMPONENT_MAX
} mmm_component_type_t;

// CRITICAL FIX: Use common priority definitions to avoid conflicts
#include "../../mmm_common.h"

// Component status
typedef enum {
    MMM_STATUS_INACTIVE = 0,
    MMM_STATUS_INITIALIZING,
    MMM_STATUS_ACTIVE,
    MMM_STATUS_DEGRADED,
    MMM_STATUS_FAILED,
    MMM_STATUS_SHUTTING_DOWN
} mmm_component_status_t;

// Master control structure
struct mmm_master_control {
    uint64_t system_id;
    uint32_t component_count;
    uint32_t active_threads;
    uint64_t total_memory;
    uint32_t optimization_level;
    bool orchestrator_active;
    pthread_mutex_t control_mutex;
    pthread_cond_t control_cond;
    struct timespec start_time;
    void *private_data;
};

// Component information
struct mmm_component_info {
    uint32_t component_id;
    mmm_component_type_t component_type;
    mmm_priority_t priority;
    mmm_component_status_t status;
    uint64_t memory_usage;
    uint32_t cpu_usage_percent;
    struct timespec last_update;
    char name[64];
    void *component_data;
};

// System status
struct mmm_system_status {
    bool orchestrator_active;
    uint32_t total_components;
    uint32_t active_components;
    uint32_t failed_components;
    uint64_t total_memory_allocated;
    uint64_t total_memory_free;
    uint32_t system_load_percent;
    double performance_score;
    struct timespec last_update;
};

// Orchestrator configuration
typedef struct {
    uint32_t max_components;
    uint32_t thread_pool_size;
    uint32_t monitoring_interval_ms;
    uint32_t optimization_interval_ms;
    bool auto_recovery_enabled;
    bool performance_monitoring_enabled;
    uint32_t shutdown_timeout_ms;
} mmm_orchestrator_config_t;

// Function declarations

/**
 * Initialize master control system
 * @param control Master control structure to initialize
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_master_control_init(mmm_master_control_t *control);

/**
 * Start system orchestration
 * @param control Master control structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_orchestrator_start(mmm_master_control_t *control);

/**
 * Stop system orchestration gracefully
 * @param control Master control structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_orchestrator_shutdown(mmm_master_control_t *control);

/**
 * Get current system status
 * @param control Master control structure
 * @param status Output system status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_system_status(mmm_master_control_t *control, mmm_system_status_t *status);

/**
 * Register a component with the orchestrator
 * @param control Master control structure
 * @param component Component information
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_register_component(mmm_master_control_t *control, mmm_component_info_t *component);

/**
 * Unregister a component from the orchestrator
 * @param control Master control structure
 * @param component_id Component ID to unregister
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_unregister_component(mmm_master_control_t *control, uint32_t component_id);

/**
 * Update component status
 * @param control Master control structure
 * @param component_id Component ID
 * @param status New status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_update_component_status(mmm_master_control_t *control, uint32_t component_id, 
                               mmm_component_status_t status);

/**
 * Get component information
 * @param control Master control structure
 * @param component_id Component ID
 * @param component Output component information
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_component_info(mmm_master_control_t *control, uint32_t component_id,
                          mmm_component_info_t *component);

/**
 * Configure orchestrator settings
 * @param config Orchestrator configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_orchestrator(mmm_orchestrator_config_t *config);

/**
 * Cleanup master control resources
 * @param control Master control structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_master_control_cleanup(mmm_master_control_t *control);

/**
 * Trigger system-wide optimization
 * @param control Master control structure
 * @param optimization_type Type of optimization to perform
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_trigger_system_optimization(mmm_master_control_t *control, uint32_t optimization_type);

/**
 * Get orchestrator performance metrics
 * @param control Master control structure
 * @param metrics Output performance metrics
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_orchestrator_metrics(mmm_master_control_t *control, void *metrics);

#endif /* MMM_ORCHESTRATOR_H */
