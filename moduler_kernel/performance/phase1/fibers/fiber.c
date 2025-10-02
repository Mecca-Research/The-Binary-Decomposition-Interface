
/**
 * @file fiber.c
 * @brief Implementation of lightweight fibers
 */

#include "fiber.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

// Global fiber ID counter
static _Atomic uint64_t g_next_fiber_id = 1;

// Thread-local current fiber
static __thread fiber_t* g_current_fiber = NULL;

// Thread-local current scheduler (set by scheduler, used by fiber_entry_wrapper)
static __thread void* g_current_scheduler = NULL;

// Forward declaration of fiber entry wrapper
static void fiber_entry_wrapper(void);

// Forward declaration of scheduler yield (to avoid circular dependency)
extern void fiber_scheduler_yield(void* scheduler);

fiber_t* fiber_create(fiber_func_t entry, void* arg, size_t stack_size, uint32_t priority) {
    if (!entry || priority >= FIBER_NUM_PRIORITIES) {
        return NULL;
    }
    
    // Use default stack size if not specified
    if (stack_size == 0) {
        stack_size = FIBER_DEFAULT_STACK_SIZE;
    }
    
    // Allocate fiber structure
    fiber_t* fiber = calloc(1, sizeof(fiber_t));
    if (!fiber) {
        return NULL;
    }
    
    // Allocate stack (16-byte aligned for x86-64 ABI)
    fiber->stack_base = aligned_alloc(16, stack_size);
    if (!fiber->stack_base) {
        free(fiber);
        return NULL;
    }
    
    // Initialize fiber
    fiber->stack_size = stack_size;
    fiber->stack_top = (char*)fiber->stack_base + stack_size;
    fiber->state = FIBER_STATE_READY;
    fiber->priority = priority;
    fiber->fiber_id = atomic_fetch_add(&g_next_fiber_id, 1);
    fiber->entry = entry;
    fiber->arg = arg;
    
    // Initialize context for first run
    // Stack grows downward, so start at top
    void** stack_ptr = (void**)fiber->stack_top;
    
    // Push return address (fiber_entry_wrapper)
    stack_ptr--;
    *stack_ptr = (void*)fiber_entry_wrapper;
    
    // Set up initial context
    fiber->context.rsp = stack_ptr;
    fiber->context.rbp = stack_ptr;
    fiber->context.rip = (void*)fiber_entry_wrapper;
    
    return fiber;
}

void fiber_destroy(fiber_t* fiber) {
    if (fiber) {
        if (fiber->stack_base) {
            free(fiber->stack_base);
        }
        free(fiber);
    }
}

// Assembly implementation of fiber_switch (x86-64)
// Saves callee-saved registers and switches stacks
__attribute__((naked))
void fiber_switch(fiber_t* from, fiber_t* to) {
    __asm__ __volatile__(
        // Save current fiber's context (from->context)
        "movq %%rsp, 0(%rdi)\n"   // Save RSP
        "movq %%rbp, 8(%rdi)\n"   // Save RBP
        "movq %%rbx, 16(%rdi)\n"  // Save RBX
        "movq %%r12, 24(%rdi)\n"  // Save R12
        "movq %%r13, 32(%rdi)\n"  // Save R13
        "movq %%r14, 40(%rdi)\n"  // Save R14
        "movq %%r15, 48(%rdi)\n"  // Save R15
        
        // Save return address as RIP
        "movq 0(%%rsp), %%rax\n"
        "movq %%rax, 56(%rdi)\n"  // Save RIP
        
        // Restore target fiber's context (to->context)
        "movq 0(%rsi), %%rsp\n"   // Restore RSP
        "movq 8(%rsi), %%rbp\n"   // Restore RBP
        "movq 16(%rsi), %%rbx\n"  // Restore RBX
        "movq 24(%rsi), %%r12\n"  // Restore R12
        "movq 32(%rsi), %%r13\n"  // Restore R13
        "movq 40(%rsi), %%r14\n"  // Restore R14
        "movq 48(%rsi), %%r15\n"  // Restore R15
        
        // Jump to saved RIP
        "movq 56(%rsi), %%rax\n"
        "jmpq *%%rax\n"
        :
        :
        : "memory"
    );
}

// Fiber entry wrapper - called when fiber first runs
static void fiber_entry_wrapper(void) {
    fiber_t* fiber = g_current_fiber;
    if (fiber && fiber->entry) {
        // Call user's entry function
        fiber->entry(fiber->arg);
        
        // Mark fiber as dead when entry returns
        fiber->state = FIBER_STATE_DEAD;
    }
    
    // BUG FIX 1 (P0): Yield back to scheduler instead of spinning forever
    // When a fiber completes, we must return control to the scheduler
    // so it can schedule the next fiber or return to the caller.
    // The old code had an infinite pause loop here which caused hangs.
    if (g_current_scheduler) {
        fiber_scheduler_yield(g_current_scheduler);
    }
    
    // Should never reach here if scheduler is set properly
    // If we do, infinite loop to prevent undefined behavior
    while (1) {
        __asm__ __volatile__("pause");
    }
}

uint64_t fiber_current_id(void) {
    return g_current_fiber ? g_current_fiber->fiber_id : 0;
}

const char* fiber_state_to_string(fiber_state_t state) {
    switch (state) {
        case FIBER_STATE_READY: return "READY";
        case FIBER_STATE_RUNNING: return "RUNNING";
        case FIBER_STATE_BLOCKED: return "BLOCKED";
        case FIBER_STATE_DEAD: return "DEAD";
        default: return "UNKNOWN";
    }
}

// Internal function to set current fiber (used by scheduler)
void fiber_set_current(fiber_t* fiber) {
    g_current_fiber = fiber;
}

// Internal function to get current fiber (used by scheduler)
fiber_t* fiber_get_current(void) {
    return g_current_fiber;
}

// Internal function to set current scheduler (used by scheduler)
void fiber_set_scheduler(void* scheduler) {
    g_current_scheduler = scheduler;
}
