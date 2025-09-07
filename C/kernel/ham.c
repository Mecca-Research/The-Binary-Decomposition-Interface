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
            if (ham_regions[i].interned_motif) {
            // motif_dict_release(dict, ham_regions[i].interned_motif->hash);
            // The region's base pointer doesn't own the memory, so don't free it.
            } else {
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

// --- Intelligence function implementations ---
// A simple entropy calculation based on byte value distribution.
// A real implementation would be more sophisticated.
static float calculate_entropy(const void* data, size_t size) {
    if (!data || size == 0) return 0.0f;
    unsigned int counts[256] = {0};
    for (size_t i = 0; i < size; i++) {
        counts[((const uint8_t*)data)[i]]++;
    }

    float entropy = 0.0f;
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            float p = (float)counts[i] / size;
            entropy -= p * log2f(p);
        }
    }
    return entropy / 8.0f; // Normalize to a 0-1 range (max entropy is 8 bits/byte)
}

static int ham_update_stats_internal(RegionId id) {
    for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            HamRegion* region = &ham_regions[i];
            region->stats.access_count++;
            // region->stats.last_access_cycle = get_current_cycle(); // Placeholder
            if(region->base) {
                region->stats.entropy_score = calculate_entropy(region->base, region->capacity_bytes);
            }
            return 0;
        }
    }
    return -1;
}

static int ham_demote_check_internal(RegionId id) {
    for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            HamRegion* region = &ham_regions[i];
            // Simple policy: if a region is ACTIVE and has been accessed
            // less than 5 times, demote it to DORMANT.
            if (region->tier == HAM_ACTIVE && region->stats.access_count < 5) {
                printf("HAM: Demoting Region %llu to DORMANT.\n", (unsigned long long)id);
                region->tier = HAM_DORMANT;
            }
            return 0;
        }
    }
    return -1;
}

static int ham_intern_check_internal(RegionId id, MotifDictionary* dict) {
     for (size_t i = 0; i < region_count; ++i) {
        if (ham_regions[i].id == id) {
            HamRegion* region = &ham_regions[i];
            // Policy: only intern DORMANT regions.
            if (region->tier == HAM_DORMANT && region->base && !region->interned_motif) {
                printf("HAM: Attempting to intern Region %llu.\n", (unsigned long long)id);
                Motif* motif = motif_dict_intern(dict, region->base, region->capacity_bytes);
                if (motif) {
                    // If the motif was new, ref_count is 1. If it already existed,
                    // ref_count is > 1, meaning we found a duplicate.
                    if (motif->ref_count > 1) {
                        printf("HAM: Found duplicate! Compressing Region %llu.\n", (unsigned long long)id);
                        free(region->base); // Free the original, redundant memory.
                        region->base = motif->data; // Point to the single interned copy.
                        region->interned_motif = motif;
                    } else {
                        // This was the first time seeing this data.
                        region->interned_motif = motif;
                        // We could free the original and point to the motif's copy,
                        // but they are identical, so we just link them.
                        free(motif->data); // The dict made a copy, free it.
                        motif->data = region->base; // Point dict's copy to our memory.
                    }
                }
            }
            return 0;
        }
    }
    return -1;
}

// --- Public HAM API ---
HamVTable HAM_DEFAULT_IMPL = {
    .alloc = ham_alloc_internal,
    .free = ham_free_internal,
    .persist = ham_persist_internal,
    .load = ham_load_internal
};
