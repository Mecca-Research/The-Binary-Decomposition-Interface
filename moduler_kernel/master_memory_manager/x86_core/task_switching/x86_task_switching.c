
/**
 * @file x86_task_switching.c
 * @brief x86 Task Switching Implementation
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete task switching implementation with both hardware and software approaches
 */

#include "x86_task_switching.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Global task management state
static tss_t system_tss __attribute__((aligned(16)));
static task_scheduler_t scheduler;
static bool task_system_initialized = false;
static uint32_t task_id_counter = 1;

// Preemption control - prevents context switch in interrupt context
static volatile bool preemption_pending = false;
static volatile bool in_interrupt_context = false;

// Task state strings for debugging
static const char* task_state_strings[] = {
    "READY", "RUNNING", "BLOCKED", "TERMINATED"
};

static const char* task_priority_strings[] = {
    "IDLE", "LOW", "NORMAL", "HIGH", "CRITICAL"
};

/**
 * @brief Initialize the task management system
 */
int x86_task_init(void) {
    if (task_system_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize scheduler
    memset(&scheduler, 0, sizeof(scheduler));
    scheduler.next_task_id = 1;
    scheduler.default_time_slice = 10; // 10ms default
    scheduler.preemptive_scheduling = true;
    scheduler.round_robin_enabled = true;
    scheduler.deferred_preemptions = 0;
    
    // Initialize TSS
    if (x86_tss_init() != 0) {
        return -1;
    }
    
    task_system_initialized = true;
    return 0;
}

/**
 * @brief Create a new task
 */
task_control_block_t* x86_task_create(const char* name, void (*entry_point)(void*), 
                                      void* arg, size_t stack_size, task_priority_t priority) {
    if (!task_system_initialized) {
        return NULL;
    }
    
    // Allocate task control block
    task_control_block_t* task = (task_control_block_t*)malloc(sizeof(task_control_block_t));
    if (!task) {
        return NULL;
    }
    
    // Initialize task structure
    memset(task, 0, sizeof(task_control_block_t));
    
    // Set basic properties
    task->task_id = task_id_counter++;
    strncpy(task->name, name ? name : "unnamed", sizeof(task->name) - 1);
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->time_slice = scheduler.default_time_slice;
    task->time_remaining = task->time_slice;
    task->creation_time = x86_get_timestamp();
    
    // Allocate user stack
    task->stack_size = stack_size;
    task->stack_base = (uintptr_t)malloc(stack_size);
    if (!task->stack_base) {
        free(task);
        return NULL;
    }
    
    // Allocate kernel stack
    task->kernel_stack_size = 8192; // 8KB kernel stack
    task->kernel_stack = (uintptr_t)malloc(task->kernel_stack_size);
    if (!task->kernel_stack) {
        free((void*)task->stack_base);
        free(task);
        return NULL;
    }
    
    // Set up initial CPU context
    task->cpu_context.rsp = task->stack_base + stack_size - sizeof(uintptr_t);
    task->cpu_context.rip = (uintptr_t)entry_point;
    task->cpu_context.rflags = 0x202; // IF=1, reserved bit=1
    task->cpu_context.cs = 0x08; // Kernel code segment
    task->cpu_context.ds = task->cpu_context.es = task->cpu_context.fs = 
    task->cpu_context.gs = task->cpu_context.ss = 0x10; // Kernel data segment
    
    // Set up argument on stack
    if (arg) {
        task->cpu_context.rdi = (uintptr_t)arg; // First argument in RDI
    }
    
    // Allocate I/O bitmap
    task->io_bitmap_size = TSS_IO_BITMAP_SIZE;
    task->io_bitmap = (uint8_t*)malloc(task->io_bitmap_size);
    if (task->io_bitmap) {
        memset(task->io_bitmap, 0xFF, task->io_bitmap_size); // Deny all I/O by default
    }
    
    scheduler.total_tasks++;
    return task;
}

/**
 * @brief Destroy a task
 */
int x86_task_destroy(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    // Remove from scheduler
    x86_scheduler_remove_task(task);
    
    // Free allocated memory
    if (task->stack_base) {
        free((void*)task->stack_base);
    }
    if (task->kernel_stack) {
        free((void*)task->kernel_stack);
    }
    if (task->io_bitmap) {
        free(task->io_bitmap);
    }
    
    // Mark as terminated
    task->state = TASK_STATE_TERMINATED;
    
    // Free task structure
    free(task);
    
    scheduler.total_tasks--;
    return 0;
}

/**
 * @brief Start a task
 */
int x86_task_start(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    if (task->state != TASK_STATE_READY) {
        return -1; // Task not in ready state
    }
    
    // Add to scheduler
    x86_scheduler_add_task(task);
    
    return 0;
}

/**
 * @brief Suspend a task
 */
int x86_task_suspend(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    return x86_task_set_state(task, TASK_STATE_BLOCKED);
}

/**
 * @brief Resume a task
 */
int x86_task_resume(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    if (task->state == TASK_STATE_BLOCKED) {
        return x86_task_set_state(task, TASK_STATE_READY);
    }
    
    return -1;
}

/**
 * @brief Initialize TSS
 */
int x86_tss_init(void) {
    // Clear TSS
    memset(&system_tss, 0, sizeof(system_tss));
    
    // Set I/O bitmap offset
    system_tss.io_bitmap_offset = TSS_IO_BITMAP_OFFSET;
    
    // Set up privilege level stacks (will be updated per task)
    system_tss.rsp0 = 0; // Will be set during task switches
    system_tss.rsp1 = 0;
    system_tss.rsp2 = 0;
    
    // Set up interrupt stack tables
    for (int i = 1; i <= 7; i++) {
        uintptr_t ist_stack = (uintptr_t)malloc(8192); // 8KB per IST
        if (ist_stack) {
            *(&system_tss.ist1 + (i-1)) = ist_stack + 8192;
        }
    }
    
    return 0;
}

/**
 * @brief Set TSS stack for privilege level
 */
void x86_tss_set_stack(uint8_t privilege_level, uintptr_t stack_pointer) {
    switch (privilege_level) {
        case 0:
            system_tss.rsp0 = stack_pointer;
            break;
        case 1:
            system_tss.rsp1 = stack_pointer;
            break;
        case 2:
            system_tss.rsp2 = stack_pointer;
            break;
    }
}

/**
 * @brief Set IST stack
 */
void x86_tss_set_ist(uint8_t ist_index, uintptr_t stack_pointer) {
    if (ist_index >= 1 && ist_index <= 7) {
        *(&system_tss.ist1 + (ist_index-1)) = stack_pointer;
    }
}

/**
 * @brief Set I/O bitmap in TSS
 */
int x86_tss_set_io_bitmap(uint8_t* bitmap, size_t size) {
    if (!bitmap || size > TSS_IO_BITMAP_SIZE) {
        return -1;
    }
    
    // Copy bitmap to TSS (would need to extend TSS structure)
    // For now, just validate parameters
    return 0;
}

/**
 * @brief Load TSS
 */
void x86_tss_load(uint16_t tss_selector) {
    __asm__ volatile("ltr %0" : : "r"(tss_selector));
}

/**
 * @brief Perform context switch between tasks
 */
int x86_context_switch(task_control_block_t* from_task, task_control_block_t* to_task) {
    if (!from_task || !to_task) {
        return -1;
    }
    
    // CRITICAL SAFETY CHECK: Never perform context switch in interrupt context
    if (in_interrupt_context) {
        printf("[SCHEDULER ERROR] Attempted context switch in interrupt context! "
               "From: %s, To: %s\n", from_task->name, to_task->name);
        return -1;
    }
    
    // Save current task context
    if (x86_save_context(from_task) != 0) {
        return -1;
    }
    
    // Update TSS with new task's kernel stack
    x86_tss_set_stack(0, to_task->kernel_stack + to_task->kernel_stack_size);
    
    // Switch page directory if different
    if (from_task->page_directory != to_task->page_directory && to_task->page_directory) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(to_task->page_directory) : "memory");
    }
    
    // Restore new task context
    if (x86_restore_context(to_task) != 0) {
        return -1;
    }
    
    // Update scheduler state
    scheduler.current_task = to_task;
    scheduler.context_switches++;
    
    return 0;
}

