
/**
 * @file syscall_table.c
 * @brief System Call Dispatch Table
 * 
 * Phase 11: System Call Interface - Day 1
 * 
 * This file implements the syscall dispatch table that maps syscall numbers
 * to their handler functions. It includes validation, statistics tracking,
 * and efficient dispatch mechanisms.
 * 
 * Key Features:
 * - Fast O(1) syscall dispatch via direct table lookup
 * - Per-syscall statistics tracking (call count, errors, timing)
 * - Bounds checking and validation
 * - Support for fast path and zero-copy syscalls
 * - Integration with vDSO for common syscalls
 */

#include "syscalls.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ===================================================================
 * Global Syscall Table
 * =================================================================== */

/**
 * @brief Global syscall dispatch table
 * 
 * This table maps syscall numbers to their handler functions.
 * Initialized at boot time by syscall_init().
 */
static syscall_entry_t syscall_table[SYSCALL_COUNT] = {0};

/**
 * @brief Syscall table initialization flag
 */
static _Atomic(bool) syscall_table_initialized = false;

/* ===================================================================
 * Helper Functions
 * =================================================================== */

/**
 * @brief Get current time in nanoseconds
 * 
 * @return Current time in nanoseconds
 */
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Update syscall statistics
 * 
 * @param stats Statistics structure to update
 * @param start_time_ns Start time in nanoseconds
 * @param error Whether the syscall resulted in an error
 */
