
/**
 * @file smp.h
 * @brief SMP support with lock-free per-CPU run queues
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 * 
 * This header defines SMP (Symmetric Multi-Processing) support including
 * lock-free per-CPU run queues and work stealing for load balancing.
 * 
 * Key Features:
 * - Lock-free per-CPU run queues
 * - Work stealing for load balancing
 * - CPU affinity and NUMA awareness
 * - Integration with Phase 2 NUMA allocator
 */

#ifndef BDI_SMP_H
#define BDI_SMP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"
#include "task.h"

/* SMP constants */
#define SMP_MAX_CPUS            256     /* Maximum number of CPUs */
#define RUNQUEUE_SIZE           256     /* Run queue size (power of 2) */
#define RUNQUEUE_MASK           (RUNQUEUE_SIZE - 1)
#define STEAL_THRESHOLD         4       /* Min tasks before stealing */
#define STEAL_COUNT             2       /* Number of tasks to steal */

/**
 * @brief Per-CPU run queue (lock-free circular buffer)
 * 
 * Uses lock-free operations for enqueue/dequeue.
 * Based on Phase 1's lock-free ring buffer design.
 * 
 * Memory ordering:
 * - head/tail use acquire/release semantics
 * - num_tasks uses relaxed ordering (statistics)
 */
struct cpu_runqueue {
    /* Head index (dequeue position) - atomic */
    _Atomic uint64_t head;
    
    /* Tail index (enqueue position) - atomic */
    _Atomic uint64_t tail;
    
    /* Number of tasks in queue - atomic */
    _Atomic uint32_t num_tasks;
    
    /* Steal lock (0 = unlocked, 1 = locked) - atomic */
    _Atomic uint32_t steal_lock;
    
    /* CPU ID */
    uint32_t cpu_id;
    
    /* NUMA node ID */
    uint32_t numa_node;
    
    /* Task array (circular buffer) */
    struct task *tasks[RUNQUEUE_SIZE];
    
    /* Statistics */
    _Atomic uint64_t enqueue_count;
    _Atomic uint64_t dequeue_count;
    _Atomic uint64_t steal_attempts;
    _Atomic uint64_t steal_successes;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(_Atomic uint64_t) * 2 + 
                          sizeof(_Atomic uint32_t) * 2 + 
                          sizeof(uint32_t) * 2 + 
                          sizeof(struct task *) * RUNQUEUE_SIZE + 
                          sizeof(_Atomic uint64_t) * 4) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief SMP configuration
 */
struct smp_config {
    /* Number of CPUs */
    uint32_t num_cpus;
    
    /* Number of NUMA nodes */
    uint32_t num_numa_nodes;
    
    /* CPU to NUMA node mapping */
    uint32_t cpu_to_numa[SMP_MAX_CPUS];
    
    /* Work stealing parameters */
    uint32_t steal_threshold;
    uint32_t steal_count;
    
    /* Flags */
    uint32_t flags;
};

/* Global SMP configuration */
extern struct smp_config g_smp_config;

/* Per-CPU run queues */
extern struct cpu_runqueue g_cpu_runqueues[SMP_MAX_CPUS];

/**
 * @brief Initialize SMP scheduler
 */
void smp_scheduler_init(void);

/**
 * @brief Get current CPU's run queue
 * 
 * @return Pointer to current CPU's run queue
 */
[[nodiscard]] struct cpu_runqueue *get_current_runqueue(void);

/**
 * @brief Get run queue for specific CPU
 * 
 * @param cpu_id CPU ID
 * @return Pointer to CPU's run queue, or NULL if invalid CPU ID
 */
[[nodiscard]] struct cpu_runqueue *get_cpu_runqueue(uint32_t cpu_id);

/**
 * @brief Enqueue task to run queue (lock-free)
 * 
 * Uses atomic operations to safely enqueue task.
 * 
 * @param rq Run queue
 * @param task Task to enqueue
 * @return 0 on success, -1 on failure (queue full)
 */
int runqueue_enqueue(struct cpu_runqueue *rq, struct task *task);

