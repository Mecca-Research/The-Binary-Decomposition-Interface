
#ifndef GENERATIONAL_GC_H
#define GENERATIONAL_GC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mark_sweep.h"

// Generation identifiers
typedef enum {
    GEN_YOUNG = 0,      // Young generation (nursery)
    GEN_OLD = 1,        // Old generation (tenured)
    GEN_PERMANENT = 2,  // Permanent generation
    GEN_COUNT = 3
} Generation;

// Generational object header
typedef struct {
    GCObjectHeader base;
    Generation generation;
    uint32_t age;           // Number of collections survived
    bool remembered;        // In remembered set
} GenObjectHeader;

// Generational object
typedef struct GenObject {
    GenObjectHeader header;
    uint8_t data[];
} GenObject;

// Generation space
typedef struct {
    void* start;
    void* end;
    size_t size;
    size_t used;
    
    GenObject* allocation_ptr;
    GenObject* limit_ptr;
    
    uint64_t collections;
    uint64_t promotions;
} GenerationSpace;

// Remembered set for cross-generation references
typedef struct {
    GenObject** entries;
    size_t count;
    size_t capacity;
} RememberedSet;

// Generational garbage collector
typedef struct {
    GenerationSpace generations[GEN_COUNT];
    RememberedSet remembered_set;
    
    // Promotion thresholds
    uint32_t young_age_threshold;
    size_t young_size_threshold;
    
    // Collection triggers
    size_t young_gc_threshold;
    size_t old_gc_threshold;
    
    // Statistics
    uint64_t minor_collections;
    uint64_t major_collections;
    uint64_t total_promotions;
    uint64_t total_allocated;
    uint64_t total_freed;
    
    // Configuration
    bool enable_parallel_gc;
    bool enable_concurrent_gc;
    uint32_t gc_threads;
} GenerationalGC;

// Generational GC API
GenerationalGC* generational_gc_create(
    size_t young_size,
    size_t old_size,
    size_t permanent_size
);

void generational_gc_destroy(GenerationalGC* gc);

GenObject* generational_gc_allocate(
    GenerationalGC* gc,
    size_t size,
    uint32_t type_id,
    Generation generation
);

void generational_gc_free(GenerationalGC* gc, GenObject* object);

// Collection operations
bool generational_gc_minor_collect(GenerationalGC* gc, GCRootSet* roots);
bool generational_gc_major_collect(GenerationalGC* gc, GCRootSet* roots);
bool generational_gc_full_collect(GenerationalGC* gc, GCRootSet* roots);

// Promotion
GenObject* generational_gc_promote(GenerationalGC* gc, GenObject* object, Generation target_gen);
bool generational_gc_should_promote(const GenerationalGC* gc, const GenObject* object);

// Remembered set management
bool remembered_set_add(RememberedSet* set, GenObject* object);
bool remembered_set_remove(RememberedSet* set, GenObject* object);
void remembered_set_clear(RememberedSet* set);

// Write barrier for cross-generation references
void generational_gc_write_barrier(
    GenerationalGC* gc,
    GenObject* parent,
    GenObject* child
);

// Statistics
void generational_gc_get_stats(
    const GenerationalGC* gc,
    uint64_t* minor_collections,
    uint64_t* major_collections,
    uint64_t* total_promotions,
    size_t* young_used,
    size_t* old_used
);

void generational_gc_reset_stats(GenerationalGC* gc);

// Configuration
void generational_gc_set_age_threshold(GenerationalGC* gc, uint32_t threshold);
void generational_gc_set_size_threshold(GenerationalGC* gc, size_t threshold);
void generational_gc_enable_parallel(GenerationalGC* gc, bool enable, uint32_t threads);
void generational_gc_enable_concurrent(GenerationalGC* gc, bool enable);

#endif // GENERATIONAL_GC_H
