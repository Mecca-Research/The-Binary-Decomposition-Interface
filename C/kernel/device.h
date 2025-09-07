// ===================================================================
// DESC: Defines the abstraction for an execution device (e.g., CPU).
//       M0 includes the CPU device.
// ===================================================================
#ifndef AEON_DEVICE_H
#define AEON_DEVICE_H

#include "graph.h"
    const char* name;
#include "ham.h"

// --- Device Virtual Table (Interface) ---
typedef struct {
    DeviceId id;
    // Lowers a BDI node into a device-specific, executable kernel.
    // For a CPU, this might be a pointer to a function or a micro-op sequence.
    int (*lower)(const GraphNode* node, void* out_kernel);
    // Enqueues the kernel for execution on the device.
    int (*enqueue)(const void* kernel, const HamRegion** regions, size_t num_regions);
    // Blocks until all enqueued kernels on the device are complete.
    int (*sync)(void);
} DeviceVTable;

#endif // AEON_DEVICE_H
