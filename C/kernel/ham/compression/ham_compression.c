
// ===================================================================
// Phase 5.4: HAM Compression via Motif Interning Implementation
// ===================================================================
#include "ham_compression.h"
#include <stdlib.h>
#include <string.h>

// --- Motif Extraction ---

Motif** motif_extract(const void* data, size_t size, size_t* out_count) {
    if (!data || size == 0 || !out_count) return NULL;
    
    // Simple pattern extraction: find repeated byte sequences
    const uint8_t* bytes = (const uint8_t*)data;
    
    // Allocate motif array
    size_t motif_capacity = 16;
    Motif** motifs = malloc(sizeof(Motif*) * motif_capacity);
    if (!motifs) return NULL;
    
    size_t motif_count = 0;
    
    // Look for 4-byte patterns
    const size_t pattern_len = 4;
    
    for (size_t i = 0; i + pattern_len <= size; i++) {
        // Count occurrences of this pattern
        uint32_t frequency = 0;
        
        for (size_t j = i; j + pattern_len <= size; j++) {
            if (memcmp(bytes + i, bytes + j, pattern_len) == 0) {
                frequency++;
            }
        }
        
        // If pattern appears multiple times, create motif
        if (frequency > 2) {
            Motif* motif = malloc(sizeof(Motif));
            if (!motif) continue;
            
            motif->pattern = malloc(pattern_len);
            if (!motif->pattern) {
                free(motif);
                continue;
            }
            
            memcpy(motif->pattern, bytes + i, pattern_len);
            motif->pattern_len = pattern_len;
            motif->frequency = frequency;
            motif->motif_id = motif_count;
            
            if (motif_count >= motif_capacity) {
                motif_capacity *= 2;
                motifs = realloc(motifs, sizeof(Motif*) * motif_capacity);
            }
            
            motifs[motif_count++] = motif;
            
            // Skip past this pattern
            i += pattern_len - 1;
        }
    }
    
    *out_count = motif_count;
    return motifs;
}

void motif_array_free(Motif** motifs, size_t count) {
    if (!motifs) return;
    
    for (size_t i = 0; i < count; i++) {
        if (motifs[i]) {
            free(motifs[i]->pattern);
            free(motifs[i]);
        }
    }
    
    free(motifs);
}

// --- Compression ---

int ham_compress_region(HamRegion* region) {
    if (!region || !region->base) return -1;
    
    // Extract motifs
    size_t motif_count = 0;
    Motif** motifs = motif_extract(region->base, region->capacity_bytes, &motif_count);
    
    if (!motifs || motif_count == 0) {
        motif_array_free(motifs, motif_count);
        return -1;
    }
    
    // For now, just store the first motif
    // TODO: Implement full compression with motif dictionary
    if (motif_count > 0) {
        region->interned_motif = motifs[0];
        
        // Free other motifs
        for (size_t i = 1; i < motif_count; i++) {
            free(motifs[i]->pattern);
            free(motifs[i]);
        }
    }
    
    free(motifs);
    
    return 0;
}

int ham_decompress_region(HamRegion* region) {
    if (!region || !region->interned_motif) return -1;
    
    // TODO: Implement decompression
    // For now, just clear the motif
    free(region->interned_motif->pattern);
    free(region->interned_motif);
    region->interned_motif = NULL;
    
    return 0;
}

CompressionStats ham_get_compression_stats(const HamRegion* region) {
    CompressionStats stats = {0};
    
    if (!region) return stats;
    
    stats.original_size = region->capacity_bytes;
    
    if (region->interned_motif) {
        // Estimate compressed size
        size_t motif_size = region->interned_motif->pattern_len;
        size_t motif_freq = region->interned_motif->frequency;
        
        // Compressed size = original - (motif_size * (freq - 1))
        stats.compressed_size = stats.original_size - (motif_size * (motif_freq - 1));
        stats.compression_ratio = (float)stats.original_size / (float)stats.compressed_size;
        stats.num_motifs = 1;
    } else {
        stats.compressed_size = stats.original_size;
        stats.compression_ratio = 1.0f;
        stats.num_motifs = 0;
    }
    
    return stats;
}
