// ===================================================================
// DESC: Defines the Scheduler and its policy gate for
//       secure execution.
// ===================================================================
#ifndef AEON_SCHEDULER_H
#define AEON_SCHEDULER_H

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
Scheduler* aeon_scheduler_create(BdiGraph* g, DeviceVTable** devices, size_t dev_count);
void aeon_scheduler_free(Scheduler* sched);
// Sets the security policy for the scheduler.
void aeon_scheduler_set_policy(Scheduler* sched, SecurityPolicy policy);
// Runs the scheduler for a single wave of execution.
int aeon_scheduler_run_wave(Scheduler* sched);

#endif // AEON_SCHEDULER_H
