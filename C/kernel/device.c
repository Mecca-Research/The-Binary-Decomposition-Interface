// ===================================================================
// DESC: Implements the simple CPU device backend for executing nodes.
// ===================================================================
#include "device.h"
#include <stdio.h> // For printf

// --- CPU Device Kernel Implementations ---
// These are the "micro-op" functions that the `lower` step points to.
// They take raw pointers to memory regions and perform the operation.

static void cpu_kernel_const(void** inputs, void* outputs[1]) {
    // The "constant" value is often embedded in the kernel or a metadata region.
    // For M0, we assume it's pre-loaded into the output region by the scheduler.
    // This function is effectively a no-op for this simple model.
}

static void cpu_kernel_add_f32(void** inputs, void* outputs[1]) {
    float* in1 = (float*)inputs[0];
    float* in2 = (float*)inputs[1];
    float* out = (float*)outputs[0];
    *out = *in1 + *in2;
}

static void cpu_kernel_mul_f32(void** inputs, void* outputs[1]) {
    float* in1 = (float*)inputs[0];
    float* in2 = (float*)inputs[1];
    float* out = (float*)outputs[0];
    *out = *in1 * *in2;
}

static void cpu_kernel_relu_f32(void** inputs, void* outputs[1]) {
    float* in = (float*)inputs[0];
    float* out = (float*)outputs[0];
    *out = *in > 0 ? *in : 0;
}

// --- DeviceVTable Function Implementations ---

static int cpu_lower(const GraphNode* node, void* out_kernel) {
    // This function maps an OpCode to a specific kernel function pointer.
    void (**kernel_ptr)(void**, void**) = (void (**)(void**, void**))out_kernel;

    switch(node->op) {
        case OP_CONST:
            *kernel_ptr = cpu_kernel_const;
            break;
        case OP_ADD:
            // In a real system, you'd check node->out_type to select the correct kernel.
            *kernel_ptr = cpu_kernel_add_f32;
            break;
        case OP_MUL:
            *kernel_ptr = cpu_kernel_mul_f32;
            break;
        case OP_RELU:
            *kernel_ptr = cpu_kernel_relu_f32;
            break;
        // OP_MATMUL would require a more complex kernel.
        default:
            fprintf(stderr, "CPU_DEVICE Error: Cannot lower OpCode %d\n", node->op);
            return -1; // Failure
    }
    return 0; // Success
}

static int cpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    // For a simple, synchronous CPU, "enqueue" means "execute immediately."
    void (*kernel_func)(void**, void**) = (void (*)(void**, void**))kernel;

    // A real scheduler would map node inputs/outputs to these region pointers.
    // This is a placeholder for that complex logic.
    void* inputs[8] = {0};
    void* outputs[1] = {0};
    
    // Example: Assume region 0 is input 1, region 1 is input 2, region 2 is output.
    if (num_regions > 0) inputs[0] = regions[0]->base;
    if (num_regions > 1) inputs[1] = regions[1]->base;
    if (num_regions > 2) outputs[0] = regions[2]->base;

    printf("CPU_DEVICE: Executing kernel...\n");
    kernel_func(inputs, outputs);

    return 0; // Success
}

static int cpu_sync() {
    // For a synchronous CPU device, this is a no-op as enqueue executes immediately.
    return 0; // Success
}

// --- Public Device API Implementation ---
DeviceVTable CPU_DEVICE_IMPL = {
    .id = 1,
    .name = "CPU_Baseline",
    .lower = cpu_lower,
    .enqueue = cpu_enqueue,
    .sync = cpu_sync
};
