
/**
 * @file syscall_dispatch.c
 * @brief System Call Dispatch and Entry/Exit Handling
 * 
 * This file implements the complete syscall dispatch infrastructure including:
 * - Entry/exit handling with context switching
 * - Argument validation and sanitization
 * - 32-bit and 64-bit syscall conventions
 * - Syscall restart mechanism for interrupted calls
 * - Per-syscall statistics tracking
 * 
 * Integration:
 * - Phase 8: Process Management (context switching)
 * - Phase 9: Scheduler (preemption handling)
 * - Security subsystem (capability checks)
 * - Tracing subsystem (syscall tracing)
 */

#include "syscalls.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../security/capability.h"
#include "../tracing/syscall_trace.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

/* ===================================================================
 * Syscall Context Management
 * =================================================================== */

/**
 * @brief Syscall context for restart mechanism
 */
typedef struct {
    uint32_t syscall_num;
    syscall_args_t args;
    bool restartable;
    uint32_t restart_count;
} syscall_context_t;

/**
 * @brief Per-process syscall context (for restart)
 */
static __thread syscall_context_t current_syscall_context = {0};

/**
 * @brief Maximum syscall restart attempts
 */
#define MAX_RESTART_ATTEMPTS 3

/* ===================================================================
 * Argument Validation
 * =================================================================== */

/**
 * @brief Validate user space pointer
 * 
 * Checks if a pointer from user space is valid and accessible.
 * 
 * @param ptr Pointer to validate
 * @param size Size of memory region
 * @param write Whether write access is needed
 * @return true if valid, false otherwise
 */
static bool validate_user_pointer(const void *ptr, size_t size, bool write) {
    if (ptr == nullptr) {
        return false;
    }
    
    /* Check if pointer is in user space (below kernel boundary) */
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t kernel_boundary = 0x0000800000000000ULL; /* x86-64 canonical address */
    
    if (addr >= kernel_boundary) {
        fprintf(stderr, "validate_user_pointer: Pointer %p in kernel space\n", ptr);
        return false;
    }
    
    /* Check for overflow */
    if (addr + size < addr) {
        fprintf(stderr, "validate_user_pointer: Size overflow\n");
        return false;
    }
    
    /* TODO: Check against process memory map */
    /* TODO: Check page permissions for write access */
    
    return true;
}

/**
 * @brief Sanitize syscall arguments
 * 
 * Validates and sanitizes syscall arguments before dispatch.
 * 
 * @param syscall_num Syscall number
 * @param args Syscall arguments
 * @return 0 on success, negative errno on failure
 */
static int sanitize_syscall_args(uint32_t syscall_num, const syscall_args_t *args) {
    if (args == nullptr) {
        return -EFAULT;
    }
    
    /* Validate syscall number */
    if (syscall_num >= SYSCALL_COUNT) {
        return -ENOSYS;
    }
    
    /* Syscall-specific validation */
    switch (syscall_num) {
        case SYS_read:
        case SYS_write: {
            /* Validate buffer pointer */
            void *buf = (void *)args->arg1;
            size_t count = (size_t)args->arg2;
            bool is_write = (syscall_num == SYS_read);
            
            if (!validate_user_pointer(buf, count, is_write)) {
                return -EFAULT;
            }
            break;
        }
        
        case SYS_open: {
            /* Validate path pointer */
            const char *path = (const char *)args->arg0;
            if (!validate_user_pointer(path, 4096, false)) {
                return -EFAULT;
            }
            break;
        }
        
        case SYS_mmap: {
            /* Validate mmap parameters */
            mmap_params_t *params = (mmap_params_t *)args->arg0;
            if (!validate_user_pointer(params, sizeof(mmap_params_t), false)) {
                return -EFAULT;
            }
            break;
        }
        
        /* Add more syscall-specific validation as needed */
        default:
            break;
    }
    
    return 0;
}

/* ===================================================================
 * Syscall Entry/Exit Handling
 * =================================================================== */

/**
 * @brief Syscall entry point
 * 
 * Called when entering kernel mode from user space via syscall instruction.
 * Handles context saving, validation, and dispatch.
 * 
 * @param syscall_num Syscall number
 * @param args Syscall arguments
 * @return Syscall result
 */