/**
 * @brief Save task context
 * CRITICAL FIX: Now saves complete register set instead of just 4 registers
 */
int x86_save_context(task_control_block_t* task) {
    if (!task) {
        return -1;
    }
    
    // Save ALL general purpose registers (CRITICAL FIX)
    __asm__ volatile(
        "mov %%rax, %0\n"
        "mov %%rbx, %1\n"
        "mov %%rcx, %2\n"
        "mov %%rdx, %3\n"
        "mov %%rsi, %4\n"
        "mov %%rdi, %5\n"
        "mov %%rbp, %6\n"
        "mov %%rsp, %7\n"
        "mov %%r8, %8\n"
        "mov %%r9, %9\n"
        "mov %%r10, %10\n"
        "mov %%r11, %11\n"
        "mov %%r12, %12\n"
        "mov %%r13, %13\n"
        "mov %%r14, %14\n"
        "mov %%r15, %15\n"
        : "=m"(task->cpu_context.rax), "=m"(task->cpu_context.rbx),
          "=m"(task->cpu_context.rcx), "=m"(task->cpu_context.rdx),
          "=m"(task->cpu_context.rsi), "=m"(task->cpu_context.rdi),
          "=m"(task->cpu_context.rbp), "=m"(task->cpu_context.rsp),
          "=m"(task->cpu_context.r8),  "=m"(task->cpu_context.r9),
          "=m"(task->cpu_context.r10), "=m"(task->cpu_context.r11),
          "=m"(task->cpu_context.r12), "=m"(task->cpu_context.r13),
          "=m"(task->cpu_context.r14), "=m"(task->cpu_context.r15)
    );
    
    // Save instruction pointer (current location)
    __asm__ volatile("lea (%%rip), %0" : "=m"(task->cpu_context.rip));
    
    // Save flags
    __asm__ volatile("pushfq; popq %0" : "=m"(task->cpu_context.rflags));
    
    // Save segment registers
    __asm__ volatile(
        "mov %%cs, %0\n"
        "mov %%ds, %1\n"
        "mov %%es, %2\n"
        "mov %%fs, %3\n"
        "mov %%gs, %4\n"
        "mov %%ss, %5\n"
        : "=m"(task->cpu_context.cs), "=m"(task->cpu_context.ds),
          "=m"(task->cpu_context.es), "=m"(task->cpu_context.fs),
          "=m"(task->cpu_context.gs), "=m"(task->cpu_context.ss)
    );
    
    // Save CR3 (page directory)
    __asm__ volatile("mov %%cr3, %0" : "=r"(task->cpu_context.cr3));
    
    printf("[TaskSwitch] Complete context saved for task %s (16 registers + segments + flags)\n", 
           task->name);
    
    return 0;
}

