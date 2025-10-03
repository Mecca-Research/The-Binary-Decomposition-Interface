// ===================================================================
// DESC: Implements the conceptual GPU backend API.
//       This simulates a wrapper around a real GPU framework like CUDA.
// ===================================================================
#include "c23_compat.h"
#include "gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- GPU Backend State (Simulation) ---
static bool gpu_initialized = false;
static size_t gpu_mem_allocated = 0;
#define GPU_MEMORY_CAPACITY (1024 * 1024) // Simulate 1MB of GPU memory

[[nodiscard]] int gpu_init() {
    if (gpu_initialized) return 0;
    printf("GPU_BACKEND: Initializing GPU... OK.\n");
    gpu_initialized = true;
    gpu_mem_allocated = 0;
    return 0;
}

void gpu_shutdown() {
    if (!gpu_initialized) return;
    printf("GPU_BACKEND: Shutting down GPU.\n");
    gpu_initialized = false;
}
[[nodiscard]] 
void* gpu_alloc(size_t size_bytes) {
    if (!gpu_initialized || gpu_mem_allocated + size_bytes > GPU_MEMORY_CAPACITY) {
        return nullptr;
    }
    // In a real implementation, this would return a device pointer.
    // We'll return a host pointer from malloc to simulate it.
    void* ptr = malloc(size_bytes);
    if (ptr) {
        gpu_mem_allocated += size_bytes;
        printf("GPU_BACKEND: Allocated %zu bytes on device.\n", size_bytes);
    }
    return ptr;
}

void gpu_free(void* device_ptr) {
    // In a real implementation, we would need to know the size to decrement
    // gpu_mem_allocated. For this simulation, we'll just free the host pointer.
    free(device_ptr);
}

int gpu_memcpy_h2d(void* device_dst, const void* host_src, size_t size_bytes) {
    printf("GPU_BACKEND: Copying %zu bytes Host -> Device.\n", size_bytes);
    memcpy(device_dst, host_src, size_bytes);
    return 0;
}

int gpu_memcpy_d2h(void* host_dst, const void* device_src, size_t size_bytes) {
    printf("GPU_BACKEND: Copying %zu bytes Device -> Host.\n", size_bytes);
    memcpy(host_dst, device_src, size_bytes);
    return 0;
}

int gpu_launch_kernel(GpuKernel kernel, void** args) {
    printf("GPU_BACKEND: Launching kernel '%s'...\n", kernel.kernel_name);
    // This is the simulation point. A real backend would use the CUDA/ROCm
    // driver API to launch the kernel on the hardware.
    // For our test, we'll just print that it happened.
    return 0;
}

int gpu_sync() {
    printf("GPU_BACKEND: Synchronizing device.\n");
    return 0;
}
