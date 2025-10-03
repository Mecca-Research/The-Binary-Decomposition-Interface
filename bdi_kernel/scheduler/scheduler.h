
// ===================================================================
// DESC: Enhanced Scheduler with C23 features, multi-level scheduling,
//       and fairness algorithms
// Phase 9: Scheduler Integration & Fairness
// ===================================================================
#ifndef AEON_SCHEDULER_H
#define AEON_SCHEDULER_H

#include "../kernel/c23_compat.h"
#include "../kernel/graph.h"
#include "../device/device.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* Forward declaration for process integration */
typedef uint64_t ProcessId;

/* ===================================================================
 * Scheduling Constants
 * =================================================================== */

/* Time slice constants (nanoseconds) */
#define SCHED_LATENCY_NS        6000000ULL      /* 6ms target latency */
#define SCHED_MIN_GRANULARITY   750000ULL       /* 0.75ms minimum slice */
#define SCHED_WAKEUP_GRAN       1000000ULL      /* 1ms wakeup granularity */

/* Priority constants */
#define SCHED_PRIO_MIN          -20             /* Highest priority (nice) */
#define SCHED_PRIO_MAX          19              /* Lowest priority (nice) */
#define SCHED_PRIO_DEFAULT      0               /* Default priority */
#define SCHED_RT_PRIO_MAX       99              /* Max RT priority */

/* Queue sizes */
#define SCHED_MAX_READY_NODES   4096            /* Max ready nodes */
#define SCHED_MAX_RT_PRIO       100             /* RT priority levels */
#define SCHED_MAX_DEVICES       16              /* Max devices */

/* Validation using _Static_assert */
_Static_assert(SCHED_MIN_GRANULARITY < SCHED_LATENCY_NS, 
               "Min granularity must be less than target latency");
_Static_assert(SCHED_MAX_RT_PRIO == 100, 
               "RT priority levels must be 100");
_Static_assert((SCHED_MAX_READY_NODES & (SCHED_MAX_READY_NODES - 1)) == 0,
               "Ready queue size must be power of 2");

/* ===================================================================
 * Scheduling Policy Enumerations
 * =================================================================== */

/**
 * @brief Scheduling policies
 */
typedef enum {
    SCHED_NORMAL = 0,       /* CFS - Completely Fair Scheduler */
    SCHED_FIFO = 1,         /* Real-time FIFO */
    SCHED_RR = 2,           /* Real-time Round-Robin */
    SCHED_BATCH = 3,        /* Batch processing */
    SCHED_IDLE = 4,         /* Idle tasks */
    SCHED_DEADLINE = 5      /* Deadline scheduling */
} SchedPolicy;

/**
 * @brief Scheduler states (atomic)
 */
typedef enum {
    SCHED_STATE_STOPPED = 0,
    SCHED_STATE_RUNNING = 1,
    SCHED_STATE_PAUSED = 2
} SchedState;

/* ===================================================================
 * Security Policy (from original)
 * =================================================================== */

/**
 * @brief Security Policy for the Scheduler
 */
typedef struct {
    bool secure_mode;               /* If true, all nodes must pass proof gate */
    uint32_t required_proof_class;  /* Minimum proof class required */
} SecurityPolicy;

/* ===================================================================
 * Scheduling Statistics
 * =================================================================== */

/**
 * @brief Per-scheduler statistics
 */
typedef struct {
    _Atomic uint64_t total_scheduled;       /* Total nodes scheduled */
    _Atomic uint64_t total_preemptions;     /* Total preemptions */
    _Atomic uint64_t total_migrations;      /* Total task migrations */
    _Atomic uint64_t total_context_switches;/* Total context switches */
    _Atomic uint64_t cfs_scheduled;         /* CFS tasks scheduled */
    _Atomic uint64_t rt_scheduled;          /* RT tasks scheduled */
    _Atomic uint64_t dl_scheduled;          /* Deadline tasks scheduled */
    uint64_t last_balance_time;             /* Last load balance time */
} SchedStatistics;

/* ===================================================================
 * Scheduler Structure (Enhanced)
 * =================================================================== */

/**
 * @brief Main Scheduler Structure
 * 
 * Manages multi-level scheduling with CFS, RT, and Deadline policies.
 * Uses atomic operations for lock-free state management.
 */
