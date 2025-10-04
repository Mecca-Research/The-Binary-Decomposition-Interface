
#include "mark_sweep.h"
#include <stdlib.h>
#include <string.h>

MarkSweepGC* mark_sweep_create(size_t heap_size) {
    MarkSweepGC* gc = (MarkSweepGC*)calloc(1, sizeof(MarkSweepGC));
    if (!gc) return NULL;
    
    gc->heap_start = (GCObject*)malloc(heap_size);
    if (!gc->heap_start) {
        free(gc);
        return NULL;
    }
    
    gc->heap_size = heap_size;
    gc->heap_end = (GCObject*)((uint8_t*)gc->heap_start + heap_size);
    gc->heap_used = 0;
    
    // Initialize free list with entire heap
    gc->free_list = gc->heap_start;
    gc->free_list->header.size = heap_size - sizeof(GCObjectHeader);
    gc->free_list->header.marked = false;
    gc->free_list->header.pinned = false;
    gc->free_list->header.next = NULL;
    gc->free_count = 1;
    
    gc->gc_threshold = heap_size / 2;
    gc->gc_growth_factor = 1.5;
    gc->enable_compaction = false;
    
    return gc;
}

void mark_sweep_destroy(MarkSweepGC* gc) {
    if (!gc) return;
    
    free(gc->heap_start);
    free(gc);
}

GCObject* mark_sweep_allocate(MarkSweepGC* gc, size_t size, uint32_t type_id) {
    if (!gc || size == 0) return NULL;
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    size_t total_size = size + sizeof(GCObjectHeader);
    
    // Find suitable free block
    GCObject* prev = NULL;
    GCObject* current = gc->free_list;
    
    while (current) {
        if (current->header.size >= size) {
            // Found suitable block
            GCObject* allocated = current;
            
            // Split block if large enough
            if (current->header.size >= total_size + sizeof(GCObjectHeader) + 64) {
                GCObject* remainder = (GCObject*)((uint8_t*)current + total_size);
                remainder->header.size = current->header.size - total_size;
                remainder->header.marked = false;
                remainder->header.pinned = false;
                remainder->header.next = current->header.next;
                
                if (prev) {
                    prev->header.next = remainder;
                } else {
                    gc->free_list = remainder;
                }
            } else {
                // Use entire block
                if (prev) {
                    prev->header.next = current->header.next;
                } else {
                    gc->free_list = current->header.next;
                }
                gc->free_count--;
            }
            
            allocated->header.size = size;
            allocated->header.type_id = type_id;
            allocated->header.marked = false;
            allocated->header.pinned = false;
            allocated->header.next = NULL;
            
            gc->heap_used += total_size;
            gc->objects_allocated++;
            gc->bytes_allocated += size;
            
            return allocated;
        }
        
        prev = current;
        current = current->header.next;
    }
    
    return NULL;
}

void mark_sweep_free(MarkSweepGC* gc, GCObject* object) {
    if (!gc || !object) return;
    
    size_t total_size = object->header.size + sizeof(GCObjectHeader);
    
    // Add to free list
    object->header.next = gc->free_list;
    gc->free_list = object;
    gc->free_count++;
    
    gc->heap_used -= total_size;
    gc->objects_freed++;
    gc->bytes_freed += object->header.size;
}

static void mark_object(GCObject* object) {
    if (!object || object->header.marked) return;
    
    object->header.marked = true;
    
    // In production: Recursively mark referenced objects
    // This would traverse object fields and mark reachable objects
}

static void mark_phase(MarkSweepGC* gc, GCRootSet* roots) {
    if (!gc || !roots) return;
    
    // Mark all objects reachable from roots
    for (size_t i = 0; i < roots->root_count; i++) {
        mark_object(roots->roots[i]);
    }
}

static void sweep_phase(MarkSweepGC* gc) {
    if (!gc) return;
    
    GCObject* current = gc->heap_start;
    
    while ((uint8_t*)current < (uint8_t*)gc->heap_end) {
        if (!current->header.marked && !current->header.pinned) {
            // Object is garbage, free it
            mark_sweep_free(gc, current);
        } else {
            // Clear mark for next collection
            current->header.marked = false;
        }
        
        // Move to next object
        size_t total_size = current->header.size + sizeof(GCObjectHeader);
        current = (GCObject*)((uint8_t*)current + total_size);
    }
}

