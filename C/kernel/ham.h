// ===================================================================
// DESC: Defines the structures and interface for Hierarchical
//       Access Memory (HAM). M0 includes two tiers: CRITICAL & ACTIVE.
// ===================================================================
#ifndef AEON_HAM_H
#define AEON_HAM_H

#include "graph.h" // For RegionId

// --- Memory regions (HAM tiers) for M0 ---
typedef enum {
    HAM_CRITICAL, // Hot working set, pinned in fastest memory
    HAM_ACTIVE    // Near-term use, general purpose
} HamTier;

typedef struct {
    RegionId id;
    HamTier tier;
    size_t capacity_bytes;
    void* base; // Mapped host pointer
} HamRegion;

// --- HAM Virtual Table (Interface) ---
typedef struct {
    // Allocates a new region of a specific tier and size.
    int (*alloc)(RegionId* out_id, HamTier tier, size_t size_bytes, void** out_ptr);
    // Frees a previously allocated region.
    int (*free)(RegionId id);
    // Persists a region's data to its archive path.
    int (*persist)(RegionId id);
    // Loads a region's data from its archive path.
    int (*load)(RegionId id);
} HamVTable;

#endif // AEON_HAM_H
