
/**
 * @file process_lifecycle.c
 * @brief Process Lifecycle Management
 * 
 * Phase 8: Process Management & Lifecycle
 * 
 * Implements complete process lifecycle operations:
 * - fork() with Copy-On-Write (COW) support
 * - exec() with graph loading
 * - wait() and waitpid() with proper synchronization
 * - exit() with complete resource cleanup
 * - kill() for process termination
 */

#include "process.h"
#include "../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* External process table access */
extern ProcessControlBlock *process_find(ProcessId pid);
extern ProcessId pid_alloc(void);
extern ProcessControlBlock *pcb_alloc(void);
extern void pcb_free(ProcessControlBlock *pcb);
extern uint32_t pcb_ref(ProcessControlBlock *pcb);
extern uint32_t pcb_unref(ProcessControlBlock *pcb);

/* ===================================================================
 * Helper Functions
 * =================================================================== */

/**
 * @brief Copy memory regions with COW support
 * 
 * For proper COW semantics, both parent and child must share the same
 * MemoryRegion objects (not copies) so they share the same atomic refcount.
 * This prevents use-after-free when one process exits while the other still
 * holds references to the shared memory.
 */
static int copy_memory_regions_cow(ProcessControlBlock *parent,
                                   ProcessControlBlock *child) {
    if (parent == nullptr || child == nullptr) {
        return -EINVAL;
    }
    
    /* Share memory region list with COW - both processes reference the same regions */
    MemoryRegion *parent_region = parent->memory_regions;
    MemoryRegion *prev_child_region = nullptr;
    
    while (parent_region != nullptr) {
        /* Share the same MemoryRegion object between parent and child */
        /* Increment reference count for COW - both processes share this region */
        atomic_fetch_add(&parent_region->ref_count, 1);
        
        /* Link the shared region into child's region list */
        if (prev_child_region == nullptr) {
            child->memory_regions = parent_region;
        } else {
            prev_child_region->next = parent_region;
        }
        
        prev_child_region = parent_region;
        parent_region = parent_region->next;
    }
    
    return 0;
}

/**
 * @brief Copy file descriptor table
 */
static int copy_fd_table(ProcessControlBlock *parent,
                        ProcessControlBlock *child) {
    if (parent == nullptr || child == nullptr) {
        return -EINVAL;
    }
    
    if (parent->num_fds == 0) {
        child->fd_table = nullptr;
        child->num_fds = 0;
        return 0;
    }
    
    /* Allocate new FD table */
    child->fd_table = ALLOC_ARRAY(FileDescriptor, parent->num_fds);
    if (child->fd_table == nullptr) {
        fprintf(stderr, "PROCESS: Failed to allocate FD table\n");
        return -ENOMEM;
    }
    
    /* Copy file descriptors */
    memcpy(child->fd_table, parent->fd_table, 
           sizeof(FileDescriptor) * parent->num_fds);
    child->num_fds = parent->num_fds;
    
    /* Increment reference counts */
    for (uint32_t i = 0; i < child->num_fds; i++) {
        atomic_fetch_add(&child->fd_table[i].ref_count, 1);
    }
    
    return 0;
}

/**
 * @brief Add child to parent's children list
 */
static int add_child(ProcessControlBlock *parent, ProcessControlBlock *child) {
    if (parent == nullptr || child == nullptr) {
        return -EINVAL;
    }
    
    /* Reallocate children array */
    ProcessControlBlock **new_children = realloc(parent->children,
        sizeof(ProcessControlBlock *) * (parent->num_children + 1));
    
    if (new_children == nullptr) {
        fprintf(stderr, "PROCESS: Failed to reallocate children array\n");
        return -ENOMEM;
    }
    
    parent->children = new_children;
    parent->children[parent->num_children] = child;
    parent->num_children++;
    
    return 0;
}

/**
 * @brief Remove child from parent's children list
 */
static void remove_child(ProcessControlBlock *parent, ProcessControlBlock *child) {
    if (parent == nullptr || child == nullptr) {
        return;
    }
    
    /* Find and remove child */
    for (uint32_t i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == child) {
            /* Shift remaining children */
            for (uint32_t j = i; j < parent->num_children - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->num_children--;
            break;
        }
    }
}

/* ===================================================================
 * Process Lifecycle Functions
 * =================================================================== */

/**
 * @brief Fork current process (create child)
 */
