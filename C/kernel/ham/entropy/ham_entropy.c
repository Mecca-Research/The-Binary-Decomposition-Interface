
// ===================================================================
// Phase 5.4: HAM Entropy-Based Scoring Implementation
// ===================================================================
#include "ham_entropy.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Entropy Computation ---

float ham_compute_entropy(const void* data, size_t size) {
    if (!data || size == 0) return 0.0f;
    
    // Count byte frequencies
    uint32_t freq[256] = {0};
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i++) {
        freq[bytes[i]]++;
    }
    
    // Compute Shannon entropy
    float entropy = 0.0f;
    
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            float p = (float)freq[i] / (float)size;
            entropy -= p * log2f(p);
        }
    }
    
    return entropy;
}

int ham_update_entropy_score(HamRegion* region) {
    if (!region || !region->base) return -1;
    
    float entropy = ham_compute_entropy(region->base, region->capacity_bytes);
    region->stats.entropy_score = entropy;
    
    return 0;
}

// --- Access Pattern Analysis ---

AccessPattern* access_pattern_create(size_t capacity) {
    AccessPattern* pattern = malloc(sizeof(AccessPattern));
    if (!pattern) return NULL;
    
    pattern->history_capacity = capacity;
    pattern->history_size = 0;
    pattern->entropy_score = 0.0f;
    pattern->access_history = malloc(sizeof(uint64_t) * capacity);
    
    if (!pattern->access_history) {
        free(pattern);
        return NULL;
    }
    
    return pattern;
}

void access_pattern_free(AccessPattern* pattern) {
    if (!pattern) return;
    
    free(pattern->access_history);
    free(pattern);
}

int access_pattern_record(AccessPattern* pattern, uint64_t cycle) {
    if (!pattern) return -1;
    
    if (pattern->history_size >= pattern->history_capacity) {
        // Shift history
        memmove(pattern->access_history, pattern->access_history + 1, 
                sizeof(uint64_t) * (pattern->history_capacity - 1));
        pattern->history_size = pattern->history_capacity - 1;
    }
    
    pattern->access_history[pattern->history_size++] = cycle;
    
    // Update entropy score
    pattern->entropy_score = access_pattern_compute_entropy(pattern);
    
    return 0;
}

float access_pattern_compute_entropy(const AccessPattern* pattern) {
    if (!pattern || pattern->history_size == 0) return 0.0f;
    
    // Compute inter-access time deltas
    if (pattern->history_size < 2) return 0.0f;
    
    uint64_t* deltas = malloc(sizeof(uint64_t) * (pattern->history_size - 1));
    if (!deltas) return 0.0f;
    
    for (size_t i = 1; i < pattern->history_size; i++) {
        deltas[i - 1] = pattern->access_history[i] - pattern->access_history[i - 1];
    }
    
    // Quantize deltas into buckets
    uint32_t buckets[16] = {0};
    
    for (size_t i = 0; i < pattern->history_size - 1; i++) {
        uint64_t delta = deltas[i];
        int bucket = 0;
        
        if (delta < 10) bucket = 0;
        else if (delta < 100) bucket = 1;
        else if (delta < 1000) bucket = 2;
        else bucket = 3;
        
        buckets[bucket]++;
    }
    
    // Compute entropy
    float entropy = 0.0f;
    size_t total = pattern->history_size - 1;
    
    for (int i = 0; i < 16; i++) {
        if (buckets[i] > 0) {
            float p = (float)buckets[i] / (float)total;
            entropy -= p * log2f(p);
        }
    }
    
    free(deltas);
    return entropy;
}
