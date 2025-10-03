// ===================================================================
// DESC: Defines the interface for the FPGA synthesis
//       and bitstream management backend.
// PHASE 13: Modernized with C23 features
// ===================================================================
#ifndef AEON_FPGA_BACKEND_H
#define AEON_FPGA_BACKEND_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "graph.h"

// C23 constexpr for FPGA limits
constexpr size_t FPGA_MAX_BITSTREAM_SIZE = (64 * 1024 * 1024); // 64MB
constexpr int FPGA_MAX_REGIONS = 8;
constexpr int FPGA_MAX_CACHED_BITSTREAMS = 16;

// Represents a compiled hardware bitstream for a BDI subgraph.
typedef struct {
    uint64_t subgraph_hash;      // Hash of the source BDI subgraph
    void* bitstream_data;        // Pointer to the binary bitstream
    size_t bitstream_size;
    _Atomic bool is_loaded;      // Is the bitstream currently configured on the FPGA?
    _Atomic int ref_count;       // Reference count for caching
    int region_id;               // FPGA region for partial reconfiguration
} FpgaBitstream;

// Validate bitstream structure
_Static_assert(sizeof(FpgaBitstream) <= 64, "FpgaBitstream structure too large");

// FPGA Region for partial reconfiguration
typedef struct {
    int region_id;
    _Atomic bool is_occupied;
    size_t region_size;
    FpgaBitstream* current_bitstream;
} FpgaRegion;

// FPGA Device State
typedef struct {
    _Atomic bool initialized;
    _Atomic int active_syntheses;
    FpgaRegion regions[FPGA_MAX_REGIONS];
    _Atomic int bitstreams_cached;
} FpgaDeviceState;

// --- FPGA Backend API ---
// This API represents the functions our FPGA device will use to manage
// the software-to-hardware pipeline.

// Initializes the FPGA backend.
[[nodiscard]] int fpga_init(void);

void fpga_shutdown(void);

// Takes a BDI subgraph and "synthesizes" it into a bitstream.
// In our simulation, this will generate a dummy bitstream.
[[nodiscard]] FpgaBitstream* fpga_synthesize_subgraph(BdiGraph* g, NodeId start_node, NodeId end_node);

// Loads a bitstream onto the FPGA hardware.
[[nodiscard]] int fpga_load_bitstream(FpgaBitstream* bitstream);

// Loads a bitstream onto a specific FPGA region (partial reconfiguration).
[[nodiscard]] int fpga_load_bitstream_region(FpgaBitstream* bitstream, int region_id);

// Unloads a bitstream from a region.
[[nodiscard]] int fpga_unload_region(int region_id);

// Frees the memory for a bitstream object.
void fpga_free_bitstream(FpgaBitstream* bitstream);

// Gets current device state.
[[nodiscard]] FpgaDeviceState* fpga_get_state(void);

// Checks if a bitstream is cached.
[[nodiscard]] FpgaBitstream* fpga_get_cached_bitstream(uint64_t subgraph_hash);

#endif // AEON_FPGA_BACKEND_H
