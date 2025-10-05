
// ===================================================================
// Phase 5.2: CPU Backend Implementation
// DESC: CPU kernel execution for BDI graphs
// ===================================================================
/**
 * @file cpu_backend.h
 * @brief Backend Code Generation
 * @details This file provides the cpu backend functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_CPU_BACKEND_H
#define AEON_CPU_BACKEND_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../ham/ham.h"
#include "../../device/device.h"
#include <stdint.h>

// --- CPU Kernel Structure ---
typedef void (*CpuKernelFunc)(const void* inputs[], void* output, size_t count);

typedef struct {
    CpuKernelFunc execute;
    OpCode opcode;
    BdiType type;
    size_t input_count;
    void* metadata;  // Optional kernel-specific data
} CpuKernel;

// --- CPU Backend API ---
[[nodiscard]] int cpu_lower(const GraphNode* node, void** out_kernel);
[[nodiscard]] int cpu_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions);
[[nodiscard]] int cpu_sync(void);

// --- CPU Device VTable ---
extern DeviceVTable cpu_device_vtable;

// --- Kernel Implementations ---
void cpu_kernel_add(const void* inputs[], void* output, size_t count);
void cpu_kernel_mul(const void* inputs[], void* output, size_t count);
void cpu_kernel_relu(const void* inputs[], void* output, size_t count);
void cpu_kernel_matmul(const void* inputs[], void* output, size_t count);

#endif // AEON_CPU_BACKEND_H
