
/*
 * Scheduler Statistics and Metrics
 */

#ifndef BDI_SCHED_STATS_H
#define BDI_SCHED_STATS_H

#include <stdint.h>
#include <stdbool.h>

// Per-CPU scheduler statistics
typedef struct {
    uint32_t cpu_id;
    uint64_t run_queue_length;
    uint64_t context_switches;
    uint64_t task_migrations;
    uint64_t load_balance_count;
    uint64_t idle_time_ns;
    uint64_t busy_time_ns;
    double cpu_utilization;
} cpu_sched_stats_t;

// Per-task scheduler statistics
typedef struct {
    uint32_t task_id;
    uint64_t total_runtime_ns;
    uint64_t wait_time_ns;
    uint64_t sleep_time_ns;
    uint32_t voluntary_switches;
    uint32_t involuntary_switches;
    uint32_t migrations;
    int32_t priority;
    int32_t nice_value;
} task_sched_stats_t;

// Global scheduler statistics
typedef struct {
    uint64_t total_tasks;
    uint64_t running_tasks;
    uint64_t sleeping_tasks;
    uint64_t blocked_tasks;
    uint64_t total_context_switches;
    uint64_t total_migrations;
    uint64_t work_stealing_attempts;
    uint64_t work_stealing_successes;
    double avg_latency_us;
    double max_latency_us;
} global_sched_stats_t;

// API
void sched_stats_init(void);
void sched_stats_update_cpu(uint32_t cpu_id, cpu_sched_stats_t *stats);
void sched_stats_update_task(uint32_t task_id, task_sched_stats_t *stats);
void sched_stats_record_context_switch(uint32_t cpu_id);
void sched_stats_record_migration(uint32_t from_cpu, uint32_t to_cpu);
void sched_stats_record_work_steal(uint32_t thief_cpu, uint32_t victim_cpu, bool success);

cpu_sched_stats_t sched_stats_get_cpu(uint32_t cpu_id);
task_sched_stats_t sched_stats_get_task(uint32_t task_id);
global_sched_stats_t sched_stats_get_global(void);

void sched_stats_print_report(void);
void sched_stats_export_json(const char *filename);

#endif // BDI_SCHED_STATS_H
