
// ===================================================================
// DESC: Enhanced VM implementation with GC integration
// ===================================================================

#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default heap sizes
#define DEFAULT_NURSERY_SIZE (1024 * 1024)      // 1MB
#define DEFAULT_OLD_GEN_SIZE (10 * 1024 * 1024) // 10MB
#define DEFAULT_PERM_SIZE (256 * 1024)          // 256KB

// Helper function to collect VM roots
static void vm_collect_roots(EnhancedVM* vm, GCRootSet* roots);

EnhancedVM* enhanced_vm_create(size_t heap_size) {
    // Split heap: 10% nursery, 90% old generation
    size_t nursery_size = heap_size / 10;
    size_t old_gen_size = heap_size - nursery_size;
    return enhanced_vm_create_with_sizes(nursery_size, old_gen_size);
}

EnhancedVM* enhanced_vm_create_with_sizes(size_t nursery_size, size_t old_gen_size) {
    EnhancedVM* vm = (EnhancedVM*)malloc(sizeof(EnhancedVM));
    if (!vm) return NULL;
    
    // Initialize base VM
    vm->base_vm = (VM*)malloc(sizeof(VM));
    if (!vm->base_vm) {
        free(vm);
        return NULL;
    }
    vm_init(vm->base_vm);
    
    // Initialize GC
    vm->gc = generational_gc_create(nursery_size, old_gen_size, DEFAULT_PERM_SIZE);
    if (!vm->gc) {
        free(vm->base_vm);
        free(vm);
        return NULL;
    }
    
    // Initialize root set
    vm->gc_roots = gc_root_set_create();
    if (!vm->gc_roots) {
        generational_gc_destroy(vm->gc);
        free(vm->base_vm);
        free(vm);
        return NULL;
    }
    
    // Initialize JIT components (NULL for now)
    vm->jit_compiler = NULL;
    vm->hot_path_detector = NULL;
    vm->tiered_compilation = NULL;
    
    // Configuration
    vm->enable_jit = false;
    vm->enable_gc = true;
    vm->enable_profiling = false;
    vm->gc_threshold = 80;  // Trigger at 80% full
    
    // Statistics
    vm->total_executions = 0;
    vm->jit_executions = 0;
    vm->interpreter_executions = 0;
    vm->gc_collections = 0;
    vm->gc_bytes_allocated = 0;
    vm->gc_bytes_freed = 0;
    
    return vm;
}

void enhanced_vm_destroy(EnhancedVM* vm) {
    if (!vm) return;
    
    // Destroy GC
    if (vm->gc) {
        generational_gc_destroy(vm->gc);
    }
    
    // Destroy root set
    if (vm->gc_roots) {
        gc_root_set_destroy(vm->gc_roots);
    }
    
    // Destroy base VM
    if (vm->base_vm) {
        vm_free(vm->base_vm);
        free(vm->base_vm);
    }
    
    // Destroy JIT components (when implemented)
    // TODO: Destroy JIT compiler, hot path detector, tiered compilation
    
    free(vm);
}

void* vm_alloc(EnhancedVM* vm, size_t size) {
    if (!vm || !vm->gc) return NULL;
    
    // Try to allocate from GC
    GenObject* obj = generational_gc_allocate(vm->gc, size, 0, GEN_YOUNG);
    
    if (!obj) {
        // Allocation failed, trigger GC and retry
        vm_gc_collect(vm);
        obj = generational_gc_allocate(vm->gc, size, 0, GEN_YOUNG);
        
        if (!obj) {
            // Still failed, out of memory
            fprintf(stderr, "VM: Out of memory (requested %zu bytes)\n", size);
            return NULL;
        }
    }
    
    vm->gc_bytes_allocated += size;
    return obj;
}

void* vm_alloc_object(EnhancedVM* vm, size_t size, uint32_t type_id) {
    if (!vm || !vm->gc) return NULL;
    
    GenObject* obj = generational_gc_allocate(vm->gc, size, type_id, GEN_YOUNG);
    
    if (!obj) {
        // Allocation failed, trigger GC and retry
        vm_gc_collect(vm);
        obj = generational_gc_allocate(vm->gc, size, type_id, GEN_YOUNG);
        
        if (!obj) {
            fprintf(stderr, "VM: Out of memory (requested %zu bytes, type %u)\n", size, type_id);
            return NULL;
        }
    }
    
    vm->gc_bytes_allocated += size;
    return obj;
}

