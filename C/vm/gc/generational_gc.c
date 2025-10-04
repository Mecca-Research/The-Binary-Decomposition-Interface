
#include "generational_gc.h"
#include <stdlib.h>
#include <string.h>

GenerationalGC* generational_gc_create(
    size_t young_size,
    size_t old_size,
    size_t permanent_size
) {
    GenerationalGC* gc = (GenerationalGC*)calloc(1, sizeof(GenerationalGC));
    if (!gc) return NULL;
    
    // Initialize young generation
    gc->generations[GEN_YOUNG].size = young_size;
    gc->generations[GEN_YOUNG].start = malloc(young_size);
    if (!gc->generations[GEN_YOUNG].start) {
        free(gc);
        return NULL;
    }
    gc->generations[GEN_YOUNG].end = (uint8_t*)gc->generations[GEN_YOUNG].start + young_size;
    gc->generations[GEN_YOUNG].allocation_ptr = (GenObject*)gc->generations[GEN_YOUNG].start;
    gc->generations[GEN_YOUNG].limit_ptr = (GenObject*)gc->generations[GEN_YOUNG].end;
    
    // Initialize old generation
    gc->generations[GEN_OLD].size = old_size;
    gc->generations[GEN_OLD].start = malloc(old_size);
    if (!gc->generations[GEN_OLD].start) {
        free(gc->generations[GEN_YOUNG].start);
        free(gc);
        return NULL;
    }
    gc->generations[GEN_OLD].end = (uint8_t*)gc->generations[GEN_OLD].start + old_size;
    gc->generations[GEN_OLD].allocation_ptr = (GenObject*)gc->generations[GEN_OLD].start;
    gc->generations[GEN_OLD].limit_ptr = (GenObject*)gc->generations[GEN_OLD].end;
    
    // Initialize permanent generation
    gc->generations[GEN_PERMANENT].size = permanent_size;
    gc->generations[GEN_PERMANENT].start = malloc(permanent_size);
    if (!gc->generations[GEN_PERMANENT].start) {
        free(gc->generations[GEN_YOUNG].start);
        free(gc->generations[GEN_OLD].start);
        free(gc);
        return NULL;
    }
    gc->generations[GEN_PERMANENT].end = (uint8_t*)gc->generations[GEN_PERMANENT].start + permanent_size;
    gc->generations[GEN_PERMANENT].allocation_ptr = (GenObject*)gc->generations[GEN_PERMANENT].start;
    gc->generations[GEN_PERMANENT].limit_ptr = (GenObject*)gc->generations[GEN_PERMANENT].end;
    
    // Initialize remembered set
    gc->remembered_set.capacity = 256;
    gc->remembered_set.entries = (GenObject**)calloc(gc->remembered_set.capacity, sizeof(GenObject*));
    if (!gc->remembered_set.entries) {
        free(gc->generations[GEN_YOUNG].start);
        free(gc->generations[GEN_OLD].start);
        free(gc->generations[GEN_PERMANENT].start);
        free(gc);
        return NULL;
    }
    
    // Set default thresholds
    gc->young_age_threshold = 3;
    gc->young_size_threshold = 1024;
    gc->young_gc_threshold = young_size / 2;
    gc->old_gc_threshold = old_size / 2;
    
    gc->enable_parallel_gc = false;
    gc->enable_concurrent_gc = false;
    gc->gc_threads = 1;
    
    return gc;
}

void generational_gc_destroy(GenerationalGC* gc) {
    if (!gc) return;
    
    free(gc->generations[GEN_YOUNG].start);
    free(gc->generations[GEN_OLD].start);
    free(gc->generations[GEN_PERMANENT].start);
    free(gc->remembered_set.entries);
    free(gc);
}

