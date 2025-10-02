// ===================================================================
// DESC: Implements the simple CPU device backend for executing nodes.
// ===================================================================
#include "device.h"
#include "gpu_backend.h" 
#include "fpga_backend.h"
#include <stdio.h>
#include <stdlib.h>

// --- CPU Device Kernel Implementations ---
// These are the "micro-op" functions that the `lower` step points to.
// They take raw pointers to memory regions and perform the operation.

static void cpu_kernel_const(void** inputs, void* outputs[1]) {
    *(float*)outputs[0] = *(float*)inputs[0] + *(float*)inputs[1];
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

// --- Learning Kernels ---
// A simplified gradient calculation for a single neuron: grad_in = grad_out * weight
static void cpu_kernel_grad_f32(void** inputs, void* outputs[1]) {
    float* grad_out = (float*)inputs[0]; // Gradient from the next layer
    float* weight = (float*)inputs[1];   // Weight of the connection
    float* grad_in = (float*)outputs[0]; // Gradient for the previous layer
    *grad_in = *grad_out * *weight;
    // Note: A full backprop implementation is much more complex, involving derivatives
    // of activation functions, but this demonstrates the data flow.
}

// A simplified parameter update: param = param - learning_rate * grad
static void cpu_kernel_update_f32(void** inputs, void* outputs[1]) {
    float* param = (float*)inputs[0];  // The parameter to update (e.g., a weight)
    float* grad = (float*)inputs[1];   // The calculated gradient for this parameter
    float* lr = (float*)inputs[2];     // The learning rate
    
    *param -= *lr * *grad;
    // This kernel modifies its input region directly (side-effect).
    // The output is not used.
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
        case OP_GRAD:   *kernel_ptr = cpu_kernel_grad_f32; break;
        case OP_UPDATE: *kernel_ptr = cpu_kernel_update_f32; break;
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
    // For many ops, the last region is the output
    if (num_regions > 0) outputs[0] = regions[num_regions-1]->base;
    printf("CPU_DEVICE: Executing kernel...\n");
    kernel_func(inputs, outputs);

    return 0; // Success
}

    // For a synchronous CPU device, this is a no-op as enqueue executes immediately.
static int cpu_sync() { return 0; }
    DeviceVTable CPU_DEVICE_IMPL = {
    .id = DEVICE_ID_CPU, .name = "CPU_Baseline", .lower = cpu_lower, .enqueue = cpu_enqueue, .sync = cpu_sync
};

static int gpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    // This function is the bridge to the gpu_backend.c API.
    GpuKernel gpu_kernel = *(GpuKernel*)kernel;

    // A real implementation would involve:
    // 1. Allocating GPU memory for each region (gpu_alloc).
    // 2. Copying input data from host (HAM region) to device (gpu_memcpy_h2d).
    // 3. Setting up kernel launch arguments (an array of device pointers).
    // 4. Launching the kernel (gpu_launch_kernel).
    // 5. Copying results back from device to host (gpu_memcpy_d2h).
    // 6. Freeing GPU memory (gpu_free).
    
    // For our M3 test, we will just simulate the launch.
    return gpu_launch_kernel(gpu_kernel, nullptr);
}

static int gpu_sync_device() {
    return gpu_sync();
}

// --- FPGA Device Implementation ---
// The 'lower' function for the FPGA is the synthesis step.
static int fpga_lower(const GraphNode* node, void* out_kernel) {
    // We assume the node is a special SUBGRAPH_BEGIN node.
    // The scheduler needs to find the matching SUBGRAPH_END to define the region.
    // For this simulation, we'll pass the node itself as the "kernel".
    *(const GraphNode**)out_kernel = node;
    return 0;
}

// The 'enqueue' function for the FPGA is the bitstream loading and execution step.
static int fpga_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    const GraphNode* subgraph_start_node = *(const GraphNode**)kernel;

    // A real implementation would traverse the graph from this start node
    // to find the end of the subgraph. For M5, we'll assume it's just this one node.
    FpgaBitstream* bitstream = fpga_synthesize_subgraph(nullptr, subgraph_start_node->id, subgraph_start_node->id);
    if (!bitstream) {
        fprintf(stderr, "FPGA_DEVICE Error: Synthesis failed.\n");
        return -1;
    }
    
    int result = fpga_load_bitstream(bitstream);
    
    // After loading, a real implementation would copy data from HAM regions to the FPGA,
    // trigger the execution, and copy results back.
    
    fpga_free_bitstream(bitstream);
    return result;
}

static int fpga_sync() {
    printf("FPGA_DEVICE: Synchronizing device.\n");
    return 0;
}

// --- Public Device API Implementation ---
DeviceVTable CPU_DEVICE_IMPL = {
    .id = 1, 
    .name = "CPU_Baseline",
    .lower = cpu_lower,
    .enqueue = cpu_enqueue,
    .sync = cpu_sync
};

DeviceVTable GPU_DEVICE_IMPL = {
    .id = DEVICE_ID_GPU,
    .name = "GPU_Generic",
    .lower = gpu_lower,
    .enqueue = gpu_enqueue,
    .sync = gpu_sync_device
};

DeviceVTable FPGA_DEVICE_IMPL = {
    .id = DEVICE_ID_FPGA,
    .name = "FPGA_Reconfigurable",
    .lower = fpga_lower,
    .enqueue = fpga_enqueue,
    .sync = fpga_sync
};
