
/**
 * @file scheduler.c
 * @brief Core scheduler implementation with C23 atomics
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 * 
 * This file implements the core scheduler using C23 atomic operations for
 * lock-free concurrency. The scheduler manages task execution across all CPUs
 * with minimal overhead and optimal cache locality.
 */

#include "scheduler.h"
#include "task.h"
#include "smp.h"
#include "ipi.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

/* Global scheduler state */
struct scheduler_state g_scheduler = {0};

/* Per-CPU scheduler data */
struct cpu_scheduler g_cpu_schedulers[SCHEDULER_MAX_CPUS] = {0};

/* Forward declarations */
static void context_switch(struct task *prev, struct task *next);
static struct task *select_next_task(void);
static void update_time_slice(void);

/**
 * @brief Initialize the scheduler
 */
void scheduler_init(void) {
    /* Initialize global scheduler state with C23 atomics */
    atomic_init(&g_scheduler.current_task_id, 0);
    atomic_init(&g_scheduler.num_running_tasks, 0);
    atomic_init(&g_scheduler.flags, SCHED_FLAG_PREEMPT);
    atomic_init(&g_scheduler.total_context_switches, 0);
    atomic_init(&g_scheduler.tick_count, 0);
    atomic_init(&g_scheduler.next_wakeup_time, 0);
    
    /* Set default parameters */
    g_scheduler.time_slice_ticks = TIME_SLICE_MS;
    g_scheduler.min_tick_interval_us = MIN_TICK_INTERVAL_US;
    g_scheduler.max_tick_interval_us = MAX_TICK_INTERVAL_US;
    
    /* Initialize per-CPU scheduler data */
    uint32_t num_cpus = get_num_cpus();
    for (uint32_t i = 0; i < num_cpus; i++) {
        struct cpu_scheduler *cpu_sched = &g_cpu_schedulers[i];
        
        cpu_sched->current_task = NULL;
        cpu_sched->idle_task = NULL;  /* Created later */
        cpu_sched->cpu_id = i;
        cpu_sched->numa_node = get_cpu_numa_node(i);
        cpu_sched->time_slice_remaining = g_scheduler.time_slice_ticks;
        
        atomic_init(&cpu_sched->cpu_flags, 0);
        atomic_init(&cpu_sched->context_switches, 0);
        atomic_init(&cpu_sched->idle_time, 0);
    }
    
    printf("[Scheduler] Initialized with %u CPUs\n", num_cpus);
}

/**
 * @brief Main scheduling function
 */
void schedule(void) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    struct task *prev = cpu_sched->current_task;
    struct task *next = select_next_task();
    
    /* If no task selected, run idle task */
    if (next == NULL) {
        next = cpu_sched->idle_task;
        if (next == NULL) {
            /* No idle task yet, just return */
            return;
        }
    }
    
    /* If same task, no context switch needed */
    if (prev == next) {
        return;
    }
    
    /* Perform context switch */
    context_switch(prev, next);
    
    /* Update statistics (relaxed ordering for counters) */
    atomic_fetch_add_explicit(&g_scheduler.total_context_switches, 1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&cpu_sched->context_switches, 1,
                              memory_order_relaxed);
}

/**
 * @brief Select next task to run
 * 
 * Uses lock-free operations to access run queue.
 * 
 * @return Next task to run, or NULL if no tasks available
 */
static struct task *select_next_task(void) {
    struct cpu_runqueue *rq = get_current_runqueue();
    
    /* Try to dequeue from local run queue (lock-free) */
    struct task *task = runqueue_dequeue(rq);
    if (task != NULL) {
        return task;
    }
    
    /* Local queue empty, try work stealing */
    task = steal_task();
    if (task != NULL) {
        return task;
    }
    
    /* No tasks available */
    return NULL;
}

/**
 * @brief Perform context switch
 * 
 * @param prev Previous task
 * @param next Next task
 */
static void context_switch(struct task *prev, struct task *next) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    
    /* Update current task pointer with release semantics */
    cpu_sched->current_task = next;
    
    /* Update task states atomically */
    if (prev != NULL && prev != cpu_sched->idle_task) {
        /* Previous task goes back to ready state */
        task_set_state(prev, TASK_RUNNING, TASK_READY);
    }
    
    if (next != NULL && next != cpu_sched->idle_task) {
        /* Next task enters running state */
        task_set_state(next, TASK_READY, TASK_RUNNING);
    }
    
    /* Reset time slice */
    cpu_sched->time_slice_remaining = g_scheduler.time_slice_ticks;
    
    /* TODO: Actual context switch (save/restore registers, stack pointer, etc.) */
    /* This would involve assembly code to save/restore CPU state */
    /* For now, we just update the task pointers */
}

/**
 * @brief Scheduler tick handler
 */
void scheduler_tick(void) {
    /* Update tick count (relaxed ordering) */
    atomic_fetch_add_explicit(&g_scheduler.tick_count, 1,
                              memory_order_relaxed);
    
    /* Update time slice */
    update_time_slice();
    
    /* Wake up expired sleeping tasks */
    uint64_t current_time = atomic_load_explicit(&g_scheduler.tick_count,
                                                 memory_order_relaxed);
    scheduler_wakeup_expired(current_time);
    
    /* Check if reschedule needed */
    if (scheduler_need_resched()) {
        schedule();
    }
}

