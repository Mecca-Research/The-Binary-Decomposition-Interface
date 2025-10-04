
#ifndef MARK_SWEEP_H
#define MARK_SWEEP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Object header for mark-sweep GC
typedef struct {
    uint32_t size;           // Object size in bytes
    uint32_t type_id;        // Type identifier
    bool marked;             // Mark bit for GC
    bool pinned;             // Pinned objects are not collected
    struct GCObject* next;   // Free list pointer
} GCObjectHeader;

// GC object
typedef struct GCObject {
    GCObjectHeader header;
    uint8_t data[];          // Flexible array member for object data
} GCObject;

// Mark-sweep collector
typedef struct {
    GCObject* heap_start;
    GCObject* heap_end;
    size_t heap_size;
    size_t heap_used;
    
    GCObject* free_list;
    size_t free_count;
    
    // GC statistics
    uint64_t collections;
    uint64_t objects_allocated;
    uint64_t objects_freed;
    uint64_t bytes_allocated;
    uint64_t bytes_freed;
    
    // GC configuration
    size_t gc_threshold;
    double gc_growth_factor;
    bool enable_compaction;
} MarkSweepGC;

// Root set for GC
typedef struct {
    GCObject** roots;
    size_t root_count;
    size_t root_capacity;
} GCRootSet;

// Mark-sweep GC API
MarkSweepGC* mark_sweep_create(size_t heap_size);
void mark_sweep_destroy(MarkSweepGC* gc);

GCObject* mark_sweep_allocate(MarkSweepGC* gc, size_t size, uint32_t type_id);
void mark_sweep_free(MarkSweepGC* gc, GCObject* object);

bool mark_sweep_collect(MarkSweepGC* gc, GCRootSet* roots);
void mark_sweep_compact(MarkSweepGC* gc);

// Root set management
GCRootSet* gc_root_set_create(void);
void gc_root_set_destroy(GCRootSet* roots);
bool gc_root_set_add(GCRootSet* roots, GCObject* object);
bool gc_root_set_remove(GCRootSet* roots, GCObject* object);

// Object operations
void gc_object_pin(GCObject* object);
void gc_object_unpin(GCObject* object);
bool gc_object_is_marked(const GCObject* object);
size_t gc_object_size(const GCObject* object);

// Statistics
void mark_sweep_get_stats(
    const MarkSweepGC* gc,
    uint64_t* collections,
    uint64_t* objects_allocated,
    uint64_t* objects_freed,
    size_t* heap_used,
    size_t* heap_size
);

void mark_sweep_reset_stats(MarkSweepGC* gc);

// Configuration
void mark_sweep_set_threshold(MarkSweepGC* gc, size_t threshold);
void mark_sweep_set_growth_factor(MarkSweepGC* gc, double factor);
void mark_sweep_enable_compaction(MarkSweepGC* gc, bool enable);

#endif // MARK_SWEEP_H
