// ===================================================================
// DESC: Defines the structures and interface for Hierarchical
//       Access Memory (HAM). M0 includes two tiers: CRITICAL & ACTIVE.
// ===================================================================
#ifndef AEON_HAM_H
#define AEON_HAM_H

#include "c23_compat.h"

#include "graph.h" // For RegionId
#include "motif.h" 

// --- Statistics for a memory region ---
typedef struct {
    uint64_t access_count;
    uint64_t last_access_cycle; // Or timestamp
    float entropy_score;      // Score from 0.0 to 1.0+
} HamStats;

// --- Memory regions (HAM tiers) for M0 ---
typedef enum {
    HAM_CRITICAL, // Hot working set, pinned in fastest memory
    HAM_ACTIVE    // Near-term use, general purpose
    HAM_DORMANT, // NEW in M2: Cold, compressible data
    HAM_ARCHIVE
} HamTier;

typedef struct {
    RegionId id;
    HamTier tier;
    size_t capacity_bytes;
    void* base; // Mapped host pointer
    char path[256];
    // --- NEW in M2: Intelligent Memory Fields ---
    HamStats stats;
    Motif* interned_motif; // If not nullptr, this region is compressed.
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
    // --- Intelligence Functions ---
    // Called on memory access to update stats.
    int (*update_stats)(RegionId id);
    // Evaluates a region and potentially demotes it (e.g., ACTIVE -> DORMANT).
    int (*demote_check)(RegionId id);
    // Checks if a region can be compressed by interning its data.
    int (*intern_check)(RegionId id, MotifDictionary* dict);
} HamVTable;

// --- Test helpers to inspect region state ---
HamTier ham_get_region_tier(RegionId id);
NODISCARD void* ham_get_region_base(RegionId id);


// ===================================================================
// C23 ENHANCEMENTS - Phase 1
// ===================================================================

// HAM tier constants
constexpr size_t HAM_MIN_REGION_SIZE = 4096;  // 4KB minimum
constexpr size_t HAM_MAX_REGION_SIZE = 1073741824;  // 1GB maximum
constexpr size_t HAM_DEFAULT_REGION_SIZE = 1048576;  // 1MB default

// HAM statistics constants
constexpr float HAM_ENTROPY_THRESHOLD = 0.7f;
constexpr uint64_t HAM_ACCESS_THRESHOLD = 1000;

// C23 static assertions
_Static_assert(sizeof(HamStats) <= 32, "HamStats should be compact");
_Static_assert(sizeof(HamTier) <= 4, "HamTier should be 32-bit enum");

#endif // AEON_HAM_H