/**
 * @brief Update time slice for current task
 */
static void update_time_slice(void) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    
    if (cpu_sched->time_slice_remaining > 0) {
        cpu_sched->time_slice_remaining--;
    }
}

/**
 * @brief Get current running task
 */
struct task *get_current_task(void) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    return cpu_sched->current_task;
}

/**
 * @brief Get number of running tasks
 */
uint32_t get_num_running_tasks(void) {
    return atomic_load_explicit(&g_scheduler.num_running_tasks,
                               memory_order_acquire);
}

/**
 * @brief Get scheduler statistics
 */
void get_scheduler_stats(struct scheduler_stats *stats) {
    if (stats == NULL) {
        return;
    }
    
    stats->total_context_switches = atomic_load_explicit(
        &g_scheduler.total_context_switches, memory_order_relaxed);
    stats->total_ticks = atomic_load_explicit(
        &g_scheduler.tick_count, memory_order_relaxed);
    stats->num_running_tasks = atomic_load_explicit(
        &g_scheduler.num_running_tasks, memory_order_acquire);
    
    /* TODO: Count tasks in different states */
    stats->num_tasks = stats->num_running_tasks;
    stats->num_blocked_tasks = 0;
    stats->num_sleeping_tasks = 0;
}

/**
 * @brief Enable/disable tickless mode
 */
void scheduler_set_tickless(bool enable) {
    uint32_t flags = atomic_load_explicit(&g_scheduler.flags,
                                         memory_order_acquire);
    
    if (enable) {
        flags |= SCHED_FLAG_TICKLESS;
    } else {
        flags &= ~SCHED_FLAG_TICKLESS;
    }
    
    atomic_store_explicit(&g_scheduler.flags, flags,
                         memory_order_release);
}

/**
 * @brief Set minimum tick interval
 */
void scheduler_set_min_tick_interval(uint32_t interval_us) {
    g_scheduler.min_tick_interval_us = interval_us;
}

/**
 * @brief Set maximum tick interval
 */
void scheduler_set_max_tick_interval(uint32_t interval_us) {
    g_scheduler.max_tick_interval_us = interval_us;
}

/**
 * @brief Set time slice duration
 */
void scheduler_set_time_slice(uint32_t time_slice_ms) {
    g_scheduler.time_slice_ticks = time_slice_ms;
}

/**
 * @brief Check if preemption is needed
 */
bool scheduler_need_resched(void) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    
    /* Check if time slice expired */
    if (cpu_sched->time_slice_remaining == 0) {
        return true;
    }
    
    /* Check if higher priority task is ready */
    struct cpu_runqueue *rq = get_current_runqueue();
    if (!runqueue_is_empty(rq)) {
        struct task *next = runqueue_peek(rq);
        struct task *current = cpu_sched->current_task;
        
        if (next != NULL && current != NULL) {
            /* TODO: Compare priorities */
            /* For now, always reschedule if tasks are waiting */
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Force reschedule on current CPU
 */
void scheduler_resched(void) {
    schedule();
}

/**
 * @brief Get per-CPU scheduler data
 */
struct cpu_scheduler *get_cpu_scheduler(uint32_t cpu_id) {
    if (cpu_id >= SCHEDULER_MAX_CPUS) {
        return NULL;
    }
    return &g_cpu_schedulers[cpu_id];
}

/**
 * @brief Get current CPU's scheduler data
 */
struct cpu_scheduler *get_current_cpu_scheduler(void) {
    uint32_t cpu_id = get_current_cpu_id();
    return get_cpu_scheduler(cpu_id);
}

/**
 * @brief Idle loop for CPU with no tasks
 */
void scheduler_idle(void) {
    struct cpu_scheduler *cpu_sched = get_current_cpu_scheduler();
    
    /* Set idle flag */
    uint32_t flags = atomic_load_explicit(&cpu_sched->cpu_flags,
                                         memory_order_acquire);
    flags |= SCHED_FLAG_IDLE;
    atomic_store_explicit(&cpu_sched->cpu_flags, flags,
                         memory_order_release);
    
    /* Enter low-power state */
    /* TODO: Use CPU-specific halt instruction */
    /* For now, just busy-wait */
    while (runqueue_is_empty(get_current_runqueue())) {
        /* Wait for interrupt or task */
        __asm__ volatile("pause");
    }
    
    /* Clear idle flag */
    flags = atomic_load_explicit(&cpu_sched->cpu_flags,
                                memory_order_acquire);
    flags &= ~SCHED_FLAG_IDLE;
    atomic_store_explicit(&cpu_sched->cpu_flags, flags,
                         memory_order_release);
}

/**
 * @brief Calculate next wakeup time for tickless mode
 */
uint64_t scheduler_next_wakeup(void) {
    /* TODO: Find earliest sleeping task wakeup time */
    /* For now, return 0 (no sleeping tasks) */
    return 0;
}

/**
 * @brief Wake up sleeping tasks whose time has expired
 */
uint32_t scheduler_wakeup_expired(uint64_t current_time) {
    /* TODO: Iterate through sleeping tasks and wake up expired ones */
    /* For now, return 0 (no tasks woken) */
    (void)current_time;
    return 0;
}


