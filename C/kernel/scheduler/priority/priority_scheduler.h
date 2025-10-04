
// ===================================================================
// Phase 5.3: Priority Scheduler
// DESC: Priority-based scheduling with deadlines
// ===================================================================
#ifndef AEON_PRIORITY_SCHEDULER_H
#define AEON_PRIORITY_SCHEDULER_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../device/device.h"
#include <stdint.h>
#include <stdatomic.h>

// --- Scheduled Node ---
typedef struct {
    NodeId node_id;
    int32_t priority;
    uint64_t deadline;  // Timestamp in cycles
    atomic_bool scheduled;
} ScheduledNode;

// --- Priority Scheduler ---
typedef struct {
    BdiGraph* graph;
    DeviceVTable** devices;
    size_t device_count;
    ScheduledNode* scheduled_nodes;
    size_t node_count;
    uint64_t current_cycle;
    atomic_bool running;
} PriorityScheduler;

// --- Priority Scheduler API ---
[[nodiscard]] PriorityScheduler* priority_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count);
void priority_scheduler_free(PriorityScheduler* sched);
[[nodiscard]] int scheduler_set_priority(PriorityScheduler* sched, NodeId node_id, int32_t priority);
[[nodiscard]] int scheduler_set_deadline(PriorityScheduler* sched, NodeId node_id, uint64_t deadline);
[[nodiscard]] int priority_scheduler_run(PriorityScheduler* sched);

// --- Utility Functions ---
[[nodiscard]] ScheduledNode* find_highest_priority_node(PriorityScheduler* sched);
[[nodiscard]] bool is_deadline_missed(const ScheduledNode* node, uint64_t current_cycle);

#endif // AEON_PRIORITY_SCHEDULER_H
