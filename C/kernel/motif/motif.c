// ===================================================================
// DESC: Implements the logic for symbolic compression via
//       motif interning.
// ===================================================================
#include "motif.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Simple hash function for data blocks (e.g., djb2)
static uint64_t hash_data(const void* data, size_t size) {
    uint64_t hash = 5381;
    const unsigned char* str = (const unsigned char*)data;
    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + str[i]; // hash * 33 + c
    }
    return hash;
}

void motif_dict_init(MotifDictionary* dict) {
    if (!dict) return;
    dict->motifs = NULL;
    dict->count = 0;
    dict->capacity = 0;
}

void motif_dict_free(MotifDictionary* dict) {
    if (!dict) return;
    for (size_t i = 0; i < dict->count; i++) {
        free(dict->motifs[i].data);
    }
    free(dict->motifs);
    motif_dict_init(dict);
}

Motif* motif_dict_intern(MotifDictionary* dict, void* data, size_t size) {
    if (!dict || !data || size == 0) return NULL;
    
    uint64_t hash = hash_data(data, size);

    // 1. Check if a motif with this hash and data already exists.
    for (size_t i = 0; i < dict->count; i++) {
        if (dict->motifs[i].hash == hash &&
            dict->motifs[i].size == size &&
            memcmp(dict->motifs[i].data, data, size) == 0)
        {
            dict->motifs[i].ref_count++;
            return &dict->motifs[i];
        }
    }

    // 2. If not found, create a new motif.
    if (dict->count >= dict->capacity) {
        size_t new_capacity = dict->capacity < 8 ? 8 : dict->capacity * 2;
        Motif* new_motifs = (Motif*)realloc(dict->motifs, new_capacity * sizeof(Motif));
        if (!new_motifs) return NULL;
        dict->motifs = new_motifs;
        dict->capacity = new_capacity;
    }

    void* motif_data = malloc(size);
    if (!motif_data) return NULL;
    memcpy(motif_data, data, size);

    dict->motifs[dict->count] = (Motif){
        .hash = hash,
        .data = motif_data,
        .size = size,
        .ref_count = 1
    };
    
    return &dict->motifs[dict->count++];
}

void motif_dict_release(MotifDictionary* dict, uint64_t hash) {
    // In a real system, releasing a motif would require finding it and
    // decrementing its ref_count. If the count reaches zero, the motif's
    // data can be freed. This is stubbed for M2.
}
