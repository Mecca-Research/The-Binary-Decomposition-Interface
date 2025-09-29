
/**
 * @file x86_task_switching.h
 * @brief x86 Task Switching Implementation (Hardware TSS vs Software Scheduling)
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete task switching implementation with both hardware and software approaches
 */

#ifndef X86_TASK_SWITCHING_H
#define X86_TASK_SWITCHING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Task State Segment (TSS) Constants
#define TSS_SIZE                    104
#define TSS_IO_BITMAP_OFFSET        104
#define TSS_IO_BITMAP_SIZE          8192
#define TSS_TOTAL_SIZE              (TSS_SIZE + TSS_IO_BITMAP_SIZE)

// Task States
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_state_t;

// Task Priority Levels
typedef enum {
    TASK_PRIORITY_IDLE = 0,
    TASK_PRIORITY_LOW = 1,
    TASK_PRIORITY_NORMAL = 2,
    TASK_PRIORITY_HIGH = 3,
    TASK_PRIORITY_CRITICAL = 4
} task_priority_t;

/**
 * @brief Task State Segment (TSS) Structure for x86-64
 */
typedef struct {
    uint32_t reserved1;         // 0x00
    uint64_t rsp0;              // 0x04 - Stack pointer for privilege level 0
    uint64_t rsp1;              // 0x0C - Stack pointer for privilege level 1
    uint64_t rsp2;              // 0x14 - Stack pointer for privilege level 2
    uint64_t reserved2;         // 0x1C
    uint64_t ist1;              // 0x24 - Interrupt Stack Table 1
    uint64_t ist2;              // 0x2C - Interrupt Stack Table 2
    uint64_t ist3;              // 0x34 - Interrupt Stack Table 3
    uint64_t ist4;              // 0x3C - Interrupt Stack Table 4
    uint64_t ist5;              // 0x44 - Interrupt Stack Table 5
    uint64_t ist6;              // 0x4C - Interrupt Stack Table 6
    uint64_t ist7;              // 0x54 - Interrupt Stack Table 7
    uint64_t reserved3;         // 0x5C
    uint16_t reserved4;         // 0x64
    uint16_t io_bitmap_offset;  // 0x66 - I/O Permission Bitmap offset
} __attribute__((packed)) tss_t;

/**
 * @brief Software Task Control Block
 */
typedef struct task_control_block {
    // Task identification
    uint32_t task_id;
    char name[32];
    
    // Task state
    task_state_t state;
    task_priority_t priority;
    uint32_t time_slice;
    uint32_t time_remaining;
    
    // CPU context (saved during context switch)
    struct {
        uint64_t rax, rbx, rcx, rdx;
        uint64_t rsi, rdi, rbp, rsp;
        uint64_t r8, r9, r10, r11;
        uint64_t r12, r13, r14, r15;
        uint64_t rip;
        uint64_t rflags;
        uint16_t cs, ds, es, fs, gs, ss;
        uint64_t cr3;  // Page directory base
    } cpu_context;
    
    // Stack information
    uintptr_t stack_base;
    size_t stack_size;
    uintptr_t kernel_stack;
    size_t kernel_stack_size;
    
    // Memory management
    uintptr_t page_directory;
    
    // Scheduling information
    uint64_t creation_time;
    uint64_t last_run_time;
    uint64_t total_run_time;
    uint32_t run_count;
    
    // Task relationships
    struct task_control_block* parent;
    struct task_control_block* next;
    struct task_control_block* prev;
    
    // I/O permissions (for TSS compatibility)
    uint8_t* io_bitmap;
    size_t io_bitmap_size;
    
    // Task-specific data
    void* task_data;
    
} task_control_block_t;

/**
 * @brief Task Scheduler Structure
 */
typedef struct {
    task_control_block_t* current_task;
    task_control_block_t* ready_queue[5]; // One queue per priority level
    task_control_block_t* blocked_queue;
    
    uint32_t next_task_id;
    uint32_t total_tasks;
    uint32_t context_switches;
    
    // Scheduling parameters
    uint32_t default_time_slice;
    bool preemptive_scheduling;
    bool round_robin_enabled;
    
    // Performance metrics
    uint64_t total_scheduler_time;
    uint64_t last_schedule_time;
    
} task_scheduler_t;

/**
 * @brief Context Switch Frame (used by assembly code)
 */
typedef struct {
    uint64_t r15, r14, r13, r12;
    uint64_t rbp, rbx;
    uint64_t rip;  // Return address
} context_switch_frame_t;

// Core Task Management Functions
int x86_task_init(void);
task_control_block_t* x86_task_create(const char* name, void (*entry_point)(void*), 
                                      void* arg, size_t stack_size, task_priority_t priority);
