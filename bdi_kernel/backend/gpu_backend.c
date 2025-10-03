
// ===================================================================
// DESC: Implements the conceptual GPU backend API.
//       This simulates a wrapper around a real GPU framework like CUDA.
// PHASE 13: Modernized with C23 features + Day 3 optimizations
// ===================================================================
#include "gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// Allocation Tracking for Memory Accounting
// ============================================================================

// Allocation metadata for tracking sizes
typedef struct {
    void* ptr;
    size_t size;
} GpuAllocation;

#define MAX_GPU_ALLOCATIONS 1024
static GpuAllocation gpu_allocations[MAX_GPU_ALLOCATIONS];
static _Atomic uint32_t gpu_allocation_count = 0;

// ============================================================================
// Kernel Compilation Cache
// ============================================================================

constexpr int KERNEL_CACHE_SIZE = 256;
constexpr int KERNEL_CACHE_LRU_MAX_AGE = 1000;

typedef struct KernelCacheEntry {
    uint64_t kernel_hash;
    void* compiled_kernel;
    size_t kernel_size;
    _Atomic int access_count;
    _Atomic uint64_t last_access_time;
    _Atomic bool is_valid;
} KernelCacheEntry;

typedef struct {
    KernelCacheEntry entries[KERNEL_CACHE_SIZE];
    _Atomic int entry_count;
    _Atomic uint64_t cache_hits;
    _Atomic uint64_t cache_misses;
    _Atomic uint64_t evictions;
} KernelCache;

static KernelCache kernel_cache = {
    .entry_count = 0,
    .cache_hits = 0,
    .cache_misses = 0,
    .evictions = 0
};

// Simple hash function for kernel
static uint64_t hash_kernel(const char* kernel_name, int grid_x, int grid_y, 
                            int block_x, int block_y) {
    uint64_t hash = 5381;
    
    // Hash kernel name
    const char* str = kernel_name;
    while (*str) {
        hash = ((hash << 5) + hash) + (*str++);
    }
    
    // Hash dimensions
    hash = ((hash << 5) + hash) + grid_x;
    hash = ((hash << 5) + hash) + grid_y;
    hash = ((hash << 5) + hash) + block_x;
    hash = ((hash << 5) + hash) + block_y;
    
    return hash;
}

// Get current timestamp
static uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Find kernel in cache
[[nodiscard]] static KernelCacheEntry* kernel_cache_find(uint64_t hash) {
    for (int i = 0; i < KERNEL_CACHE_SIZE; i++) {
        if (atomic_load(&kernel_cache.entries[i].is_valid) &&
            kernel_cache.entries[i].kernel_hash == hash) {
            atomic_fetch_add(&kernel_cache.entries[i].access_count, 1);
            atomic_store(&kernel_cache.entries[i].last_access_time, get_timestamp());
            atomic_fetch_add(&kernel_cache.cache_hits, 1);
            return &kernel_cache.entries[i];
        }
    }
    
    atomic_fetch_add(&kernel_cache.cache_misses, 1);
    return nullptr;
}

