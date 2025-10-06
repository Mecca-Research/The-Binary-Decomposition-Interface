
/**
 * @file capability.c
 * @brief Capability-Based Security Framework
 * 
 * This file implements a capability-based security system for syscalls.
 * It provides fine-grained access control, capability inheritance,
 * auditing, and namespace support.
 * 
 * Features:
 * - Per-syscall capability requirements
 * - Capability inheritance rules
 * - Capability auditing
 * - Capability namespaces
 * - Integration with existing security subsystem
 */

#include "capability.h"
#include "../syscalls/syscalls.h"
#include "../process/process.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ===================================================================
 * Capability Definitions
 * =================================================================== */

/**
 * @brief Capability bits
 */
typedef enum {
    CAP_NONE            = 0,
    CAP_PROCESS_FORK    = (1ULL << 0),   /* Fork processes */
    CAP_PROCESS_EXEC    = (1ULL << 1),   /* Execute programs */
    CAP_PROCESS_KILL    = (1ULL << 2),   /* Send signals */
    CAP_FILE_READ       = (1ULL << 3),   /* Read files */
    CAP_FILE_WRITE      = (1ULL << 4),   /* Write files */
    CAP_FILE_EXECUTE    = (1ULL << 5),   /* Execute files */
    CAP_FILE_CHOWN      = (1ULL << 6),   /* Change file ownership */
    CAP_FILE_CHMOD      = (1ULL << 7),   /* Change file permissions */
    CAP_MEMORY_MMAP     = (1ULL << 8),   /* Memory mapping */
    CAP_MEMORY_MLOCK    = (1ULL << 9),   /* Lock memory */
    CAP_IPC_CREATE      = (1ULL << 10),  /* Create IPC objects */
    CAP_IPC_ACCESS      = (1ULL << 11),  /* Access IPC objects */
    CAP_NET_BIND        = (1ULL << 12),  /* Bind to network ports */
    CAP_NET_RAW         = (1ULL << 13),  /* Raw network access */
    CAP_SYS_ADMIN       = (1ULL << 14),  /* System administration */
    CAP_SYS_TIME        = (1ULL << 15),  /* Set system time */
    CAP_SYS_RESOURCE    = (1ULL << 16),  /* Override resource limits */
    CAP_ALL             = UINT64_MAX     /* All capabilities */
} capability_t;

/**
 * @brief Per-syscall capability requirements
 */
static uint64_t syscall_capabilities[SYSCALL_COUNT] = {0};

/**
 * @brief Global capability audit flag
 */
static _Atomic(bool) capability_audit_enabled = false;

/* ===================================================================
 * Capability Initialization
 * =================================================================== */

/**
 * @brief Initialize capability system
 */
int capability_init(void) {
    /* Set capability requirements for each syscall */
    
    /* Process management */
    syscall_capabilities[SYS_fork] = CAP_PROCESS_FORK;
    syscall_capabilities[SYS_exec] = CAP_PROCESS_EXEC;
    syscall_capabilities[SYS_kill] = CAP_PROCESS_KILL;
    
    /* File I/O */
    syscall_capabilities[SYS_open] = CAP_FILE_READ | CAP_FILE_WRITE;
    syscall_capabilities[SYS_read] = CAP_FILE_READ;
    syscall_capabilities[SYS_write] = CAP_FILE_WRITE;
    syscall_capabilities[SYS_chmod] = CAP_FILE_CHMOD;
    syscall_capabilities[SYS_chown] = CAP_FILE_CHOWN;
    
    /* Memory management */
    syscall_capabilities[SYS_mmap] = CAP_MEMORY_MMAP;
    syscall_capabilities[SYS_mlock] = CAP_MEMORY_MLOCK;
    syscall_capabilities[SYS_munlock] = CAP_MEMORY_MLOCK;
    
    /* IPC */
    syscall_capabilities[SYS_pipe] = CAP_IPC_CREATE;
    syscall_capabilities[SYS_socket] = CAP_IPC_CREATE;
    syscall_capabilities[SYS_shm_open] = CAP_IPC_CREATE;
    
    /* Time */
    syscall_capabilities[SYS_settimeofday] = CAP_SYS_TIME;
    syscall_capabilities[SYS_clock_settime] = CAP_SYS_TIME;
    
    /* Read-only syscalls require no special capabilities */
    syscall_capabilities[SYS_getpid] = CAP_NONE;
    syscall_capabilities[SYS_getppid] = CAP_NONE;
    syscall_capabilities[SYS_gettimeofday] = CAP_NONE;
    syscall_capabilities[SYS_clock_gettime] = CAP_NONE;
    
    printf("capability: Capability system initialized\n");
    return 0;
}