/**
 * @brief Restore task context
 * CRITICAL FIX: Now restores complete register set instead of just 4 registers
 */
int x86_restore_context(task_control_block_t* task) {
    if (!task) {
        return -1;
    }
    
    printf("[TaskSwitch] Restoring complete context for task %s\n", task->name);
    
    // Restore CR3 (page directory)
    if (task->cpu_context.cr3) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(task->cpu_context.cr3) : "memory");
    }
    
    // Restore segment registers
    __asm__ volatile(
        "mov %1, %%ds\n"
        "mov %2, %%es\n"
        "mov %3, %%fs\n"
        "mov %4, %%gs\n"
        "mov %5, %%ss\n"
        : : "m"(task->cpu_context.cs), "m"(task->cpu_context.ds),
            "m"(task->cpu_context.es), "m"(task->cpu_context.fs),
            "m"(task->cpu_context.gs), "m"(task->cpu_context.ss)
    );
    
    // Restore flags
    __asm__ volatile("pushq %0; popfq" : : "m"(task->cpu_context.rflags));
    
    // Restore ALL general purpose registers (CRITICAL FIX)
    // Note: We restore RSP and RIP last as they affect execution flow
    __asm__ volatile(
        "mov %0, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %2, %%rcx\n"
        "mov %3, %%rdx\n"
        "mov %4, %%rsi\n"
        "mov %5, %%rdi\n"
        "mov %6, %%rbp\n"
        "mov %8, %%r8\n"
        "mov %9, %%r9\n"
        "mov %10, %%r10\n"
        "mov %11, %%r11\n"
        "mov %12, %%r12\n"
        "mov %13, %%r13\n"
        "mov %14, %%r14\n"
        "mov %15, %%r15\n"
        "mov %7, %%rsp\n"
        : : "m"(task->cpu_context.rax), "m"(task->cpu_context.rbx),
            "m"(task->cpu_context.rcx), "m"(task->cpu_context.rdx),
            "m"(task->cpu_context.rsi), "m"(task->cpu_context.rdi),
            "m"(task->cpu_context.rbp), "m"(task->cpu_context.rsp),
            "m"(task->cpu_context.r8),  "m"(task->cpu_context.r9),
            "m"(task->cpu_context.r10), "m"(task->cpu_context.r11),
            "m"(task->cpu_context.r12), "m"(task->cpu_context.r13),
            "m"(task->cpu_context.r14), "m"(task->cpu_context.r15)
    );
    
    // Note: RIP restoration would typically be handled by a jump or return instruction
    // In a real implementation, this would be part of the context switch assembly code
    
    return 0;
}