// Add kernel to cache (LRU eviction)
[[nodiscard]] static KernelCacheEntry* kernel_cache_add(uint64_t hash, void* compiled_kernel, 
                                                        size_t kernel_size) {
    // Try to find empty slot first
    for (int i = 0; i < KERNEL_CACHE_SIZE; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&kernel_cache.entries[i].is_valid, &expected, true)) {
            kernel_cache.entries[i].kernel_hash = hash;
            kernel_cache.entries[i].compiled_kernel = compiled_kernel;
            kernel_cache.entries[i].kernel_size = kernel_size;
            atomic_store(&kernel_cache.entries[i].access_count, 1);
            atomic_store(&kernel_cache.entries[i].last_access_time, get_timestamp());
            atomic_fetch_add(&kernel_cache.entry_count, 1);
            return &kernel_cache.entries[i];
        }
    }
    
    // Cache full, find LRU entry to evict
    int lru_index = 0;
    uint64_t oldest_time = atomic_load(&kernel_cache.entries[0].last_access_time);
    
    for (int i = 1; i < KERNEL_CACHE_SIZE; i++) {
        uint64_t time = atomic_load(&kernel_cache.entries[i].last_access_time);
        if (time < oldest_time) {
            oldest_time = time;
            lru_index = i;
        }
    }
    
    // Evict LRU entry
    KernelCacheEntry* entry = &kernel_cache.entries[lru_index];
    if (entry->compiled_kernel != nullptr) {
        free(entry->compiled_kernel);
    }
    
    entry->kernel_hash = hash;
    entry->compiled_kernel = compiled_kernel;
    entry->kernel_size = kernel_size;
    atomic_store(&entry->access_count, 1);
    atomic_store(&entry->last_access_time, get_timestamp());
    atomic_fetch_add(&kernel_cache.evictions, 1);
    
    return entry;
}

// Get cache statistics
void gpu_kernel_cache_stats(void) {
    printf("\n=== GPU Kernel Cache Statistics ===\n");
    printf("Entries: %d / %d\n", atomic_load(&kernel_cache.entry_count), KERNEL_CACHE_SIZE);
    printf("Cache hits: %llu\n", (unsigned long long)atomic_load(&kernel_cache.cache_hits));
    printf("Cache misses: %llu\n", (unsigned long long)atomic_load(&kernel_cache.cache_misses));
    printf("Evictions: %llu\n", (unsigned long long)atomic_load(&kernel_cache.evictions));
    
    uint64_t total = atomic_load(&kernel_cache.cache_hits) + atomic_load(&kernel_cache.cache_misses);
    if (total > 0) {
        double hit_rate = (double)atomic_load(&kernel_cache.cache_hits) * 100.0 / total;
        printf("Hit rate: %.2f%%\n", hit_rate);
    }
    printf("====================================\n\n");
}

// ============================================================================
// Asynchronous Execution Support
// ============================================================================

typedef struct AsyncKernelWork {
    GpuKernel kernel;
    void** args;
    void (*completion_callback)(int result, void* user_data);
    void* user_data;
    _Atomic bool completed;
    int result;
} AsyncKernelWork;

constexpr int MAX_ASYNC_WORK = 128;

typedef struct {
    AsyncKernelWork work_items[MAX_ASYNC_WORK];
    _Atomic int work_count;
    _Atomic uint64_t total_async_launches;
} AsyncExecutor;

static AsyncExecutor async_executor = {
    .work_count = 0,
    .total_async_launches = 0
};

// ============================================================================
// CUDA/OpenCL Abstraction Layer
// ============================================================================
// NOTE: GpuBackendType typedef removed - already declared in gpu_backend.h

typedef struct {
    GpuBackendType type;
    const char* name;
    bool (*init_func)(void);
    void (*shutdown_func)(void);
    void* (*alloc_func)(size_t);
    void (*free_func)(void*);
} GpuBackendImpl;

static GpuBackendType current_backend = GPU_BACKEND_SIMULATION;

// Simulation backend functions
static bool sim_init(void) { return true; }
static void sim_shutdown(void) {}
static void* sim_alloc(size_t size) { return malloc(size); }
static void sim_free(void* ptr) { free(ptr); }

static GpuBackendImpl backends[] = {
    {GPU_BACKEND_CUDA, "CUDA", sim_init, sim_shutdown, sim_alloc, sim_free},
    {GPU_BACKEND_OPENCL, "OpenCL", sim_init, sim_shutdown, sim_alloc, sim_free},
    {GPU_BACKEND_SIMULATION, "Simulation", sim_init, sim_shutdown, sim_alloc, sim_free}
};

// Select GPU backend at runtime
[[nodiscard]] int gpu_select_backend(GpuBackendType type) {
    if (type >= 3) return -1;
    
    current_backend = type;
    printf("GPU_BACKEND: Selected backend: %s\n", backends[type].name);
    return 0;
}