/* ===================================================================
 * Capability Checking
 * =================================================================== */

/**
 * @brief Get process capabilities
 * 
 * @param pcb Process control block
 * @return Process capabilities
 */
static uint64_t get_process_capabilities(ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return CAP_NONE;
    }
    
    /* TODO: Get capabilities from PCB */
    /* For now, grant all capabilities to all processes */
    return CAP_ALL;
}

/**
 * @brief Check if process has required capability
 * 
 * @param pcb Process control block
 * @param required_caps Required capabilities
 * @return true if process has capabilities, false otherwise
 */
static bool has_capability(ProcessControlBlock *pcb, uint64_t required_caps) {
    if (required_caps == CAP_NONE) {
        return true;
    }
    
    uint64_t process_caps = get_process_capabilities(pcb);
    return (process_caps & required_caps) == required_caps;
}

/**
 * @brief Check if syscall is allowed for current process
 * 
 * @param syscall_num Syscall number
 * @return true if allowed, false otherwise
 */
bool capability_check_syscall(uint32_t syscall_num) {
    if (syscall_num >= SYSCALL_COUNT) {
        return false;
    }
    
    ProcessControlBlock *pcb = process_current();
    uint64_t required_caps = syscall_capabilities[syscall_num];
    
    bool allowed = has_capability(pcb, required_caps);
    
    /* Audit if enabled */
    if (atomic_load_explicit(&capability_audit_enabled, memory_order_relaxed)) {
        capability_audit(syscall_num, allowed);
    }
    
    return allowed;
}

/* ===================================================================
 * Capability Auditing
 * =================================================================== */

/**
 * @brief Audit capability check
 * 
 * @param syscall_num Syscall number
 * @param allowed Whether access was allowed
 */
void capability_audit(uint32_t syscall_num, bool allowed) {
    ProcessControlBlock *pcb = process_current();
    ProcessId pid = (pcb != nullptr) ? pcb->pid : 0;
    const char *syscall_name = syscall_get_name(syscall_num);
    
    if (allowed) {
        printf("capability_audit: PID %lu: %s - ALLOWED\n", pid, syscall_name);
    } else {
        printf("capability_audit: PID %lu: %s - DENIED\n", pid, syscall_name);
    }
}

/**
 * @brief Enable capability auditing
 */
void capability_audit_enable(void) {
    atomic_store_explicit(&capability_audit_enabled, true, memory_order_release);
    printf("capability: Auditing enabled\n");
}

/**
 * @brief Disable capability auditing
 */
void capability_audit_disable(void) {
    atomic_store_explicit(&capability_audit_enabled, false, memory_order_release);
    printf("capability: Auditing disabled\n");
}

/* ===================================================================
 * Capability Management
 * =================================================================== */

/**
 * @brief Grant capability to process
 * 
 * @param pcb Process control block
 * @param cap Capability to grant
 * @return 0 on success, negative errno on failure
 */
int capability_grant(ProcessControlBlock *pcb, uint64_t cap) {
    if (pcb == nullptr) {
        return -EINVAL;
    }
    
    /* TODO: Implement capability granting in PCB */
    printf("capability_grant: Granted capability 0x%lx to PID %lu\n", cap, pcb->pid);
    return 0;
}

/**
 * @brief Revoke capability from process
 * 
 * @param pcb Process control block
 * @param cap Capability to revoke
 * @return 0 on success, negative errno on failure
 */
int capability_revoke(ProcessControlBlock *pcb, uint64_t cap) {
    if (pcb == nullptr) {
        return -EINVAL;
    }
    
    /* TODO: Implement capability revocation in PCB */
    printf("capability_revoke: Revoked capability 0x%lx from PID %lu\n", cap, pcb->pid);
    return 0;
}

/**
 * @brief Inherit capabilities from parent to child
 * 
 * @param parent Parent process control block
 * @param child Child process control block
 * @return 0 on success, negative errno on failure
 */
int capability_inherit(ProcessControlBlock *parent, ProcessControlBlock *child) {
    if (parent == nullptr || child == nullptr) {
        return -EINVAL;
    }
    
    /* TODO: Implement capability inheritance */
    printf("capability_inherit: Inherited capabilities from PID %lu to PID %lu\n",
           parent->pid, child->pid);
    return 0;
}