int x86_task_destroy(task_control_block_t* task);
int x86_task_start(task_control_block_t* task);
int x86_task_suspend(task_control_block_t* task);
int x86_task_resume(task_control_block_t* task);

// Hardware TSS Management
int x86_tss_init(void);
void x86_tss_set_stack(uint8_t privilege_level, uintptr_t stack_pointer);
void x86_tss_set_ist(uint8_t ist_index, uintptr_t stack_pointer);
int x86_tss_set_io_bitmap(uint8_t* bitmap, size_t size);
void x86_tss_load(uint16_t tss_selector);

// Software Context Switching
int x86_context_switch(task_control_block_t* from_task, task_control_block_t* to_task);
int x86_save_context(task_control_block_t* task);
int x86_restore_context(task_control_block_t* task);

// Task Scheduler Functions
int x86_scheduler_init(void);
void x86_scheduler_start(void);
void x86_scheduler_stop(void);
task_control_block_t* x86_scheduler_get_next_task(void);
void x86_scheduler_add_task(task_control_block_t* task);
void x86_scheduler_remove_task(task_control_block_t* task);
void x86_scheduler_yield(void);
void x86_scheduler_preempt(void);

// Task State Management
int x86_task_set_state(task_control_block_t* task, task_state_t new_state);
task_state_t x86_task_get_state(task_control_block_t* task);
int x86_task_set_priority(task_control_block_t* task, task_priority_t priority);
task_priority_t x86_task_get_priority(task_control_block_t* task);

// Current Task Management
task_control_block_t* x86_task_get_current(void);
uint32_t x86_task_get_current_id(void);
const char* x86_task_get_current_name(void);

// Task Synchronization Primitives
typedef struct {
    volatile int locked;
    task_control_block_t* owner;
    task_control_block_t* wait_queue;
} task_mutex_t;

typedef struct {
    volatile int count;
    int max_count;
    task_control_block_t* wait_queue;
} task_semaphore_t;

int x86_mutex_init(task_mutex_t* mutex);
int x86_mutex_lock(task_mutex_t* mutex);
int x86_mutex_unlock(task_mutex_t* mutex);
int x86_mutex_trylock(task_mutex_t* mutex);

int x86_semaphore_init(task_semaphore_t* sem, int initial_count, int max_count);
int x86_semaphore_wait(task_semaphore_t* sem);
int x86_semaphore_signal(task_semaphore_t* sem);
int x86_semaphore_trywait(task_semaphore_t* sem);

// Performance and Debugging
void x86_task_dump_info(task_control_block_t* task);
void x86_scheduler_dump_stats(void);
void x86_task_dump_all_tasks(void);
uint64_t x86_task_get_run_time(task_control_block_t* task);
uint32_t x86_scheduler_get_context_switch_count(void);

// Timer Integration (for preemptive scheduling)
void x86_task_timer_handler(void);
int x86_task_set_time_slice(task_control_block_t* task, uint32_t time_slice);
uint32_t x86_task_get_time_slice(task_control_block_t* task);

// Memory Management Integration
int x86_task_set_page_directory(task_control_block_t* task, uintptr_t page_dir);
uintptr_t x86_task_get_page_directory(task_control_block_t* task);
int x86_task_map_memory(task_control_block_t* task, uintptr_t virtual_addr, 
                       uintptr_t physical_addr, size_t size, uint32_t flags);

// I/O Permission Management
int x86_task_set_io_permission(task_control_block_t* task, uint16_t port, bool allow);
bool x86_task_check_io_permission(task_control_block_t* task, uint16_t port);
int x86_task_copy_io_bitmap(task_control_block_t* dest, task_control_block_t* src);

// Task Communication
typedef struct {
    void* data;
    size_t size;
    task_control_block_t* sender;
    uint64_t timestamp;
} task_message_t;

int x86_task_send_message(task_control_block_t* dest_task, const void* data, size_t size);
int x86_task_receive_message(task_message_t* message, uint32_t timeout_ms);
int x86_task_peek_message(task_message_t* message);

// Assembly functions (implemented in assembly)
extern void x86_task_entry_point(void);
extern uint64_t x86_get_timestamp(void);
extern void x86_switch_to_user_mode(uintptr_t entry_point, uintptr_t stack_pointer);

// Utility functions
const char* x86_task_state_to_string(task_state_t state);
const char* x86_task_priority_to_string(task_priority_t priority);
bool x86_task_is_valid(task_control_block_t* task);

#ifdef __cplusplus
}
#endif

#endif // X86_TASK_SWITCHING_H
