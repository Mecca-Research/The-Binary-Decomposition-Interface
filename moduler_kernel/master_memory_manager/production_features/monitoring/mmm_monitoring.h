
/*
 * Master Memory Manager - Phase 4 Real-time Monitoring
 * Real-time system monitoring and performance analytics
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_MONITORING_H
#define MMM_MONITORING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Metric types
typedef enum {
    MMM_METRIC_CPU_USAGE = 1,
    MMM_METRIC_MEMORY_USAGE,
    MMM_METRIC_DISK_USAGE,
    MMM_METRIC_NETWORK_USAGE,
    MMM_METRIC_CACHE_HIT_RATE,
    MMM_METRIC_OPERATIONS_PER_SECOND,
    MMM_METRIC_RESPONSE_TIME,
    MMM_METRIC_ERROR_RATE,
    MMM_METRIC_THROUGHPUT,
    MMM_METRIC_LATENCY,
    MMM_METRIC_CUSTOM
} mmm_metric_type_t;

// Alert actions
#define MMM_ALERT_ACTION_LOG        0x01
#define MMM_ALERT_ACTION_EMAIL      0x02
#define MMM_ALERT_ACTION_SMS        0x04
#define MMM_ALERT_ACTION_CALLBACK   0x08
#define MMM_ALERT_ACTION_WEBHOOK    0x10
#define MMM_ALERT_ACTION_AUTO_SCALE 0x20

// Comparison operators
typedef enum {
    MMM_COMPARE_EQUAL = 1,
    MMM_COMPARE_NOT_EQUAL,
    MMM_COMPARE_GREATER_THAN,
    MMM_COMPARE_LESS_THAN,
    MMM_COMPARE_GREATER_EQUAL,
    MMM_COMPARE_LESS_EQUAL
} mmm_comparison_t;

// Monitoring configuration
typedef struct {
    uint32_t sampling_interval_ms;
    uint32_t metrics_buffer_size;
    uint32_t alert_threshold_count;
    bool telemetry_enabled;
    bool real_time_enabled;
    bool aggregation_enabled;
    uint32_t retention_period_hours;
    char metrics_endpoint[256];
    char alert_endpoint[256];
} mmm_monitoring_config_t;

// System metrics
typedef struct {
    double cpu_usage;
    double memory_usage;
    double disk_usage;
    double network_rx_mbps;
    double network_tx_mbps;
    uint32_t active_connections;
    uint32_t thread_count;
    uint32_t process_count;
    struct timespec timestamp;
} mmm_system_metrics_t;

// Performance metrics
typedef struct {
    uint64_t operations_per_second;
    uint64_t average_latency_ns;
    uint64_t p95_latency_ns;
    uint64_t p99_latency_ns;
    double cache_hit_rate;
    double error_rate;
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    struct timespec timestamp;
} mmm_performance_metrics_t;

// Alert configuration
typedef struct {
    uint32_t alert_id;
    mmm_metric_type_t metric_type;
    double threshold_value;
    mmm_comparison_t comparison;
    uint32_t action;
    uint32_t cooldown_ms;
    bool enabled;
    char name[64];
    char description[128];
    void (*callback)(uint32_t alert_id, double value);
} mmm_alert_config_t;

// Telemetry data
typedef struct {
    uint32_t data_points;
    uint32_t buffer_size;
    struct {
        mmm_metric_type_t type;
        double value;
        struct timespec timestamp;
        char source[32];
    } *metrics;
    struct timespec collection_start;
    struct timespec collection_end;
} mmm_telemetry_data_t;

// Monitoring dashboard data
typedef struct {
    mmm_system_metrics_t current_system;
    mmm_performance_metrics_t current_performance;
    uint32_t active_alerts;
    uint32_t total_alerts_today;
    double system_health_score;
    double performance_score;
    uint64_t uptime_seconds;
    struct timespec last_update;
} mmm_monitoring_dashboard_t;

// Function declarations

/**
 * Initialize monitoring system
 * @param config Monitoring configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_monitoring_init(mmm_monitoring_config_t *config);

/**
 * Start monitoring
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_monitoring_start(void);

/**
 * Stop monitoring
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_monitoring_stop(void);

/**
 * Collect system metrics
 * @param metrics Output system metrics
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_collect_system_metrics(mmm_system_metrics_t *metrics);

/**
 * Collect performance metrics
 * @param metrics Output performance metrics
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_collect_performance_metrics(mmm_performance_metrics_t *metrics);

/**
 * Configure alert
 * @param config Alert configuration
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_configure_alert(mmm_alert_config_t *config);

/**
 * Remove alert
 * @param alert_id Alert ID to remove
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_remove_alert(uint32_t alert_id);

/**
 * Get active alerts
 * @param alerts Output array of active alerts
 * @param max_alerts Maximum number of alerts to return
 * @return Number of active alerts, negative on error
 */
int mmm_get_active_alerts(mmm_alert_config_t *alerts, uint32_t max_alerts);

/**
 * Export telemetry data
 * @param telemetry Output telemetry data
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_export_telemetry_data(mmm_telemetry_data_t *telemetry);

/**
 * Get monitoring dashboard
 * @param dashboard Output dashboard data
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_get_monitoring_dashboard(mmm_monitoring_dashboard_t *dashboard);

/**
 * Record custom metric
 * @param name Metric name
 * @param value Metric value
 * @param timestamp Metric timestamp (NULL for current time)
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_record_custom_metric(const char *name, double value, struct timespec *timestamp);

/**
 * Get metric history
 * @param metric_type Type of metric
 * @param start_time Start time for history
 * @param end_time End time for history
 * @param values Output array of values
 * @param max_values Maximum number of values to return
 * @return Number of values returned, negative on error
 */
int mmm_get_metric_history(mmm_metric_type_t metric_type, struct timespec *start_time,
                          struct timespec *end_time, double *values, uint32_t max_values);

/**
 * Calculate system health score
 * @param score Output health score (0-100)
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_calculate_health_score(double *score);

/**
 * Cleanup monitoring system
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_monitoring_cleanup(void);

#endif /* MMM_MONITORING_H */