static void update_stats(syscall_stats_t *stats, uint64_t start_time_ns, bool error) {
    if (stats == nullptr) {
        return;
    }
    
    uint64_t end_time_ns = get_time_ns();
    uint64_t elapsed_ns = end_time_ns - start_time_ns;
    
    /* Update call count */
    atomic_fetch_add_explicit(&stats->call_count, 1, memory_order_relaxed);
    
    /* Update error count if applicable */
    if (error) {
        atomic_fetch_add_explicit(&stats->error_count, 1, memory_order_relaxed);
    }
    
    /* Update timing statistics */
    atomic_fetch_add_explicit(&stats->total_time_ns, elapsed_ns, memory_order_relaxed);
    
    /* Update min time (using compare-exchange loop) */
    uint64_t current_min = atomic_load_explicit(&stats->min_time_ns, memory_order_relaxed);
    while (elapsed_ns < current_min || current_min == 0) {
        if (atomic_compare_exchange_weak_explicit(&stats->min_time_ns, &current_min, elapsed_ns,
                                                   memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
    
    /* Update max time (using compare-exchange loop) */
    uint64_t current_max = atomic_load_explicit(&stats->max_time_ns, memory_order_relaxed);
    while (elapsed_ns > current_max) {
        if (atomic_compare_exchange_weak_explicit(&stats->max_time_ns, &current_max, elapsed_ns,
                                                   memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

/* ===================================================================
 * Syscall Table Registration
 * =================================================================== */

/**
 * @brief Register a syscall handler in the table
 * 
 * @param num Syscall number
 * @param handler Handler function
 * @param name Syscall name (for debugging)
 * @param flags Syscall flags
 * @return 0 on success, negative errno on failure
 */
int register_syscall(uint32_t num, syscall_handler_t handler, 
                    const char *name, uint32_t flags) {
    if (num >= SYSCALL_COUNT) {
        fprintf(stderr, "syscall_table: Invalid syscall number %u\n", num);
        return -EINVAL;
    }
    
    if (handler == nullptr) {
        fprintf(stderr, "syscall_table: nullptr handler for syscall %u (%s)\n", num, name);
        return -EINVAL;
    }
    
    syscall_table[num].handler = handler;
    syscall_table[num].name = name;
    syscall_table[num].flags = flags;
    
    /* Initialize statistics to zero */
    atomic_store_explicit(&syscall_table[num].stats.call_count, 0, memory_order_relaxed);
    atomic_store_explicit(&syscall_table[num].stats.error_count, 0, memory_order_relaxed);
    atomic_store_explicit(&syscall_table[num].stats.total_time_ns, 0, memory_order_relaxed);
    atomic_store_explicit(&syscall_table[num].stats.min_time_ns, UINT64_MAX, memory_order_relaxed);
    atomic_store_explicit(&syscall_table[num].stats.max_time_ns, 0, memory_order_relaxed);
    
    return 0;
}

/* ===================================================================
 * Syscall Table Initialization
 * =================================================================== */

/**
 * @brief Initialize the syscall table
 * 
 * Registers all syscall handlers with their respective numbers.
 * This function must be called during kernel initialization.
 * 
 * @return 0 on success, negative errno on failure
 */
int syscall_init(void) {
    /* Check if already initialized */
    bool expected = false;
    if (!atomic_compare_exchange_strong_explicit(&syscall_table_initialized, &expected, true,
                                                  memory_order_acquire, memory_order_relaxed)) {
        fprintf(stderr, "syscall_table: Already initialized\n");
        return -EBUSY;
    }
    
    printf("syscall_table: Initializing syscall table with %d syscalls\n", SYSCALL_COUNT);
    
    /* Register Process Management Syscalls */
    register_syscall(SYS_fork, sys_fork, "fork", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_exec, sys_exec, "exec", 0);
    register_syscall(SYS_exit, sys_exit, "exit", 0);
    register_syscall(SYS_wait, sys_wait, "wait", 0);
    register_syscall(SYS_waitpid, sys_waitpid, "waitpid", 0);
    register_syscall(SYS_getpid, sys_getpid, "getpid", SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_getppid, sys_getppid, "getppid", SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_kill, sys_kill, "kill", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_signal, sys_signal, "signal", 0);
    register_syscall(SYS_getpriority, sys_getpriority, "getpriority", SYSCALL_FLAG_READONLY);
    
    /* Register File I/O Syscalls */
    register_syscall(SYS_open, sys_open, "open", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_close, sys_close, "close", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_read, sys_read, "read", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_write, sys_write, "write", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_lseek, sys_lseek, "lseek", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_stat, sys_stat, "stat", SYSCALL_FLAG_READONLY);
    register_syscall(SYS_fstat, sys_fstat, "fstat", SYSCALL_FLAG_READONLY);
    register_syscall(SYS_access, sys_access, "access", SYSCALL_FLAG_READONLY);
    register_syscall(SYS_dup, sys_dup, "dup", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_dup2, sys_dup2, "dup2", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_ioctl, sys_ioctl, "ioctl", 0);
    
    /* Register Directory Operations */
    register_syscall(SYS_mkdir, sys_mkdir, "mkdir", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_rmdir, sys_rmdir, "rmdir", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_chdir, sys_chdir, "chdir", 0);
    register_syscall(SYS_getcwd, sys_getcwd, "getcwd", SYSCALL_FLAG_READONLY);
    register_syscall(SYS_unlink, sys_unlink, "unlink", SYSCALL_FLAG_BATCHABLE);
    
    /* Register Memory Management Syscalls */
    register_syscall(SYS_mmap, sys_mmap, "mmap", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_munmap, sys_munmap, "munmap", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_mprotect, sys_mprotect, "mprotect", SYSCALL_FLAG_BATCHABLE);
    register_syscall(SYS_brk, sys_brk, "brk", 0);
    
    /* Register IPC Syscalls */
    register_syscall(SYS_pipe, sys_pipe, "pipe", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_pipe2, sys_pipe2, "pipe2", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_socket, sys_socket, "socket", 0);
    register_syscall(SYS_shm_open, sys_shm_open, "shm_open", SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_shm_close, sys_shm_close, "shm_close", 0);
    
    /* Register Time Syscalls */
    register_syscall(SYS_gettimeofday, sys_gettimeofday, "gettimeofday", 
                    SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_clock_gettime, sys_clock_gettime, "clock_gettime", 
                    SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_nanosleep, sys_nanosleep, "nanosleep", 0);
    
    /* Register Fast Path Syscalls */
    register_syscall(SYS_vdso_getpid, sys_vdso_getpid, "vdso_getpid", 
                    SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_vdso_gettimeofday, sys_vdso_gettimeofday, "vdso_gettimeofday", 
                    SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_vdso_clock_gettime, sys_vdso_clock_gettime, "vdso_clock_gettime", 
                    SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    register_syscall(SYS_batch, sys_batch, "batch", 0);
    register_syscall(SYS_zerocopy_read, sys_zerocopy_read, "zerocopy_read", 
                    SYSCALL_FLAG_ZERO_COPY);
    register_syscall(SYS_zerocopy_write, sys_zerocopy_write, "zerocopy_write", 
                    SYSCALL_FLAG_ZERO_COPY);
    
    printf("syscall_table: Base initialization complete\n");
    
    /* Register extended syscalls (all 108 syscalls) */
    int ext_result = syscall_register_extended();
    if (ext_result != 0) {
        fprintf(stderr, "syscall_table: Failed to register extended syscalls: %d\n", ext_result);
        return ext_result;
    }
    
    printf("syscall_table: Full initialization complete with all %d syscalls\n", SYSCALL_COUNT);
    return 0;
}

/* ===================================================================
 * Syscall Dispatch
 * =================================================================== */

/**
 * @brief Dispatch a system call
 * 
 * This is the main entry point for all system calls. It performs
 * validation, calls the appropriate handler, and updates statistics.
 * 
 * @param syscall_num System call number
 * @param args Syscall arguments
 * @return Syscall result (negative errno on error)
 */
int64_t syscall_dispatch(uint32_t syscall_num, const syscall_args_t *args) {
    /* Validate syscall number */
    if (syscall_num >= SYSCALL_COUNT) {
        fprintf(stderr, "syscall_dispatch: Invalid syscall number %u\n", syscall_num);
        return -ENOSYS;
    }
    
    /* Check if syscall table is initialized */
    if (!atomic_load_explicit(&syscall_table_initialized, memory_order_acquire)) {
        fprintf(stderr, "syscall_dispatch: Syscall table not initialized\n");
        return -EAGAIN;
    }
    
    /* Get syscall entry */
    syscall_entry_t *entry = &syscall_table[syscall_num];
    
    /* Validate handler */
    if (entry->handler == nullptr) {
        fprintf(stderr, "syscall_dispatch: No handler for syscall %u\n", syscall_num);
        return -ENOSYS;
    }
    
    /* Validate arguments */
    if (args == nullptr) {
        fprintf(stderr, "syscall_dispatch: nullptr arguments for syscall %u (%s)\n", 
                syscall_num, entry->name);
        return -EFAULT;
    }
    
    /* Record start time for statistics */
    uint64_t start_time_ns = get_time_ns();
    
    /* Call the handler */
    int64_t result = entry->handler(args);
    
    /* Update statistics */
    update_stats(&entry->stats, start_time_ns, result < 0);
    
    return result;
}

/* ===================================================================
 * Syscall Information Functions
 * =================================================================== */

/**
 * @brief Get syscall statistics
 * 
 * @param syscall_num System call number
 * @return Pointer to statistics, or nullptr if invalid
 */
const syscall_stats_t *syscall_get_stats(uint32_t syscall_num) {
    if (syscall_num >= SYSCALL_COUNT) {
        return nullptr;
    }
    
    return &syscall_table[syscall_num].stats;
}

/**
 * @brief Get syscall name
 * 
 * @param syscall_num System call number
 * @return Syscall name, or nullptr if invalid
 */
const char *syscall_get_name(uint32_t syscall_num) {
    if (syscall_num >= SYSCALL_COUNT) {
        return nullptr;
    }
    
    return syscall_table[syscall_num].name;
}

/**
 * @brief Print syscall statistics
 * 
 * Prints statistics for all syscalls to stdout.
 * Useful for debugging and performance analysis.
 */
void syscall_print_stats(void) {
    printf("\n=== Syscall Statistics ===\n");
    printf("%-5s %-25s %-12s %-12s %-12s %-12s %-12s\n",
           "Num", "Name", "Calls", "Errors", "Avg(ns)", "Min(ns)", "Max(ns)");
    printf("--------------------------------------------------------------------------------\n");
    
    for (uint32_t i = 0; i < SYSCALL_COUNT; i++) {
        if (syscall_table[i].handler == nullptr) {
            continue;
        }
        
        const syscall_stats_t *stats = &syscall_table[i].stats;
        uint64_t call_count = atomic_load_explicit(&stats->call_count, memory_order_relaxed);
        
        if (call_count == 0) {
            continue;
        }
        
        uint64_t error_count = atomic_load_explicit(&stats->error_count, memory_order_relaxed);
        uint64_t total_time = atomic_load_explicit(&stats->total_time_ns, memory_order_relaxed);
        uint64_t min_time = atomic_load_explicit(&stats->min_time_ns, memory_order_relaxed);
        uint64_t max_time = atomic_load_explicit(&stats->max_time_ns, memory_order_relaxed);
        uint64_t avg_time = total_time / call_count;
        
        printf("%-5u %-25s %-12lu %-12lu %-12lu %-12lu %-12lu\n",
               i, syscall_table[i].name, call_count, error_count, avg_time, min_time, max_time);
    }
    
    printf("================================================================================\n");
}
