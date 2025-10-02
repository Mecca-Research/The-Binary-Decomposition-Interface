
/**
 * @file scheduler.h
 * @brief Core scheduler with C23 atomics and lock-free operations
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 * 
 * This header defines the core scheduler interface using C23 atomic operations
 * for lock-free concurrency. The scheduler manages task execution across all
 * CPUs with minimal overhead and optimal cache locality.
 * 
 * Key Features:
 * - C23 _Atomic types for scheduler state
 * - Lock-free state transitions
 * - Tickless scheduling for power efficiency
 * - Integration with per-CPU run queues
 * - Explicit memory ordering for correctness
 */

#ifndef BDI_SCHEDULER_H
#define BDI_SCHEDULER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"
#include "task.h"

/* Scheduler constants */
#define TIME_SLICE_MS           10      /* Default time slice in milliseconds */
#define MIN_TICK_INTERVAL_US    1000    /* Minimum tick interval (1ms) */
#define MAX_TICK_INTERVAL_US    100000  /* Maximum tick interval (100ms) */
#define SCHEDULER_MAX_CPUS      256     /* Maximum number of CPUs */

/* Scheduler flags */
#define SCHED_FLAG_TICKLESS     (1U << 0)  /* Tickless mode enabled */
#define SCHED_FLAG_PREEMPT      (1U << 1)  /* Preemption enabled */
#define SCHED_FLAG_IDLE         (1U << 2)  /* Scheduler is idle */
#define SCHED_FLAG_STOPPING     (1U << 3)  /* Scheduler is stopping */

/**
 * @brief Global scheduler state
 * 
 * Uses C23 atomic types for lock-free access from multiple CPUs.
 * Memory ordering is explicit for correctness and performance.
 */
struct scheduler_state {
    /* Current task ID (atomic) */
    _Atomic uint64_t current_task_id;
    
    /* Number of running tasks (atomic) */
    _Atomic uint32_t num_running_tasks;
    
    /* Scheduler flags (atomic) */
    _Atomic uint32_t flags;
    
    /* Total context switches (atomic, relaxed ordering) */
    _Atomic uint64_t total_context_switches;
    
    /* Tick count (atomic, relaxed ordering) */
    _Atomic uint64_t tick_count;
    
    /* Next wakeup time for tickless mode (atomic) */
    _Atomic uint64_t next_wakeup_time;
    
    /* Time slice in ticks */
    uint32_t time_slice_ticks;
    
    /* Tickless mode parameters */
    uint32_t min_tick_interval_us;
    uint32_t max_tick_interval_us;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(_Atomic uint64_t) * 4 + 
                          sizeof(_Atomic uint32_t) * 2 + 
                          sizeof(uint32_t) * 3) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Per-CPU scheduler data
 * 
 * Each CPU has its own scheduler data to minimize cache contention.
 * Aligned to cache line boundary to prevent false sharing.
 */
struct cpu_scheduler {
    /* Current running task on this CPU */
    struct task *current_task;
    
    /* Idle task for this CPU */
    struct task *idle_task;
    
    /* CPU ID */
    uint32_t cpu_id;
    
    /* NUMA node ID */
    uint32_t numa_node;
    
    /* Current time slice remaining (ticks) */
    uint32_t time_slice_remaining;
    
    /* Flags for this CPU scheduler */
    _Atomic uint32_t cpu_flags;
    
    /* Statistics */
    _Atomic uint64_t context_switches;
    _Atomic uint64_t idle_time;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(struct task *) * 2 + 
                          sizeof(uint32_t) * 3 + 
                          sizeof(_Atomic uint32_t) + 
                          sizeof(_Atomic uint64_t) * 2) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Scheduler statistics
 */
struct scheduler_stats {
    uint64_t total_context_switches;
    uint64_t total_ticks;
    uint32_t num_tasks;
    uint32_t num_running_tasks;
    uint32_t num_blocked_tasks;
    uint32_t num_sleeping_tasks;
};

/* Global scheduler state */
extern struct scheduler_state g_scheduler;

/* Per-CPU scheduler data */
extern struct cpu_scheduler g_cpu_schedulers[SCHEDULER_MAX_CPUS];

/**
 * @brief Initialize the scheduler
 * 
 * Called once at boot to initialize global scheduler state and per-CPU data.
 * Sets up tickless timer and configures scheduling parameters.
 */
void scheduler_init(void);

/**
 * @brief Main scheduling function
 * 
 * Selects the next task to run and performs context switch.
 * Uses lock-free operations to access run queues.
 * 
 * Called from:
 * - Timer interrupt (preemptive scheduling)
 * - task_yield() (voluntary scheduling)
 * - task_block() (blocking operations)
 */
void schedule(void);

/**
 * @brief Scheduler tick handler
 * 
 * Called from timer interrupt to update scheduler state.
 * In tickless mode, frequency is dynamically adjusted.
 */
void scheduler_tick(void);

/**
 * @brief Get current running task
 * 
 * @return Pointer to current task, or NULL if no task running
 */
[[nodiscard]] struct task *get_current_task(void);

/**
 * @brief Get number of running tasks
 * 
 * @return Number of tasks in TASK_RUNNING state
 */
[[nodiscard]] uint32_t get_num_running_tasks(void);

/**
 * @brief Get scheduler statistics
 * 
 * @param stats Pointer to stats structure to fill
 */
void get_scheduler_stats(struct scheduler_stats *stats);

/**
 * @brief Enable/disable tickless mode
 * 
 * @param enable true to enable tickless mode, false to disable
 */
void scheduler_set_tickless(bool enable);

/**
 * @brief Set minimum tick interval for tickless mode
 * 
 * @param interval_us Minimum interval in microseconds
 */
void scheduler_set_min_tick_interval(uint32_t interval_us);

/**
 * @brief Set maximum tick interval for tickless mode
 * 
 * @param interval_us Maximum interval in microseconds
 */
void scheduler_set_max_tick_interval(uint32_t interval_us);

/**
 * @brief Set time slice duration
 * 
 * @param time_slice_ms Time slice in milliseconds
 */
void scheduler_set_time_slice(uint32_t time_slice_ms);

/**
 * @brief Check if preemption is needed
 * 
 * @return true if current task should be preempted
 */
[[nodiscard]] bool scheduler_need_resched(void);

/**
 * @brief Force reschedule on current CPU
 */
void scheduler_resched(void);

/**
 * @brief Get per-CPU scheduler data
 * 
 * @param cpu_id CPU ID
 * @return Pointer to per-CPU scheduler data
 */
[[nodiscard]] struct cpu_scheduler *get_cpu_scheduler(uint32_t cpu_id);

/**
 * @brief Get current CPU's scheduler data
 * 
 * @return Pointer to current CPU's scheduler data
 */
[[nodiscard]] struct cpu_scheduler *get_current_cpu_scheduler(void);

/**
 * @brief Idle loop for CPU with no tasks
 * 
 * Called when no tasks are ready to run.
 * Enters low-power state and waits for interrupt.
 */
void scheduler_idle(void);

/**
 * @brief Calculate next wakeup time for tickless mode
 * 
 * @return Next wakeup time in microseconds, or 0 if no sleeping tasks
 */
[[nodiscard]] uint64_t scheduler_next_wakeup(void);

/**
 * @brief Wake up sleeping tasks whose time has expired
 * 
 * @param current_time Current time in microseconds
 * @return Number of tasks woken up
 */
uint32_t scheduler_wakeup_expired(uint64_t current_time);

#endif /* BDI_SCHEDULER_H */
