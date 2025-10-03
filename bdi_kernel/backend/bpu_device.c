// ===================================================================
// DESC: BPU device implementation with C23 features and optimizations
// PHASE 13: Day 4 enhancements
// ===================================================================
#include "device.h"
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

constexpr int BPU_MAX_QUEUE_DEPTH = 256;
constexpr int BPU_NUM_UNITS = 4; // Multiple BPU execution units

_Static_assert(BPU_MAX_QUEUE_DEPTH > 0, "BPU queue depth must be positive");

// BPU Execution Unit
typedef struct {
    int unit_id;
    _Atomic bool is_busy;
    _Atomic int operations_executed;
    OpCode current_op;
} BpuExecutionUnit;

// BPU Device State
typedef struct {
    _Atomic bool initialized;
    _Atomic int operations_queued;
    _Atomic int operations_completed;
    _Atomic int operations_failed;
    BpuExecutionUnit units[BPU_NUM_UNITS];
    _Atomic uint64_t total_execution_time_ns;
} BpuDeviceState;

static BpuDeviceState bpu_state = {
    .initialized = false,
    .operations_queued = 0,
    .operations_completed = 0,
    .operations_failed = 0,
    .total_execution_time_ns = 0
};

// ============================================================================
// BPU Initialization
// ============================================================================

[[nodiscard]] static int bpu_init(void) {
    bool expected = false;
    if (!atomic_compare_exchange_strong(&bpu_state.initialized, &expected, true)) {
        return 0;
    }
    
    printf("BPU_DEVICE: Initializing BPU with %d execution units... OK.\n", BPU_NUM_UNITS);
    atomic_store(&bpu_state.operations_queued, 0);
    atomic_store(&bpu_state.operations_completed, 0);
    atomic_store(&bpu_state.operations_failed, 0);
    atomic_store(&bpu_state.total_execution_time_ns, 0);
    
    // Initialize execution units
    for (int i = 0; i < BPU_NUM_UNITS; i++) {
        bpu_state.units[i].unit_id = i;
        atomic_store(&bpu_state.units[i].is_busy, false);
        atomic_store(&bpu_state.units[i].operations_executed, 0);
        bpu_state.units[i].current_op = 0;
    }
    
    return 0;
}

static void bpu_shutdown(void) {
    bool expected = true;
    if (!atomic_compare_exchange_strong(&bpu_state.initialized, &expected, false)) {
        return;
    }
    
    printf("BPU_DEVICE: Shutting down BPU.\n");
    bpu_print_statistics();
}

// ============================================================================
// BPU Operations
// ============================================================================

[[nodiscard]] static int bpu_lower(const GraphNode* node, void* out_kernel) {
    if (node == nullptr || out_kernel == nullptr) {
        return -1;
    }
    
    *(OpCode*)out_kernel = node->op;
    return 0;
}

// Find available execution unit
[[nodiscard]] static int find_available_unit(void) {
    for (int i = 0; i < BPU_NUM_UNITS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&bpu_state.units[i].is_busy, 
                                          &expected, true)) {
            return i;
        }
    }
    return -1; // All units busy
}

[[nodiscard]] static int bpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    if (!atomic_load(&bpu_state.initialized)) {
        if (bpu_init() != 0) {
            return -1;
        }
    }
    
    if (kernel == nullptr) {
        atomic_fetch_add(&bpu_state.operations_failed, 1);
        return -1;
    }
    
    int queued = atomic_load(&bpu_state.operations_queued);
    if (queued >= BPU_MAX_QUEUE_DEPTH) {
        printf("BPU_DEVICE: Queue full, cannot enqueue operation.\n");
        atomic_fetch_add(&bpu_state.operations_failed, 1);
        return -1;
    }
    
    // Find available execution unit
    int unit_id = find_available_unit();
    if (unit_id < 0) {
        printf("BPU_DEVICE: All execution units busy.\n");
        atomic_fetch_add(&bpu_state.operations_failed, 1);
        return -1;
    }
    
    OpCode op = *(OpCode*)kernel;
    bpu_state.units[unit_id].current_op = op;
    atomic_fetch_add(&bpu_state.operations_queued, 1);
    
    printf("BPU_DEVICE: Enqueued operation %d on unit %d (queue depth: %d).\n", 
           op, unit_id, atomic_load(&bpu_state.operations_queued));
    
    // Simulate execution
    atomic_fetch_add(&bpu_state.units[unit_id].operations_executed, 1);
    atomic_fetch_add(&bpu_state.operations_completed, 1);
    atomic_fetch_sub(&bpu_state.operations_queued, 1);
    atomic_fetch_add(&bpu_state.total_execution_time_ns, 1000); // 1 microsecond
    
    // Release unit
    atomic_store(&bpu_state.units[unit_id].is_busy, false);
    
    return 0;
}

[[nodiscard]] static int bpu_sync(void) {
    if (!atomic_load(&bpu_state.initialized)) {
        return 0;
    }
    
    // Wait for all queued operations to complete
    while (atomic_load(&bpu_state.operations_queued) > 0) {
        // Busy wait
    }
    
    // Wait for all units to be idle
    bool all_idle = false;
    while (!all_idle) {
        all_idle = true;
        for (int i = 0; i < BPU_NUM_UNITS; i++) {
            if (atomic_load(&bpu_state.units[i].is_busy)) {
                all_idle = false;
                break;
            }
        }
    }
    
    printf("BPU_DEVICE: Synchronized (completed: %d operations).\n",
           atomic_load(&bpu_state.operations_completed));
    return 0;
}

// ============================================================================
// BPU Statistics
// ============================================================================

void bpu_print_statistics(void) {
    if (!atomic_load(&bpu_state.initialized)) {
        printf("BPU_DEVICE: Not initialized\n");
        return;
    }
    
    printf("\n=== BPU Device Statistics ===\n");
    printf("Operations queued: %d\n", atomic_load(&bpu_state.operations_queued));
    printf("Operations completed: %d\n", atomic_load(&bpu_state.operations_completed));
    printf("Operations failed: %d\n", atomic_load(&bpu_state.operations_failed));
    printf("Total execution time: %llu ns\n",
           (unsigned long long)atomic_load(&bpu_state.total_execution_time_ns));
    
    printf("\nExecution Units:\n");
    for (int i = 0; i < BPU_NUM_UNITS; i++) {
        printf("  Unit %d: %s, executed %d ops\n",
               i,
               atomic_load(&bpu_state.units[i].is_busy) ? "BUSY" : "IDLE",
               atomic_load(&bpu_state.units[i].operations_executed));
    }
    printf("==============================\n\n");
}

DeviceVTable BPU_DEVICE_IMPL = {
    .id = DEVICE_ID_BPU,
    .name = "BPU_C23_Enhanced",
    .lower = bpu_lower,
    .enqueue = bpu_enqueue,
    .sync = bpu_sync
};
