// ===================================================================
// DESC: Implements the core logic for the ProcessManager.bdi service,
//       handling fork, exit, and wait operations.
// ===================================================================
#include "process.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_PROCESSES 64
static ProcessControlBlock process_table[MAX_PROCESSES];
static ProcessId next_pid = 1;

void process_manager_init() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_UNUSED;
    }
}

// Conceptual implementation of the fork service operation.
ProcessId proc_fork(ProcessId parent_pid) {
    // 1. Find an unused PCB in the process table.
    ProcessControlBlock* pcb = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            pcb = &process_table[i];
            break;
        }
    }
    if (!pcb) return 0; // No free PCBs

    // 2. Duplicate the parent's graph and memory regions (deep copy needed).
    // This is a complex operation requiring graph and HAM APIs.
    // BdiGraph* new_graph = aion_graph_copy(parent_pcb->graph);

    // 3. Initialize the new PCB.
    pcb->pid = next_pid++;
    pcb->parent_pid = parent_pid;
    pcb->state = PROC_RUNNING;
    // pcb->graph = new_graph;
    // pcb->capabilities = parent_pcb->capabilities; // Inherit capabilities

    printf("PROC_MANAGER: Forked process %llu from parent %llu\n",
           (unsigned long long)pcb->pid, (unsigned long long)parent_pid);

    return pcb->pid;
}

// Conceptual implementation of the exit service operation.
void proc_exit(ProcessId pid) {
    // 1. Find the PCB.
    // 2. Change state to ZOMBIE.
    // 3. Clean up resources (close files, free memory regions).
    // 4. Notify the parent process.
    printf("PROC_MANAGER: Process %llu exited.\n", (unsigned long long)pid);
}
