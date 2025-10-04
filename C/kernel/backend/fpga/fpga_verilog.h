
// ===================================================================
// Phase 5.2: FPGA Backend (Verilog Generation)
// DESC: FPGA synthesis through Verilog code generation
// ===================================================================
#ifndef AEON_FPGA_VERILOG_H
#define AEON_FPGA_VERILOG_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include "../../ham/ham.h"
#include "../../device/device.h"
#include "../../graph_opt/graph_opt.h"
#include <stdint.h>

// --- FPGA Kernel Structure ---
typedef struct {
    char* verilog_code;
    size_t code_len;
    uint32_t latency_cycles;
    uint32_t resource_usage;  // LUTs, FFs, etc.
} FpgaKernel;

// --- FPGA Backend API ---
[[nodiscard]] int fpga_synthesize_subgraph(const BdiGraph* graph, const Subgraph* subgraph, FpgaKernel** out_kernel);
[[nodiscard]] int fpga_lower(const GraphNode* node, void** out_kernel);
[[nodiscard]] int fpga_enqueue(const void* kernel, const HamRegion** regions, size_t num_regions);
[[nodiscard]] int fpga_sync(void);

// --- FPGA Device VTable ---
extern DeviceVTable fpga_device_vtable;

// --- Verilog Generation ---
[[nodiscard]] char* generate_verilog_add(const GraphNode* node);
[[nodiscard]] char* generate_verilog_mul(const GraphNode* node);
[[nodiscard]] char* generate_verilog_relu(const GraphNode* node);
void fpga_kernel_free(FpgaKernel* kernel);

#endif // AEON_FPGA_VERILOG_H