/**
 * @brief Dequeue task from run queue (lock-free)
 * 
 * Uses atomic operations to safely dequeue task.
 * 
 * @param rq Run queue
 * @return Pointer to dequeued task, or NULL if queue empty
 */
[[nodiscard]] struct task *runqueue_dequeue(struct cpu_runqueue *rq);

/**
 * @brief Peek at next task without dequeuing
 * 
 * @param rq Run queue
 * @return Pointer to next task, or NULL if queue empty
 */
[[nodiscard]] struct task *runqueue_peek(struct cpu_runqueue *rq);

/**
 * @brief Check if run queue is empty
 * 
 * @param rq Run queue
 * @return true if empty, false otherwise
 */
[[nodiscard]] bool runqueue_is_empty(struct cpu_runqueue *rq);

/**
 * @brief Get number of tasks in run queue
 * 
 * @param rq Run queue
 * @return Number of tasks
 */
[[nodiscard]] uint32_t runqueue_size(struct cpu_runqueue *rq);

/**
 * @brief Steal task from another CPU's run queue
 * 
 * Attempts to steal tasks from other CPUs for load balancing.
 * Prefers stealing from same NUMA node.
 * 
 * @return Pointer to stolen task, or NULL if no tasks available
 */
[[nodiscard]] struct task *steal_task(void);

/**
 * @brief Steal task from specific CPU
 * 
 * @param cpu_id CPU ID to steal from
 * @return Pointer to stolen task, or NULL if failed
 */
[[nodiscard]] struct task *steal_task_from_cpu(uint32_t cpu_id);

/**
 * @brief Set work stealing threshold
 * 
 * @param threshold Minimum number of tasks before stealing
 */
void set_steal_threshold(uint32_t threshold);

/**
 * @brief Set work stealing count
 * 
 * @param count Number of tasks to steal at once
 */
void set_steal_count(uint32_t count);

/**
 * @brief Get number of CPUs
 * 
 * @return Number of CPUs in system
 */
[[nodiscard]] uint32_t get_num_cpus(void);

/**
 * @brief Get number of NUMA nodes
 * 
 * @return Number of NUMA nodes in system
 */
[[nodiscard]] uint32_t get_num_numa_nodes(void);

/**
 * @brief Get CPU's NUMA node
 * 
 * @param cpu_id CPU ID
 * @return NUMA node ID, or -1 if invalid CPU ID
 */
[[nodiscard]] int get_cpu_numa_node(uint32_t cpu_id);

/**
 * @brief Get current CPU's NUMA node
 * 
 * @return NUMA node ID
 */
[[nodiscard]] uint32_t get_current_numa_node(void);

/**
 * @brief Get current CPU ID
 * 
 * @return Current CPU ID
 */
[[nodiscard]] uint32_t get_current_cpu_id(void);

/**
 * @brief Check if CPU is online
 * 
 * @param cpu_id CPU ID
 * @return true if CPU is online, false otherwise
 */
[[nodiscard]] bool is_cpu_online(uint32_t cpu_id);

/**
 * @brief Get run queue statistics
 * 
 * @param rq Run queue
 * @param enqueue_count Output: number of enqueues
 * @param dequeue_count Output: number of dequeues
 * @param steal_attempts Output: number of steal attempts
 * @param steal_successes Output: number of successful steals
 */
void runqueue_get_stats(struct cpu_runqueue *rq,
                       uint64_t *enqueue_count,
                       uint64_t *dequeue_count,
                       uint64_t *steal_attempts,
                       uint64_t *steal_successes);

/**
 * @brief Balance load across CPUs
 * 
 * Redistributes tasks from overloaded CPUs to idle CPUs.
 */
void smp_balance_load(void);

/**
 * @brief Migrate task to different CPU
 * 
 * @param task Task to migrate
 * @param target_cpu Target CPU ID
 * @return 0 on success, -1 on failure
 */
int smp_migrate_task(struct task *task, uint32_t target_cpu);

#endif /* BDI_SMP_H */
