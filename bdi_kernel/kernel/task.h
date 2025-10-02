
/**
 * @file task.h
 * @brief Task management and run-to-completion fibers
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 * 
 * This header defines task management structures and APIs, including
 * run-to-completion fibers for lightweight task execution.
 * 
 * Key Features:
 * - Task structure with C23 atomic state
 * - Run-to-completion fiber model
 * - Fast context switching
 * - Atomic state transitions
 * - Efficient stack management
 */

#ifndef BDI_TASK_H
#define BDI_TASK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"

/* Task constants */
#define TASK_NAME_MAX           32      /* Maximum task name length */
#define TASK_STACK_MIN          4096    /* Minimum stack size (4KB) */
#define TASK_STACK_DEFAULT      8192    /* Default stack size (8KB) */
#define TASK_STACK_MAX          1048576 /* Maximum stack size (1MB) */
#define TASK_MAX_PRIORITY       255     /* Maximum priority value */

/* Task priorities */
#define PRIORITY_IDLE           0       /* Idle priority */
#define PRIORITY_LOW            64      /* Low priority */
#define PRIORITY_NORMAL         128     /* Normal priority */
#define PRIORITY_HIGH           192     /* High priority */
#define PRIORITY_REALTIME       255     /* Real-time priority */

/* Task flags */
#define TASK_FLAG_KERNEL        (1U << 0)  /* Kernel task */
#define TASK_FLAG_USER          (1U << 1)  /* User task */
#define TASK_FLAG_FIBER         (1U << 2)  /* Run-to-completion fiber */
#define TASK_FLAG_PINNED        (1U << 3)  /* Pinned to CPU */
#define TASK_FLAG_NUMA_LOCAL    (1U << 4)  /* NUMA-local allocation */

/**
 * @brief Task states
 * 
 * State transitions are atomic using compare-exchange operations.
 */
enum task_state {
    TASK_READY = 0,     /* Ready to run */
    TASK_RUNNING,       /* Currently running */
    TASK_BLOCKED,       /* Waiting for I/O or event */
    TASK_SLEEPING,      /* Sleeping (timer-based) */
    TASK_ZOMBIE,        /* Exited, awaiting cleanup */
    TASK_DEAD           /* Fully cleaned up */
};

/**
 * @brief Task entry point function signature
 */
typedef void (*task_entry_t)(void *arg);

/**
 * @brief Fiber entry point function signature
 */
typedef void (*fiber_entry_t)(void *arg);

/**
 * @brief Task context for context switching
 * 
 * Minimal context for fast switching.
 * Architecture-specific fields would be added here.
 */
struct task_context {
    /* Stack pointer */
    void *stack_pointer;
    
    /* Instruction pointer */
    void *instruction_pointer;
    
    /* TODO: Add architecture-specific registers */
    /* For x86_64: rsp, rbp, rip, rbx, r12-r15 */
    /* For ARM64: sp, pc, x19-x29, fp, lr */
    
    uint64_t reserved[16];  /* Reserved for arch-specific state */
};

/**
 * @brief Task structure
 * 
 * Represents a schedulable task or fiber.
 * Uses C23 atomics for state management.
 */
struct task {
    /* Task ID (unique) */
    uint64_t task_id;
    
    /* Task name (for debugging) */
    char name[TASK_NAME_MAX];
    
    /* Task state (atomic) */
    _Atomic uint32_t state;
    
    /* Task flags */
    uint32_t flags;
    
    /* Priority (0-255, higher = more important) */
    uint8_t priority;
    
    /* CPU affinity mask */
    uint32_t cpu_affinity;
    
    /* Current CPU (if running) */
    uint32_t current_cpu;
    
    /* NUMA node */
    uint32_t numa_node;
    
    /* Entry point and argument */
    task_entry_t entry_point;
    void *entry_arg;
    
    /* Stack information */
    void *stack_base;
    size_t stack_size;
    
    /* Task context for context switching */
    struct task_context context;
    
    /* Sleep wakeup time (for TASK_SLEEPING) */
    uint64_t wakeup_time;
    
    /* Statistics */
    _Atomic uint64_t total_runtime;
    _Atomic uint64_t context_switches;
    
    /* Next task in list (for run queues, etc.) */
    struct task *next;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(uint64_t) + TASK_NAME_MAX + 
                          sizeof(_Atomic uint32_t) + sizeof(uint32_t) * 4 + 
                          sizeof(uint8_t) + sizeof(task_entry_t) + 
                          sizeof(void *) * 2 + sizeof(size_t) + 
                          sizeof(struct task_context) + sizeof(uint64_t) + 
                          sizeof(_Atomic uint64_t) * 2 + 
                          sizeof(struct task *)) % 64)];
} __attribute__((aligned(64)));

/**
 * @brief Initialize task subsystem
 */
void task_init(void);