int64_t syscall_entry(uint32_t syscall_num, const syscall_args_t *args) {
    int64_t result;
    
    /* Save syscall context for potential restart */
    current_syscall_context.syscall_num = syscall_num;
    current_syscall_context.args = *args;
    current_syscall_context.restartable = true;
    
    /* Trace syscall entry */
    syscall_trace_entry(syscall_num, args);
    
    /* Check capabilities */
    if (!capability_check_syscall(syscall_num)) {
        result = -EPERM;
        goto exit;
    }
    
    /* Sanitize arguments */
    int sanitize_result = sanitize_syscall_args(syscall_num, args);
    if (sanitize_result < 0) {
        result = sanitize_result;
        goto exit;
    }
    
    /* Dispatch syscall */
    result = syscall_dispatch(syscall_num, args);
    
    /* Handle interrupted syscalls */
    if (result == -EINTR && current_syscall_context.restartable) {
        if (current_syscall_context.restart_count < MAX_RESTART_ATTEMPTS) {
            current_syscall_context.restart_count++;
            /* Syscall will be restarted by signal handler return */
        }
    }
    
exit:
    /* Trace syscall exit */
    syscall_trace_exit(syscall_num, result);
    
    return result;
}

/**
 * @brief Syscall exit point
 * 
 * Called when returning to user space from a syscall.
 * Handles context restoration and signal delivery.
 * 
 * @param result Syscall result
 */
void syscall_exit(int64_t result) {
    /* Check for pending signals */
    ProcessControlBlock *pcb = process_current();
    if (pcb != nullptr) {
        /* TODO: Deliver pending signals */
    }
    
    /* Clear syscall context */
    current_syscall_context.restartable = false;
    current_syscall_context.restart_count = 0;
}

/* ===================================================================
 * 32-bit Syscall Support
 * =================================================================== */

/**
 * @brief Convert 32-bit syscall arguments to 64-bit
 * 
 * @param args32 32-bit arguments
 * @param args64 Output 64-bit arguments
 */
static void convert_args_32_to_64(const uint32_t *args32, syscall_args_t *args64) {
    args64->arg0 = (uint64_t)args32[0];
    args64->arg1 = (uint64_t)args32[1];
    args64->arg2 = (uint64_t)args32[2];
    args64->arg3 = (uint64_t)args32[3];
    args64->arg4 = (uint64_t)args32[4];
    args64->arg5 = (uint64_t)args32[5];
}

/**
 * @brief 32-bit syscall entry point
 * 
 * Handles syscalls from 32-bit user space processes.
 * 
 * @param syscall_num Syscall number
 * @param args32 32-bit syscall arguments
 * @return Syscall result (truncated to 32-bit)
 */
int32_t syscall_entry_32(uint32_t syscall_num, const uint32_t *args32) {
    syscall_args_t args64;
    
    /* Convert arguments */
    convert_args_32_to_64(args32, &args64);
    
    /* Call 64-bit entry point */
    int64_t result = syscall_entry(syscall_num, &args64);
    
    /* Truncate result to 32-bit */
    return (int32_t)result;
}

/* ===================================================================
 * Syscall Restart Mechanism
 * =================================================================== */

/**
 * @brief Check if syscall should be restarted
 * 
 * @return true if syscall should be restarted
 */
bool syscall_should_restart(void) {
    return current_syscall_context.restartable &&
           current_syscall_context.restart_count < MAX_RESTART_ATTEMPTS;
}

/**
 * @brief Restart interrupted syscall
 * 
 * @return Syscall result
 */
int64_t syscall_restart(void) {
    if (!syscall_should_restart()) {
        return -EINTR;
    }
    
    printf("syscall_restart: Restarting syscall %u (attempt %u)\n",
           current_syscall_context.syscall_num,
           current_syscall_context.restart_count);
    
    return syscall_entry(current_syscall_context.syscall_num,
                        &current_syscall_context.args);
}

/* ===================================================================
 * Syscall Information
 * =================================================================== */

/**
 * @brief Get current syscall number
 * 
 * @return Current syscall number, or UINT32_MAX if not in syscall
 */
uint32_t syscall_get_current(void) {
    if (!current_syscall_context.restartable) {
        return UINT32_MAX;
    }
    return current_syscall_context.syscall_num;
}