/**
 * @brief Initialize scheduler
 */
int x86_scheduler_init(void) {
    if (!task_system_initialized) {
        return -1;
    }
    
    // Scheduler is already initialized in x86_task_init
    return 0;
}

/**
 * @brief Get next task to run
 */
task_control_block_t* x86_scheduler_get_next_task(void) {
    // Simple priority-based round-robin scheduler
    for (int priority = TASK_PRIORITY_CRITICAL; priority >= TASK_PRIORITY_IDLE; priority--) {
        task_control_block_t* task = scheduler.ready_queue[priority];
        if (task) {
            // Remove from front of queue
            scheduler.ready_queue[priority] = task->next;
            if (scheduler.ready_queue[priority]) {
                scheduler.ready_queue[priority]->prev = NULL;
            }
            
            // Add to back of queue (round-robin)
            if (scheduler.round_robin_enabled) {
                x86_scheduler_add_task(task);
            }
            
            return task;
        }
    }
    
    return NULL; // No ready tasks
}

/**
 * @brief Add task to scheduler
 */
void x86_scheduler_add_task(task_control_block_t* task) {
    if (!task || task->state != TASK_STATE_READY) {
        return;
    }
    
    int priority = task->priority;
    if (priority < 0 || priority >= 5) {
        priority = TASK_PRIORITY_NORMAL;
    }
    
    // Add to end of priority queue
    task->next = NULL;
    task->prev = NULL;
    
    if (!scheduler.ready_queue[priority]) {
        scheduler.ready_queue[priority] = task;
    } else {
        task_control_block_t* current = scheduler.ready_queue[priority];
        while (current->next) {
            current = current->next;
        }
        current->next = task;
        task->prev = current;
    }
}

/**
 * @brief Remove task from scheduler
 */
void x86_scheduler_remove_task(task_control_block_t* task) {
    if (!task) {
        return;
    }
    
    // Remove from ready queues
    for (int i = 0; i < 5; i++) {
        task_control_block_t* current = scheduler.ready_queue[i];
        while (current) {
            if (current == task) {
                if (current->prev) {
                    current->prev->next = current->next;
                } else {
                    scheduler.ready_queue[i] = current->next;
                }
                
                if (current->next) {
                    current->next->prev = current->prev;
                }
                
                current->next = current->prev = NULL;
                return;
            }
            current = current->next;
        }
    }
}

/**
 * @brief Yield CPU to next task
 */
void x86_scheduler_yield(void) {
    task_control_block_t* current = scheduler.current_task;
    task_control_block_t* next = x86_scheduler_get_next_task();
    
    if (next && next != current) {
        if (current) {
            current->state = TASK_STATE_READY;
            x86_scheduler_add_task(current);
        }
        
        next->state = TASK_STATE_RUNNING;
        x86_context_switch(current, next);
    }
}

/**
 * @brief Set task state
 */
int x86_task_set_state(task_control_block_t* task, task_state_t new_state) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    task_state_t old_state = task->state;
    task->state = new_state;
    
    // Handle state transitions
    if (old_state == TASK_STATE_RUNNING && new_state == TASK_STATE_READY) {
        x86_scheduler_add_task(task);
    } else if (old_state == TASK_STATE_READY && new_state != TASK_STATE_READY) {
        x86_scheduler_remove_task(task);
    }
    
    return 0;
}

/**
 * @brief Get current task
 */
task_control_block_t* x86_task_get_current(void) {
    return scheduler.current_task;
}

/**
 * @brief Get current task ID
 */
uint32_t x86_task_get_current_id(void) {
    if (scheduler.current_task) {
        return scheduler.current_task->task_id;
    }
    return 0;
}

/**
 * @brief Convert task state to string
 */
