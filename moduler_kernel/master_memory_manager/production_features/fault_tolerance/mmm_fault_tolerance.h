
/*
 * Master Memory Manager - Phase 4 Fault Tolerance
 * Advanced error recovery and system resilience
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_FAULT_TOLERANCE_H
#define MMM_FAULT_TOLERANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Error types
typedef enum {
    MMM_ERROR_MEMORY_ALLOCATION_FAILURE = 1,
    MMM_ERROR_SYSTEM_OVERLOAD,
    MMM_ERROR_NETWORK_FAILURE,
    MMM_ERROR_DISK_FAILURE,
    MMM_ERROR_COMPONENT_CRASH,
    MMM_ERROR_TIMEOUT,
    MMM_ERROR_SECURITY_VIOLATION,
    MMM_ERROR_CONFIGURATION_ERROR,
    MMM_ERROR_RESOURCE_EXHAUSTION,
    MMM_ERROR_UNKNOWN
} mmm_error_type_t;

// Error severity levels
typedef enum {
    MMM_SEVERITY_LOW = 1,
    MMM_SEVERITY_MEDIUM,
    MMM_SEVERITY_HIGH,
    MMM_SEVERITY_CRITICAL
} mmm_error_severity_t;

// Circuit breaker states
typedef enum {
    MMM_CIRCUIT_CLOSED = 0,
    MMM_CIRCUIT_OPEN,
    MMM_CIRCUIT_HALF_OPEN
} mmm_circuit_breaker_state_t;

// Recovery strategies
typedef enum {
    MMM_RECOVERY_RETRY = 1,
    MMM_RECOVERY_RESTART_COMPONENT,
    MMM_RECOVERY_FAILOVER,
    MMM_RECOVERY_GRACEFUL_DEGRADATION,
    MMM_RECOVERY_CIRCUIT_BREAKER,
    MMM_RECOVERY_ROLLBACK,
    MMM_RECOVERY_MANUAL_INTERVENTION
} mmm_recovery_strategy_type_t;

// Fault tolerance configuration
typedef struct {
    bool error_detection_enabled;
    bool auto_recovery_enabled;
    bool circuit_breaker_enabled;
    bool retry_policy_enabled;
    uint32_t max_retry_attempts;
    uint32_t retry_backoff_ms;
    uint32_t circuit_breaker_threshold;
    uint32_t circuit_breaker_timeout_ms;
    double error_rate_threshold;
    uint32_t health_check_interval_ms;
    char log_path[256];
} mmm_fault_tolerance_config_t;

// Error event
typedef struct {
    uint32_t event_id;
    mmm_error_type_t error_type;
    mmm_error_severity_t severity;
    uint32_t component_id;
    char component_name[64];
    char error_message[256];
    char stack_trace[512];
    uint64_t timestamp;
    uint32_t error_count;
    bool auto_recovery_attempted;
    bool recovery_successful;
} mmm_error_event_t;

// Error detector configuration
typedef struct {
    uint32_t detection_interval_ms;
    double error_threshold;
    bool anomaly_detection_enabled;
    uint32_t sliding_window_size;
    double anomaly_threshold;
    bool pattern_detection_enabled;
} mmm_error_detector_config_t;

// Circuit breaker
typedef struct {
    uint32_t component_id;
    mmm_circuit_breaker_state_t state;
    uint32_t failure_count;
    uint32_t success_count;
    uint32_t threshold;
    uint32_t timeout_ms;
    struct timespec last_failure_time;
    struct timespec state_change_time;
    double failure_rate;
} mmm_circuit_breaker_t;

// Retry policy
typedef struct {
    uint32_t max_attempts;
    uint32_t initial_delay_ms;
    uint32_t max_delay_ms;
    double backoff_multiplier;
    bool jitter_enabled;
    bool exponential_backoff;
    uint32_t circuit_breaker_threshold;
} mmm_retry_policy_t;

// Recovery strategy
typedef struct {
    uint32_t strategy_id;
    mmm_recovery_strategy_type_t strategy_type;
    uint32_t component_id;
    uint32_t recovery_timeout_ms;
    bool fallback_enabled;
    char fallback_action[128];
    uint32_t max_recovery_attempts;
    double success_threshold;
} mmm_recovery_strategy_t;

// Health monitor configuration
typedef struct {
    uint32_t check_interval_ms;
    uint32_t timeout_ms;
    uint32_t failure_threshold;
    uint32_t recovery_threshold;
    bool deep_health_check;
    char health_check_endpoint[128];
} mmm_health_monitor_config_t;

// Resilience metrics
typedef struct {
    double availability_percentage;
    uint32_t mean_time_to_failure_ms;
    uint32_t mean_time_to_recovery_ms;
    uint32_t total_failures_today;
    uint32_t successful_recoveries_today;
    uint32_t failed_recoveries_today;
    double recovery_success_rate;
    uint32_t circuit_breakers_open;
    struct timespec last_failure_time;
    struct timespec last_recovery_time;
} mmm_resilience_metrics_t;

// Function declarations

/**
 * Initialize fault tolerance system
 * @param config Fault tolerance configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_fault_tolerance_init(mmm_fault_tolerance_config_t *config);

/**
 * Configure error detector
 * @param config Error detector configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_error_detector(mmm_error_detector_config_t *config);

/**
 * Report error event
 * @param event Error event to report
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_report_error_event(mmm_error_event_t *event);

/**
 * Get circuit breaker status
 * @param component_id Component ID
 * @param circuit_breaker Output circuit breaker status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_circuit_breaker_status(uint32_t component_id, mmm_circuit_breaker_t *circuit_breaker);

/**
 * Configure retry policy
 * @param component_id Component ID
 * @param policy Retry policy configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_retry_policy(uint32_t component_id, mmm_retry_policy_t *policy);

/**
 * Configure recovery strategy
 * @param strategy Recovery strategy configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_recovery_strategy(mmm_recovery_strategy_t *strategy);

/**
 * Trigger recovery action
 * @param component_id Component ID
 * @param strategy_type Recovery strategy type
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_trigger_recovery_action(uint32_t component_id, mmm_recovery_strategy_type_t strategy_type);

/**
 * Configure health monitor
 * @param config Health monitor configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_health_monitor(mmm_health_monitor_config_t *config);

/**
 * Perform component health check
 * @param component_id Component ID
 * @param healthy Output health status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_check_component_health(uint32_t component_id, bool *healthy);

/**
 * Get error statistics
 * @param component_id Component ID (0 for system-wide)
 * @param stats Output error statistics
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_error_statistics(uint32_t component_id, void *stats);

/**
 * Get resilience metrics
 * @param metrics Output resilience metrics
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_resilience_metrics(mmm_resilience_metrics_t *metrics);

/**
 * Enable/disable auto-recovery
 * @param component_id Component ID (0 for system-wide)
 * @param enabled Enable/disable auto-recovery
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_set_auto_recovery_enabled(uint32_t component_id, bool enabled);

/**
 * Reset circuit breaker
 * @param component_id Component ID
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_reset_circuit_breaker(uint32_t component_id);

/**
 * Get fault tolerance status
 * @param status Output status structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_fault_tolerance_status(void *status);

/**
 * Cleanup fault tolerance system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_fault_tolerance_cleanup(void);

#endif /* MMM_FAULT_TOLERANCE_H */
