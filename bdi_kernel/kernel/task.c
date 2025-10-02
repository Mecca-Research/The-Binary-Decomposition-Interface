

/**
 * @file task.c
 * @brief Task management and run-to-completion fibers implementation
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 */

#include "task.h"
#include "scheduler.h"
#include "smp.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Global task ID counter */
static _Atomic uint64_t g_next_task_id = 1;

/* Forward declarations */
static void task_wrapper(void);
static void fiber_wrapper(void);
static void setup_task_stack(struct task *task);

/**
 * @brief Initialize task subsystem
 */
void task_init(void) {
    atomic_init(&g_next_task_id, 1);
    printf("[Task] Task subsystem initialized\n");
}

/**
 * @brief Create a new task
 */
struct task *task_create(
    const char *name,
    task_entry_t entry_point,
    void *arg,
    uint32_t priority,
    size_t stack_size
) {
    if (entry_point == NULL) {
        return NULL;
    }
    
    /* Validate stack size */
    if (stack_size < TASK_STACK_MIN) {
        stack_size = TASK_STACK_MIN;
    }
    if (stack_size > TASK_STACK_MAX) {
        stack_size = TASK_STACK_MAX;
    }
    
    /* Allocate task structure (NUMA-aware) */
    struct task *task = numa_alloc_local(sizeof(struct task));
    if (task == NULL) {
        return NULL;
    }
    
    memset(task, 0, sizeof(struct task));
    
    /* Allocate stack (NUMA-aware) */
    task->stack_base = numa_alloc_local(stack_size);
    if (task->stack_base == NULL) {
        (void)numa_free(task, sizeof(struct task));
        return NULL;
    }
    
    task->stack_size = stack_size;
    
    /* Initialize task fields */
    task->task_id = atomic_fetch_add_explicit(&g_next_task_id, 1,
                                              memory_order_relaxed);
    
    if (name != NULL) {
        strncpy(task->name, name, TASK_NAME_MAX - 1);
        task->name[TASK_NAME_MAX - 1] = '\0';
    } else {
        snprintf(task->name, TASK_NAME_MAX, "task_%lu", task->task_id);
    }
    
    atomic_init(&task->state, TASK_READY);
    task->flags = TASK_FLAG_KERNEL;
    task->priority = (priority > TASK_MAX_PRIORITY) ? TASK_MAX_PRIORITY : priority;
    task->cpu_affinity = 0xFFFFFFFF;  /* All CPUs */
    task->current_cpu = (uint32_t)-1;
    task->numa_node = get_current_numa_node();
    
    task->entry_point = entry_point;
    task->entry_arg = arg;
    
    atomic_init(&task->total_runtime, 0);
    atomic_init(&task->context_switches, 0);
    
    task->next = NULL;
    
    /* Setup task stack */
    setup_task_stack(task);
    
    return task;
}

/**
 * @brief Create a run-to-completion fiber
 */
struct task *fiber_create(
    fiber_entry_t entry_point,
    void *arg,
    size_t stack_size
) {
    struct task *fiber = task_create(
        "fiber",
        (task_entry_t)entry_point,
        arg,
        PRIORITY_NORMAL,
        stack_size
    );
    
    if (fiber != NULL) {
        fiber->flags |= TASK_FLAG_FIBER;
    }
    
    return fiber;
}

/**
 * @brief Start a task (add to run queue)
 */
int task_start(struct task *task) {
    if (task == NULL) {
        return -1;
    }
    
    /* Transition to READY state */
    enum task_state expected = TASK_READY;
    if (!atomic_compare_exchange_strong_explicit(
            &task->state,
            &expected,
            TASK_READY,
            memory_order_acq_rel,
            memory_order_acquire)) {
        /* Task not in correct state */
        return -1;
    }
    
    /* Add to run queue */
    struct cpu_runqueue *rq = get_current_runqueue();
    if (runqueue_enqueue(rq, task) != 0) {
        return -1;
    }
    
    /* Update running task count */
    atomic_fetch_add_explicit(&g_scheduler.num_running_tasks, 1,
                              memory_order_relaxed);
    
    return 0;
}

/**
 * @brief Stop a task (remove from run queue)
 */
int task_stop(struct task *task) {
    if (task == NULL) {
        return -1;
    }
    
    /* Transition to BLOCKED state */
    enum task_state expected = TASK_READY;
    if (!atomic_compare_exchange_strong_explicit(
            &task->state,
            &expected,
            TASK_BLOCKED,
            memory_order_acq_rel,
            memory_order_acquire)) {
        /* Task not in correct state */
        return -1;
    }
    
    /* Update running task count */
    atomic_fetch_sub_explicit(&g_scheduler.num_running_tasks, 1,
                              memory_order_relaxed);
    
    return 0;
}