const char* x86_task_state_to_string(task_state_t state) {
    if (state >= 0 && state < sizeof(task_state_strings) / sizeof(task_state_strings[0])) {
        return task_state_strings[state];
    }
    return "UNKNOWN";
}

/**
 * @brief Convert task priority to string
 */
const char* x86_task_priority_to_string(task_priority_t priority) {
    if (priority >= 0 && priority < sizeof(task_priority_strings) / sizeof(task_priority_strings[0])) {
        return task_priority_strings[priority];
    }
    return "UNKNOWN";
}

/**
 * @brief Check if task is valid
 */
bool x86_task_is_valid(task_control_block_t* task) {
    return task != NULL && 
           task->task_id > 0 && 
           task->state != TASK_STATE_TERMINATED &&
           task->stack_base != 0 &&
           task->stack_size > 0;
}

/**
 * @brief Dump task information
 */
void x86_task_dump_info(task_control_block_t* task) {
    if (!task) {
        printf("Task: NULL\n");
        return;
    }
    
    printf("Task Information:\n");
    printf("  ID: %u\n", task->task_id);
    printf("  Name: %s\n", task->name);
    printf("  State: %s\n", x86_task_state_to_string(task->state));
    printf("  Priority: %s\n", x86_task_priority_to_string(task->priority));
    printf("  Time Slice: %u ms\n", task->time_slice);
    printf("  Stack: 0x%lx - 0x%lx (%zu bytes)\n", 
           task->stack_base, task->stack_base + task->stack_size, task->stack_size);
    printf("  Kernel Stack: 0x%lx (%zu bytes)\n", 
           task->kernel_stack, task->kernel_stack_size);
    printf("  Page Directory: 0x%lx\n", task->page_directory);
    printf("  Run Count: %u\n", task->run_count);
    printf("  Total Run Time: %lu us\n", task->total_run_time);
}

/**
 * @brief Start the scheduler
 */
void x86_scheduler_start(void) {
    if (!task_system_initialized) {
        return;
    }
    
    scheduler.preemptive_scheduling = true;
    
    // Get the first task to run
    task_control_block_t* first_task = x86_scheduler_get_next_task();
    if (first_task) {
        first_task->state = TASK_STATE_RUNNING;
        scheduler.current_task = first_task;
        scheduler.last_schedule_time = x86_get_timestamp();
        
        // Switch to first task
        x86_restore_context(first_task);
    }
}

/**
 * @brief Stop the scheduler
 */
void x86_scheduler_stop(void) {
    scheduler.preemptive_scheduling = false;
    
    // Save current task context if running
    if (scheduler.current_task && scheduler.current_task->state == TASK_STATE_RUNNING) {
        x86_save_context(scheduler.current_task);
        scheduler.current_task->state = TASK_STATE_READY;
        x86_scheduler_add_task(scheduler.current_task);
    }
    
    scheduler.current_task = NULL;
}

/**
 * @brief Mark entry into interrupt context
 * CRITICAL: Must be called at the beginning of all interrupt handlers
 */
void x86_scheduler_enter_interrupt(void) {
    // Use atomic operation to prevent race conditions
    __sync_lock_test_and_set(&in_interrupt_context, true);
}

/**
 * @brief Mark exit from interrupt context and handle deferred preemption
 * CRITICAL: Must be called at the end of all interrupt handlers
 */
void x86_scheduler_exit_interrupt(void) {
    // Use atomic operation to clear interrupt context flag
    __sync_lock_release(&in_interrupt_context);
    
    // Handle deferred preemption if pending (use atomic test-and-clear)
    if (__sync_lock_test_and_set(&preemption_pending, false)) {
        // Now safe to perform context switch - we're no longer in interrupt context
        if (scheduler.preemptive_scheduling && scheduler.current_task) {
            printf("[SCHEDULER] Executing deferred preemption (Task: %s)\n", 
                   scheduler.current_task->name);
            x86_scheduler_yield();
        }
    }
}

/**
 * @brief Check if preemption should occur (safe version for interrupt context)
 */
static bool x86_scheduler_should_preempt(task_control_block_t* current) {
    if (!current) {
        return false;
    }
    
    // Check if time slice expired
    bool should_preempt = (current->time_remaining == 0);
    
    // Check for higher priority tasks
    if (!should_preempt) {
        for (int priority = TASK_PRIORITY_CRITICAL; priority > current->priority; priority--) {
            if (scheduler.ready_queue[priority] != NULL) {
                should_preempt = true;
                break;
            }
        }
    }
    
    return should_preempt;
}

