
// ===================================================================
// Phase 5.3: Work Stealing Scheduler
// DESC: Lock-free work stealing for parallel execution
// ===================================================================
/**
 * @file worksteal_scheduler.h
 * @brief Task Scheduling System
 * @details This file provides the worksteal scheduler functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_WORKSTEAL_SCHEDULER_H
#define AEON_WORKSTEAL_SCHEDULER_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../device/device.h"
#include <stdint.h>
#include <stdatomic.h>
#include <threads.h>

// --- Lock-Free Queue ---
#define QUEUE_SIZE 1024

typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    NodeId items[QUEUE_SIZE];
} LockFreeQueue;

// --- Work Stealing Scheduler ---
typedef struct {
    BdiGraph* graph;
    DeviceVTable** devices;
    size_t device_count;
    LockFreeQueue* local_queue;
    LockFreeQueue** remote_queues;
    size_t num_workers;
    thrd_t* worker_threads;
    atomic_bool running;
} WorkStealingScheduler;

// --- Work Stealing API ---
[[nodiscard]] WorkStealingScheduler* worksteal_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count, size_t num_workers);
void worksteal_scheduler_free(WorkStealingScheduler* sched);
[[nodiscard]] int worksteal_scheduler_run(WorkStealingScheduler* sched);
void worksteal_scheduler_stop(WorkStealingScheduler* sched);

// --- Queue Operations ---
[[nodiscard]] LockFreeQueue* queue_create(void);
void queue_free(LockFreeQueue* queue);
[[nodiscard]] bool queue_push(LockFreeQueue* queue, NodeId node_id);
[[nodiscard]] bool queue_pop(LockFreeQueue* queue, NodeId* out_node_id);
[[nodiscard]] bool queue_steal(LockFreeQueue* queue, NodeId* out_node_id);

// --- Worker Thread ---
int worker_thread(void* arg);
[[nodiscard]] bool steal_work(WorkStealingScheduler* sched, size_t worker_id, NodeId* out_node_id);

#endif // AEON_WORKSTEAL_SCHEDULER_H
