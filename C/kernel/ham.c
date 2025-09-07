// ===================================================================
// DESC: Implements a simple, non-concurrent two-tiered HAM allocator.
// ===================================================================
#include "ham.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // For memcpy

#define MAX_REGIONS 256
static HamRegion ham_regions[MAX_REGIONS];
static size_t region_count = 0;

// alloc function 
static int ham_alloc_internal(RegionId* out_id, HamTier tier, size_t size_bytes, void** out_ptr) {
    if (region_count >= MAX_REGIONS) return -1;
    void* ptr = NULL;
    if (tier == HAM_CRITICAL || tier == HAM_ACTIVE) {
        ptr = malloc(size_bytes);
        if (!ptr) return -1;
    }

    RegionId id = region_count + 1;
    ham_regions[region_count] = (HamRegion){
        .id = id,
        .tier = tier,
        .capacity_bytes = size_bytes,
        .base = ptr // NULL for ARCHIVE tier initially
    };
    snprintf(ham_regions[region_count].path, 256, "archive_region_%llu.bin", (unsigned long long)id);
    
    *out_id = id;
    if (out_ptr) *out_ptr = ptr;
    region_count++;
    return 0;
}

// free function 
static int ham_free_internal(RegionId id) {
     for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            free(ham_regions[i].base);
            if (i < region_count - 1) {
                ham_regions[i] = ham_regions[region_count - 1];
            }
            region_count--;
            return 0;
        }
    }
    return -1;
}

// --- Persistence function implementations ---
static int ham_persist_internal(RegionId id) {
    if (id == 0 || id > region_count) return -1;
    HamRegion* region = NULL;
    for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            region = &ham_regions[i];
            break;
        }
    }
    if (!region || !region->base) return -1;

    FILE* f = fopen(region->path, "wb");
    if (!f) {
        perror("HAM persist failed to open file");
        return -1;
    }
    fwrite(region->base, 1, region->capacity_bytes, f);
    fclose(f);
    printf("HAM: Persisted Region %llu to %s\n", (unsigned long long)id, region->path);
    return 0;
}

static int ham_load_internal(RegionId id) {
    if (id == 0 || id > region_count) return -1;
    HamRegion* region = NULL;
     for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            region = &ham_regions[i];
            break;
        }
    }
    if (!region) return -1;
    if (!region->base) { // Allocate memory if it's not already in RAM
        region->base = malloc(region->capacity_bytes);
        if (!region->base) return -1;
    }

    FILE* f = fopen(region->path, "rb");
    if (!f) {
        // This is not an error if the file doesn't exist yet
        return 0; 
    }
    fread(region->base, 1, region->capacity_bytes, f);
    fclose(f);
    printf("HAM: Loaded Region %llu from %s\n", (unsigned long long)id, region->path);
    return 0;
}

// --- Public HAM API ---
HamVTable HAM_DEFAULT_IMPL = {
    .alloc = ham_alloc_internal,
    .free = ham_free_internal,
    .persist = ham_persist_internal,
    .load = ham_load_internal
};
