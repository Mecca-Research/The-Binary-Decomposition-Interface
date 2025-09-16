// ===================================================================
// DESC: Defines the interface for the FPGA synthesis
//       and bitstream management backend.
// ===================================================================
#ifndef AEON_FPGA_BACKEND_H
#define AEON_FPGA_BACKEND_H

#include "graph.h"

// Represents a compiled hardware bitstream for a BDI subgraph.
typedef struct {
    uint64_t subgraph_hash; // Hash of the source BDI subgraph
    void* bitstream_data;   // Pointer to the binary bitstream
    size_t bitstream_size;
    bool is_loaded;         // Is the bitstream currently configured on the FPGA?
} FpgaBitstream;

// --- FPGA Backend API ---
// This API represents the functions our FPGA device will use to manage
// the software-to-hardware pipeline.

// Initializes the FPGA backend.
int fpga_init();
void fpga_shutdown();

// Takes a BDI subgraph and "synthesizes" it into a bitstream.
// In our simulation, this will generate a dummy bitstream.
FpgaBitstream* fpga_synthesize_subgraph(BdiGraph* g, NodeId start_node, NodeId end_node);

// Loads a bitstream onto the FPGA hardware.
int fpga_load_bitstream(FpgaBitstream* bitstream);

// Frees the memory for a bitstream object.
void fpga_free_bitstream(FpgaBitstream* bitstream);

#endif // AEON_FPGA_BACKEND_H