GenObject* generational_gc_allocate(
    GenerationalGC* gc,
    size_t size,
    uint32_t type_id,
    Generation generation
) {
    if (!gc || size == 0 || generation >= GEN_COUNT) return NULL;
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    size_t total_size = size + sizeof(GenObjectHeader);
    
    GenerationSpace* gen_space = &gc->generations[generation];
    
    // Check if enough space
    if ((uint8_t*)gen_space->allocation_ptr + total_size > (uint8_t*)gen_space->limit_ptr) {
        return NULL;
    }
    
    GenObject* object = gen_space->allocation_ptr;
    gen_space->allocation_ptr = (GenObject*)((uint8_t*)gen_space->allocation_ptr + total_size);
    
    object->header.base.size = size;
    object->header.base.type_id = type_id;
    object->header.base.marked = false;
    object->header.base.pinned = false;
    object->header.base.next = NULL;
    object->header.generation = generation;
    object->header.age = 0;
    object->header.remembered = false;
    
    gen_space->used += total_size;
    gc->total_allocated += size;
    
    return object;
}

void generational_gc_free(GenerationalGC* gc, GenObject* object) {
    if (!gc || !object) return;
    
    Generation gen = object->header.generation;
    size_t total_size = object->header.base.size + sizeof(GenObjectHeader);
    
    gc->generations[gen].used -= total_size;
    gc->total_freed += object->header.base.size;
}

static void mark_young_generation(GenerationalGC* gc, GCRootSet* roots) {
    // Mark objects in young generation reachable from roots
    for (size_t i = 0; i < roots->root_count; i++) {
        GenObject* obj = (GenObject*)roots->roots[i];
        if (obj && obj->header.generation == GEN_YOUNG) {
            obj->header.base.marked = true;
        }
    }
    
    // Mark objects referenced from remembered set
    for (size_t i = 0; i < gc->remembered_set.count; i++) {
        GenObject* obj = gc->remembered_set.entries[i];
        if (obj) {
            obj->header.base.marked = true;
        }
    }
}

// Forwarding table for tracking object relocations during compaction
typedef struct {
    GenObject* old_addr;
    GenObject* new_addr;
} ForwardingEntry;

#define MAX_FORWARDING_ENTRIES 1024
static ForwardingEntry forwarding_table[MAX_FORWARDING_ENTRIES];
static size_t forwarding_count = 0;

static bool add_forwarding_entry(GenObject* old_addr, GenObject* new_addr) {
    if (forwarding_count < MAX_FORWARDING_ENTRIES) {
        forwarding_table[forwarding_count].old_addr = old_addr;
        forwarding_table[forwarding_count].new_addr = new_addr;
        forwarding_count++;
        return true;
    }
    return false;  // Table full - caller must handle overflow
}

static GenObject* resolve_forwarding(GenObject* addr) {
    for (size_t i = 0; i < forwarding_count; i++) {
        if (forwarding_table[i].old_addr == addr) {
            return forwarding_table[i].new_addr;
        }
    }
    return addr;  // Not forwarded, return original
}

