
/**
 * @file process_manager.c
 * @brief Process Manager Implementation
 * 
 * Phase 8: Process Management & Lifecycle
 * 
 * Implements the core process management functionality including:
 * - Lock-free process table with atomic PID allocation
 * - Process creation and destruction
 * - Atomic state transitions
 * - Reference counting
 * - Integration with memory management
 */

#include "process.h"
#include "../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===================================================================
 * Global Process Table
 * =================================================================== */

static ProcessTable g_process_table = {
    .processes = {nullptr},
    .next_pid = ATOMIC_VAR_INIT(KERNEL_PID + 1),
    .total_processes = ATOMIC_VAR_INIT(0),
    .active_processes = ATOMIC_VAR_INIT(0),
    .zombie_processes = ATOMIC_VAR_INIT(0),
    .total_forks = ATOMIC_VAR_INIT(0),
    .total_execs = ATOMIC_VAR_INIT(0),
    .total_exits = ATOMIC_VAR_INIT(0),
    .total_waits = ATOMIC_VAR_INIT(0)
};

/* Thread-local current process */
static _Thread_local ProcessControlBlock *current_process = nullptr;

/* ===================================================================
 * Process Management Functions
 * =================================================================== */

/**
 * @brief Initialize process management subsystem
 */
int process_init(void) {
    printf("PROCESS: Initializing process management subsystem\n");
    
    /* Initialize process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        g_process_table.processes[i] = nullptr;
    }
    
    /* Create kernel process (PID 1) */
    ProcessControlBlock *kernel_pcb = pcb_alloc();
    if (kernel_pcb == nullptr) {
        fprintf(stderr, "PROCESS: Failed to allocate kernel PCB\n");
        return -1;
    }
    
    kernel_pcb->pid = KERNEL_PID;
    kernel_pcb->parent_pid = 0;
    kernel_pcb->pgid = KERNEL_PID;
    kernel_pcb->sid = KERNEL_PID;
    atomic_store(&kernel_pcb->state, PROC_RUNNING);
    atomic_store(&kernel_pcb->flags, PROC_FLAG_KERNEL | PROC_FLAG_SYSTEM);
    kernel_pcb->priority = PRIORITY_MAX;
    kernel_pcb->nice = 0;
    kernel_pcb->cpu_affinity = -1;  /* Any CPU */
    kernel_pcb->numa_node = 0;
    strncpy(kernel_pcb->name, "kernel", MAX_PROCESS_NAME - 1);
    kernel_pcb->name[MAX_PROCESS_NAME - 1] = '\0';
    
    /* Set kernel process as current */
    g_process_table.processes[KERNEL_PID] = kernel_pcb;
    current_process = kernel_pcb;
    
    atomic_fetch_add(&g_process_table.total_processes, 1);
    atomic_fetch_add(&g_process_table.active_processes, 1);
    
    printf("PROCESS: Kernel process created (PID %llu)\n", 
           (unsigned long long)KERNEL_PID);
    
    return 0;
}

/**
 * @brief Shutdown process management subsystem
 */
void process_shutdown(void) {
    printf("PROCESS: Shutting down process management subsystem\n");
    
    /* Clean up all processes */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        ProcessControlBlock *pcb = g_process_table.processes[i];
        if (pcb != nullptr) {
            pcb_free(pcb);
            g_process_table.processes[i] = nullptr;
        }
    }
    
    printf("PROCESS: Process management shutdown complete\n");
}

/**
 * @brief Allocate a new PCB
 */
ProcessControlBlock *pcb_alloc(void) {
    /* Allocate PCB using memory management */
    ProcessControlBlock *pcb = ALLOC(ProcessControlBlock);
    if (pcb == nullptr) {
        fprintf(stderr, "PROCESS: Failed to allocate PCB\n");
        return nullptr;
    }
    
    /* Initialize PCB */
    memset(pcb, 0, sizeof(ProcessControlBlock));
    
    /* Initialize atomic fields */
    atomic_init(&pcb->state, PROC_UNUSED);
    atomic_init(&pcb->flags, 0);
    atomic_init(&pcb->ref_count, 1);
    
    /* Initialize statistics */
    atomic_init(&pcb->stats.cpu_time_ns, 0);
    atomic_init(&pcb->stats.user_time_ns, 0);
    atomic_init(&pcb->stats.system_time_ns, 0);
    atomic_init(&pcb->stats.page_faults, 0);
    atomic_init(&pcb->stats.context_switches, 0);
    atomic_init(&pcb->stats.syscalls, 0);
    atomic_init(&pcb->stats.ipc_sends, 0);
    atomic_init(&pcb->stats.ipc_recvs, 0);
    
    /* Set default values */
    pcb->priority = PRIORITY_DEFAULT;
    pcb->nice = 0;
    pcb->cpu_affinity = -1;  /* Any CPU */
    pcb->numa_node = numa_current_node();
    
    return pcb;
}