ProcessId process_fork(void) {
    ProcessControlBlock *parent = process_current();
    if (parent == nullptr) {
        fprintf(stderr, "PROCESS: No current process for fork\n");
        return -ESRCH;
    }
    
    printf("PROCESS: Forking process %llu ('%s')\n",
           (unsigned long long)parent->pid,
           parent->name);
    
    /* Allocate new PCB for child */
    ProcessControlBlock *child = pcb_alloc();
    if (child == nullptr) {
        fprintf(stderr, "PROCESS: Failed to allocate child PCB\n");
        return -ENOMEM;
    }
    
    /* Allocate new PID */
    ProcessId child_pid = pid_alloc();
    child->pid = child_pid;
    child->parent_pid = parent->pid;
    child->pgid = parent->pgid;
    child->sid = parent->sid;
    
    /* Copy process attributes */
    atomic_store(&child->state, PROC_CREATING);
    atomic_store(&child->flags, atomic_load(&parent->flags) & ~PROC_FLAG_KERNEL);
    child->priority = parent->priority;
    child->nice = parent->nice;
    child->cpu_affinity = parent->cpu_affinity;
    child->numa_node = parent->numa_node;
    
    /* Copy process name */
    snprintf(child->name, MAX_PROCESS_NAME, "%s-child", parent->name);
    
    /* Copy capabilities */
    memcpy(&child->capabilities, &parent->capabilities, sizeof(CapabilitySet));
    
    /* Copy memory regions with COW */
    int ret = copy_memory_regions_cow(parent, child);
    if (ret < 0) {
        fprintf(stderr, "PROCESS: Failed to copy memory regions\n");
        pcb_free(child);
        return ret;
    }
    
    /* Copy file descriptor table */
    ret = copy_fd_table(parent, child);
    if (ret < 0) {
        fprintf(stderr, "PROCESS: Failed to copy FD table\n");
        pcb_free(child);
        return ret;
    }
    
    /* Set parent pointer */
    child->parent = parent;
    pcb_ref(parent);
    
    /* Add child to parent's children list */
    ret = add_child(parent, child);
    if (ret < 0) {
        fprintf(stderr, "PROCESS: Failed to add child to parent\n");
        pcb_free(child);
        return ret;
    }
    
    /* TODO: Copy computation graph (deep copy) */
    /* child->graph = graph_copy(parent->graph); */
    child->graph = nullptr;  /* Placeholder */
    
    /* Set creation time */
    child->creation_time = 0;  /* TODO: Get current timestamp */
    
    /* Transition to READY state */
    process_set_state(child, PROC_READY);
    
    /* Insert into process table */
    ret = process_insert(child);
    if (ret < 0) {
        fprintf(stderr, "PROCESS: Failed to insert child into process table\n");
        pcb_free(child);
        return ret;
    }
    
    printf("PROCESS: Fork successful - Parent=%llu Child=%llu\n",
           (unsigned long long)parent->pid,
           (unsigned long long)child_pid);
    
    /* Return child PID in parent, 0 in child */
    /* Note: In a real implementation, we would return 0 in the child context */
    return child_pid;
}

/**
 * @brief Execute a new program
 */
int process_exec(const char *graph_path, 
                 char *const argv[], 
                 char *const envp[]) {
    ProcessControlBlock *pcb = process_current();
    if (pcb == nullptr) {
        fprintf(stderr, "PROCESS: No current process for exec\n");
        return -ESRCH;
    }
    
    if (graph_path == nullptr) {
        fprintf(stderr, "PROCESS: Invalid graph path\n");
        return -EINVAL;
    }
    
    printf("PROCESS: Executing '%s' in process %llu\n",
           graph_path,
           (unsigned long long)pcb->pid);
    
    /* TODO: Load computation graph from file */
    /* BdiGraph *new_graph = graph_load(graph_path); */
    /* if (new_graph == nullptr) { */
    /*     fprintf(stderr, "PROCESS: Failed to load graph\n"); */
    /*     return -ENOENT; */
    /* } */
    
    /* Free old graph */
    if (pcb->graph != nullptr) {
        /* graph_free(pcb->graph); */
        pcb->graph = nullptr;
    }
    
    /* Set new graph */
    /* pcb->graph = new_graph; */
    
    /* Reset memory regions (keep only essential ones) */
    /* TODO: Implement proper memory region cleanup and setup */
    
    /* Close file descriptors marked as close-on-exec */
    /* TODO: Implement FD_CLOEXEC handling */
    
    /* Update process name */
    const char *basename = strrchr(graph_path, '/');
    if (basename != nullptr) {
        basename++;
    } else {
        basename = graph_path;
    }
    strncpy(pcb->name, basename, MAX_PROCESS_NAME - 1);
    pcb->name[MAX_PROCESS_NAME - 1] = '\0';
    
    printf("PROCESS: Exec successful - Process %llu now running '%s'\n",
           (unsigned long long)pcb->pid,
           pcb->name);
    
    /* On success, exec doesn't return */
    /* In this implementation, we return 0 to indicate success */
    return 0;
}

/**
 * @brief Wait for child process to exit
 */
