// ===================================================================
// DESC: Defines the Process Control Block (PCB) and related structures
//       for the Aeon Process Model.
// ===================================================================
/**
 * @file process..h
 * @brief Process. API
 * @details This file provides the process. functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_PROCESS_H
#define AEON_PROCESS_H

#include "c23_compat.h"
#include "graph.h" // For BdiGraph and basic types
#include <stdint.h>

typedef uint64_t ProcessId;

typedef enum {
    PROC_UNUSED,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_ZOMBIE
} ProcessState;

// Represents the capabilities and permissions of a process.
typedef struct {
    uint64_t allowed_services_mask; // Bitmask for which services can be called
} CapabilitySet;

// The Process Control Block (PCB)
typedef struct {
    ProcessId pid;
    ProcessId parent_pid;
    ProcessState state;
    BdiGraph* graph; // The computation graph for this process
    CapabilitySet capabilities;
    // Add other fields like memory regions, open files, etc.
} ProcessControlBlock;

#endif // AEON_PROCESS_H
