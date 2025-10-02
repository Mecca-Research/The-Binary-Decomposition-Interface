
/**
 * @file smp.c
 * @brief SMP support with lock-free per-CPU run queues implementation
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 */

#include "smp.h"
#include "scheduler.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

/* Global SMP configuration */
struct smp_config g_smp_config = {0};

/* Per-CPU run queues */
struct cpu_runqueue g_cpu_runqueues[SMP_MAX_CPUS] = {0};

/* Forward declarations */
static bool try_acquire_steal_lock(struct cpu_runqueue *rq);
static void release_steal_lock(struct cpu_runqueue *rq);

/**
 * @brief Initialize SMP scheduler
 */
void smp_scheduler_init(void) {
    /* Initialize SMP configuration */
    g_smp_config.num_cpus = 1;  /* TODO: Detect actual CPU count */
    g_smp_config.num_numa_nodes = 1;  /* TODO: Detect actual NUMA nodes */
    g_smp_config.steal_threshold = STEAL_THRESHOLD;
    g_smp_config.steal_count = STEAL_COUNT;
    g_smp_config.flags = 0;
    
    /* Initialize CPU to NUMA mapping */
    for (uint32_t i = 0; i < SMP_MAX_CPUS; i++) {
        g_smp_config.cpu_to_numa[i] = 0;  /* TODO: Detect actual mapping */
    }
    
    /* Initialize per-CPU run queues */
    for (uint32_t i = 0; i < g_smp_config.num_cpus; i++) {
        struct cpu_runqueue *rq = &g_cpu_runqueues[i];
        
        atomic_init(&rq->head, 0);
        atomic_init(&rq->tail, 0);
        atomic_init(&rq->num_tasks, 0);
        atomic_init(&rq->steal_lock, 0);
        
        rq->cpu_id = i;
        rq->numa_node = g_smp_config.cpu_to_numa[i];
        
        memset(rq->tasks, 0, sizeof(rq->tasks));
        
        atomic_init(&rq->enqueue_count, 0);
        atomic_init(&rq->dequeue_count, 0);
        atomic_init(&rq->steal_attempts, 0);
        atomic_init(&rq->steal_successes, 0);
    }
    
    printf("[SMP] Initialized %u CPUs, %u NUMA nodes\n",
           g_smp_config.num_cpus, g_smp_config.num_numa_nodes);
}

/**
 * @brief Get current CPU's run queue
 */
struct cpu_runqueue *get_current_runqueue(void) {
    uint32_t cpu_id = get_current_cpu_id();
    return get_cpu_runqueue(cpu_id);
}

/**
 * @brief Get run queue for specific CPU
 */
struct cpu_runqueue *get_cpu_runqueue(uint32_t cpu_id) {
    if (cpu_id >= g_smp_config.num_cpus) {
        return NULL;
    }
    return &g_cpu_runqueues[cpu_id];
}

/**
 * @brief Enqueue task to run queue (lock-free)
 */
int runqueue_enqueue(struct cpu_runqueue *rq, struct task *task) {
    if (rq == NULL || task == NULL) {
        return -1;
    }
    
    /* Load tail with acquire semantics */
    uint64_t tail = atomic_load_explicit(&rq->tail, memory_order_acquire);
    uint64_t head = atomic_load_explicit(&rq->head, memory_order_acquire);
    
    /* Check if queue is full */
    if (tail - head >= RUNQUEUE_SIZE) {
        return -1;  /* Queue full */
    }
    
    /* Calculate index */
    uint64_t index = tail & RUNQUEUE_MASK;
    
    /* Store task pointer */
    rq->tasks[index] = task;
    
    /* Update tail with release semantics */
    atomic_store_explicit(&rq->tail, tail + 1, memory_order_release);
    
    /* Update task count (relaxed ordering for statistics) */
    atomic_fetch_add_explicit(&rq->num_tasks, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&rq->enqueue_count, 1, memory_order_relaxed);
    
    return 0;
}

/**
 * @brief Dequeue task from run queue (lock-free)
 */
struct task *runqueue_dequeue(struct cpu_runqueue *rq) {
    if (rq == NULL) {
        return NULL;
    }
    
    /* Load head with acquire semantics */
    uint64_t head = atomic_load_explicit(&rq->head, memory_order_acquire);
    uint64_t tail = atomic_load_explicit(&rq->tail, memory_order_acquire);
    
    /* Check if queue is empty */
    if (head >= tail) {
        return NULL;  /* Queue empty */
    }
    
