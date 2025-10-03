// ===================================================================
// DESC: Unified dispatch interface for all backend devices
//       (GPU, FPGA, BPU) with automatic device selection and
//       load balancing.
// PHASE 13: Backend Acceleration - Day 1
// ===================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// C23 constexpr for dispatch limits
constexpr int MAX_BACKEND_DEVICES = 16;
constexpr int MAX_WORK_QUEUE_SIZE = 1024;

// Backend device types
typedef enum {
    BACKEND_TYPE_NONE = 0,
    BACKEND_TYPE_GPU = 1,
    BACKEND_TYPE_FPGA = 2,
    BACKEND_TYPE_BPU = 3,
    BACKEND_TYPE_CPU = 4
} BackendType;

// Device capabilities flags
typedef enum {
    CAPABILITY_COMPUTE = (1 << 0),      // General computation
    CAPABILITY_MEMORY = (1 << 1),       // Large memory operations
    CAPABILITY_PARALLEL = (1 << 2),     // Parallel execution
    CAPABILITY_FIXED_FUNCTION = (1 << 3), // Fixed function hardware
    CAPABILITY_ASYNC = (1 << 4),        // Asynchronous execution
    CAPABILITY_ZERO_COPY = (1 << 5)     // Zero-copy data transfer
} DeviceCapability;

// Device state
typedef enum {
    DEVICE_STATE_UNINITIALIZED = 0,
    DEVICE_STATE_IDLE = 1,
    DEVICE_STATE_BUSY = 2,
    DEVICE_STATE_ERROR = 3
} DeviceState;

// Backend device descriptor
typedef struct {
    int device_id;
    BackendType type;
    const char* name;
    uint32_t capabilities;              // Bitmask of DeviceCapability
    _Atomic DeviceState state;
    _Atomic int workload;               // Current workload (0-100)
    _Atomic uint64_t operations_completed;
    _Atomic uint64_t operations_failed;
    size_t memory_capacity;
    _Atomic size_t memory_used;
    void* device_handle;                // Opaque device-specific handle
} BackendDevice;

_Static_assert(sizeof(BackendDevice) <= 128, "BackendDevice structure too large");

// Work item for dispatch
typedef struct {
    uint64_t work_id;
    BackendType preferred_type;
    uint32_t required_capabilities;
    void* work_data;
    size_t work_size;
    void (*completion_callback)(uint64_t work_id, int result);
} WorkItem;

// Global dispatch state
typedef struct {
    _Atomic bool initialized;
    BackendDevice devices[MAX_BACKEND_DEVICES];
    _Atomic int device_count;
    _Atomic uint64_t next_work_id;
    _Atomic int total_workload;
} DispatchState;

static DispatchState dispatch_state = {
    .initialized = false,
    .device_count = 0,
    .next_work_id = 1,
    .total_workload = 0
};

// ============================================================================
// Device Enumeration and Initialization
// ============================================================================

/**
 * Initialize the unified dispatch system
 */
[[nodiscard]] int backend_dispatch_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&dispatch_state.initialized, &expected, true)) {
        return 0; // Already initialized
    }
    
    printf("BACKEND_DISPATCH: Initializing unified dispatch system...\n");
    
    atomic_store(&dispatch_state.device_count, 0);
    atomic_store(&dispatch_state.next_work_id, 1);
    atomic_store(&dispatch_state.total_workload, 0);
    
    // Initialize all device slots
    for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
        dispatch_state.devices[i].device_id = -1;
        dispatch_state.devices[i].type = BACKEND_TYPE_NONE;
        dispatch_state.devices[i].name = nullptr;
        dispatch_state.devices[i].capabilities = 0;
        atomic_store(&dispatch_state.devices[i].state, DEVICE_STATE_UNINITIALIZED);
        atomic_store(&dispatch_state.devices[i].workload, 0);
        atomic_store(&dispatch_state.devices[i].operations_completed, 0);
        atomic_store(&dispatch_state.devices[i].operations_failed, 0);
        dispatch_state.devices[i].memory_capacity = 0;
        atomic_store(&dispatch_state.devices[i].memory_used, 0);
        dispatch_state.devices[i].device_handle = nullptr;
    }
    
    printf("BACKEND_DISPATCH: Initialization complete.\n");
    return 0;
}