static void evacuate_young_generation(GenerationalGC* gc) {
    GenerationSpace* young = &gc->generations[GEN_YOUNG];
    GenObject* current = (GenObject*)young->start;
    GenObject* compact_ptr = (GenObject*)young->start;  // Compaction destination
    
    // Reset forwarding table
    forwarding_count = 0;
    bool compaction_enabled = true;
    size_t survivor_count = 0;
    
    // PRE-PASS: Count survivors to check if compaction is feasible
    current = (GenObject*)young->start;
    while ((uint8_t*)current < (uint8_t*)young->allocation_ptr) {
        size_t total_size = current->header.base.size + sizeof(GenObjectHeader);
        
        if (current->header.base.marked) {
            survivor_count++;
            
            // Check if should promote (but don't promote yet)
            if (!generational_gc_should_promote(gc, current)) {
                // This object will stay in nursery and need a forwarding entry
                // (unless it's already at the compact position)
            }
        }
        
        current = (GenObject*)((uint8_t*)current + total_size);
    }
    
    // BUG FIX #2 (P1): Check if forwarding table would overflow
    // If we have too many survivors, disable compaction to avoid corruption
    if (survivor_count > MAX_FORWARDING_ENTRIES) {
        compaction_enabled = false;
    }
    
    if (compaction_enabled) {
        // PASS 1: Mark survivors and promoted objects, build forwarding table
        current = (GenObject*)young->start;
        compact_ptr = (GenObject*)young->start;
        
        while ((uint8_t*)current < (uint8_t*)young->allocation_ptr) {
            size_t total_size = current->header.base.size + sizeof(GenObjectHeader);
            
            if (current->header.base.marked) {
                // Object survived, increment age
                current->header.age++;
                
                // Check if should promote
                if (generational_gc_should_promote(gc, current)) {
                    // BUG FIX #1 (P0): Record forwarding for promoted objects
                    // Promote to old generation and get the new address
                    GenObject* new_addr = generational_gc_promote(gc, current, GEN_OLD);
                    if (new_addr) {
                        // Record forwarding: old nursery addr → new old-gen addr
                        if (!add_forwarding_entry(current, new_addr)) {
                            // Forwarding table overflow - should not happen due to pre-pass check
                            // but handle it gracefully by disabling compaction
                            compaction_enabled = false;
                            break;
                        }
                    }
                } else {
                    // Object stays in nursery - will be compacted
                    // Record forwarding entry if object will move
                    if (current != compact_ptr) {
                        if (!add_forwarding_entry(current, compact_ptr)) {
                            // Forwarding table overflow
                            compaction_enabled = false;
                            break;
                        }
                    }
                    // Advance compaction pointer (but don't move yet)
                    compact_ptr = (GenObject*)((uint8_t*)compact_ptr + total_size);
                }
                
                current->header.base.marked = false;
            } else {
                // Object is garbage - free it and don't compact
                generational_gc_free(gc, current);
            }
            
            current = (GenObject*)((uint8_t*)current + total_size);
        }
        
        if (compaction_enabled) {
            // PASS 2: Compact survivors to the beginning
            current = (GenObject*)young->start;
            compact_ptr = (GenObject*)young->start;
            
            while ((uint8_t*)current < (uint8_t*)young->allocation_ptr) {
                size_t total_size = current->header.base.size + sizeof(GenObjectHeader);
                
                // Check if this object is a survivor (not promoted, not garbage)
                // A survivor is one that has a forwarding entry OR is already at compact_ptr
                bool is_survivor = false;
                for (size_t i = 0; i < forwarding_count; i++) {
                    if (forwarding_table[i].old_addr == current) {
                        // Check if it's promoted (new_addr is in old gen) or staying in nursery
                        GenObject* new_addr = forwarding_table[i].new_addr;
                        if ((uint8_t*)new_addr >= (uint8_t*)young->start && 
                            (uint8_t*)new_addr < (uint8_t*)young->end) {
                            // Staying in nursery - this is a survivor to compact
                            is_survivor = true;
                        }
                        break;
                    }
                }
                
                // Also check if object is at the compact position (no forwarding needed)
                if (current == compact_ptr && current->header.generation == GEN_YOUNG) {
                    // Check if it wasn't promoted (age should be > 0 if it survived)
                    if (current->header.age > 0) {
                        is_survivor = true;
                    }
                }
                
                if (is_survivor) {
                    // Move object to compacted position if needed
                    if (current != compact_ptr) {
                        memmove(compact_ptr, current, total_size);
                    }
                    compact_ptr = (GenObject*)((uint8_t*)compact_ptr + total_size);
                }
                
                current = (GenObject*)((uint8_t*)current + total_size);
            }
            
            // Update allocation pointer to point after compacted survivors
            young->allocation_ptr = compact_ptr;
            young->used = (uint8_t*)compact_ptr - (uint8_t*)young->start;
        }
    }
    
    // BUG FIX #2 (P1): Fallback to no-compaction mode if table would overflow
    if (!compaction_enabled) {
        // Reset forwarding table since we're not compacting
        forwarding_count = 0;
        
        // Process objects without compaction
        current = (GenObject*)young->start;
        GenObject* last_survivor = NULL;
        
        while ((uint8_t*)current < (uint8_t*)young->allocation_ptr) {
            size_t total_size = current->header.base.size + sizeof(GenObjectHeader);
            
            if (current->header.base.marked) {
                // Object survived, increment age
                current->header.age++;
                
                // Check if should promote
                if (generational_gc_should_promote(gc, current)) {
                    // Promote to old generation
                    generational_gc_promote(gc, current, GEN_OLD);
                } else {
                    // Object stays in nursery - keep in place (no compaction)
                    last_survivor = current;
                }
                
                current->header.base.marked = false;
            } else {
                // Object is garbage - free it
                generational_gc_free(gc, current);
            }
            
            current = (GenObject*)((uint8_t*)current + total_size);
        }
        
        // Update allocation pointer to point after last survivor
        if (last_survivor) {
            size_t last_size = last_survivor->header.base.size + sizeof(GenObjectHeader);
            young->allocation_ptr = (GenObject*)((uint8_t*)last_survivor + last_size);
        } else {
            // No survivors, reset nursery
            young->allocation_ptr = (GenObject*)young->start;
        }
        young->used = (uint8_t*)young->allocation_ptr - (uint8_t*)young->start;
    }
    
    // PASS 3: Update all references using forwarding table
    // This is critical to prevent dangling pointers!
    
    // Update remembered set entries
    for (size_t i = 0; i < gc->remembered_set.count; i++) {
        GenObject* old_ref = gc->remembered_set.entries[i];
        GenObject* new_ref = resolve_forwarding(old_ref);
        if (new_ref != old_ref) {
            gc->remembered_set.entries[i] = new_ref;
        }
    }
}