    /* Calculate index */
    uint64_t index = head & RUNQUEUE_MASK;
    
    /* Load task pointer */
    struct task *task = rq->tasks[index];
    
    /* Clear slot */
    rq->tasks[index] = NULL;
    
    /* Update head with release semantics */
    atomic_store_explicit(&rq->head, head + 1, memory_order_release);
    
    /* Update task count (relaxed ordering for statistics) */
    atomic_fetch_sub_explicit(&rq->num_tasks, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&rq->dequeue_count, 1, memory_order_relaxed);
    
    return task;
}

/**
 * @brief Peek at next task without dequeuing
 */
struct task *runqueue_peek(struct cpu_runqueue *rq) {
    if (rq == NULL) {
        return NULL;
    }
    
    /* Load head with acquire semantics */
    uint64_t head = atomic_load_explicit(&rq->head, memory_order_acquire);
    uint64_t tail = atomic_load_explicit(&rq->tail, memory_order_acquire);
    
    /* Check if queue is empty */
    if (head >= tail) {
        return NULL;  /* Queue empty */
    }
    
    /* Calculate index */
    uint64_t index = head & RUNQUEUE_MASK;
    
    /* Return task pointer without removing */
    return rq->tasks[index];
}

/**
 * @brief Check if run queue is empty
 */
bool runqueue_is_empty(struct cpu_runqueue *rq) {
    if (rq == NULL) {
        return true;
    }
    
    uint64_t head = atomic_load_explicit(&rq->head, memory_order_acquire);
    uint64_t tail = atomic_load_explicit(&rq->tail, memory_order_acquire);
    
    return head >= tail;
}

/**
 * @brief Get number of tasks in run queue
 */
uint32_t runqueue_size(struct cpu_runqueue *rq) {
    if (rq == NULL) {
        return 0;
    }
    
    return atomic_load_explicit(&rq->num_tasks, memory_order_relaxed);
}

/**
 * @brief Steal task from another CPU's run queue
 */
struct task *steal_task(void) {
    uint32_t current_cpu = get_current_cpu_id();
    uint32_t num_cpus = g_smp_config.num_cpus;
    uint32_t current_numa = get_current_numa_node();
    
    /* First pass: Try to steal from same NUMA node */
    for (uint32_t i = 1; i < num_cpus; i++) {
        uint32_t victim_cpu = (current_cpu + i) % num_cpus;
        
        if (get_cpu_numa_node(victim_cpu) == (int)current_numa) {
            struct task *task = steal_task_from_cpu(victim_cpu);
            if (task != NULL) {
                return task;
            }
        }
    }
    
    /* Second pass: Try to steal from other NUMA nodes */
    for (uint32_t i = 1; i < num_cpus; i++) {
        uint32_t victim_cpu = (current_cpu + i) % num_cpus;
        
        if (get_cpu_numa_node(victim_cpu) != (int)current_numa) {
            struct task *task = steal_task_from_cpu(victim_cpu);
            if (task != NULL) {
                return task;
            }
        }
    }
    
    return NULL;  /* No tasks to steal */
}

/**
 * @brief Steal task from specific CPU
 */
struct task *steal_task_from_cpu(uint32_t cpu_id) {
    struct cpu_runqueue *victim_rq = get_cpu_runqueue(cpu_id);
    if (victim_rq == NULL) {
        return NULL;
    }
    
    /* Update steal attempts (relaxed ordering) */
    atomic_fetch_add_explicit(&victim_rq->steal_attempts, 1,
                              memory_order_relaxed);
    
    /* Check if victim has enough tasks */
    uint32_t num_tasks = runqueue_size(victim_rq);
    if (num_tasks < g_smp_config.steal_threshold) {
        return NULL;  /* Not enough tasks to steal */
    }
    
    /* Try to acquire steal lock */
    if (!try_acquire_steal_lock(victim_rq)) {
        return NULL;  /* Lock held by another thief */
    }
    
    /* Steal tasks */
    uint32_t steal_count = g_smp_config.steal_count;
    if (steal_count > num_tasks / 2) {
        steal_count = num_tasks / 2;  /* Steal at most half */
    }
    
    struct task *first_task = NULL;
    struct cpu_runqueue *local_rq = get_current_runqueue();
    
    for (uint32_t i = 0; i < steal_count; i++) {
        struct task *task = runqueue_dequeue(victim_rq);
        if (task == NULL) {
            break;
        }
        
        if (i == 0) {
            first_task = task;  /* Return first task */
        } else {
            /* Enqueue remaining tasks to local queue */
            runqueue_enqueue(local_rq, task);
        }
    }
    
