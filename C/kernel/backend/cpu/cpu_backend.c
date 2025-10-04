
// ===================================================================
// Phase 5.2: CPU Backend Implementation
// ===================================================================
#include "cpu_backend.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Kernel Implementations ---

void cpu_kernel_add(const void* inputs[], void* output, size_t count) {
    const float* a = (const float*)inputs[0];
    const float* b = (const float*)inputs[1];
    float* c = (float*)output;
    
    for (size_t i = 0; i < count; i++) {
        c[i] = a[i] + b[i];
    }
}

void cpu_kernel_mul(const void* inputs[], void* output, size_t count) {
    const float* a = (const float*)inputs[0];
    const float* b = (const float*)inputs[1];
    float* c = (float*)output;
    
    for (size_t i = 0; i < count; i++) {
        c[i] = a[i] * b[i];
    }
}

void cpu_kernel_relu(const void* inputs[], void* output, size_t count) {
    const float* a = (const float*)inputs[0];
    float* b = (float*)output;
    
    for (size_t i = 0; i < count; i++) {
        b[i] = (a[i] > 0.0f) ? a[i] : 0.0f;
    }
}

void cpu_kernel_matmul(const void* inputs[], void* output, size_t count) {
    // Simplified matrix multiplication (assumes square matrices)
    const float* a = (const float*)inputs[0];
    const float* b = (const float*)inputs[1];
    float* c = (float*)output;
    
    size_t n = (size_t)sqrt((double)count);
    
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            float sum = 0.0f;
            for (size_t k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

// --- CPU Backend Functions ---

int cpu_lower(const GraphNode* node, void** out_kernel) {
    if (!node || !out_kernel) return -1;
    
    CpuKernel* kernel = malloc(sizeof(CpuKernel));
    if (!kernel) return -1;
    
    kernel->opcode = node->opcode;
    kernel->type = node->type;
    kernel->input_count = node->input_count;
    kernel->metadata = NULL;
    
    // Map opcode to kernel function
    switch (node->opcode) {
        case OP_ADD:
            kernel->execute = cpu_kernel_add;
            break;
        case OP_MUL:
            kernel->execute = cpu_kernel_mul;
            break;
        case OP_RELU:
            kernel->execute = cpu_kernel_relu;
            break;
        case OP_MATMUL:
            kernel->execute = cpu_kernel_matmul;
            break;
        default:
            free(kernel);
            return -1;
    }
    
    *out_kernel = kernel;
    return 0;
}

int cpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions) {
    if (!kernel || !regions) return -1;
    
    const CpuKernel* cpu_kernel = (const CpuKernel*)kernel;
    
    // Prepare input pointers
    const void* inputs[8] = {0};
    for (size_t i = 0; i < cpu_kernel->input_count && i < num_regions - 1; i++) {
        inputs[i] = regions[i]->base;
    }
    
    // Output is the last region
    void* output = (void*)regions[num_regions - 1]->base;
    
    // Execute kernel
    size_t count = regions[num_regions - 1]->capacity_bytes / sizeof(float);
    cpu_kernel->execute(inputs, output, count);
    
    return 0;
}

int cpu_sync(void) {
    // CPU execution is synchronous, nothing to do
    return 0;
}

// --- CPU Device VTable ---
DeviceVTable cpu_device_vtable = {
    .id = DEVICE_ID_CPU,
    .name = "CPU",
    .lower = cpu_lower,
    .enqueue = cpu_enqueue,
    .sync = cpu_sync
};