/**
 * @brief Free a PCB
 */
void pcb_free(ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return;
    }
    
    /* Free memory regions using proper COW refcount handling */
    while (pcb->memory_regions != nullptr) {
        MemoryRegion *region = pcb->memory_regions;
        pcb->memory_regions = region->next;
        free_memory_region(region);
    }
    
    /* Free file descriptor table */
    if (pcb->fd_table != nullptr) {
        FREE_ARRAY(pcb->fd_table, FileDescriptor, pcb->num_fds);
    }
    
    /* Free IPC handles */
    if (pcb->ipc_handles != nullptr) {
        for (uint32_t i = 0; i < pcb->num_ipc_handles; i++) {
            if (pcb->ipc_handles[i] != nullptr) {
                ipc_close(pcb->ipc_handles[i]);
            }
        }
        free(pcb->ipc_handles);
    }
    
    /* Free children array */
    if (pcb->children != nullptr) {
        free(pcb->children);
    }
    
    /* Free the PCB itself */
    FREE(pcb, ProcessControlBlock);
}

/**
 * @brief Increment PCB reference count
 */
uint32_t pcb_ref(ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return 0;
    }
    
    return atomic_fetch_add(&pcb->ref_count, 1) + 1;
}

/**
 * @brief Decrement PCB reference count
 */
uint32_t pcb_unref(ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return 0;
    }
    
    uint32_t refs = atomic_fetch_sub(&pcb->ref_count, 1) - 1;
    if (refs == 0) {
        /* Last reference, free the PCB */
        pcb_free(pcb);
    }
    
    return refs;
}

/**
 * @brief Allocate a new PID atomically
 */
ProcessId pid_alloc(void) {
    /* Atomic fetch-and-add for lock-free PID allocation */
    ProcessId pid = atomic_fetch_add(&g_process_table.next_pid, 1);
    
    /* Wrap around if we exceed MAX_PROCESSES */
    if (pid >= MAX_PROCESSES) {
        /* Reset to KERNEL_PID + 1 and try again */
        atomic_store(&g_process_table.next_pid, KERNEL_PID + 1);
        pid = atomic_fetch_add(&g_process_table.next_pid, 1);
    }
    
    return pid;
}

/**
 * @brief Find process by PID (lock-free)
 */
ProcessControlBlock *process_find(ProcessId pid) {
    if (pid == INVALID_PID || pid >= MAX_PROCESSES) {
        return nullptr;
    }
    
    /* Lock-free read from process table */
    ProcessControlBlock *pcb = g_process_table.processes[pid];
    
    /* Verify the PCB is valid and has the correct PID */
    if (pcb != nullptr && pcb->pid == pid) {
        ProcessState state = atomic_load(&pcb->state);
        if (state != PROC_UNUSED && state != PROC_DEAD) {
            return pcb;
        }
    }
    
    return nullptr;
}

/**
 * @brief Insert process into process table
 * 
 * This function provides external access to insert a PCB into the process table.
 * Used by process_fork() to register newly created child processes.
 * 
 * @param pcb Process control block to insert
 * @return 0 on success, negative error code on failure
 */
int process_insert(ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return -EINVAL;
    }
    
    ProcessId pid = pcb->pid;
    if (pid == INVALID_PID || pid >= MAX_PROCESSES) {
        return -EINVAL;
    }
    
    /* Insert into process table */
    g_process_table.processes[pid] = pcb;
    
    /* Update statistics */
    atomic_fetch_add(&g_process_table.total_processes, 1);
    atomic_fetch_add(&g_process_table.active_processes, 1);
    
    return 0;
}

/**
 * @brief Get current process
 */
ProcessControlBlock *process_current(void) {
    return current_process;
}

/**
 * @brief Set current process
 */
void process_set_current(ProcessControlBlock *pcb) {
    current_process = pcb;
}

/**
 * @brief Get process state
 */
ProcessState process_get_state(const ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        return PROC_UNUSED;
    }
    
    return (ProcessState)atomic_load(&pcb->state);
}

/**
 * @brief Set process state (atomic)
 */
bool process_set_state(ProcessControlBlock *pcb, ProcessState new_state) {
    if (pcb == nullptr) {
        return false;
    }
    
    ProcessState old_state = atomic_load(&pcb->state);
    atomic_store(&pcb->state, new_state);
    
    /* Update process counts */
    if (old_state == PROC_ZOMBIE && new_state != PROC_ZOMBIE) {
        atomic_fetch_sub(&g_process_table.zombie_processes, 1);
    } else if (old_state != PROC_ZOMBIE && new_state == PROC_ZOMBIE) {
        atomic_fetch_add(&g_process_table.zombie_processes, 1);
    }
    
    if ((old_state == PROC_UNUSED || old_state == PROC_DEAD) && 
        (new_state != PROC_UNUSED && new_state != PROC_DEAD)) {
        atomic_fetch_add(&g_process_table.active_processes, 1);
    } else if ((old_state != PROC_UNUSED && old_state != PROC_DEAD) && 
               (new_state == PROC_UNUSED || new_state == PROC_DEAD)) {
        atomic_fetch_sub(&g_process_table.active_processes, 1);
    }
    
    return true;
}

