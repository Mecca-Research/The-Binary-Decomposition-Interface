// ===================================================================
// DESC: Defines the interface for the FPGA synthesis
//       and bitstream management backend.
// PHASE 13: Modernized with C23 features + Day 4 enhancements
// ===================================================================
#ifndef AEON_FPGA_BACKEND_H
#define AEON_FPGA_BACKEND_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "graph.h"

constexpr size_t FPGA_MAX_BITSTREAM_SIZE = (64 * 1024 * 1024);
constexpr int FPGA_MAX_REGIONS = 8;
constexpr int FPGA_MAX_CACHED_BITSTREAMS = 16;

typedef struct {
    uint64_t subgraph_hash;
    void* bitstream_data;
    size_t bitstream_size;
    _Atomic bool is_loaded;
    _Atomic int ref_count;
    int region_id;
} FpgaBitstream;

_Static_assert(sizeof(FpgaBitstream) <= 64, "FpgaBitstream structure too large");

typedef struct {
    int region_id;
    _Atomic bool is_occupied;
    size_t region_size;
    FpgaBitstream* current_bitstream;
} FpgaRegion;

typedef struct {
    _Atomic bool initialized;
    _Atomic int active_syntheses;
    FpgaRegion regions[FPGA_MAX_REGIONS];
    _Atomic int bitstreams_cached;
} FpgaDeviceState;

// Core functions
[[nodiscard]] int fpga_init(void);
void fpga_shutdown(void);
[[nodiscard]] FpgaBitstream* fpga_synthesize_subgraph(BdiGraph* g, NodeId start_node, NodeId end_node);
[[nodiscard]] int fpga_load_bitstream(FpgaBitstream* bitstream);
[[nodiscard]] int fpga_load_bitstream_region(FpgaBitstream* bitstream, int region_id);
[[nodiscard]] int fpga_unload_region(int region_id);
[[nodiscard]] int fpga_reconfigure_region(int region_id, FpgaBitstream* new_bitstream);
void fpga_free_bitstream(FpgaBitstream* bitstream);
[[nodiscard]] FpgaDeviceState* fpga_get_state(void);
[[nodiscard]] FpgaBitstream* fpga_get_cached_bitstream(uint64_t subgraph_hash);

// Synthesis pipeline
[[nodiscard]] uint64_t fpga_synthesis_request(BdiGraph* g, NodeId start_node, NodeId end_node);
[[nodiscard]] int fpga_synthesis_status(uint64_t job_id, FpgaBitstream** out_bitstream);

// Memory management
[[nodiscard]] void* fpga_alloc_managed(size_t size_bytes, uint32_t flags);
void fpga_free_managed(void* device_ptr, size_t size_bytes);
[[nodiscard]] int fpga_transfer_zerocopy(void* fpga_dst, const void* host_src, size_t size_bytes);

// Statistics
void fpga_print_statistics(void);

#endif // AEON_FPGA_BACKEND_H