// ============================================================================
// GPU Backend State
// ============================================================================

static GpuDeviceState gpu_state = {
    .initialized = false,
    .mem_allocated = 0,
    .active_kernels = 0
};

// ============================================================================
// Core GPU Functions
// ============================================================================

[[nodiscard]] int gpu_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&gpu_state.initialized, &expected, true)) {
        return 0; // Already initialized
    }
    
    printf("GPU_BACKEND: Initializing GPU (%s)... OK.\n", backends[current_backend].name);
    atomic_store(&gpu_state.mem_allocated, 0);
    atomic_store(&gpu_state.active_kernels, 0);
    
    // Initialize allocation tracking
    atomic_store(&gpu_allocation_count, 0);
    for (int i = 0; i < MAX_GPU_ALLOCATIONS; i++) {
        gpu_allocations[i].ptr = nullptr;
        gpu_allocations[i].size = 0;
    }
    
    // Initialize streams
    for (int i = 0; i < GPU_MAX_STREAMS; i++) {
        gpu_state.streams[i].stream_id = i;
        atomic_store(&gpu_state.streams[i].is_active, false);
        gpu_state.streams[i].priority = 0;
    }
    
    // Initialize kernel cache
    for (int i = 0; i < KERNEL_CACHE_SIZE; i++) {
        kernel_cache.entries[i].kernel_hash = 0;
        kernel_cache.entries[i].compiled_kernel = nullptr;
        kernel_cache.entries[i].kernel_size = 0;
        atomic_store(&kernel_cache.entries[i].access_count, 0);
        atomic_store(&kernel_cache.entries[i].last_access_time, 0);
        atomic_store(&kernel_cache.entries[i].is_valid, false);
    }
    
    // Initialize async executor
    for (int i = 0; i < MAX_ASYNC_WORK; i++) {
        atomic_store(&async_executor.work_items[i].completed, true);
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
    
    // Cleanup kernel cache
    for (int i = 0; i < KERNEL_CACHE_SIZE; i++) {
        if (atomic_load(&kernel_cache.entries[i].is_valid)) {
            if (kernel_cache.entries[i].compiled_kernel != nullptr) {
                free(kernel_cache.entries[i].compiled_kernel);
            }
            atomic_store(&kernel_cache.entries[i].is_valid, false);
        }
    }
    
    // Print final statistics
    gpu_kernel_cache_stats();
}

[[nodiscard]] void* gpu_alloc(size_t size_bytes) {
    if (!atomic_load(&gpu_state.initialized)) {
        return nullptr;
    }
    
    size_t current_allocated = atomic_load(&gpu_state.mem_allocated);
    if (current_allocated + size_bytes > GPU_MEMORY_CAPACITY) {
        return nullptr;
    }
    
    void* ptr = backends[current_backend].alloc_func(size_bytes);
    if (ptr != nullptr) {
        atomic_fetch_add(&gpu_state.mem_allocated, size_bytes);
        
        // Track allocation for proper memory accounting
        uint32_t idx = atomic_fetch_add(&gpu_allocation_count, 1);
        if (idx < MAX_GPU_ALLOCATIONS) {
            gpu_allocations[idx].ptr = ptr;
            gpu_allocations[idx].size = size_bytes;
        }
        
        printf("GPU_BACKEND: Allocated %zu bytes on device (total: %zu).\n", 
               size_bytes, atomic_load(&gpu_state.mem_allocated));
    }
    return ptr;
}

