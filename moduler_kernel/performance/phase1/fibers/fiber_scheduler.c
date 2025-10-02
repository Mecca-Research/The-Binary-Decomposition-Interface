
/**
 * @file fiber_scheduler.c
 * @brief Implementation of per-core fiber scheduler
 */

#include "fiber_scheduler.h"
#include <stdlib.h>
#include <string.h>

// External functions from fiber.c
extern void fiber_set_current(fiber_t* fiber);
extern fiber_t* fiber_get_current(void);

// Idle fiber entry point
static void idle_fiber_entry(void* arg) {
    (void)arg;
    // Idle loop - just yield forever
    while (1) {
        __asm__ __volatile__("pause");
    }
}

fiber_scheduler_t* fiber_scheduler_create(uint32_t core_id) {
    fiber_scheduler_t* scheduler = calloc(1, sizeof(fiber_scheduler_t));
    if (!scheduler) {
        return NULL;
    }
    
    scheduler->core_id = core_id;
    scheduler->pool_capacity = 64;  // Initial pool capacity
    
    // Create idle fiber
    scheduler->idle_fiber = fiber_create(idle_fiber_entry, NULL, 0, FIBER_PRIORITY_MIN);
    if (!scheduler->idle_fiber) {
        free(scheduler);
        return NULL;
    }
    
    return scheduler;
}

void fiber_scheduler_destroy(fiber_scheduler_t* scheduler) {
    if (!scheduler) {
        return;
    }
    
    // Destroy all fibers in ready queues
    for (int i = 0; i < FIBER_NUM_PRIORITIES; i++) {
        fiber_t* fiber = scheduler->ready_queues[i];
        while (fiber) {
            fiber_t* next = fiber->next;
            fiber_destroy(fiber);
            fiber = next;
        }
    }
    
    // Destroy fiber pool
    fiber_t* fiber = scheduler->fiber_pool;
    while (fiber) {
        fiber_t* next = fiber->next;
        fiber_destroy(fiber);
        fiber = next;
    }
    
    // Destroy idle fiber
    if (scheduler->idle_fiber) {
        fiber_destroy(scheduler->idle_fiber);
    }
    
    free(scheduler);
}

// Internal: Add fiber to ready queue
static void enqueue_fiber(fiber_scheduler_t* scheduler, fiber_t* fiber) {
    uint32_t priority = fiber->priority;
    
    // Add to tail of priority queue
    if (!scheduler->ready_queues[priority]) {
        scheduler->ready_queues[priority] = fiber;
        fiber->next = NULL;
        fiber->prev = NULL;
    } else {
        // Find tail
        fiber_t* tail = scheduler->ready_queues[priority];
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = fiber;
        fiber->prev = tail;
        fiber->next = NULL;
    }
    
    fiber->state = FIBER_STATE_READY;
}

// Internal: Remove fiber from ready queue
static void dequeue_fiber(fiber_scheduler_t* scheduler, fiber_t* fiber) {
    uint32_t priority = fiber->priority;
    
    if (fiber->prev) {
        fiber->prev->next = fiber->next;
    } else {
        scheduler->ready_queues[priority] = fiber->next;
    }
    
    if (fiber->next) {
        fiber->next->prev = fiber->prev;
    }
    
    fiber->next = NULL;
    fiber->prev = NULL;
}

// Internal: Get next fiber to run (highest priority)
static fiber_t* get_next_fiber(fiber_scheduler_t* scheduler) {
    // Scan priority queues from highest to lowest
    for (int i = 0; i < FIBER_NUM_PRIORITIES; i++) {
        if (scheduler->ready_queues[i]) {
            fiber_t* fiber = scheduler->ready_queues[i];
            dequeue_fiber(scheduler, fiber);
            return fiber;
        }
    }
    
    // No ready fibers, return idle fiber
    return scheduler->idle_fiber;
}