/**
 * Shutdown the dispatch system
 */
void backend_dispatch_shutdown(void) {
    bool expected = true;
    if (!atomic_compare_exchange_strong(&dispatch_state.initialized, &expected, false)) {
        return; // Not initialized
    }
    
    printf("BACKEND_DISPATCH: Shutting down dispatch system...\n");
    
    int count = atomic_load(&dispatch_state.device_count);
    printf("BACKEND_DISPATCH: Managed %d devices during session.\n", count);
}

/**
 * Register a backend device with the dispatch system
 */
[[nodiscard]] int backend_register_device(BackendType type, const char* name,
                                          uint32_t capabilities, size_t memory_capacity,
                                          void* device_handle) {
    if (!atomic_load(&dispatch_state.initialized)) {
        backend_dispatch_init();
    }
    
    int count = atomic_load(&dispatch_state.device_count);
    if (count >= MAX_BACKEND_DEVICES) {
        printf("BACKEND_DISPATCH: Cannot register device, maximum reached.\n");
        return -1;
    }
    
    // Find first available slot
    for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
        if (dispatch_state.devices[i].device_id == -1) {
            dispatch_state.devices[i].device_id = i;
            dispatch_state.devices[i].type = type;
            dispatch_state.devices[i].name = name;
            dispatch_state.devices[i].capabilities = capabilities;
            atomic_store(&dispatch_state.devices[i].state, DEVICE_STATE_IDLE);
            atomic_store(&dispatch_state.devices[i].workload, 0);
            dispatch_state.devices[i].memory_capacity = memory_capacity;
            atomic_store(&dispatch_state.devices[i].memory_used, 0);
            dispatch_state.devices[i].device_handle = device_handle;
            
            atomic_fetch_add(&dispatch_state.device_count, 1);
            
            printf("BACKEND_DISPATCH: Registered device %d: %s (type=%d, caps=0x%x)\n",
                   i, name, type, capabilities);
            return i;
        }
    }
    
    return -1;
}

/**
 * Enumerate all registered devices
 */
[[nodiscard]] int backend_enumerate_devices(BackendDevice** out_devices, int* out_count) {
    if (!atomic_load(&dispatch_state.initialized)) {
        return -1;
    }
    
    if (out_devices == nullptr || out_count == nullptr) {
        return -1;
    }
    
    *out_devices = dispatch_state.devices;
    *out_count = atomic_load(&dispatch_state.device_count);
    
    return 0;
}

// ============================================================================
// Device Capability Detection
// ============================================================================

/**
 * Get capabilities of a specific device
 */
[[nodiscard]] uint32_t backend_get_capabilities(int device_id) {
    if (device_id < 0 || device_id >= MAX_BACKEND_DEVICES) {
        return 0;
    }
    
    if (dispatch_state.devices[device_id].device_id == -1) {
        return 0;
    }
    
    return dispatch_state.devices[device_id].capabilities;
}

/**
 * Check if device has specific capability
 */
[[nodiscard]] bool backend_has_capability(int device_id, DeviceCapability capability) {
    uint32_t caps = backend_get_capabilities(device_id);
    return (caps & capability) != 0;
}

/**
 * Get device state
 */
[[nodiscard]] DeviceState backend_get_device_state(int device_id) {
    if (device_id < 0 || device_id >= MAX_BACKEND_DEVICES) {
        return DEVICE_STATE_UNINITIALIZED;
    }
    
    return atomic_load(&dispatch_state.devices[device_id].state);
}

// ============================================================================
// Automatic Device Selection
// ============================================================================

/**
 * Select best device for given work requirements
 * Returns device_id or -1 if no suitable device found
 */
