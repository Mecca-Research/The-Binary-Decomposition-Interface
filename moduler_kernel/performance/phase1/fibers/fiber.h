
/**
 * @file fiber.h
 * @brief Lightweight fiber (user-space thread) implementation
 * 
 * Fibers are cooperative, lightweight execution contexts that enable
 * run-to-completion scheduling without kernel involvement.
 */

#ifndef PHASE1_FIBER_H
#define PHASE1_FIBER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fiber states
typedef enum {
    FIBER_STATE_READY = 0,      // Ready to run
    FIBER_STATE_RUNNING,        // Currently executing
    FIBER_STATE_BLOCKED,        // Waiting for event
    FIBER_STATE_DEAD            // Finished execution
} fiber_state_t;

// Fiber priority levels (0 = highest, 15 = lowest)
#define FIBER_PRIORITY_MAX 0
#define FIBER_PRIORITY_HIGH 3
#define FIBER_PRIORITY_NORMAL 7
#define FIBER_PRIORITY_LOW 11
#define FIBER_PRIORITY_MIN 15
#define FIBER_NUM_PRIORITIES 16

// Default stack size (64KB)
#define FIBER_DEFAULT_STACK_SIZE (64 * 1024)

// Fiber entry point function
typedef void (*fiber_func_t)(void* arg);

// Forward declarations
typedef struct fiber fiber_t;
typedef struct fiber_context fiber_context_t;

/**
 * @brief Fiber execution context
 * 
 * Minimal context for fast switching (x86-64):
 * - Stack pointer (RSP)
 * - Base pointer (RBP)
 * - Callee-saved registers (RBX, R12-R15)
 * - Instruction pointer (RIP)
 */
struct fiber_context {
    void* rsp;      // Stack pointer
    void* rbp;      // Base pointer
    void* rbx;      // Callee-saved
    void* r12;      // Callee-saved
    void* r13;      // Callee-saved
    void* r14;      // Callee-saved
    void* r15;      // Callee-saved
    void* rip;      // Instruction pointer
};

/**
 * @brief Fiber structure
 */
struct fiber {
    // Execution context
    fiber_context_t context;
    void* stack_base;
    void* stack_top;
    size_t stack_size;
    
    // Scheduling metadata
    fiber_state_t state;
    uint32_t priority;
    uint64_t fiber_id;
    
    // Entry point
    fiber_func_t entry;
    void* arg;
    
    // Yield/resume support
    void* yield_value;
    fiber_t* yield_to;
    
    // Linked list for scheduler queues
    fiber_t* next;
    fiber_t* prev;
    
    // Performance counters
    uint64_t run_count;
    uint64_t total_runtime_ns;
    uint64_t last_run_time_ns;
};

/**
 * @brief Create a new fiber
 * 
 * @param entry Entry point function
 * @param arg Argument to pass to entry function
 * @param stack_size Stack size in bytes (0 = default)
 * @param priority Priority level (0-15)
 * @return Pointer to fiber, or NULL on failure
 */
fiber_t* fiber_create(fiber_func_t entry, void* arg, size_t stack_size, uint32_t priority);

/**
 * @brief Destroy a fiber
 * 
 * @param fiber Fiber to destroy
 */
void fiber_destroy(fiber_t* fiber);

/**
 * @brief Switch from current fiber to target fiber
 * 
 * Saves current fiber's context and restores target fiber's context.
 * This is a low-level primitive used by the scheduler.
 * 
 * @param from Current fiber (context will be saved here)
 * @param to Target fiber (context will be restored from here)
 */
void fiber_switch(fiber_t* from, fiber_t* to);

/**
 * @brief Get current fiber ID
 * 
 * @return Current fiber ID, or 0 if not in fiber context
 */
uint64_t fiber_current_id(void);

/**
 * @brief Get fiber state string
 * 
 * @param state Fiber state
 * @return String representation
 */
const char* fiber_state_to_string(fiber_state_t state);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_FIBER_H
