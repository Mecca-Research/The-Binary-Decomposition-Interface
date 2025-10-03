// ===================================================================
// DESC: Defines the interface and data structures for
//       interacting with a GPU, conceptually wrapping a framework
//       like CUDA or ROCm.
// PHASE 13: Modernized with C23 features
// ===================================================================
#ifndef AEON_GPU_BACKEND_H
#define AEON_GPU_BACKEND_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "ham.h" // For HamRegion

// C23 constexpr for device limits
constexpr size_t GPU_MEMORY_CAPACITY = (1024 * 1024 * 1024); // 1GB
constexpr int GPU_MAX_GRID_DIM = 65535;
constexpr int GPU_MAX_BLOCK_DIM = 1024;
constexpr int GPU_MAX_STREAMS = 32;

// --- GPU Kernel Representation ---
// This struct represents a compiled GPU kernel ready for launch.
typedef struct {
    const char* kernel_name; // Name of the kernel function to launch
    int grid_dim_x;
    int grid_dim_y;
    int block_dim_x;
    int block_dim_y;
    void* kernel_handle;     // Opaque handle to compiled kernel
    uint64_t kernel_hash;    // Hash for caching
} GpuKernel;

// Validate kernel structure size
_Static_assert(sizeof(GpuKernel) <= 64, "GpuKernel structure too large");

// --- GPU Stream for Asynchronous Execution ---
typedef struct {
    int stream_id;
    _Atomic bool is_active;
    int priority;
} GpuStream;

// --- GPU Device State ---
typedef struct {
    _Atomic bool initialized;
    _Atomic size_t mem_allocated;
    _Atomic int active_kernels;
    GpuStream streams[GPU_MAX_STREAMS];
} GpuDeviceState;

// --- GPU Backend API ---
// This API represents the functions our GPU device will use.
// In a real implementation, these would be wrappers around cudaMalloc,
// cudaMemcpy, cuLaunchKernel, etc.

// Initializes the GPU device.
[[nodiscard]] int gpu_init(void);

// Frees all GPU resources.
void gpu_shutdown(void);

// Allocates memory on the GPU device.
[[nodiscard]] void* gpu_alloc(size_t size_bytes);

// Frees memory on the GPU device.
void gpu_free(void* device_ptr);

// Copies memory from Host (CPU) to Device (GPU).
[[nodiscard]] int gpu_memcpy_h2d(void* device_dst, const void* host_src, size_t size_bytes);

// Copies memory from Device (GPU) to Host (CPU).
[[nodiscard]] int gpu_memcpy_d2h(void* host_dst, const void* device_src, size_t size_bytes);

// Launches a kernel on the GPU.
[[nodiscard]] int gpu_launch_kernel(GpuKernel kernel, void** args);

// Launches a kernel asynchronously on a specific stream.
[[nodiscard]] int gpu_launch_kernel_async(GpuKernel kernel, void** args, GpuStream* stream);

// Blocks until the GPU has finished all enqueued work.
[[nodiscard]] int gpu_sync(void);

// Synchronizes a specific stream.
[[nodiscard]] int gpu_stream_sync(GpuStream* stream);

// Creates a new GPU stream.
[[nodiscard]] GpuStream* gpu_stream_create(int priority);

// Destroys a GPU stream.
void gpu_stream_destroy(GpuStream* stream);

// Gets current device state.
[[nodiscard]] GpuDeviceState* gpu_get_state(void);

#endif // AEON_GPU_BACKEND_H
