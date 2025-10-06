
/*
 * BDI Kernel Profiling Infrastructure
 * System-wide profiling, metrics collection, and performance monitoring
 */

#ifndef BDI_PROFILER_H
#define BDI_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Profiling categories
typedef enum {
    PROFILE_MEMORY,
    PROFILE_SCHEDULER,
    PROFILE_PROCESS,
    PROFILE_DEVICE,
    PROFILE_SYSCALL,
    PROFILE_INTERRUPT,
    PROFILE_CUSTOM
} profile_category_t;

// Performance counter
typedef struct {
    const char *name;
    uint64_t count;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
} perf_counter_t;

// Profiler configuration
typedef struct {
    bool enabled;
    bool collect_stack_traces;
    bool collect_timestamps;
    uint32_t sampling_rate_hz;
    const char *output_file;
} profiler_config_t;

// Profiler API
int profiler_init(profiler_config_t *config);
void profiler_shutdown(void);
void profiler_enable(void);
void profiler_disable(void);

// Event recording
void profiler_record_event(profile_category_t category, const char *event_name, uint64_t duration_ns);
void profiler_start_event(profile_category_t category, const char *event_name);
void profiler_end_event(profile_category_t category, const char *event_name);

// Counter management
perf_counter_t* profiler_get_counter(const char *name);
void profiler_increment_counter(const char *name);
void profiler_add_to_counter(const char *name, uint64_t value);

// Reporting
void profiler_print_report(void);
void profiler_export_json(const char *filename);
void profiler_export_csv(const char *filename);

// Real-time metrics
typedef struct {
    uint64_t timestamp_ns;
    double cpu_usage_percent;
    uint64_t memory_used_bytes;
    uint64_t memory_free_bytes;
    uint32_t active_processes;
    uint32_t context_switches_per_sec;
    uint32_t interrupts_per_sec;
} system_metrics_t;

system_metrics_t profiler_get_system_metrics(void);

#endif // BDI_PROFILER_H