uint64_t fiber_scheduler_spawn(fiber_scheduler_t* scheduler,
                                fiber_func_t entry,
                                void* arg,
                                size_t stack_size,
                                uint32_t priority) {
    if (!scheduler || !entry) {
        return 0;
    }
    
    // Try to reuse fiber from pool
    fiber_t* fiber = NULL;
    if (scheduler->fiber_pool) {
        fiber = scheduler->fiber_pool;
        scheduler->fiber_pool = fiber->next;
        scheduler->pool_size--;
        
        // Reinitialize fiber
        fiber->entry = entry;
        fiber->arg = arg;
        fiber->priority = priority;
        fiber->state = FIBER_STATE_READY;
        fiber->yield_value = NULL;
        fiber->yield_to = NULL;
        fiber->next = NULL;
        fiber->prev = NULL;
    } else {
        // Create new fiber
        fiber = fiber_create(entry, arg, stack_size, priority);
        if (!fiber) {
            return 0;
        }
        scheduler->total_fibers_created++;
    }
    
    // Add to ready queue
    enqueue_fiber(scheduler, fiber);
    
    return fiber->fiber_id;
}

void fiber_scheduler_yield(fiber_scheduler_t* scheduler) {
    if (!scheduler || !scheduler->current_fiber) {
        return;
    }
    
    fiber_t* current = scheduler->current_fiber;
    
    // Put current fiber back in ready queue
    enqueue_fiber(scheduler, current);
    
    // Get next fiber to run
    fiber_t* next = get_next_fiber(scheduler);
    
    // Switch to next fiber
    scheduler->current_fiber = next;
    next->state = FIBER_STATE_RUNNING;
    fiber_set_current(next);
    
    scheduler->total_switches++;
    scheduler->total_yields++;
    
    fiber_switch(current, next);
}

void fiber_scheduler_block(fiber_scheduler_t* scheduler, void* yield_value) {
    if (!scheduler || !scheduler->current_fiber) {
        return;
    }
    
    fiber_t* current = scheduler->current_fiber;
    current->state = FIBER_STATE_BLOCKED;
    current->yield_value = yield_value;
    
    // Get next fiber to run (don't enqueue current)
    fiber_t* next = get_next_fiber(scheduler);
    
    // Switch to next fiber
    scheduler->current_fiber = next;
    next->state = FIBER_STATE_RUNNING;
    fiber_set_current(next);
    
    scheduler->total_switches++;
    
    fiber_switch(current, next);
}

bool fiber_scheduler_unblock(fiber_scheduler_t* scheduler, uint64_t fiber_id) {
    if (!scheduler) {
        return false;
    }
    
    // Search for fiber in all queues and blocked state
    // This is a simplified implementation - in production, maintain a hash table
    
    // For now, just return false (fiber not found)
    // TODO: Implement proper fiber tracking
    return false;
}

void fiber_scheduler_run(fiber_scheduler_t* scheduler) {
    if (!scheduler) {
        return;
    }
    
    // Get first fiber to run
    fiber_t* next = get_next_fiber(scheduler);
    if (!next) {
        return;  // No fibers to run
    }
    
    // Set up initial context
    scheduler->current_fiber = next;
    next->state = FIBER_STATE_RUNNING;
    fiber_set_current(next);
    
    // Jump to fiber (this will start the scheduler loop)
    // When fibers yield, they'll call fiber_scheduler_yield which switches context
    // The scheduler runs until all fibers are complete or blocked
    
    // Create a dummy "main" fiber to switch from
    fiber_t main_fiber = {0};
    fiber_switch(&main_fiber, next);
    
    // When we return here, all fibers are done
}

void fiber_scheduler_get_stats(const fiber_scheduler_t* scheduler,
                                uint64_t* total_switches,
                                uint64_t* total_yields,
                                uint64_t* total_fibers) {
    if (!scheduler) {
        return;
    }
    
    if (total_switches) {
        *total_switches = scheduler->total_switches;
    }
    if (total_yields) {
        *total_yields = scheduler->total_yields;
    }
    if (total_fibers) {
        *total_fibers = scheduler->total_fibers_created;
    }
}