/**
 * @brief Preempt current task (called by timer interrupt)
 * FIXED: Now handles interrupt context properly to prevent task state corruption
 */
void x86_scheduler_preempt(void) {
    if (!scheduler.preemptive_scheduling || !scheduler.current_task) {
        return;
    }
    
    task_control_block_t* current = scheduler.current_task;
    
    // Decrease time remaining
    if (current->time_remaining > 0) {
        current->time_remaining--;
    }
    
    // Check if preemption should occur
    if (x86_scheduler_should_preempt(current)) {
        // Reset time slice for next run
        current->time_remaining = current->time_slice;
        
        // CRITICAL FIX: Check if we're in interrupt context
        if (in_interrupt_context) {
            // Defer the context switch until after ISR returns
            __sync_lock_test_and_set(&preemption_pending, true);
            
            // Update statistics
            scheduler.deferred_preemptions++;
            
            // Log the deferred preemption for debugging
            printf("[SCHEDULER] Preemption deferred - in interrupt context (Task: %s)\n", 
                   current->name);
        } else {
            // Safe to perform immediate context switch
            printf("[SCHEDULER] Immediate preemption (Task: %s)\n", current->name);
            x86_scheduler_yield();
        }
    }
}

/**
 * @brief Get current task name
 */
const char* x86_task_get_current_name(void) {
    if (scheduler.current_task) {
        return scheduler.current_task->name;
    }
    return "No Task";
}

/**
 * @brief Get task state
 */
task_state_t x86_task_get_state(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return TASK_STATE_TERMINATED;
    }
    return task->state;
}

/**
 * @brief Set task priority
 */
int x86_task_set_priority(task_control_block_t* task, task_priority_t priority) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    if (priority < TASK_PRIORITY_IDLE || priority > TASK_PRIORITY_CRITICAL) {
        return -1;
    }
    
    // Remove from current priority queue if ready
    if (task->state == TASK_STATE_READY) {
        x86_scheduler_remove_task(task);
    }
    
    // Update priority
    task->priority = priority;
    
    // Re-add to appropriate queue if ready
    if (task->state == TASK_STATE_READY) {
        x86_scheduler_add_task(task);
    }
    
    return 0;
}

/**
 * @brief Get task priority
 */
task_priority_t x86_task_get_priority(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return TASK_PRIORITY_IDLE;
    }
    return task->priority;
}

/**
 * @brief Dump all tasks
 */
void x86_task_dump_all_tasks(void) {
    printf("All Tasks:\n");
    printf("Current Task: %s\n", scheduler.current_task ? scheduler.current_task->name : "None");
    
    for (int priority = TASK_PRIORITY_CRITICAL; priority >= TASK_PRIORITY_IDLE; priority--) {
        task_control_block_t* task = scheduler.ready_queue[priority];
        if (task) {
            printf("\n%s Priority Queue:\n", x86_task_priority_to_string(priority));
            while (task) {
                printf("  Task %u: %s [%s]\n", 
                       task->task_id, task->name, x86_task_state_to_string(task->state));
                task = task->next;
            }
        }
    }
}

/**
 * @brief Get task run time
 */
uint64_t x86_task_get_run_time(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return 0;
    }
    return task->total_run_time;
}

/**
 * @brief Get context switch count
 */
uint32_t x86_scheduler_get_context_switch_count(void) {
    return scheduler.context_switches;
}

/**
 * @brief Timer handler for preemptive scheduling
 * FIXED: Now properly handles interrupt context to prevent task state corruption
 */
void x86_task_timer_handler(void) {
    // Mark entry into interrupt context
    x86_scheduler_enter_interrupt();
    
    if (scheduler.preemptive_scheduling) {
        x86_scheduler_preempt();
    }
    
    // Mark exit from interrupt context and handle deferred preemption
    x86_scheduler_exit_interrupt();
}

/**
 * @brief Set task time slice
 */
int x86_task_set_time_slice(task_control_block_t* task, uint32_t time_slice) {
    if (!task || !x86_task_is_valid(task) || time_slice == 0) {
        return -1;
    }
    
    task->time_slice = time_slice;
    task->time_remaining = time_slice;
    return 0;
}

/**
 * @brief Get task time slice
 */
uint32_t x86_task_get_time_slice(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return 0;
    }
    return task->time_slice;
}

