// ===================================================================
// DESC: Defines structures for symbolic compression. A "motif"
//       is a unique, deduplicated data pattern stored once and
//       referenced multiple times.
// ===================================================================
#ifndef AEON_MOTIF_H
#define AEON_MOTIF_H

#include "c23_compat.h"

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
NODISCARD Motif* motif_dict_intern(MotifDictionary* dict, void* data, size_t size);
// Decrements the reference count of a motif.
void motif_dict_release(MotifDictionary* dict, uint64_t hash);


// ===================================================================
// C23 ENHANCEMENTS - Phase 1
// ===================================================================

// Motif dictionary constants
constexpr size_t MOTIF_DICT_SIZE = 65536;  // 64K entries
constexpr size_t MOTIF_MAX_LENGTH = 256;
constexpr size_t MOTIF_MIN_FREQUENCY = 2;

// C23 static assertions
_Static_assert(sizeof(Motif) <= 64, "Motif should fit in cache line");

#endif // AEON_MOTIF_H
