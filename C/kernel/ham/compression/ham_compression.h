
// ===================================================================
// Phase 5.4: HAM Compression via Motif Interning
// DESC: Pattern-based compression for memory regions
// ===================================================================
/**
 * @file ham_compression.h
 * @brief Ham Compression API
 * @details This file provides the ham compression functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef AEON_HAM_COMPRESSION_H
#define AEON_HAM_COMPRESSION_H

#include "../../c23_compat.h"
#include "../../ham/ham.h"
#include "../../motif/motif.h"
#include <stdint.h>

// --- Compression API ---
[[nodiscard]] int ham_compress_region(HamRegion* region);
[[nodiscard]] int ham_decompress_region(HamRegion* region);

// --- Motif Extraction ---
[[nodiscard]] Motif** motif_extract(const void* data, size_t size, size_t* out_count);
void motif_array_free(Motif** motifs, size_t count);

// --- Compression Statistics ---
typedef struct {
    size_t original_size;
    size_t compressed_size;
    float compression_ratio;
    size_t num_motifs;
} CompressionStats;

[[nodiscard]] CompressionStats ham_get_compression_stats(const HamRegion* region);

#endif // AEON_HAM_COMPRESSION_H
