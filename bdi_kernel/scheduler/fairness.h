
// ===================================================================
// DESC: Fairness scheduler - Enhanced scheduling with fairness algorithms
// ===================================================================
#ifndef AEON_FAIRNESS_H
#define AEON_FAIRNESS_H

#include "scheduler.h"
#include <stdint.h>
#include <stdbool.h>

// --- Latency Classes ---
typedef enum {
    LATENCY_CLASS_REALTIME = 0,     // Hard real-time, highest priority
    LATENCY_CLASS_INTERACTIVE = 1,  // Interactive tasks, low latency
    LATENCY_CLASS_NORMAL = 2,       // Normal batch processing
    LATENCY_CLASS_BACKGROUND = 3,   // Background tasks, lowest priority
    LATENCY_CLASS_COUNT = 4
} latency_class_t;

// --- Scheduling Policy ---
typedef enum {
    SCHED_POLICY_CFS = 0,           // Completely Fair Scheduler
    SCHED_POLICY_RT = 1,            // Real-time scheduler
    SCHED_POLICY_DEADLINE = 2,      // Deadline scheduler
    SCHED_POLICY_BATCH = 3          // Batch scheduler
} sched_policy_t;

// --- Node Scheduling Information ---
typedef struct {
    NodeId node_id;
    latency_class_t latency_class;
    sched_policy_t policy;
    
    // Timing information
    uint64_t arrival_time;          // When node became ready
    uint64_t deadline;              // Deadline for completion (if applicable)
    uint64_t period;                // Period for periodic tasks
    uint64_t runtime;               // Actual runtime so far
    uint64_t vruntime;              // Virtual runtime (for CFS)
    
    // Priority and weights
    int32_t priority;               // Static priority (-20 to +19)
    uint32_t weight;                // Scheduling weight
    uint32_t nice_value;            // Nice value (0-39)
    
    // Device affinity
    uint32_t device_mask;           // Bitmask of allowed devices
    DeviceId preferred_device;      // Preferred device for execution
    
    // Statistics
    uint64_t total_runtime;         // Total runtime across all executions
    uint64_t wait_time;             // Total time spent waiting
    uint32_t execution_count;       // Number of times executed
    
    // State
    bool is_running;                // Currently executing
    bool is_preemptible;            // Can be preempted
    DeviceId assigned_device;       // Device currently assigned to
} node_sched_info_t;

// --- Per-Device Run Queue ---
typedef struct {
    DeviceId device_id;
    
    // CFS red-black tree (simplified as array for now)
    node_sched_info_t* cfs_queue[256];
    uint32_t cfs_count;
    uint64_t min_vruntime;          // Minimum vruntime in CFS queue
    
    // Real-time queues (one per priority level)
    node_sched_info_t* rt_queues[100]; // Priority 0-99
    uint32_t rt_counts[100];
    uint32_t rt_active_bitmap;      // Bitmap of active RT priority levels
    
    // Deadline queue (sorted by deadline)
    node_sched_info_t* dl_queue[64];
    uint32_t dl_count;
    
    // Current running node
    node_sched_info_t* current;
    uint64_t current_start_time;
    
    // Load balancing
    uint32_t load_weight;           // Total weight of runnable tasks
    uint32_t nr_running;            // Number of runnable tasks
    
    // Statistics
    uint64_t total_runtime;
    uint64_t idle_time;
    uint32_t context_switches;
} device_runqueue_t;

// --- Fair Scheduler ---
typedef struct {
    Scheduler* base_scheduler;      // Base scheduler
    
    // Per-device run queues
    device_runqueue_t device_queues[16]; // Support up to 16 devices
    uint32_t num_devices;
    
    // Global scheduling parameters
    uint64_t sched_latency;         // Target scheduling latency (ns)
    uint64_t min_granularity;       // Minimum time slice (ns)
    uint64_t wakeup_granularity;    // Wakeup granularity (ns)
    
    // Load balancing
    uint64_t last_balance_time;
    uint32_t balance_interval;      // Load balance interval (ms)
    
    // Time keeping
    uint64_t (*get_time_ns)(void);  // Function to get current time in nanoseconds
    
    // Statistics
    uint64_t total_nodes_scheduled;
    uint64_t total_context_switches;
    uint64_t total_migrations;
    
    bool initialized;
} fair_scheduler_t;

// --- Function Declarations ---

