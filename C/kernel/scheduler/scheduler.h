// ===================================================================
// DESC: Defines the Scheduler and its policy gate for
//       secure execution.
// ===================================================================
/**
 * @file scheduler.h
 * @brief Task Scheduling System
 * @details This file provides the scheduler functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_SCHEDULER_H
#define AEON_SCHEDULER_H

#include "c23_compat.h"
#include "graph.h"
#include "device.h"

// --- Security Policy for the Scheduler ---
typedef struct {
    bool secure_mode; // If true, all nodes must pass the proof gate.
    uint32_t required_proof_class; // The minimum proof class required to run.
} SecurityPolicy;

// --- Scheduler Structure ---
// Manages the ready set of nodes and dispatches them to devices.
typedef struct {
    BdiGraph* graph;
    DeviceVTable** devices; // Array of available device backends.
    size_t device_count;
    SecurityPolicy policy;
    NodeId* ready_set; // Dynamic array of ready node IDs.
    size_t ready_count;
    size_t ready_capacity;
} Scheduler;

// --- Scheduler API ---
[[nodiscard]] Scheduler* aeon_scheduler_create(BdiGraph* g, DeviceVTable** devices, size_t dev_count);
void aeon_scheduler_free(Scheduler* sched);
// Sets the security policy for the scheduler.
void aeon_scheduler_set_policy(Scheduler* sched, SecurityPolicy policy);
// Runs the scheduler for a single wave of execution.
int aeon_scheduler_run_wave(Scheduler* sched);


// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Scheduler requires at least 32-bit pointers");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // AEON_SCHEDULER_H