void vm_gc_collect(EnhancedVM* vm) {
    if (!vm || !vm->gc || !vm->enable_gc) return;
    
    // Collect roots from VM
    vm_collect_roots(vm, vm->gc_roots);
    
    // Get stats before collection
    size_t young_before = vm->gc->generations[GEN_YOUNG].used;
    size_t old_before = vm->gc->generations[GEN_OLD].used;
    
    // Trigger minor GC
    generational_gc_minor_collect(vm->gc, vm->gc_roots);
    
    // Update statistics
    vm->gc_collections++;
    
    // Calculate freed bytes
    size_t young_after = vm->gc->generations[GEN_YOUNG].used;
    size_t old_after = vm->gc->generations[GEN_OLD].used;
    
    if (young_before > young_after) {
        vm->gc_bytes_freed += (young_before - young_after);
    }
    
    // Check if major GC is needed
    GenerationSpace* old_gen = &vm->gc->generations[GEN_OLD];
    if (old_gen->used > old_gen->size * 0.8) {
        generational_gc_major_collect(vm->gc, vm->gc_roots);
        vm->gc_collections++;
        
        if (old_before > old_after) {
            vm->gc_bytes_freed += (old_before - old_after);
        }
    }
}

void vm_write_barrier(EnhancedVM* vm, void* old_obj, void* new_value) {
    if (!vm || !vm->gc) return;
    
    // Guard against NULL operands - legitimate cases include:
    // - Clearing a field: obj->field = NULL
    // - Holder is absent: NULL->field = obj
    if (!old_obj || !new_value) return;
    
    GenObject* old = (GenObject*)old_obj;
    GenObject* new = (GenObject*)new_value;
    
    // Check if this is an old→young pointer
    if (old->header.generation == GEN_OLD && 
        new->header.generation == GEN_YOUNG) {
        // Add to remembered set
        generational_gc_write_barrier(vm->gc, old, new);
    }
}

void vm_register_root(EnhancedVM* vm, GCObject** root) {
    if (!vm || !root || !vm->gc_roots) return;
    gc_root_set_add(vm->gc_roots, *root);
}

void vm_unregister_root(EnhancedVM* vm, GCObject** root) {
    if (!vm || !root || !vm->gc_roots) return;
    gc_root_set_remove(vm->gc_roots, *root);
}

// Helper to collect all VM roots
static void vm_collect_roots(EnhancedVM* vm, GCRootSet* roots) {
    if (!vm || !roots) return;
    
    // NOTE: We do NOT clear the root set here!
    // The root set already contains all registered roots via vm_register_root()
    // We just need to ensure it's passed to the GC
    
    // Add VM stack as roots
    // For the basic VM, the stack contains doubles, not object references
    // In a real implementation, we would scan the stack for object pointers
    // For now, the registered roots (via vm_register_root) are sufficient
    
    // TODO: Add VM stack frames when implemented
    // TODO: Add global variables when implemented
    // TODO: Add current execution frame when implemented
}

// Configuration functions
void enhanced_vm_enable_jit(EnhancedVM* vm, bool enable) {
    if (vm) vm->enable_jit = enable;
}

void enhanced_vm_enable_gc(EnhancedVM* vm, bool enable) {
    if (vm) vm->enable_gc = enable;
}

void enhanced_vm_enable_profiling(EnhancedVM* vm, bool enable) {
    if (vm) vm->enable_profiling = enable;
}

// Statistics functions
void enhanced_vm_get_stats(
    const EnhancedVM* vm,
    uint64_t* total_executions,
    uint64_t* jit_executions,
    uint64_t* interpreter_executions
) {
    if (!vm) return;
    
    if (total_executions) *total_executions = vm->total_executions;
    if (jit_executions) *jit_executions = vm->jit_executions;
    if (interpreter_executions) *interpreter_executions = vm->interpreter_executions;
}

void enhanced_vm_get_gc_stats(
    const EnhancedVM* vm,
    uint64_t* gc_collections,
    uint64_t* gc_bytes_allocated,
    uint64_t* gc_bytes_freed,
    size_t* young_used,
    size_t* old_used
) {
    if (!vm) return;
    
    if (gc_collections) *gc_collections = vm->gc_collections;
    if (gc_bytes_allocated) *gc_bytes_allocated = vm->gc_bytes_allocated;
    if (gc_bytes_freed) *gc_bytes_freed = vm->gc_bytes_freed;
    
    if (vm->gc) {
        if (young_used) *young_used = vm->gc->generations[GEN_YOUNG].used;
        if (old_used) *old_used = vm->gc->generations[GEN_OLD].used;
    }
}

// Execution functions (stubs for now)
bool enhanced_vm_execute(EnhancedVM* vm, const Chunk* chunk) {
    if (!vm || !vm->base_vm) return false;
    
    vm->total_executions++;
    vm->interpreter_executions++;
    
    // For now, just use the base VM interpreter
    InterpretResult result = vm_interpret(vm->base_vm, (Chunk*)chunk);
    
    return result == INTERPRET_OK;
}

bool enhanced_vm_execute_function(EnhancedVM* vm, uint32_t function_id) {
    if (!vm) return false;
    
    vm->total_executions++;
    
    // TODO: Implement function execution
    fprintf(stderr, "VM: Function execution not yet implemented\n");
    
    return false;
}