bool generational_gc_minor_collect(GenerationalGC* gc, GCRootSet* roots) {
    if (!gc || !roots) return false;
    
    mark_young_generation(gc, roots);
    evacuate_young_generation(gc);
    
    // CRITICAL: Update root references after compaction
    // This prevents dangling pointers to moved objects
    for (size_t i = 0; i < roots->root_count; i++) {
        GenObject* old_root = (GenObject*)roots->roots[i];
        if (old_root && old_root->header.generation == GEN_YOUNG) {
            GenObject* new_root = resolve_forwarding(old_root);
            if (new_root != old_root) {
                roots->roots[i] = (GCObject*)new_root;
            }
        }
    }
    
    gc->minor_collections++;
    gc->generations[GEN_YOUNG].collections++;
    
    return true;
}

bool generational_gc_major_collect(GenerationalGC* gc, GCRootSet* roots) {
    if (!gc || !roots) return false;
    
    // Collect old generation (similar to mark-sweep)
    // In production: Implement full mark-sweep for old generation
    
    gc->major_collections++;
    gc->generations[GEN_OLD].collections++;
    
    return true;
}

bool generational_gc_full_collect(GenerationalGC* gc, GCRootSet* roots) {
    if (!gc || !roots) return false;
    
    // Collect all generations
    generational_gc_minor_collect(gc, roots);
    generational_gc_major_collect(gc, roots);
    
    return true;
}

GenObject* generational_gc_promote(GenerationalGC* gc, GenObject* object, Generation target_gen) {
    if (!gc || !object || target_gen >= GEN_COUNT) return NULL;
    if (object->header.generation >= target_gen) return NULL;
    
    size_t size = object->header.base.size;
    uint32_t type_id = object->header.base.type_id;
    
    // Allocate in target generation
    GenObject* new_object = generational_gc_allocate(gc, size, type_id, target_gen);
    if (!new_object) return NULL;
    
    // Copy data
    memcpy(new_object->data, object->data, size);
    new_object->header.age = object->header.age;
    
    // Update statistics
    gc->total_promotions++;
    gc->generations[object->header.generation].promotions++;
    
    // BUG FIX #1 (P0): Return the new address in old generation
    // This allows the caller to record forwarding entries for promoted objects
    return new_object;
}