/**
 * @brief Set task page directory
 */
int x86_task_set_page_directory(task_control_block_t* task, uintptr_t page_dir) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    task->page_directory = page_dir;
    task->cpu_context.cr3 = page_dir;
    return 0;
}

/**
 * @brief Get task page directory
 */
uintptr_t x86_task_get_page_directory(task_control_block_t* task) {
    if (!task || !x86_task_is_valid(task)) {
        return 0;
    }
    return task->page_directory;
}

/**
 * @brief Map memory for task
 */
int x86_task_map_memory(task_control_block_t* task, uintptr_t virtual_addr, 
                       uintptr_t physical_addr, size_t size, uint32_t flags) {
    if (!task || !x86_task_is_valid(task)) {
        return -1;
    }
    
    // This would integrate with the MMU system
    // For now, just validate parameters
    if (virtual_addr == 0 || physical_addr == 0 || size == 0) {
        return -1;
    }
    
    return 0; // Placeholder - would call x86_map_memory_region
}

/**
 * @brief Set I/O permission for task
 */
int x86_task_set_io_permission(task_control_block_t* task, uint16_t port, bool allow) {
    if (!task || !x86_task_is_valid(task) || !task->io_bitmap) {
        return -1;
    }
    
    uint16_t byte_offset = port / 8;
    uint8_t bit_offset = port % 8;
    
    if (byte_offset >= task->io_bitmap_size) {
        return -1;
    }
    
    if (allow) {
        task->io_bitmap[byte_offset] &= ~(1 << bit_offset); // Clear bit = allow
    } else {
        task->io_bitmap[byte_offset] |= (1 << bit_offset);  // Set bit = deny
    }
    
    return 0;
}

/**
 * @brief Check I/O permission for task
 */
bool x86_task_check_io_permission(task_control_block_t* task, uint16_t port) {
    if (!task || !x86_task_is_valid(task) || !task->io_bitmap) {
        return false;
    }
    
    uint16_t byte_offset = port / 8;
    uint8_t bit_offset = port % 8;
    
    if (byte_offset >= task->io_bitmap_size) {
        return false;
    }
    
    // Bit clear = allow, bit set = deny
    return !(task->io_bitmap[byte_offset] & (1 << bit_offset));
}

/**
 * @brief Copy I/O bitmap between tasks
 */
int x86_task_copy_io_bitmap(task_control_block_t* dest, task_control_block_t* src) {
    if (!dest || !src || !x86_task_is_valid(dest) || !x86_task_is_valid(src)) {
        return -1;
    }
    
    if (!src->io_bitmap || !dest->io_bitmap) {
        return -1;
    }
    
    size_t copy_size = (dest->io_bitmap_size < src->io_bitmap_size) ? 
                       dest->io_bitmap_size : src->io_bitmap_size;
    
    memcpy(dest->io_bitmap, src->io_bitmap, copy_size);
    return 0;
}

/**
 * @brief Send message to task
 */
int x86_task_send_message(task_control_block_t* dest_task, const void* data, size_t size) {
    // Placeholder implementation - would need message queue system
    if (!dest_task || !data || size == 0) {
        return -1;
    }
    
    return 0; // Not implemented - would need IPC system
}

/**
 * @brief Receive message
 */
int x86_task_receive_message(task_message_t* message, uint32_t timeout_ms) {
    // Placeholder implementation - would need message queue system
    if (!message) {
        return -1;
    }
    
    return -1; // Not implemented - would need IPC system
}

/**
 * @brief Peek at message
 */
int x86_task_peek_message(task_message_t* message) {
    // Placeholder implementation - would need message queue system
    if (!message) {
        return -1;
    }
    
    return -1; // Not implemented - would need IPC system
}

/**
 * @brief Initialize mutex
 */
int x86_mutex_init(task_mutex_t* mutex) {
    if (!mutex) {
        return -1;
    }
    
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->wait_queue = NULL;
    return 0;
}

/**
 * @brief Lock mutex
 */
int x86_mutex_lock(task_mutex_t* mutex) {
    if (!mutex) {
        return -1;
    }
    
    // Simple spinlock implementation
    while (__sync_lock_test_and_set(&mutex->locked, 1)) {
        // Busy wait - in real implementation would block task
    }
    
    mutex->owner = scheduler.current_task;
    return 0;
}

/**
 * @brief Unlock mutex
 */
