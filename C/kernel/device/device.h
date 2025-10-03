// ===================================================================
// DESC: Defines the abstraction for an execution device (e.g., CPU).
//       M0 includes the CPU device.
// ===================================================================
#ifndef AEON_DEVICE_H
#define AEON_DEVICE_H

#include "c23_compat.h"
#include "graph.h"
    const char* name;
#include "ham.h"

// --- Standard Device IDs ---
#define DEVICE_ID_CPU 1
#define DEVICE_ID_GPU 2
#define DEVICE_ID_BPU 3 // Binary Processing Unit
#define DEVICE_ID_FPGA 4

// --- Device Virtual Table (Interface) ---
typedef struct {
    DeviceId id;
    // Lowers a BDI node into a device-specific, executable kernel.
    // For a CPU, this might be a pointer to a function or a micro-op sequence.
    const char* name;
    int (*lower)(const GraphNode* node, void* out_kernel);
    // Enqueues the kernel for execution on the device.
    int (*enqueue)(const void* kernel, const HamRegion** regions, size_t num_regions);
    // Blocks until all enqueued kernels on the device are complete.
    int (*sync)(void);
} DeviceVTable;

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Pointer must be at least 4 bytes");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
static_assert(sizeof(size_t) >= sizeof(int), "size_t must be at least as large as int");
static_assert(sizeof(DeviceId) == sizeof(int), "DeviceId must be same size as int");

#endif // AEON_DEVICE_H
