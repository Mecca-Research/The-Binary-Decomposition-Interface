// ===================================================================
// DESC: Implements a simple, non-concurrent two-tiered HAM allocator.
// ===================================================================
#include "ham.h"
#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For error messages

// --- HAM State (Global for simplicity in this MVP) ---
// In a real kernel, this would be part of a managed state struct.
#define MAX_REGIONS 256
static HamRegion ham_regions[MAX_REGIONS];
static size_t region_count = 0;

// --- Private Helper Functions ---
static int ham_alloc_internal(RegionId* out_id, HamTier tier, size_t size_bytes, void** out_ptr) {
    if (region_count >= MAX_REGIONS) {
        fprintf(stderr, "HAM Error: Max regions reached.\n");
        return -1;
    }

    void* ptr = malloc(size_bytes);
    if (!ptr) {
        perror("HAM malloc failed");
        return -1;
    }

    RegionId id = region_count + 1; // 1-based RegionId
    ham_regions[region_count] = (HamRegion){
        .id = id,
        .tier = tier,
        .capacity_bytes = size_bytes,
        .base = ptr
    };
    
    *out_id = id;
    *out_ptr = ptr;
    region_count++;

    return 0; // Success
}

static int ham_free_internal(RegionId id) {
    if (id == 0 || id > region_count) {
        return -1; // Invalid ID
    }
    // In a real system, you'd need a more robust way to find the region.
    // This simple linear scan works for the M0 prototype.
    for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            free(ham_regions[i].base);
            // Invalidate the entry
            if (i < region_count - 1) {
                ham_regions[i] = ham_regions[region_count - 1];
            }
            region_count--;
            return 0; // Success
        }
    }
    return -1; // Not found
}

// --- Public HAM API Implementation ---
HamVTable HAM_DEFAULT_IMPL = {
    .alloc = ham_alloc_internal,
    .free = ham_free_internal
};