/**
 * @brief Compare and exchange process state (atomic)
 */
bool process_cas_state(ProcessControlBlock *pcb,
                       ProcessState expected,
                       ProcessState desired) {
    if (pcb == nullptr) {
        return false;
    }
    
    uint32_t expected_val = expected;
    bool success = atomic_compare_exchange_strong(&pcb->state, 
                                                   &expected_val, 
                                                   desired);
    
    if (success) {
        /* Update process counts */
        if (expected == PROC_ZOMBIE && desired != PROC_ZOMBIE) {
            atomic_fetch_sub(&g_process_table.zombie_processes, 1);
        } else if (expected != PROC_ZOMBIE && desired == PROC_ZOMBIE) {
            atomic_fetch_add(&g_process_table.zombie_processes, 1);
        }
        
        if ((expected == PROC_UNUSED || expected == PROC_DEAD) && 
            (desired != PROC_UNUSED && desired != PROC_DEAD)) {
            atomic_fetch_add(&g_process_table.active_processes, 1);
        } else if ((expected != PROC_UNUSED && expected != PROC_DEAD) && 
                   (desired == PROC_UNUSED || desired == PROC_DEAD)) {
            atomic_fetch_sub(&g_process_table.active_processes, 1);
        }
    }
    
    return success;
}

/**
 * @brief Get process statistics
 */
void process_get_stats(const ProcessControlBlock *pcb, ProcessStats *stats) {
    if (pcb == nullptr || stats == nullptr) {
        return;
    }
    
    /* Atomic reads of statistics */
    stats->cpu_time_ns = atomic_load(&pcb->stats.cpu_time_ns);
    stats->user_time_ns = atomic_load(&pcb->stats.user_time_ns);
    stats->system_time_ns = atomic_load(&pcb->stats.system_time_ns);
    stats->page_faults = atomic_load(&pcb->stats.page_faults);
    stats->context_switches = atomic_load(&pcb->stats.context_switches);
    stats->syscalls = atomic_load(&pcb->stats.syscalls);
    stats->ipc_sends = atomic_load(&pcb->stats.ipc_sends);
    stats->ipc_recvs = atomic_load(&pcb->stats.ipc_recvs);
}

/**
 * @brief Print process information
 */
void process_print_info(const ProcessControlBlock *pcb) {
    if (pcb == nullptr) {
        printf("PROCESS: Invalid PCB\n");
        return;
    }
    
    ProcessState state = process_get_state(pcb);
    uint32_t flags = atomic_load(&pcb->flags);
    
    printf("PROCESS: PID=%llu Name='%s' State=%s\n",
           (unsigned long long)pcb->pid,
           pcb->name,
           process_state_name(state));
    
    printf("  Parent=%llu PGID=%llu SID=%llu\n",
           (unsigned long long)pcb->parent_pid,
           (unsigned long long)pcb->pgid,
           (unsigned long long)pcb->sid);
    
    printf("  Priority=%u Nice=%d CPU=%d NUMA=%d\n",
           pcb->priority,
           pcb->nice,
           pcb->cpu_affinity,
           pcb->numa_node);
    
    printf("  Flags=0x%08X RefCount=%u\n",
           flags,
           atomic_load(&pcb->ref_count));
    
    ProcessStats stats;
    process_get_stats(pcb, &stats);
    
    printf("  CPU Time=%llu ns User=%llu ns System=%llu ns\n",
           (unsigned long long)stats.cpu_time_ns,
           (unsigned long long)stats.user_time_ns,
           (unsigned long long)stats.system_time_ns);
    
    printf("  Page Faults=%llu Context Switches=%llu Syscalls=%llu\n",
           (unsigned long long)stats.page_faults,
           (unsigned long long)stats.context_switches,
           (unsigned long long)stats.syscalls);
    
    printf("  IPC Sends=%llu IPC Recvs=%llu\n",
           (unsigned long long)stats.ipc_sends,
           (unsigned long long)stats.ipc_recvs);
}

/**
 * @brief Get process state name
 */
const char *process_state_name(ProcessState state) {
    switch (state) {
        case PROC_UNUSED:    return "UNUSED";
        case PROC_CREATING:  return "CREATING";
        case PROC_READY:     return "READY";
        case PROC_RUNNING:   return "RUNNING";
        case PROC_SLEEPING:  return "SLEEPING";
        case PROC_WAITING:   return "WAITING";
        case PROC_ZOMBIE:    return "ZOMBIE";
        case PROC_DEAD:      return "DEAD";
        default:             return "UNKNOWN";
    }
}
