// ===================================================================
// DESC: A stub implementation for the BPU device.
// ===================================================================
#include "c23_compat.h"
#include "device.h"
#include <stdio.h>

static int bpu_lower(const GraphNode* node, void* out_kernel) {
    // The BPU is for simple, disaggregated arithmetic.
    // The "kernel" could just be the OpCode itself.
    *(OpCode*)out_kernel = node->op;
    return 0;
}

static int bpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    OpCode op = *(OpCode*)kernel;
    printf("BPU_DEVICE: Enqueued operation %d. (STUB)\n", op);
    // A real BPU would execute the operation here.
    return 0;
}

static int bpu_sync() {
    // Synchronous for now, so no-op.
    return 0;
}

DeviceVTable BPU_DEVICE_IMPL = {
    .id = DEVICE_ID_BPU,
    .name = "BPU_Stub",
    .lower = bpu_lower,
    .enqueue = bpu_enqueue,
    .sync = bpu_sync
};