/**
 * @brief Destroy a task (cleanup resources)
 */
void task_destroy(struct task *task) {
    if (task == NULL) {
        return;
    }
    
    /* Transition to DEAD state */
    atomic_store_explicit(&task->state, TASK_DEAD,
                         memory_order_release);
    
    /* Free stack */
    if (task->stack_base != NULL) {
        (void)numa_free(task->stack_base, task->stack_size);
    }
    
    /* Free task structure */
    (void)numa_free(task, sizeof(struct task));
}

/**
 * @brief Yield CPU to another task
 */
void task_yield(void) {
    /* Add current task back to run queue */
    struct task *current = get_current_task();
    if (current != NULL) {
        struct cpu_runqueue *rq = get_current_runqueue();
        runqueue_enqueue(rq, current);
    }
    
    /* Trigger reschedule */
    schedule();
}

/**
 * @brief Sleep for specified milliseconds
 */
void task_sleep(uint32_t milliseconds) {
    struct task *current = get_current_task();
    if (current == NULL) {
        return;
    }
    
    /* Calculate wakeup time */
    uint64_t current_time = atomic_load_explicit(&g_scheduler.tick_count,
                                                 memory_order_relaxed);
    current->wakeup_time = current_time + milliseconds;
    
    /* Transition to SLEEPING state */
    task_set_state(current, TASK_RUNNING, TASK_SLEEPING);
    
    /* Trigger reschedule */
    schedule();
}

/**
 * @brief Block task (waiting for event)
 */
void task_block(struct task *task) {
    if (task == NULL) {
        return;
    }
    
    /* Transition to BLOCKED state */
    task_set_state(task, TASK_RUNNING, TASK_BLOCKED);
    
    /* Update running task count */
    atomic_fetch_sub_explicit(&g_scheduler.num_running_tasks, 1,
                              memory_order_relaxed);
}

/**
 * @brief Unblock task (event occurred)
 * 
 * BUGFIX: Check runqueue_enqueue return value to prevent task loss
 * and inconsistent statistics when queue is full.
 */
void task_unblock(struct task *task) {
    if (task == NULL) {
        return;
    }
    
    /* Transition to READY state */
    if (task_set_state(task, TASK_BLOCKED, TASK_READY)) {
        /* Add back to run queue */
        struct cpu_runqueue *rq = get_current_runqueue();
        int result = runqueue_enqueue(rq, task);
        
        /* Only update counter if enqueue succeeded - THIS PREVENTS TASK LOSS */
        if (result == 0) {
            atomic_fetch_add_explicit(&g_scheduler.num_running_tasks, 1,
                                      memory_order_relaxed);
        } else {
            /* Enqueue failed, revert state back to BLOCKED */
            task_set_state(task, TASK_READY, TASK_BLOCKED);
            /* TODO: Could log error or retry on different CPU */
        }
    }
}

/**
 * @brief Get task state (atomic)
 */
enum task_state task_get_state(struct task *task) {
    if (task == NULL) {
        return TASK_DEAD;
    }
    
    return atomic_load_explicit(&task->state, memory_order_acquire);
}

/**
 * @brief Set task state (atomic)
 */
bool task_set_state(struct task *task, 
                    enum task_state old_state,
                    enum task_state new_state) {
    if (task == NULL) {
        return false;
    }
    
    enum task_state expected = old_state;
    return atomic_compare_exchange_strong_explicit(
        &task->state,
        &expected,
        new_state,
        memory_order_acq_rel,
        memory_order_acquire
    );
}

/**
 * @brief Set task CPU affinity
 */
int task_set_affinity(struct task *task, uint32_t cpu_mask) {
    if (task == NULL) {
        return -1;
    }
    
    task->cpu_affinity = cpu_mask;
    return 0;
}

/**
 * @brief Get task CPU affinity
 */
uint32_t task_get_affinity(struct task *task) {
    if (task == NULL) {
        return 0;
    }
    
    return task->cpu_affinity;
}

/**
 * @brief Pin task to specific CPU
 */
int task_pin_to_cpu(struct task *task, uint32_t cpu_id) {
    if (task == NULL || cpu_id >= get_num_cpus()) {
        return -1;
    }
    
    task->cpu_affinity = (1U << cpu_id);
    task->flags |= TASK_FLAG_PINNED;
    
    return 0;
}

/**
 * @brief Pin task to NUMA node
 */
