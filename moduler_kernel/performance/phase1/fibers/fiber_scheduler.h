
/**
 * @file fiber_scheduler.h
 * @brief Per-core run-to-completion fiber scheduler
 * 
 * Implements cooperative scheduling with priority support.
 * No preemption in hot path - fibers yield explicitly.
 */

#ifndef PHASE1_FIBER_SCHEDULER_H
#define PHASE1_FIBER_SCHEDULER_H

#include "fiber.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct fiber_scheduler fiber_scheduler_t;

/**
 * @brief Fiber scheduler structure (per-core)
 */
struct fiber_scheduler {
    // Core affinity
    uint32_t core_id;
    
    // Priority queues (one per priority level)
    fiber_t* ready_queues[FIBER_NUM_PRIORITIES];
    
    // Currently running fiber
    fiber_t* current_fiber;
    
    // Idle fiber (runs when no work available)
    fiber_t* idle_fiber;
    
    // Fiber pool for reuse
    fiber_t* fiber_pool;
    size_t pool_size;
    size_t pool_capacity;
    
    // Statistics
    uint64_t total_switches;
    uint64_t total_yields;
    uint64_t total_fibers_created;
    uint64_t total_fibers_destroyed;
};

/**
 * @brief Create a new fiber scheduler
 * 
 * @param core_id Core ID for affinity
 * @return Pointer to scheduler, or NULL on failure
 */
fiber_scheduler_t* fiber_scheduler_create(uint32_t core_id);

/**
 * @brief Destroy fiber scheduler
 * 
 * @param scheduler Scheduler to destroy
 */
void fiber_scheduler_destroy(fiber_scheduler_t* scheduler);

/**
 * @brief Spawn a new fiber
 * 
 * @param scheduler Scheduler
 * @param entry Entry point function
 * @param arg Argument to pass to entry function
 * @param stack_size Stack size (0 = default)
 * @param priority Priority level (0-15)
 * @return Fiber ID, or 0 on failure
 */
uint64_t fiber_scheduler_spawn(fiber_scheduler_t* scheduler,
                                fiber_func_t entry,
                                void* arg,
                                size_t stack_size,
                                uint32_t priority);

/**
 * @brief Yield current fiber
 * 
 * Voluntarily gives up CPU to allow other fibers to run.
 * Current fiber is placed back in ready queue.
 * 
 * @param scheduler Scheduler
 */
void fiber_scheduler_yield(fiber_scheduler_t* scheduler);

/**
 * @brief Block current fiber
 * 
 * Removes current fiber from ready queue.
 * Fiber must be explicitly unblocked to run again.
 * 
 * @param scheduler Scheduler
 * @param yield_value Optional value to return from yield
 */
void fiber_scheduler_block(fiber_scheduler_t* scheduler, void* yield_value);

/**
 * @brief Unblock a fiber
 * 
 * Places fiber back in ready queue.
 * 
 * @param scheduler Scheduler
 * @param fiber_id Fiber ID to unblock
 * @return true if fiber was unblocked, false if not found
 */
bool fiber_scheduler_unblock(fiber_scheduler_t* scheduler, uint64_t fiber_id);

/**
 * @brief Run scheduler loop
 * 
 * Runs fibers until all are complete or blocked.
 * This is the main scheduler loop.
 * 
 * @param scheduler Scheduler
 */
void fiber_scheduler_run(fiber_scheduler_t* scheduler);

/**
 * @brief Get scheduler statistics
 * 
 * @param scheduler Scheduler
 * @param total_switches Output: total context switches
 * @param total_yields Output: total yields
 * @param total_fibers Output: total fibers created
 */
void fiber_scheduler_get_stats(const fiber_scheduler_t* scheduler,
                                uint64_t* total_switches,
                                uint64_t* total_yields,
                                uint64_t* total_fibers);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_FIBER_SCHEDULER_H
