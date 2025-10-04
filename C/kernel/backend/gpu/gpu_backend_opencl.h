
// ===================================================================
// Phase 5.2: GPU Backend (OpenCL) Implementation
// DESC: GPU kernel execution using OpenCL
// ===================================================================
#ifndef AEON_GPU_BACKEND_OPENCL_H
#define AEON_GPU_BACKEND_OPENCL_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../ham/ham.h"
#include "../../device/device.h"
#include <stdint.h>

// --- GPU Buffer Structure ---
typedef struct {
    void* device_ptr;
    size_t size;
    void* cl_buffer;  // cl_mem handle (opaque)
} GpuBuffer;

// --- GPU Kernel Structure ---
typedef struct {
    void* cl_kernel;  // cl_kernel handle (opaque)
    void* cl_program; // cl_program handle (opaque)
    OpCode opcode;
    BdiType type;
    size_t input_count;
    size_t work_size;
} GpuKernel;

// --- GPU Backend API ---
[[nodiscard]] int gpu_init(void);
void gpu_shutdown(void);
[[nodiscard]] int gpu_lower(const GraphNode* node, void** out_kernel);
[[nodiscard]] int gpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions);
[[nodiscard]] int gpu_sync(void);

// --- GPU Device VTable ---
extern DeviceVTable gpu_device_vtable;

// --- Buffer Management ---
[[nodiscard]] GpuBuffer* gpu_buffer_create(size_t size);
void gpu_buffer_free(GpuBuffer* buffer);
[[nodiscard]] int gpu_buffer_write(GpuBuffer* buffer, const void* data, size_t size);
[[nodiscard]] int gpu_buffer_read(GpuBuffer* buffer, void* data, size_t size);

#endif // AEON_GPU_BACKEND_OPENCL_H
