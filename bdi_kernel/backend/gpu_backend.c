// ===================================================================
// DESC: Implements the conceptual GPU backend API.
//       This simulates a wrapper around a real GPU framework like CUDA.
// PHASE 13: Modernized with C23 features
// ===================================================================
#include "gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- GPU Backend State (Simulation) ---
static GpuDeviceState gpu_state = {
    .initialized = false,
    .mem_allocated = 0,
    .active_kernels = 0
};

[[nodiscard]] int gpu_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&gpu_state.initialized, &expected, true)) {
        return 0; // Already initialized
    }
    
    printf("GPU_BACKEND: Initializing GPU... OK.\n");
    atomic_store(&gpu_state.mem_allocated, 0);
    atomic_store(&gpu_state.active_kernels, 0);
    
    // Initialize streams
    for (int i = 0; i < GPU_MAX_STREAMS; i++) {
        gpu_state.streams[i].stream_id = i;
        atomic_store(&gpu_state.streams[i].is_active, false);
        gpu_state.streams[i].priority = 0;
    }
    
    return 0;
}

void gpu_shutdown(void) {
    bool expected = true;
    if (!atomic_compare_exchange_strong(&gpu_state.initialized, &expected, false)) {
        return; // Not initialized
    }
    
    printf("GPU_BACKEND: Shutting down GPU.\n");
    
    // Cleanup all streams
    for (int i = 0; i < GPU_MAX_STREAMS; i++) {
        atomic_store(&gpu_state.streams[i].is_active, false);
    }
}

[[nodiscard]] void* gpu_alloc(size_t size_bytes) {
    if (!atomic_load(&gpu_state.initialized)) {
        return nullptr;
    }
    
    size_t current_allocated = atomic_load(&gpu_state.mem_allocated);
    if (current_allocated + size_bytes > GPU_MEMORY_CAPACITY) {
        return nullptr;
    }
    
    // In a real implementation, this would return a device pointer.
    // We'll return a host pointer from malloc to simulate it.
    void* ptr = malloc(size_bytes);
    if (ptr != nullptr) {
        atomic_fetch_add(&gpu_state.mem_allocated, size_bytes);
        printf("GPU_BACKEND: Allocated %zu bytes on device (total: %zu).\n", 
               size_bytes, atomic_load(&gpu_state.mem_allocated));
    }
    return ptr;
}

void gpu_free(void* device_ptr) {
    if (device_ptr == nullptr) {
        return;
    }
    
    // In a real implementation, we would track allocation sizes
    // For simulation, just free the host pointer
    free(device_ptr);
}

[[nodiscard]] int gpu_memcpy_h2d(void* device_dst, const void* host_src, size_t size_bytes) {
    if (device_dst == nullptr || host_src == nullptr) {
        return -1;
    }
    
    printf("GPU_BACKEND: Copying %zu bytes Host -> Device.\n", size_bytes);
    memcpy(device_dst, host_src, size_bytes);
    return 0;
}

[[nodiscard]] int gpu_memcpy_d2h(void* host_dst, const void* device_src, size_t size_bytes) {
    if (host_dst == nullptr || device_src == nullptr) {
        return -1;
    }
    
    printf("GPU_BACKEND: Copying %zu bytes Device -> Host.\n", size_bytes);
    memcpy(host_dst, device_src, size_bytes);
    return 0;
}

[[nodiscard]] int gpu_launch_kernel(GpuKernel kernel, void** args) {
    if (!atomic_load(&gpu_state.initialized)) {
        return -1;
    }
    
    atomic_fetch_add(&gpu_state.active_kernels, 1);
    
    printf("GPU_BACKEND: Launching kernel '%s' [Grid: %dx%d, Block: %dx%d]\n",
           kernel.kernel_name,
           kernel.grid_dim_x, kernel.grid_dim_y,
           kernel.block_dim_x, kernel.block_dim_y);
    
    // Simulate kernel execution
    atomic_fetch_sub(&gpu_state.active_kernels, 1);
    return 0;
}

[[nodiscard]] int gpu_launch_kernel_async(GpuKernel kernel, void** args, GpuStream* stream) {
    if (!atomic_load(&gpu_state.initialized) || stream == nullptr) {
        return -1;
    }
    
    if (!atomic_load(&stream->is_active)) {
        return -1;
    }
    
    atomic_fetch_add(&gpu_state.active_kernels, 1);
    
    printf("GPU_BACKEND: Launching kernel '%s' async on stream %d [Grid: %dx%d, Block: %dx%d]\n",
           kernel.kernel_name, stream->stream_id,
           kernel.grid_dim_x, kernel.grid_dim_y,
           kernel.block_dim_x, kernel.block_dim_y);
    
    // Simulate async kernel execution
    atomic_fetch_sub(&gpu_state.active_kernels, 1);
    return 0;
}

[[nodiscard]] int gpu_sync(void) {
    if (!atomic_load(&gpu_state.initialized)) {
        return -1;
    }
    
    // Wait for all active kernels to complete
    while (atomic_load(&gpu_state.active_kernels) > 0) {
        // Busy wait (in real implementation, use proper synchronization)
    }
    
    printf("GPU_BACKEND: Device synchronized.\n");
    return 0;
}

[[nodiscard]] int gpu_stream_sync(GpuStream* stream) {
    if (stream == nullptr || !atomic_load(&stream->is_active)) {
        return -1;
    }
    
    printf("GPU_BACKEND: Stream %d synchronized.\n", stream->stream_id);
    return 0;
}

[[nodiscard]] GpuStream* gpu_stream_create(int priority) {
    if (!atomic_load(&gpu_state.initialized)) {
        return nullptr;
    }
    
    // Find an inactive stream
    for (int i = 0; i < GPU_MAX_STREAMS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&gpu_state.streams[i].is_active, &expected, true)) {
            gpu_state.streams[i].priority = priority;
            printf("GPU_BACKEND: Created stream %d with priority %d.\n", i, priority);
            return &gpu_state.streams[i];
        }
    }
    
    return nullptr; // No available streams
}

void gpu_stream_destroy(GpuStream* stream) {
    if (stream == nullptr) {
        return;
    }
    
    atomic_store(&stream->is_active, false);
    printf("GPU_BACKEND: Destroyed stream %d.\n", stream->stream_id);
}

[[nodiscard]] GpuDeviceState* gpu_get_state(void) {
    return &gpu_state;
}
