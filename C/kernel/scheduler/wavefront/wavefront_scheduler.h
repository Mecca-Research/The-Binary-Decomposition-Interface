
// ===================================================================
// Phase 5.3: Wavefront Scheduler
// DESC: Parallel execution using wavefront scheduling
// ===================================================================
#ifndef AEON_WAVEFRONT_SCHEDULER_H
#define AEON_WAVEFRONT_SCHEDULER_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../device/device.h"
#include <stdint.h>
#include <stdatomic.h>

// --- Wavefront Structure ---
typedef struct {
    NodeId* ready_nodes;
    size_t ready_count;
    size_t ready_capacity;
    atomic_size_t completed_count;
    uint32_t wavefront_id;
} Wavefront;

// --- Wavefront Scheduler ---
typedef struct {
    BdiGraph* graph;
    DeviceVTable** devices;
    size_t device_count;
    Wavefront* wavefronts;
    size_t wavefront_count;
    size_t wavefront_capacity;
    bool* visited;  // Persistent visited state across wavefront calls
    atomic_bool running;
} WavefrontScheduler;

// --- Wavefront Scheduler API ---
[[nodiscard]] WavefrontScheduler* wavefront_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count);
void wavefront_scheduler_free(WavefrontScheduler* sched);
[[nodiscard]] int scheduler_get_next_wavefront(WavefrontScheduler* sched, Wavefront** out_wavefront);
[[nodiscard]] int scheduler_execute_wavefront(WavefrontScheduler* sched, Wavefront* wavefront);
[[nodiscard]] int wavefront_scheduler_run(WavefrontScheduler* sched);

// --- Wavefront Utilities ---
[[nodiscard]] Wavefront* wavefront_create(uint32_t id);
void wavefront_free(Wavefront* wf);
[[nodiscard]] int wavefront_add_node(Wavefront* wf, NodeId node_id);
[[nodiscard]] bool wavefront_is_complete(const Wavefront* wf);

#endif // AEON_WAVEFRONT_SCHEDULER_H