[[nodiscard]] int backend_select_device(BackendType preferred_type,
                                        uint32_t required_capabilities,
                                        size_t required_memory) {
    if (!atomic_load(&dispatch_state.initialized)) {
        return -1;
    }
    
    int best_device = -1;
    int best_score = -1;
    
    for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
        BackendDevice* dev = &dispatch_state.devices[i];
        
        // Skip unregistered or error devices
        if (dev->device_id == -1) continue;
        DeviceState state = atomic_load(&dev->state);
        if (state == DEVICE_STATE_UNINITIALIZED || state == DEVICE_STATE_ERROR) {
            continue;
        }
        
        // Check required capabilities
        if ((dev->capabilities & required_capabilities) != required_capabilities) {
            continue;
        }
        
        // Check memory requirements
        size_t mem_used = atomic_load(&dev->memory_used);
        if (dev->memory_capacity > 0 && mem_used + required_memory > dev->memory_capacity) {
            continue;
        }
        
        // Calculate selection score
        int score = 0;
        
        // Prefer requested type
        if (dev->type == preferred_type) {
            score += 100;
        }
        
        // Prefer less loaded devices
        int workload = atomic_load(&dev->workload);
        score += (100 - workload);
        
        // Prefer devices with more available memory
        if (dev->memory_capacity > 0) {
            int mem_available_pct = (int)(((dev->memory_capacity - mem_used) * 100) / dev->memory_capacity);
            score += mem_available_pct / 2;
        }
        
        if (score > best_score) {
            best_score = score;
            best_device = i;
        }
    }
    
    if (best_device >= 0) {
        printf("BACKEND_DISPATCH: Selected device %d (%s) with score %d\n",
               best_device, dispatch_state.devices[best_device].name, best_score);
    }
    
    return best_device;
}

// ============================================================================
// Load Balancing
// ============================================================================

/**
 * Balance load across all devices
 * Returns recommended device for new work
 */
[[nodiscard]] int backend_balance_load(BackendType preferred_type) {
    if (!atomic_load(&dispatch_state.initialized)) {
        return -1;
    }
    
    int min_workload = 101;
    int best_device = -1;
    
    // First pass: find device of preferred type with minimum workload
    for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
        BackendDevice* dev = &dispatch_state.devices[i];
        
        if (dev->device_id == -1) continue;
        if (dev->type != preferred_type) continue;
        
        DeviceState state = atomic_load(&dev->state);
        if (state != DEVICE_STATE_IDLE && state != DEVICE_STATE_BUSY) {
            continue;
        }
        
        int workload = atomic_load(&dev->workload);
        if (workload < min_workload) {
            min_workload = workload;
            best_device = i;
        }
    }
    
    // Second pass: if no device of preferred type, find any available device
    if (best_device == -1) {
        min_workload = 101;
        for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
            BackendDevice* dev = &dispatch_state.devices[i];
            
            if (dev->device_id == -1) continue;
            
            DeviceState state = atomic_load(&dev->state);
            if (state != DEVICE_STATE_IDLE && state != DEVICE_STATE_BUSY) {
                continue;
            }
            
            int workload = atomic_load(&dev->workload);
            if (workload < min_workload) {
                min_workload = workload;
                best_device = i;
            }
        }
    }
    
    return best_device;
}

/**
 * Update device workload
 */
void backend_update_workload(int device_id, int workload_delta) {
    if (device_id < 0 || device_id >= MAX_BACKEND_DEVICES) {
        return;
    }
    
    BackendDevice* dev = &dispatch_state.devices[device_id];
    if (dev->device_id == -1) return;
    
    int old_workload = atomic_fetch_add(&dev->workload, workload_delta);
    int new_workload = old_workload + workload_delta;
    
    // Clamp to 0-100 range
    if (new_workload < 0) {
        atomic_store(&dev->workload, 0);
        new_workload = 0;
    } else if (new_workload > 100) {
        atomic_store(&dev->workload, 100);
        new_workload = 100;
    }
    
    // Update device state based on workload
    if (new_workload == 0) {
        atomic_store(&dev->state, DEVICE_STATE_IDLE);
    } else {
        atomic_store(&dev->state, DEVICE_STATE_BUSY);
    }
}