typedef struct Scheduler {
    /* Core components */
    BdiGraph* graph;                        /* BDI graph */
    DeviceVTable** devices;                 /* Device backends */
    size_t device_count;                    /* Number of devices */
    
    /* Security */
    SecurityPolicy policy;                  /* Security policy */
    
    /* Scheduler state (atomic) */
    _Atomic SchedState state;               /* Current state */
    _Atomic uint64_t tick_count;            /* Scheduler ticks */
    
    /* Ready queues (lock-free) */
    NodeId* ready_set;                      /* Dynamic array of ready nodes */
    _Atomic size_t ready_count;             /* Number of ready nodes */
    size_t ready_capacity;                  /* Capacity of ready set */
    
    /* Multi-level scheduling */
    void* cfs_scheduler;                    /* CFS scheduler instance */
    void* rt_scheduler;                     /* RT scheduler instance */
    void* dl_scheduler;                     /* Deadline scheduler instance */
    void* device_scheduler;                 /* Device scheduler instance */
    
    /* Time management */
    uint64_t (*get_time_ns)(void);          /* Time function */
    uint64_t last_tick_time;                /* Last tick timestamp */
    
    /* Statistics */
    SchedStatistics stats;                  /* Scheduler statistics */
    
    /* Configuration */
    uint64_t sched_latency;                 /* Target scheduling latency */
    uint64_t min_granularity;               /* Minimum time slice */
    uint32_t balance_interval_ms;           /* Load balance interval */
    
    bool initialized;                       /* Initialization flag */
} Scheduler;

/* ===================================================================
 * Scheduler API (with [[nodiscard]] annotations)
 * =================================================================== */

/**
 * @brief Create a new scheduler
 * @param g BDI graph
 * @param devices Array of device backends
 * @param dev_count Number of devices
 * @return Scheduler instance or nullptr on failure
 */
[[nodiscard]]
Scheduler* aeon_scheduler_create(BdiGraph* g, DeviceVTable** devices, size_t dev_count);

/**
 * @brief Free scheduler resources
 * @param sched Scheduler instance
 */
void aeon_scheduler_free(Scheduler* sched);

/**
 * @brief Initialize scheduler with fairness algorithms
 * @param sched Scheduler instance
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_init(Scheduler* sched);

/**
 * @brief Shutdown scheduler
 * @param sched Scheduler instance
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_shutdown(Scheduler* sched);

/**
 * @brief Set security policy
 * @param sched Scheduler instance
 * @param policy Security policy
 */
void aeon_scheduler_set_policy(Scheduler* sched, SecurityPolicy policy);

/**
 * @brief Run scheduler for a single wave (original API)
 * @param sched Scheduler instance
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_run_wave(Scheduler* sched);

/**
 * @brief Main scheduling loop (new multi-level scheduler)
 * @param sched Scheduler instance
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_schedule(Scheduler* sched);

/**
 * @brief Add a node to the scheduler
 * @param sched Scheduler instance
 * @param node_id Node ID
 * @param policy Scheduling policy
 * @param priority Priority (for RT/CFS)
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_add_node(Scheduler* sched, NodeId node_id, 
                            SchedPolicy policy, int32_t priority);

/**
 * @brief Remove a node from the scheduler
 * @param sched Scheduler instance
 * @param node_id Node ID
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_remove_node(Scheduler* sched, NodeId node_id);

/**
 * @brief Preempt current task on a device
 * @param sched Scheduler instance
 * @param device_id Device ID
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_preempt(Scheduler* sched, DeviceId device_id);

/**
 * @brief Perform load balancing across devices
 * @param sched Scheduler instance
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_balance_load(Scheduler* sched);

/**
 * @brief Scheduler tick handler (called periodically)
 * @param sched Scheduler instance
 */
void aeon_scheduler_tick(Scheduler* sched);

/**
 * @brief Get scheduler statistics
 * @param sched Scheduler instance
 * @param stats Output statistics structure
 * @return 0 on success, negative on error
 */
[[nodiscard]]
int aeon_scheduler_get_stats(Scheduler* sched, SchedStatistics* stats);

/**
 * @brief Print scheduler statistics
 * @param sched Scheduler instance
 */
void aeon_scheduler_print_stats(Scheduler* sched);

/**
 * @brief Set scheduler state atomically
 * @param sched Scheduler instance
 * @param new_state New state
 * @return Previous state
 */
SchedState aeon_scheduler_set_state(Scheduler* sched, SchedState new_state);

/**
 * @brief Get scheduler state atomically
 * @param sched Scheduler instance
 * @return Current state
 */
SchedState aeon_scheduler_get_state(Scheduler* sched);

/* ===================================================================
 * Utility Functions
 * =================================================================== */

/**
 * @brief Default time function (nanoseconds)
 * @return Current time in nanoseconds
 */
uint64_t aeon_scheduler_default_time_ns(void);

/**
 * @brief Convert priority to weight
 * @param prio Priority value
 * @return Weight value
 */
uint32_t aeon_scheduler_prio_to_weight(int32_t prio);

/**
 * @brief Convert priority to inverse weight multiplier
 * @param prio Priority value
 * @return Inverse weight multiplier
 */
uint32_t aeon_scheduler_prio_to_wmult(int32_t prio);

#endif // AEON_SCHEDULER_H
