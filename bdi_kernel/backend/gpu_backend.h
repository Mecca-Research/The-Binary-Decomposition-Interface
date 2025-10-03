// ===================================================================
// DESC: Defines the interface and data structures for
//       interacting with a GPU, conceptually wrapping a framework
//       like CUDA or ROCm.
// PHASE 13: Modernized with C23 features + Day 3 enhancements
// ===================================================================
#ifndef AEON_GPU_BACKEND_H
#define AEON_GPU_BACKEND_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "ham.h" // For HamRegion

// C23 constexpr for device limits
constexpr size_t GPU_MEMORY_CAPACITY = (1024 * 1024 * 1024); // 1GB
constexpr int GPU_MAX_GRID_DIM = 65535;
constexpr int GPU_MAX_BLOCK_DIM = 1024;
constexpr int GPU_MAX_STREAMS = 32;

// GPU Backend Types
typedef enum {
    GPU_BACKEND_CUDA = 0,
    GPU_BACKEND_OPENCL = 1,
    GPU_BACKEND_SIMULATION = 2
} GpuBackendType;

// --- GPU Kernel Representation ---
typedef struct {
    const char* kernel_name;
    int grid_dim_x;
    int grid_dim_y;
    int block_dim_x;
    int block_dim_y;
    void* kernel_handle;
    uint64_t kernel_hash;
} GpuKernel;

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

// ============================================================================
// Core GPU Functions
// ============================================================================

[[nodiscard]] int gpu_init(void);
void gpu_shutdown(void);
[[nodiscard]] void* gpu_alloc(size_t size_bytes);
void gpu_free(void* device_ptr);
[[nodiscard]] int gpu_memcpy_h2d(void* device_dst, const void* host_src, size_t size_bytes);
[[nodiscard]] int gpu_memcpy_d2h(void* host_dst, const void* device_src, size_t size_bytes);

// ============================================================================
// Kernel Launch Functions
// ============================================================================

[[nodiscard]] int gpu_launch_kernel(GpuKernel kernel, void** args);
[[nodiscard]] int gpu_launch_kernel_async(GpuKernel kernel, void** args, GpuStream* stream);
[[nodiscard]] int gpu_launch_kernel_async_callback(GpuKernel kernel, void** args, 
                                                   GpuStream* stream,
                                                   void (*callback)(int, void*),
                                                   void* user_data);
[[nodiscard]] int gpu_sync(void);
[[nodiscard]] int gpu_stream_sync(GpuStream* stream);

// ============================================================================
// Stream Management
// ============================================================================

[[nodiscard]] GpuStream* gpu_stream_create(int priority);
void gpu_stream_destroy(GpuStream* stream);

// ============================================================================
// Backend Selection
// ============================================================================

[[nodiscard]] int gpu_select_backend(GpuBackendType type);

// ============================================================================
// Memory Management (Unified)
// ============================================================================

[[nodiscard]] void* gpu_alloc_managed(size_t size_bytes, uint32_t flags);
void gpu_free_managed(void* device_ptr, size_t size_bytes);
[[nodiscard]] int gpu_memcpy_h2d_zerocopy(void* device_dst, const void* host_src, size_t size_bytes);
[[nodiscard]] int gpu_memcpy_d2h_zerocopy(void* host_dst, const void* device_src, size_t size_bytes);

// ============================================================================
// Statistics and Monitoring
// ============================================================================

[[nodiscard]] GpuDeviceState* gpu_get_state(void);
void gpu_kernel_cache_stats(void);
void gpu_print_statistics(void);

#endif // AEON_GPU_BACKEND_H