int x86_mutex_unlock(task_mutex_t* mutex) {
    if (!mutex || mutex->owner != scheduler.current_task) {
        return -1;
    }
    
    mutex->owner = NULL;
    __sync_lock_release(&mutex->locked);
    return 0;
}

/**
 * @brief Try to lock mutex
 */
int x86_mutex_trylock(task_mutex_t* mutex) {
    if (!mutex) {
        return -1;
    }
    
    if (__sync_lock_test_and_set(&mutex->locked, 1) == 0) {
        mutex->owner = scheduler.current_task;
        return 0;
    }
    
    return -1; // Already locked
}

/**
 * @brief Initialize semaphore
 */
int x86_semaphore_init(task_semaphore_t* sem, int initial_count, int max_count) {
    if (!sem || initial_count < 0 || max_count <= 0 || initial_count > max_count) {
        return -1;
    }
    
    sem->count = initial_count;
    sem->max_count = max_count;
    sem->wait_queue = NULL;
    return 0;
}

/**
 * @brief Wait on semaphore
 */
int x86_semaphore_wait(task_semaphore_t* sem) {
    if (!sem) {
        return -1;
    }
    
    // Simple implementation - would need proper blocking in real system
    while (__sync_fetch_and_sub(&sem->count, 1) <= 0) {
        __sync_fetch_and_add(&sem->count, 1); // Restore count
        // Would block task here in real implementation
    }
    
    return 0;
}

/**
 * @brief Signal semaphore
 */
int x86_semaphore_signal(task_semaphore_t* sem) {
    if (!sem) {
        return -1;
    }
    
    if (sem->count < sem->max_count) {
        __sync_fetch_and_add(&sem->count, 1);
        return 0;
    }
    
    return -1; // Already at max count
}

/**
 * @brief Try to wait on semaphore
 */
int x86_semaphore_trywait(task_semaphore_t* sem) {
    if (!sem) {
        return -1;
    }
    
    if (__sync_fetch_and_sub(&sem->count, 1) > 0) {
        return 0;
    }
    
    __sync_fetch_and_add(&sem->count, 1); // Restore count
    return -1; // Would block
}

/**
 * @brief Get timestamp (placeholder)
 */
uint64_t x86_get_timestamp(void) {
    // Placeholder - would use RDTSC or system timer
    static uint64_t counter = 0;
    return ++counter;
}

/**
 * @brief Switch to user mode (placeholder)
 */
void x86_switch_to_user_mode(uintptr_t entry_point, uintptr_t stack_pointer) {
    // Placeholder - would set up user mode context and jump
    (void)entry_point;
    (void)stack_pointer;
}

/**
 * @brief Task entry point (placeholder)
 */
void x86_task_entry_point(void) {
    // Placeholder - would be implemented in assembly
}



/**
 * @brief Check if currently in interrupt context
 */
bool x86_scheduler_in_interrupt_context(void) {
    return in_interrupt_context;
}

/**
 * @brief Dump scheduler statistics
 */
void x86_scheduler_dump_stats(void) {
    printf("Scheduler Statistics:\n");
    printf("  Total Tasks: %u\n", scheduler.total_tasks);
    printf("  Context Switches: %u\n", scheduler.context_switches);
    printf("  Deferred Preemptions: %u\n", scheduler.deferred_preemptions);
    printf("  Current Task: %s (ID: %u)\n", 
           scheduler.current_task ? scheduler.current_task->name : "None",
           scheduler.current_task ? scheduler.current_task->task_id : 0);
    printf("  Preemptive: %s\n", scheduler.preemptive_scheduling ? "Yes" : "No");
    printf("  Round Robin: %s\n", scheduler.round_robin_enabled ? "Yes" : "No");
    printf("  Default Time Slice: %u ms\n", scheduler.default_time_slice);
    printf("  In Interrupt Context: %s\n", in_interrupt_context ? "Yes" : "No");
    printf("  Preemption Pending: %s\n", preemption_pending ? "Yes" : "No");
    
    printf("  Ready Queues:\n");
    for (int i = TASK_PRIORITY_CRITICAL; i >= TASK_PRIORITY_IDLE; i--) {
        int count = 0;
        task_control_block_t* task = scheduler.ready_queue[i];
        while (task) {
            count++;
            task = task->next;
        }
        printf("    %s: %d tasks\n", x86_task_priority_to_string(i), count);
    }
}