// Scheduler management
int fair_scheduler_init(fair_scheduler_t* fs, Scheduler* base_scheduler);
int fair_scheduler_shutdown(fair_scheduler_t* fs);

// Node management
int fair_scheduler_add_node(fair_scheduler_t* fs, NodeId node_id, latency_class_t latency_class);
int fair_scheduler_remove_node(fair_scheduler_t* fs, NodeId node_id);
int fair_scheduler_update_node(fair_scheduler_t* fs, NodeId node_id, node_sched_info_t* info);

// Scheduling operations
int fair_scheduler_schedule(fair_scheduler_t* fs);
node_sched_info_t* fair_scheduler_pick_next_task(fair_scheduler_t* fs, DeviceId device_id);
int fair_scheduler_preempt_current(fair_scheduler_t* fs, DeviceId device_id);

// Load balancing
int fair_scheduler_balance_load(fair_scheduler_t* fs);
int fair_scheduler_migrate_task(fair_scheduler_t* fs, node_sched_info_t* task, DeviceId from_device, DeviceId to_device);

// CFS operations
int cfs_enqueue_task(device_runqueue_t* rq, node_sched_info_t* task);
int cfs_dequeue_task(device_runqueue_t* rq, node_sched_info_t* task);
node_sched_info_t* cfs_pick_next_task(device_runqueue_t* rq);
uint64_t cfs_calc_delta_fair(uint64_t delta, node_sched_info_t* task);

// Real-time operations
int rt_enqueue_task(device_runqueue_t* rq, node_sched_info_t* task);
int rt_dequeue_task(device_runqueue_t* rq, node_sched_info_t* task);
node_sched_info_t* rt_pick_next_task(device_runqueue_t* rq);

// Deadline operations
int dl_enqueue_task(device_runqueue_t* rq, node_sched_info_t* task);
int dl_dequeue_task(device_runqueue_t* rq, node_sched_info_t* task);
node_sched_info_t* dl_pick_next_task(device_runqueue_t* rq);

// Utility functions
uint32_t sched_prio_to_weight(int32_t prio);
uint32_t sched_prio_to_wmult(int32_t prio);
uint64_t sched_slice(device_runqueue_t* rq, node_sched_info_t* task);
bool sched_should_preempt(node_sched_info_t* current, node_sched_info_t* candidate);

// Statistics and monitoring
void fair_scheduler_print_stats(fair_scheduler_t* fs);
void fair_scheduler_print_runqueue(fair_scheduler_t* fs, DeviceId device_id);

// Default time function (can be overridden)
uint64_t default_get_time_ns(void);


/* ===================================================================
 * CFS Scheduler API
 * =================================================================== */
void* fair_scheduler_create_cfs(Scheduler* base);
void fair_scheduler_destroy_cfs(void* cfs_ptr);
NodeId fair_scheduler_pick_next_cfs(void* cfs_ptr);
void fair_scheduler_tick_cfs(void* cfs_ptr, uint64_t current_time);
int fair_scheduler_enqueue_cfs(void* cfs_ptr, NodeId node_id, int nice);
int fair_scheduler_dequeue_cfs(void* cfs_ptr, NodeId node_id);

/* ===================================================================
 * RT Scheduler API
 * =================================================================== */
void* fair_scheduler_create_rt(Scheduler* base);
void fair_scheduler_destroy_rt(void* rt_ptr);
NodeId fair_scheduler_pick_next_rt(void* rt_ptr);
void fair_scheduler_tick_rt(void* rt_ptr, uint64_t current_time);
int fair_scheduler_enqueue_rt(void* rt_ptr, NodeId node_id, int priority, int policy);
int fair_scheduler_dequeue_rt(void* rt_ptr, NodeId node_id);

/* ===================================================================
 * Deadline Scheduler API
 * =================================================================== */
void* fair_scheduler_create_deadline(Scheduler* base);
void fair_scheduler_destroy_deadline(void* dl_ptr);
NodeId fair_scheduler_pick_next_deadline(void* dl_ptr);
void fair_scheduler_tick_deadline(void* dl_ptr, uint64_t current_time);
int fair_scheduler_enqueue_deadline(void* dl_ptr, NodeId node_id, 
                                    uint64_t runtime, uint64_t deadline, uint64_t period);
int fair_scheduler_dequeue_deadline(void* dl_ptr, NodeId node_id);
#endif // AEON_FAIRNESS_H
