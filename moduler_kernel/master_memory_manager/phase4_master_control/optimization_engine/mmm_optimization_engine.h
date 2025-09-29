
/*
 * Master Memory Manager - Phase 4 Optimization Engine
 * System-wide optimization and performance tuning
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_OPTIMIZATION_ENGINE_H
#define MMM_OPTIMIZATION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Optimization types
typedef enum {
    MMM_OPT_MEMORY_LAYOUT = 1,
    MMM_OPT_CACHE_LAYOUT,
    MMM_OPT_THREAD_AFFINITY,
    MMM_OPT_MEMORY_PREFETCH,
    MMM_OPT_BRANCH_PREDICTION,
    MMM_OPT_INSTRUCTION_SCHEDULING,
    MMM_OPT_DATA_LOCALITY,
    MMM_OPT_POWER_MANAGEMENT,
    MMM_OPT_THERMAL_MANAGEMENT,
    MMM_OPT_NETWORK_OPTIMIZATION
} mmm_optimization_type_t;

// Optimization levels
typedef enum {
    MMM_OPT_LEVEL_CONSERVATIVE = 1,
    MMM_OPT_LEVEL_BALANCED,
    MMM_OPT_LEVEL_AGGRESSIVE,
    MMM_OPT_LEVEL_EXTREME
} mmm_optimization_level_t;

// Optimization configuration
typedef struct {
    mmm_optimization_level_t optimization_level;
    bool adaptive_enabled;
    bool ml_enabled;
    bool real_time_enabled;
    uint32_t optimization_interval_ms;
    uint32_t learning_window_size;
    double improvement_threshold;
    uint32_t max_optimization_time_ms;
    bool thermal_aware;
    bool power_aware;
} mmm_optimization_config_t;

// Performance baseline
typedef struct {
    double cpu_utilization;
    double memory_utilization;
    double cache_hit_rate;
    uint64_t operations_per_second;
    uint64_t average_latency_ns;
    double power_consumption_watts;
    double thermal_temperature_c;
    struct timespec baseline_time;
    uint32_t measurement_duration_ms;
} mmm_performance_baseline_t;

// Optimization request
typedef struct {
    uint32_t request_id;
    mmm_optimization_type_t optimization_type;
    mmm_priority_t priority;
    double target_improvement;
    uint32_t timeout_ms;
    bool force_optimization;
    char description[128];
    struct timespec requested_at;
} mmm_optimization_request_t;

// Optimization result
typedef struct {
    uint32_t request_id;
    bool optimization_successful;
    double improvement_achieved;
    uint32_t optimization_time_ms;
    char optimization_details[256];
    mmm_performance_baseline_t before_metrics;
    mmm_performance_baseline_t after_metrics;
    struct timespec completed_at;
} mmm_optimization_result_t;

// Function declarations

/**
 * Initialize optimization engine
 * @param config Optimization configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_optimization_engine_init(mmm_optimization_config_t *config);

/**
 * Establish performance baseline
 * @param baseline Output performance baseline
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_establish_performance_baseline(mmm_performance_baseline_t *baseline);

/**
 * Trigger optimization
 * @param request Optimization request
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_trigger_optimization(mmm_optimization_request_t *request);

/**
 * Get optimization result
 * @param request Original optimization request
 * @param result Output optimization result
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_optimization_result(mmm_optimization_request_t *request, 
                               mmm_optimization_result_t *result);

/**
 * Cleanup optimization engine
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_optimization_engine_cleanup(void);

#endif /* MMM_OPTIMIZATION_ENGINE_H */