// ============================================================================
// Unified Dispatch Interface
// ============================================================================

/**
 * Dispatch work to appropriate backend device
 */
[[nodiscard]] uint64_t backend_dispatch_work(BackendType preferred_type,
                                             uint32_t required_capabilities,
                                             size_t required_memory,
                                             void* work_data,
                                             size_t work_size,
                                             void (*completion_callback)(uint64_t, int)) {
    if (!atomic_load(&dispatch_state.initialized)) {
        return 0; // Invalid work ID
    }
    
    // Select appropriate device
    int device_id = backend_select_device(preferred_type, required_capabilities, required_memory);
    if (device_id < 0) {
        printf("BACKEND_DISPATCH: No suitable device found for work.\n");
        return 0;
    }
    
    // Generate work ID
    uint64_t work_id = atomic_fetch_add(&dispatch_state.next_work_id, 1);
    
    // Update device workload
    backend_update_workload(device_id, 10); // Add 10% workload
    
    BackendDevice* dev = &dispatch_state.devices[device_id];
    printf("BACKEND_DISPATCH: Dispatched work %llu to device %d (%s)\n",
           (unsigned long long)work_id, device_id, dev->name);
    
    // In a real implementation, this would enqueue the work to the device
    // For now, simulate immediate completion
    atomic_fetch_add(&dev->operations_completed, 1);
    backend_update_workload(device_id, -10); // Remove 10% workload
    
    if (completion_callback != nullptr) {
        completion_callback(work_id, 0); // Success
    }
    
    return work_id;
}

/**
 * Get dispatch statistics
 */
void backend_get_dispatch_stats(void) {
    if (!atomic_load(&dispatch_state.initialized)) {
        printf("BACKEND_DISPATCH: Not initialized.\n");
        return;
    }
    
    printf("\n=== Backend Dispatch Statistics ===\n");
    printf("Total devices: %d\n", atomic_load(&dispatch_state.device_count));
    printf("Total workload: %d%%\n", atomic_load(&dispatch_state.total_workload));
    printf("Next work ID: %llu\n", (unsigned long long)atomic_load(&dispatch_state.next_work_id));
    
    printf("\nDevice Details:\n");
    for (int i = 0; i < MAX_BACKEND_DEVICES; i++) {
        BackendDevice* dev = &dispatch_state.devices[i];
        if (dev->device_id == -1) continue;
        
        printf("  Device %d: %s\n", i, dev->name);
        printf("    Type: %d, State: %d\n", dev->type, atomic_load(&dev->state));
        printf("    Workload: %d%%\n", atomic_load(&dev->workload));
        printf("    Operations: %llu completed, %llu failed\n",
               (unsigned long long)atomic_load(&dev->operations_completed),
               (unsigned long long)atomic_load(&dev->operations_failed));
        if (dev->memory_capacity > 0) {
            printf("    Memory: %zu / %zu bytes used\n",
                   atomic_load(&dev->memory_used), dev->memory_capacity);
        }
    }
    printf("===================================\n\n");
}

// ============================================================================
// Phase 9 Scheduler Integration
// ============================================================================

// External scheduler functions (to be linked with Phase 9)
// extern int scheduler_register_device(int device_id, int device_type);
// extern int scheduler_assign_work(int device_id, void* work);
// extern int scheduler_steal_work(int from_device, int to_device);

/**
 * Register device with scheduler (Phase 9 integration)
 */
[[nodiscard]] int backend_register_with_scheduler(int device_id) {
    if (device_id < 0 || device_id >= MAX_BACKEND_DEVICES) {
        return -1;
    }
    
    BackendDevice* dev = &dispatch_state.devices[device_id];
    if (dev->device_id == -1) {
        return -1;
    }
    
    printf("BACKEND_DISPATCH: Registering device %d with scheduler\n", device_id);
    
    // In real implementation, call scheduler_register_device
    // return scheduler_register_device(device_id, dev->type);
    
    return 0;
}

/**
 * Implement work stealing across devices (Phase 9 integration)
 */
