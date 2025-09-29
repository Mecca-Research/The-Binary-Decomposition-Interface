
/*
 * Master Memory Manager - Phase 4 Auto-scaling System
 * Dynamic resource allocation and load balancing
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_SCALING_H
#define MMM_SCALING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Scaling policy types
typedef enum {
    MMM_SCALING_POLICY_MANUAL = 1,
    MMM_SCALING_POLICY_TARGET_TRACKING,
    MMM_SCALING_POLICY_STEP_SCALING,
    MMM_SCALING_POLICY_PREDICTIVE
} mmm_scaling_policy_type_t;

// Scaling actions
typedef enum {
    MMM_SCALING_ACTION_NONE = 0,
    MMM_SCALING_ACTION_SCALE_OUT,
    MMM_SCALING_ACTION_SCALE_IN,
    MMM_SCALING_ACTION_MAINTAIN
} mmm_scaling_action_t;

// Scaling operations
typedef enum {
    MMM_SCALING_OP_ADD_INSTANCE = 1,
    MMM_SCALING_OP_REMOVE_INSTANCE,
    MMM_SCALING_OP_MODIFY_INSTANCE,
    MMM_SCALING_OP_REBALANCE
} mmm_scaling_operation_type_t;

// Load balancing algorithms
typedef enum {
    MMM_LB_ROUND_ROBIN = 1,
    MMM_LB_LEAST_CONNECTIONS,
    MMM_LB_WEIGHTED_ROUND_ROBIN,
    MMM_LB_IP_HASH,
    MMM_LB_LEAST_RESPONSE_TIME
} mmm_load_balance_algorithm_t;

// Scaling configuration
typedef struct {
    uint32_t min_instances;
    uint32_t max_instances;
    double scale_up_threshold;
    double scale_down_threshold;
    uint32_t scale_up_cooldown_ms;
    uint32_t scale_down_cooldown_ms;
    double scaling_factor;
    bool predictive_scaling_enabled;
    uint32_t evaluation_period_ms;
    uint32_t warmup_time_ms;
} mmm_scaling_config_t;

// Scaling policy
typedef struct {
    uint32_t policy_id;
    mmm_scaling_policy_type_t policy_type;
    mmm_metric_type_t target_metric;
    double target_value;
    double scale_out_adjustment;
    double scale_in_adjustment;
    bool scale_out_enabled;
    bool scale_in_enabled;
    uint32_t evaluation_periods;
    uint32_t breach_threshold;
    char name[64];
} mmm_scaling_policy_t;

// Scaling metrics
typedef struct {
    double cpu_usage;
    double memory_usage;
    double request_rate;
    double response_time_ms;
    double error_rate;
    uint32_t active_connections;
    uint32_t queue_depth;
    struct timespec timestamp;
} mmm_scaling_metrics_t;

// Scaling decision
typedef struct {
    mmm_scaling_action_t action;
    uint32_t target_instances;
    double confidence_score;
    char reason[128];
    uint32_t estimated_time_ms;
    struct timespec decision_time;
} mmm_scaling_decision_t;

// Scaling operation
typedef struct {
    uint32_t operation_id;
    mmm_scaling_operation_type_t operation_type;
    uint32_t target_count;
    uint32_t timeout_ms;
    bool force;
    char description[128];
    struct timespec scheduled_time;
} mmm_scaling_operation_t;

// Scaling status
typedef struct {
    uint32_t current_instances;
    uint32_t desired_instances;
    uint32_t healthy_instances;
    uint32_t unhealthy_instances;
    uint32_t pending_instances;
    uint32_t terminating_instances;
    mmm_scaling_action_t last_action;
    struct timespec last_scaling_time;
    uint32_t scaling_operations_today;
    double current_capacity_utilization;
} mmm_scaling_status_t;

// Load balancer configuration
typedef struct {
    mmm_load_balance_algorithm_t algorithm;
    bool health_check_enabled;
    uint32_t health_check_interval_ms;
    uint32_t health_check_timeout_ms;
    uint32_t unhealthy_threshold;
    uint32_t healthy_threshold;
    bool sticky_sessions;
    uint32_t session_timeout_ms;
    char health_check_path[128];
} mmm_load_balance_config_t;

// Instance information
typedef struct {
    uint32_t instance_id;
    char instance_name[64];
    char ip_address[16];
    uint16_t port;
    bool healthy;
    double cpu_usage;
    double memory_usage;
    uint32_t active_connections;
    uint32_t requests_per_second;
    struct timespec last_health_check;
    struct timespec created_at;
} mmm_instance_info_t;

// Function declarations

/**
 * Initialize scaling system
 * @param config Scaling configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_scaling_init(mmm_scaling_config_t *config);

/**
 * Configure scaling policy
 * @param policy Scaling policy configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_scaling_policy(mmm_scaling_policy_t *policy);

/**
 * Remove scaling policy
 * @param policy_id Policy ID to remove
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_remove_scaling_policy(uint32_t policy_id);

/**
 * Evaluate scaling decision
 * @param metrics Current system metrics
 * @param decision Output scaling decision
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_evaluate_scaling_decision(mmm_scaling_metrics_t *metrics, 
                                 mmm_scaling_decision_t *decision);

/**
 * Execute scaling operation
 * @param operation Scaling operation to execute
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_execute_scaling_operation(mmm_scaling_operation_t *operation);

/**
 * Get scaling status
 * @param status Output scaling status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_scaling_status(mmm_scaling_status_t *status);

/**
 * Configure load balancer
 * @param config Load balancer configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_load_balancer(mmm_load_balance_config_t *config);

/**
 * Add instance to load balancer
 * @param instance Instance information
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_add_instance(mmm_instance_info_t *instance);

/**
 * Remove instance from load balancer
 * @param instance_id Instance ID to remove
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_remove_instance(uint32_t instance_id);

/**
 * Get instance list
 * @param instances Output array of instances
 * @param max_instances Maximum number of instances to return
 * @return Number of instances returned, negative on error
 */
int mmm_get_instances(mmm_instance_info_t *instances, uint32_t max_instances);

/**
 * Perform health check on instance
 * @param instance_id Instance ID to check
 * @param healthy Output health status
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_check_instance_health(uint32_t instance_id, bool *healthy);

/**
 * Get load balancer statistics
 * @param stats Output statistics structure
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_load_balancer_stats(void *stats);

/**
 * Enable/disable auto-scaling
 * @param enabled Enable/disable auto-scaling
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_set_auto_scaling_enabled(bool enabled);

/**
 * Trigger manual scaling
 * @param target_instances Target number of instances
 * @param reason Reason for manual scaling
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_trigger_manual_scaling(uint32_t target_instances, const char *reason);

/**
 * Get scaling history
 * @param history Output scaling history
 * @param max_entries Maximum number of entries to return
 * @return Number of entries returned, negative on error
 */
int mmm_get_scaling_history(mmm_scaling_decision_t *history, uint32_t max_entries);

/**
 * Cleanup scaling system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_scaling_cleanup(void);

#endif /* MMM_SCALING_H */