void gpu_free(void* device_ptr) {
    if (device_ptr == nullptr) {
        return;
    }
    
    // Find allocation and decrement mem_allocated
    uint32_t count = atomic_load(&gpu_allocation_count);
    for (uint32_t i = 0; i < count && i < MAX_GPU_ALLOCATIONS; i++) {
        if (gpu_allocations[i].ptr == device_ptr) {
            size_t size = gpu_allocations[i].size;
            atomic_fetch_sub(&gpu_state.mem_allocated, size);
            
            // Clear entry
            gpu_allocations[i].ptr = nullptr;
            gpu_allocations[i].size = 0;
            
            printf("GPU_BACKEND: Freed %zu bytes (total: %zu).\n",
                   size, atomic_load(&gpu_state.mem_allocated));
            break;
        }
    }
    
    backends[current_backend].free_func(device_ptr);
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

// ============================================================================
// Kernel Compilation and Caching
// ============================================================================

[[nodiscard]] static void* compile_kernel(GpuKernel* kernel) {
    // Simulate kernel compilation
    size_t kernel_size = 1024 + (rand() % 4096);
    void* compiled = malloc(kernel_size);
    if (compiled != nullptr) {
        memset(compiled, 0xCC, kernel_size);
        printf("GPU_BACKEND: Compiled kernel '%s' (%zu bytes)\n", 
               kernel->kernel_name, kernel_size);
    }
    return compiled;
}

[[nodiscard]] static void* get_or_compile_kernel(GpuKernel* kernel) {
    // Calculate kernel hash
    uint64_t hash = hash_kernel(kernel->kernel_name, 
                                kernel->grid_dim_x, kernel->grid_dim_y,
                                kernel->block_dim_x, kernel->block_dim_y);
    
    // Check cache
    KernelCacheEntry* cached = kernel_cache_find(hash);
    if (cached != nullptr) {
        printf("GPU_BACKEND: Kernel cache HIT for '%s'\n", kernel->kernel_name);
        return cached->compiled_kernel;
    }
    
    // Cache miss, compile kernel
    printf("GPU_BACKEND: Kernel cache MISS for '%s', compiling...\n", kernel->kernel_name);
    void* compiled = compile_kernel(kernel);
    if (compiled != nullptr) {
        size_t size = 1024 + (rand() % 4096);
        kernel_cache_add(hash, compiled, size);
    }
    
    return compiled;
}

// ============================================================================
// Kernel Launch Functions
// ============================================================================

[[nodiscard]] int gpu_launch_kernel(GpuKernel kernel, void** args) {
    if (!atomic_load(&gpu_state.initialized)) {
        return -1;
    }
    
    // Get or compile kernel
    void* compiled = get_or_compile_kernel(&kernel);
    if (compiled == nullptr) {
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
    
    // Get or compile kernel
    void* compiled = get_or_compile_kernel(&kernel);
    if (compiled == nullptr) {
        return -1;
    }
    
    atomic_fetch_add(&gpu_state.active_kernels, 1);
    atomic_fetch_add(&async_executor.total_async_launches, 1);
    
    printf("GPU_BACKEND: Launching kernel '%s' async on stream %d [Grid: %dx%d, Block: %dx%d]\n",
           kernel.kernel_name, stream->stream_id,
           kernel.grid_dim_x, kernel.grid_dim_y,
           kernel.block_dim_x, kernel.block_dim_y);
    
    // Simulate async kernel execution
    atomic_fetch_sub(&gpu_state.active_kernels, 1);
    return 0;
}

// Launch kernel with completion callback
[[nodiscard]] int gpu_launch_kernel_async_callback(GpuKernel kernel, void** args, 
                                                   GpuStream* stream,
                                                   void (*callback)(int, void*),
                                                   void* user_data) {
    if (!atomic_load(&gpu_state.initialized) || stream == nullptr) {
        return -1;
    }
    
    // Find available work slot
    for (int i = 0; i < MAX_ASYNC_WORK; i++) {
        bool expected = true;
        if (atomic_compare_exchange_strong(&async_executor.work_items[i].completed, 
                                          &expected, false)) {
            AsyncKernelWork* work = &async_executor.work_items[i];
            work->kernel = kernel;
            work->args = args;
            work->completion_callback = callback;
            work->user_data = user_data;
            
            // Launch kernel
            int result = gpu_launch_kernel_async(kernel, args, stream);
            
            // Simulate completion
            work->result = result;
            atomic_store(&work->completed, true);
            
            if (callback != nullptr) {
                callback(result, user_data);
            }
            
            return 0;
        }
    }
    
    return -1; // No available work slots
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

// ============================================================================
// Stream Management
// ============================================================================

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

// ============================================================================
// Memory Management Integration (from Day 2)
// ============================================================================

extern void* backend_alloc(int device_id, size_t size, uint32_t flags);
extern void backend_free(int device_id, void* ptr, size_t size);
extern int backend_transfer_h2d_zerocopy(int device_id, void* device_ptr,
                                        const void* host_ptr, size_t size);
extern int backend_transfer_d2h_zerocopy(int device_id, void* host_ptr,
                                        const void* device_ptr, size_t size);

#define MEM_FLAG_ZERO_COPY (1 << 0)
#define MEM_FLAG_UNIFIED (1 << 1)
#define MEM_FLAG_PINNED (1 << 2)

[[nodiscard]] void* gpu_alloc_managed(size_t size_bytes, uint32_t flags) {
    if (!atomic_load(&gpu_state.initialized)) {
        return nullptr;
    }
    
    void* ptr = backend_alloc(0, size_bytes, flags);
    if (ptr != nullptr) {
        atomic_fetch_add(&gpu_state.mem_allocated, size_bytes);
        printf("GPU_BACKEND: Managed alloc %zu bytes (flags=0x%x)\n", size_bytes, flags);
    }
    return ptr;
}

void gpu_free_managed(void* device_ptr, size_t size_bytes) {
    if (device_ptr == nullptr) {
        return;
    }
    
    backend_free(0, device_ptr, size_bytes);
    atomic_fetch_sub(&gpu_state.mem_allocated, size_bytes);
    printf("GPU_BACKEND: Managed free %zu bytes\n", size_bytes);
}

[[nodiscard]] int gpu_memcpy_h2d_zerocopy(void* device_dst, const void* host_src, size_t size_bytes) {
    if (device_dst == nullptr || host_src == nullptr) {
        return -1;
    }
    
    printf("GPU_BACKEND: Zero-copy H2D %zu bytes\n", size_bytes);
    return backend_transfer_h2d_zerocopy(0, device_dst, host_src, size_bytes);
}

[[nodiscard]] int gpu_memcpy_d2h_zerocopy(void* host_dst, const void* device_src, size_t size_bytes) {
    if (host_dst == nullptr || device_src == nullptr) {
        return -1;
    }
    
    printf("GPU_BACKEND: Zero-copy D2H %zu bytes\n", size_bytes);
    return backend_transfer_d2h_zerocopy(0, host_dst, device_src, size_bytes);
}

// ============================================================================
// Statistics and Monitoring
// ============================================================================

void gpu_print_statistics(void) {
    printf("\n=== GPU Backend Statistics ===\n");
    printf("Backend: %s\n", backends[current_backend].name);
    printf("Initialized: %s\n", atomic_load(&gpu_state.initialized) ? "Yes" : "No");
    printf("Memory allocated: %zu bytes\n", atomic_load(&gpu_state.mem_allocated));
    printf("Active kernels: %d\n", atomic_load(&gpu_state.active_kernels));
    printf("Total async launches: %llu\n", 
           (unsigned long long)atomic_load(&async_executor.total_async_launches));
    
    int active_streams = 0;
    for (int i = 0; i < GPU_MAX_STREAMS; i++) {
        if (atomic_load(&gpu_state.streams[i].is_active)) {
            active_streams++;
        }
    }
    printf("Active streams: %d / %d\n", active_streams, GPU_MAX_STREAMS);
    printf("===============================\n\n");
    
    gpu_kernel_cache_stats();
}