/**
 * @brief Create a new task
 * 
 * @param name Task name (for debugging)
 * @param entry_point Entry point function
 * @param arg Argument to pass to entry point
 * @param priority Task priority (0-255)
 * @param stack_size Stack size in bytes
 * @return Pointer to created task, or NULL on failure
 */
[[nodiscard]] struct task *task_create(
    const char *name,
    task_entry_t entry_point,
    void *arg,
    uint32_t priority,
    size_t stack_size
);

/**
 * @brief Create a run-to-completion fiber
 * 
 * Fibers are lightweight tasks that run to completion without preemption.
 * 
 * @param entry_point Fiber entry point function
 * @param arg Argument to pass to entry point
 * @param stack_size Stack size in bytes
 * @return Pointer to created fiber, or NULL on failure
 */
[[nodiscard]] struct task *fiber_create(
    fiber_entry_t entry_point,
    void *arg,
    size_t stack_size
);

/**
 * @brief Start a task (add to run queue)
 * 
 * @param task Task to start
 * @return 0 on success, -1 on failure
 */
int task_start(struct task *task);

/**
 * @brief Stop a task (remove from run queue)
 * 
 * @param task Task to stop
 * @return 0 on success, -1 on failure
 */
int task_stop(struct task *task);

/**
 * @brief Destroy a task (cleanup resources)
 * 
 * @param task Task to destroy
 */
void task_destroy(struct task *task);

/**
 * @brief Yield CPU to another task
 * 
 * Voluntarily gives up CPU to allow other tasks to run.
 */
void task_yield(void);

/**
 * @brief Sleep for specified milliseconds
 * 
 * @param milliseconds Time to sleep in milliseconds
 */
void task_sleep(uint32_t milliseconds);

/**
 * @brief Block task (waiting for event)
 * 
 * @param task Task to block
 */
void task_block(struct task *task);

/**
 * @brief Unblock task (event occurred)
 * 
 * @param task Task to unblock
 */
void task_unblock(struct task *task);

/**
 * @brief Get task state (atomic)
 * 
 * @param task Task to query
 * @return Current task state
 */
[[nodiscard]] enum task_state task_get_state(struct task *task);

/**
 * @brief Set task state (atomic)
 * 
 * Uses compare-exchange to atomically transition state.
 * 
 * @param task Task to modify
 * @param old_state Expected old state
 * @param new_state New state to set
 * @return true if state was changed, false if old_state didn't match
 */
bool task_set_state(struct task *task, 
                    enum task_state old_state,
                    enum task_state new_state);

/**
 * @brief Set task CPU affinity
 * 
 * @param task Task to modify
 * @param cpu_mask CPU affinity mask (bit N = CPU N)
 * @return 0 on success, -1 on failure
 */
int task_set_affinity(struct task *task, uint32_t cpu_mask);

/**
 * @brief Get task CPU affinity
 * 
 * @param task Task to query
 * @return CPU affinity mask
 */
[[nodiscard]] uint32_t task_get_affinity(struct task *task);

/**
 * @brief Pin task to specific CPU
 * 
 * @param task Task to pin
 * @param cpu_id CPU ID to pin to
 * @return 0 on success, -1 on failure
 */
int task_pin_to_cpu(struct task *task, uint32_t cpu_id);

/**
 * @brief Pin task to NUMA node
 * 
 * @param task Task to pin
 * @param node_id NUMA node ID
 * @return 0 on success, -1 on failure
 */
int task_pin_to_numa_node(struct task *task, uint32_t node_id);

/**
 * @brief Get task by ID
 * 
 * @param task_id Task ID to search for
 * @return Pointer to task, or NULL if not found
 */
[[nodiscard]] struct task *task_get_by_id(uint64_t task_id);

/**
 * @brief Get task name
 * 
 * @param task Task to query
 * @return Task name string
 */
[[nodiscard]] const char *task_get_name(struct task *task);

/**
 * @brief Get task priority
 * 
 * @param task Task to query
 * @return Task priority (0-255)
 */
[[nodiscard]] uint8_t task_get_priority(struct task *task);

/**
 * @brief Set task priority
 * 
 * @param task Task to modify
 * @param priority New priority (0-255)
 */
void task_set_priority(struct task *task, uint8_t priority);

/**
 * @brief Get task CPU
 * 
 * @param task Task to query
 * @return CPU ID where task is running, or -1 if not running
 */
[[nodiscard]] int task_get_cpu(struct task *task);

/**
 * @brief Check if task is fiber
 * 
 * @param task Task to check
 * @return true if task is a fiber, false otherwise
 */
[[nodiscard]] bool task_is_fiber(struct task *task);

/**
 * @brief Exit current task
 * 
 * Called by task when it wants to exit.
 * Transitions to TASK_ZOMBIE state.
 */
_Noreturn void task_exit(void);

#endif /* BDI_TASK_H */
