// ===================================================================
// DESC: Defines the interface and data structures for
//       interacting with a GPU, conceptually wrapping a framework
//       like CUDA or ROCm.
// ===================================================================
#ifndef AEON_GPU_BACKEND_H
#define AEON_GPU_BACKEND_H

#include "ham.h" // For HamRegion

// --- GPU Kernel Representation ---
// This struct represents a compiled GPU kernel ready for launch.
typedef struct {
    const char* kernel_name; // Name of the kernel function to launch
    int grid_dim_x;
    int grid_dim_y;
    int block_dim_x;
    int block_dim_y;
} GpuKernel;

// --- GPU Backend API ---
// This API represents the functions our GPU device will use.
// In a real implementation, these would be wrappers around cudaMalloc,
// cudaMemcpy, cuLaunchKernel, etc.

// Initializes the GPU device.
int gpu_init();
// Frees all GPU resources.
void gpu_shutdown();
// Allocates memory on the GPU device.
void* gpu_alloc(size_t size_bytes);
// Frees memory on the GPU device.
void gpu_free(void* device_ptr);
// Copies memory from Host (CPU) to Device (GPU).
int gpu_memcpy_h2d(void* device_dst, const void* host_src, size_t size_bytes);
// Copies memory from Device (GPU) to Host (CPU).
int gpu_memcpy_d2h(void* host_dst, const void* device_src, size_t size_bytes);
// Launches a kernel on the GPU.
int gpu_launch_kernel(GpuKernel kernel, void** args);
// Blocks until the GPU has finished all enqueued work.
int gpu_sync();


#endif // AEON_GPU_BACKEND_H