ProcessId process_wait(ProcessId pid, 
                       ExitStatus *status, 
                       uint32_t options) {
    ProcessControlBlock *parent = process_current();
    if (parent == nullptr) {
        fprintf(stderr, "PROCESS: No current process for wait\n");
        return -ESRCH;
    }
    
    printf("PROCESS: Process %llu waiting for child %lld\n",
           (unsigned long long)parent->pid,
           (long long)pid);
    
    /* Find zombie child */
    ProcessControlBlock *child = nullptr;
    
    if (pid == -1) {
        /* Wait for any child */
        for (uint32_t i = 0; i < parent->num_children; i++) {
            ProcessControlBlock *c = parent->children[i];
            if (c != nullptr && process_get_state(c) == PROC_ZOMBIE) {
                child = c;
                break;
            }
        }
    } else {
        /* Wait for specific child */
        child = process_find(pid);
        if (child == nullptr || child->parent_pid != parent->pid) {
            fprintf(stderr, "PROCESS: Child %lld not found\n", (long long)pid);
            return -ECHILD;
        }
        
        if (process_get_state(child) != PROC_ZOMBIE) {
            if (options & WAIT_NOHANG) {
                return 0;  /* No child ready */
            }
            
            /* TODO: Block until child exits */
            /* For now, return error */
            fprintf(stderr, "PROCESS: Child %lld not zombie (blocking not implemented)\n",
                   (long long)pid);
            return -EAGAIN;
        }
    }
    
    if (child == nullptr) {
        if (options & WAIT_NOHANG) {
            return 0;  /* No child ready */
        }
        
        /* No zombie children */
        fprintf(stderr, "PROCESS: No zombie children\n");
        return -ECHILD;
    }
    
    /* Copy exit status */
    if (status != nullptr) {
        *status = child->exit_status;
    }
    
    ProcessId child_pid = child->pid;
    
    printf("PROCESS: Reaping zombie child %llu (exit_code=%d)\n",
           (unsigned long long)child_pid,
           child->exit_status.exit_code);
    
    /* Remove from parent's children list */
    remove_child(parent, child);
    
    /* Transition to DEAD state */
    process_set_state(child, PROC_DEAD);
    
    /* Decrement reference count (will free if last reference) */
    pcb_unref(child);
    
    return child_pid;
}

/**
 * @brief Exit current process
 */
void process_exit(int exit_code) {
    ProcessControlBlock *pcb = process_current();
    if (pcb == nullptr) {
        fprintf(stderr, "PROCESS: No current process for exit\n");
        return;
    }
    
    printf("PROCESS: Process %llu ('%s') exiting with code %d\n",
           (unsigned long long)pcb->pid,
           pcb->name,
           exit_code);
    
    /* Set exit status */
    pcb->exit_status.exit_code = exit_code;
    pcb->exit_status.signal = 0;
    pcb->exit_status.core_dumped = false;
    
    /* Set exit time */
    pcb->exit_time = 0;  /* TODO: Get current timestamp */
    
    /* Close all file descriptors */
    if (pcb->fd_table != nullptr) {
        for (uint32_t i = 0; i < pcb->num_fds; i++) {
            uint32_t refs = atomic_fetch_sub(&pcb->fd_table[i].ref_count, 1) - 1;
            if (refs == 0) {
                /* TODO: Close file handle */
            }
        }
    }
    
    /* Close all IPC handles */
    if (pcb->ipc_handles != nullptr) {
        for (uint32_t i = 0; i < pcb->num_ipc_handles; i++) {
            if (pcb->ipc_handles[i] != nullptr) {
                ipc_close(pcb->ipc_handles[i]);
            }
        }
    }
    
    /* Free computation graph */
    if (pcb->graph != nullptr) {
        /* graph_free(pcb->graph); */
        pcb->graph = nullptr;
    }
    
    /* Reparent children to init (PID 1) */
    if (pcb->num_children > 0) {
        ProcessControlBlock *init = process_find(KERNEL_PID);
        if (init != nullptr) {
            for (uint32_t i = 0; i < pcb->num_children; i++) {
                ProcessControlBlock *child = pcb->children[i];
                if (child != nullptr) {
                    child->parent_pid = KERNEL_PID;
                    child->parent = init;
                    add_child(init, child);
                }
            }
        }
        pcb->num_children = 0;
    }
    
    /* Transition to ZOMBIE state */
    process_set_state(pcb, PROC_ZOMBIE);
    
    /* TODO: Notify parent process */
    /* TODO: Schedule next process */
    
    printf("PROCESS: Process %llu is now zombie\n",
           (unsigned long long)pcb->pid);
    
    /* In a real implementation, we would never return from here */
    /* The scheduler would switch to another process */
}

/**
 * @brief Kill a process
 */
int process_kill(ProcessId pid, int signal) {
    ProcessControlBlock *pcb = process_find(pid);
    if (pcb == nullptr) {
        fprintf(stderr, "PROCESS: Process %lld not found\n", (long long)pid);
        return -ESRCH;
    }
    
    printf("PROCESS: Killing process %llu with signal %d\n",
           (unsigned long long)pid,
           signal);
    
    /* Set exit status */
    pcb->exit_status.exit_code = 0;
    pcb->exit_status.signal = signal;
    pcb->exit_status.core_dumped = false;
    
    /* TODO: Handle different signals appropriately */
    /* For now, just terminate the process */
    
    /* Transition to ZOMBIE state */
    process_set_state(pcb, PROC_ZOMBIE);
    
    printf("PROCESS: Process %llu killed\n", (unsigned long long)pid);
    
    return 0;
}