    /* Release steal lock */
    release_steal_lock(victim_rq);
    
    /* Update steal successes if we got any tasks */
    if (first_task != NULL) {
        atomic_fetch_add_explicit(&victim_rq->steal_successes, 1,
                                  memory_order_relaxed);
    }
    
    return first_task;
}

/**
 * @brief Try to acquire steal lock
 */
static bool try_acquire_steal_lock(struct cpu_runqueue *rq) {
    uint32_t expected = 0;
    return atomic_compare_exchange_strong_explicit(
        &rq->steal_lock,
        &expected,
        1,
        memory_order_acquire,
        memory_order_relaxed
    );
}

/**
 * @brief Release steal lock
 */
static void release_steal_lock(struct cpu_runqueue *rq) {
    atomic_store_explicit(&rq->steal_lock, 0, memory_order_release);
}

/**
 * @brief Set work stealing threshold
 */
void set_steal_threshold(uint32_t threshold) {
    g_smp_config.steal_threshold = threshold;
}

/**
 * @brief Set work stealing count
 */
void set_steal_count(uint32_t count) {
    g_smp_config.steal_count = count;
}

/**
 * @brief Get number of CPUs
 */
uint32_t get_num_cpus(void) {
    return g_smp_config.num_cpus;
}

/**
 * @brief Get number of NUMA nodes
 */
uint32_t get_num_numa_nodes(void) {
    return g_smp_config.num_numa_nodes;
}

/**
 * @brief Get CPU's NUMA node
 */
int get_cpu_numa_node(uint32_t cpu_id) {
    if (cpu_id >= g_smp_config.num_cpus) {
        return -1;
    }
    return (int)g_smp_config.cpu_to_numa[cpu_id];
}

/**
 * @brief Get current CPU's NUMA node
 */
uint32_t get_current_numa_node(void) {
    uint32_t cpu_id = get_current_cpu_id();
    return g_smp_config.cpu_to_numa[cpu_id];
}

/**
 * @brief Get current CPU ID
 */
uint32_t get_current_cpu_id(void) {
    /* TODO: Use CPU-specific instruction to get CPU ID */
    /* For x86_64: CPUID or read from GS segment */
    /* For ARM64: read from TPIDR_EL1 */
    /* For now, return 0 (single CPU) */
    return 0;
}

/**
 * @brief Check if CPU is online
 */
bool is_cpu_online(uint32_t cpu_id) {
    return cpu_id < g_smp_config.num_cpus;
}

/**
 * @brief Get run queue statistics
 */
void runqueue_get_stats(struct cpu_runqueue *rq,
                       uint64_t *enqueue_count,
                       uint64_t *dequeue_count,
                       uint64_t *steal_attempts,
                       uint64_t *steal_successes) {
    if (rq == NULL) {
        return;
    }
    
    if (enqueue_count != NULL) {
        *enqueue_count = atomic_load_explicit(&rq->enqueue_count,
                                             memory_order_relaxed);
    }
    
    if (dequeue_count != NULL) {
        *dequeue_count = atomic_load_explicit(&rq->dequeue_count,
                                             memory_order_relaxed);
    }
    
    if (steal_attempts != NULL) {
        *steal_attempts = atomic_load_explicit(&rq->steal_attempts,
                                              memory_order_relaxed);
    }
    
    if (steal_successes != NULL) {
        *steal_successes = atomic_load_explicit(&rq->steal_successes,
                                               memory_order_relaxed);
    }
}

/**
 * @brief Balance load across CPUs
 */
void smp_balance_load(void) {
    /* TODO: Implement load balancing algorithm */
    /* For now, work stealing handles load balancing on-demand */
}

/**
 * @brief Migrate task to different CPU
 */
int smp_migrate_task(struct task *task, uint32_t target_cpu) {
    if (task == NULL || target_cpu >= g_smp_config.num_cpus) {
        return -1;
    }
    
    /* Get target run queue */
    struct cpu_runqueue *target_rq = get_cpu_runqueue(target_cpu);
    if (target_rq == NULL) {
        return -1;
    }
    
    /* Enqueue task to target CPU */
    if (runqueue_enqueue(target_rq, task) != 0) {
        return -1;
    }
    
    /* Update task's current CPU */
    task->current_cpu = target_cpu;
    
    return 0;
}