bool generational_gc_should_promote(const GenerationalGC* gc, const GenObject* object) {
    if (!gc || !object) return false;
    
    // Promote if age threshold reached
    if (object->header.age >= gc->young_age_threshold) return true;
    
    // Promote if object is large
    if (object->header.base.size >= gc->young_size_threshold) return true;
    
    return false;
}

bool remembered_set_add(RememberedSet* set, GenObject* object) {
    if (!set || !object) return false;
    
    // Check if already in set
    for (size_t i = 0; i < set->count; i++) {
        if (set->entries[i] == object) return true;
    }
    
    // Expand if needed
    if (set->count >= set->capacity) {
        size_t new_capacity = set->capacity * 2;
        GenObject** new_entries = (GenObject**)realloc(set->entries,
                                                        new_capacity * sizeof(GenObject*));
        if (!new_entries) return false;
        
        set->entries = new_entries;
        set->capacity = new_capacity;
    }
    
    set->entries[set->count++] = object;
    object->header.remembered = true;
    
    return true;
}

bool remembered_set_remove(RememberedSet* set, GenObject* object) {
    if (!set || !object) return false;
    
    for (size_t i = 0; i < set->count; i++) {
        if (set->entries[i] == object) {
            memmove(&set->entries[i], &set->entries[i + 1],
                   (set->count - i - 1) * sizeof(GenObject*));
            set->count--;
            object->header.remembered = false;
            return true;
        }
    }
    
    return false;
}

void remembered_set_clear(RememberedSet* set) {
    if (!set) return;
    
    for (size_t i = 0; i < set->count; i++) {
        if (set->entries[i]) {
            set->entries[i]->header.remembered = false;
        }
    }
    
    set->count = 0;
}

void generational_gc_write_barrier(
    GenerationalGC* gc,
    GenObject* parent,
    GenObject* child
) {
    if (!gc || !parent || !child) return;
    
    // If old object references young object, add to remembered set
    if (parent->header.generation > child->header.generation) {
        remembered_set_add(&gc->remembered_set, child);
    }
}

void generational_gc_get_stats(
    const GenerationalGC* gc,
    uint64_t* minor_collections,
    uint64_t* major_collections,
    uint64_t* total_promotions,
    size_t* young_used,
    size_t* old_used
) {
    if (!gc) return;
    
    if (minor_collections) *minor_collections = gc->minor_collections;
    if (major_collections) *major_collections = gc->major_collections;
    if (total_promotions) *total_promotions = gc->total_promotions;
    if (young_used) *young_used = gc->generations[GEN_YOUNG].used;
    if (old_used) *old_used = gc->generations[GEN_OLD].used;
}

void generational_gc_reset_stats(GenerationalGC* gc) {
    if (!gc) return;
    
    gc->minor_collections = 0;
    gc->major_collections = 0;
    gc->total_promotions = 0;
    gc->total_allocated = 0;
    gc->total_freed = 0;
    
    for (int i = 0; i < GEN_COUNT; i++) {
        gc->generations[i].collections = 0;
        gc->generations[i].promotions = 0;
    }
}

void generational_gc_set_age_threshold(GenerationalGC* gc, uint32_t threshold) {
    if (gc) {
        gc->young_age_threshold = threshold;
    }
}

void generational_gc_set_size_threshold(GenerationalGC* gc, size_t threshold) {
    if (gc) {
        gc->young_size_threshold = threshold;
    }
}

void generational_gc_enable_parallel(GenerationalGC* gc, bool enable, uint32_t threads) {
    if (gc) {
        gc->enable_parallel_gc = enable;
        gc->gc_threads = threads;
    }
}

void generational_gc_enable_concurrent(GenerationalGC* gc, bool enable) {
    if (gc) {
        gc->enable_concurrent_gc = enable;
    }
}