int task_pin_to_numa_node(struct task *task, uint32_t node_id) {
    if (task == NULL || node_id >= get_num_numa_nodes()) {
        return -1;
    }
    
    /* Set affinity to all CPUs in NUMA node */
    uint32_t cpu_mask = 0;
    uint32_t num_cpus = get_num_cpus();
    
    for (uint32_t i = 0; i < num_cpus; i++) {
        if (get_cpu_numa_node(i) == (int)node_id) {
            cpu_mask |= (1U << i);
        }
    }
    
    task->cpu_affinity = cpu_mask;
    task->numa_node = node_id;
    task->flags |= TASK_FLAG_NUMA_LOCAL;
    
    return 0;
}

/**
 * @brief Get task by ID
 */
struct task *task_get_by_id(uint64_t task_id) {
    /* TODO: Implement task lookup table */
    (void)task_id;
    return NULL;
}

/**
 * @brief Get task name
 */
const char *task_get_name(struct task *task) {
    if (task == NULL) {
        return NULL;
    }
    
    return task->name;
}

/**
 * @brief Get task priority
 */
uint8_t task_get_priority(struct task *task) {
    if (task == NULL) {
        return 0;
    }
    
    return task->priority;
}

/**
 * @brief Set task priority
 */
void task_set_priority(struct task *task, uint8_t priority) {
    if (task == NULL) {
        return;
    }
    
    /* priority is uint8_t, TASK_MAX_PRIORITY is 255, so no need to check upper bound */
    task->priority = priority;
}

/**
 * @brief Get task CPU
 */
int task_get_cpu(struct task *task) {
    if (task == NULL) {
        return -1;
    }
    
    if (task->current_cpu == (uint32_t)-1) {
        return -1;
    }
    
    return (int)task->current_cpu;
}

/**
 * @brief Check if task is fiber
 */
bool task_is_fiber(struct task *task) {
    if (task == NULL) {
        return false;
    }
    
    return (task->flags & TASK_FLAG_FIBER) != 0;
}

/**
 * @brief Exit current task
 */
void task_exit(void) {
    struct task *current = get_current_task();
    if (current == NULL) {
        /* Should not happen, but handle gracefully */
        while (1) {
            __asm__ volatile("pause");
        }
    }
    
    /* Transition to ZOMBIE state */
    task_set_state(current, TASK_RUNNING, TASK_ZOMBIE);
    
    /* Update running task count */
    atomic_fetch_sub_explicit(&g_scheduler.num_running_tasks, 1,
                              memory_order_relaxed);
    
    /* Trigger reschedule (never returns) */
    schedule();
    
    /* Should never reach here - infinite loop as fallback */
    while (1) {
        __asm__ volatile("pause");
    }
}

/**
 * @brief Setup task stack
 * 
 * Initializes stack with task wrapper and context.
 */
static void setup_task_stack(struct task *task) {
    /* Calculate stack top (stacks grow downward) */
    void *stack_top = (uint8_t *)task->stack_base + task->stack_size;
    
    /* Setup initial stack pointer */
    task->context.stack_pointer = stack_top;
    
    /* Setup instruction pointer to task wrapper */
    /* Note: We store function pointers as void* for context switching */
    /* This is implementation-defined but common in kernel code */
    if (task->flags & TASK_FLAG_FIBER) {
        union {
            void (*func)(void);
            void *ptr;
        } u = { .func = fiber_wrapper };
        task->context.instruction_pointer = u.ptr;
    } else {
        union {
            void (*func)(void);
            void *ptr;
        } u = { .func = task_wrapper };
        task->context.instruction_pointer = u.ptr;
    }
    
    /* TODO: Setup architecture-specific context */
    /* For x86_64: push return address, setup rbp, etc. */
    /* For ARM64: setup lr, fp, etc. */
}

/**
 * @brief Task wrapper function
 * 
 * Called when task first starts executing.
 * Calls task entry point and handles exit.
 */
static void task_wrapper(void) {
    struct task *current = get_current_task();
    if (current == NULL) {
        return;
    }
    
    /* Call task entry point */
    if (current->entry_point != NULL) {
        current->entry_point(current->entry_arg);
    }
    
    /* Task returned, exit */
    task_exit();
}

/**
 * @brief Fiber wrapper function
 * 
 * Called when fiber first starts executing.
 * Calls fiber entry point and handles exit.
 */
static void fiber_wrapper(void) {
    struct task *current = get_current_task();
    if (current == NULL) {
        return;
    }
    
    /* Call fiber entry point */
    if (current->entry_point != NULL) {
        current->entry_point(current->entry_arg);
    }
    
    /* Fiber completed, exit */
    task_exit();
}