[[nodiscard]] int backend_steal_work(int from_device, int to_device) {
    if (from_device < 0 || from_device >= MAX_BACKEND_DEVICES ||
        to_device < 0 || to_device >= MAX_BACKEND_DEVICES) {
        return -1;
    }
    
    BackendDevice* from = &dispatch_state.devices[from_device];
    BackendDevice* to = &dispatch_state.devices[to_device];
    
    if (from->device_id == -1 || to->device_id == -1) {
        return -1;
    }
    
    int from_workload = atomic_load(&from->workload);
    int to_workload = atomic_load(&to->workload);
    
    // Only steal if source has significantly more work
    if (from_workload > to_workload + 20) {
        printf("BACKEND_DISPATCH: Stealing work from device %d to %d\n",
               from_device, to_device);
        
        // Transfer some workload
        backend_update_workload(from_device, -10);
        backend_update_workload(to_device, 10);
        
        // In real implementation, call scheduler_steal_work
        // return scheduler_steal_work(from_device, to_device);
        
        return 0;
    }
    
    return -1; // No work to steal
}

/**
 * Set device affinity for work (Phase 9 integration)
 */
[[nodiscard]] int backend_set_device_affinity(uint64_t work_id, int device_id) {
    if (device_id < 0 || device_id >= MAX_BACKEND_DEVICES) {
        return -1;
    }
    
    printf("BACKEND_DISPATCH: Setting affinity for work %llu to device %d\n",
           (unsigned long long)work_id, device_id);
    
    // In real implementation, update work affinity in scheduler
    return 0;
}

// ============================================================================
// Phase 7 Math Operations Integration
// ============================================================================

// External math operation functions (to be linked with Phase 7)
// extern int math_vector_add_gpu(void* a, void* b, void* result, size_t n);
// extern int math_matrix_mul_fpga(void* a, void* b, void* result, int m, int n, int k);

/**
 * Link math operations to GPU backend
 */
[[nodiscard]] int backend_math_gpu_vector_add(void* a, void* b, void* result, size_t n) {
    printf("BACKEND_DISPATCH: Dispatching vector_add to GPU (n=%zu)\n", n);
    
    // Select GPU device
    int device = backend_select_device(BACKEND_TYPE_GPU, CAPABILITY_COMPUTE, n * sizeof(float) * 3);
    if (device < 0) {
        return -1;
    }
    
    // In real implementation, call GPU vector add
    // return math_vector_add_gpu(a, b, result, n);
    
    return 0;
}

/**
 * Link math operations to FPGA backend
 */
[[nodiscard]] int backend_math_fpga_matrix_mul(void* a, void* b, void* result, 
                                               int m, int n, int k) {
    printf("BACKEND_DISPATCH: Dispatching matrix_mul to FPGA (%dx%d * %dx%d)\n", m, n, n, k);
    
    // Select FPGA device
    int device = backend_select_device(BACKEND_TYPE_FPGA, CAPABILITY_FIXED_FUNCTION, 
                                      (m * n + n * k + m * k) * sizeof(float));
    if (device < 0) {
        return -1;
    }
    
    // In real implementation, call FPGA matrix multiply
    // return math_matrix_mul_fpga(a, b, result, m, n, k);
    
    return 0;
}

/**
 * Automatic backend selection for math operations
 */
[[nodiscard]] int backend_math_auto_select(const char* operation, size_t data_size) {
    printf("BACKEND_DISPATCH: Auto-selecting backend for %s (size=%zu)\n", 
           operation, data_size);
    
    // Simple heuristic: GPU for large parallel ops, FPGA for fixed-function, BPU for simple ops
    if (data_size > 1024 * 1024) {
        return backend_select_device(BACKEND_TYPE_GPU, CAPABILITY_COMPUTE | CAPABILITY_PARALLEL, data_size);
    } else if (data_size > 4096) {
        return backend_select_device(BACKEND_TYPE_FPGA, CAPABILITY_FIXED_FUNCTION, data_size);
    } else {
        return backend_select_device(BACKEND_TYPE_BPU, CAPABILITY_COMPUTE, data_size);
    }
}