bool mark_sweep_collect(MarkSweepGC* gc, GCRootSet* roots) {
    if (!gc || !roots) return false;
    
    // BUG FIX (P1): Clear free list before rebuilding it in sweep_phase
    // Without this, sweep_phase appends to the existing list, causing
    // duplicate entries and corrupted heap_used accounting on repeated GCs
    gc->free_list = NULL;
    gc->free_count = 0;
    
    mark_phase(gc, roots);
    sweep_phase(gc);
    
    gc->collections++;
    
    // Update threshold
    gc->gc_threshold = (size_t)(gc->heap_used * gc->gc_growth_factor);
    
    return true;
}

void mark_sweep_compact(MarkSweepGC* gc) {
    if (!gc || !gc->enable_compaction) return;
    
    // In production: Implement heap compaction
    // This would move live objects together and update references
}

GCRootSet* gc_root_set_create(void) {
    GCRootSet* roots = (GCRootSet*)calloc(1, sizeof(GCRootSet));
    if (!roots) return NULL;
    
    roots->root_capacity = 64;
    roots->roots = (GCObject**)calloc(roots->root_capacity, sizeof(GCObject*));
    if (!roots->roots) {
        free(roots);
        return NULL;
    }
    
    return roots;
}

void gc_root_set_destroy(GCRootSet* roots) {
    if (!roots) return;
    
    free(roots->roots);
    free(roots);
}

bool gc_root_set_add(GCRootSet* roots, GCObject* object) {
    if (!roots || !object) return false;
    
    // Check if already in root set
    for (size_t i = 0; i < roots->root_count; i++) {
        if (roots->roots[i] == object) return true;
    }
    
    // Expand if needed
    if (roots->root_count >= roots->root_capacity) {
        size_t new_capacity = roots->root_capacity * 2;
        GCObject** new_roots = (GCObject**)realloc(roots->roots, 
                                                    new_capacity * sizeof(GCObject*));
        if (!new_roots) return false;
        
        roots->roots = new_roots;
        roots->root_capacity = new_capacity;
    }
    
    roots->roots[roots->root_count++] = object;
    return true;
}

bool gc_root_set_remove(GCRootSet* roots, GCObject* object) {
    if (!roots || !object) return false;
    
    for (size_t i = 0; i < roots->root_count; i++) {
        if (roots->roots[i] == object) {
            // Shift remaining elements
            memmove(&roots->roots[i], &roots->roots[i + 1],
                   (roots->root_count - i - 1) * sizeof(GCObject*));
            roots->root_count--;
            return true;
        }
    }
    
    return false;
}

void gc_object_pin(GCObject* object) {
    if (object) {
        object->header.pinned = true;
    }
}

void gc_object_unpin(GCObject* object) {
    if (object) {
        object->header.pinned = false;
    }
}

bool gc_object_is_marked(const GCObject* object) {
    return object ? object->header.marked : false;
}

size_t gc_object_size(const GCObject* object) {
    return object ? object->header.size : 0;
}

void mark_sweep_get_stats(
    const MarkSweepGC* gc,
    uint64_t* collections,
    uint64_t* objects_allocated,
    uint64_t* objects_freed,
    size_t* heap_used,
    size_t* heap_size
) {
    if (!gc) return;
    
    if (collections) *collections = gc->collections;
    if (objects_allocated) *objects_allocated = gc->objects_allocated;
    if (objects_freed) *objects_freed = gc->objects_freed;
    if (heap_used) *heap_used = gc->heap_used;
    if (heap_size) *heap_size = gc->heap_size;
}

void mark_sweep_reset_stats(MarkSweepGC* gc) {
    if (!gc) return;
    
    gc->collections = 0;
    gc->objects_allocated = 0;
    gc->objects_freed = 0;
    gc->bytes_allocated = 0;
    gc->bytes_freed = 0;
}

void mark_sweep_set_threshold(MarkSweepGC* gc, size_t threshold) {
    if (gc) {
        gc->gc_threshold = threshold;
    }
}

void mark_sweep_set_growth_factor(MarkSweepGC* gc, double factor) {
    if (gc && factor > 1.0) {
        gc->gc_growth_factor = factor;
    }
}

void mark_sweep_enable_compaction(MarkSweepGC* gc, bool enable) {
    if (gc) {
        gc->enable_compaction = enable;
    }
}
