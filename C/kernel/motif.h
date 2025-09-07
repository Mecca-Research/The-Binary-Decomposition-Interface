// ===================================================================
// DESC: NEW in M2. Defines structures for symbolic compression. A "motif"
//       is a unique, deduplicated data pattern stored once and
//       referenced multiple times.
// ===================================================================
#ifndef AION_MOTIF_H
#define AION_MOTIF_H

#include "graph.h" // For BdiGraph and basic types

// Represents a unique, interned data pattern.
typedef struct {
    uint64_t hash;       // Hash of the data pattern.
    void* data;          // Pointer to the single copy of the data.
    size_t size;         // Size of the data.
    size_t ref_count;    // How many regions are referencing this motif.
} Motif;

// A dictionary to store all unique motifs.
// Implemented as a simple dynamic array for M2.
typedef struct {
    Motif* motifs;
    size_t count;
    size_t capacity;
} MotifDictionary;

// --- Motif API ---
void motif_dict_init(MotifDictionary* dict);
void motif_dict_free(MotifDictionary* dict);
// Interns a block of data: finds an existing motif or creates a new one.
Motif* motif_dict_intern(MotifDictionary* dict, void* data, size_t size);
// Decrements the reference count of a motif.
void motif_dict_release(MotifDictionary* dict, uint64_t hash);

#endif // AION_MOTIF_H
