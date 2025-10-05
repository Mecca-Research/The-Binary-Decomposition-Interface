
/**
 * @file vm.h
 * @brief Virtual Machine Core API
 * @details This file provides the vm functionality for the BDI virtual machine execution environment.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BCI_VM_ENHANCED_H
#define BCI_VM_ENHANCED_H

#include "bci_vm.h"
#include "jit/jit_compiler.h"
#include "jit/hot_path.h"
#include "jit/tiered_compilation.h"
#include "gc/mark_sweep.h"
#include "gc/generational_gc.h"

// Enhanced VM with JIT and GC
typedef struct {
    VM* base_vm;
    
    // JIT compilation
    JITCompiler* jit_compiler;
    HotPathDetector* hot_path_detector;
    TieredCompilationManager* tiered_compilation;
    
    // Garbage collection
    GenerationalGC* gc;
    GCRootSet* gc_roots;
    
    // Configuration
    bool enable_jit;
    bool enable_gc;
    bool enable_profiling;
    
    // GC Configuration
    size_t gc_threshold;  // Trigger GC when nursery reaches this %
    
    // Statistics
    uint64_t total_executions;
    uint64_t jit_executions;
    uint64_t interpreter_executions;
    uint64_t gc_collections;
    uint64_t gc_bytes_allocated;
    uint64_t gc_bytes_freed;
} EnhancedVM;

// Enhanced VM API
EnhancedVM* enhanced_vm_create(size_t heap_size);
EnhancedVM* enhanced_vm_create_with_sizes(size_t nursery_size, size_t old_gen_size);
void enhanced_vm_destroy(EnhancedVM* vm);

bool enhanced_vm_execute(EnhancedVM* vm, const Chunk* chunk);
bool enhanced_vm_execute_function(EnhancedVM* vm, uint32_t function_id);

// Enhanced VM execution with result capture (mirrors BciVmResult)
typedef struct {
    bool success;
    double result_value;
} EnhancedVmResult;

EnhancedVmResult enhanced_vm_execute_with_result(EnhancedVM* vm, const Chunk* chunk);

// Configuration
void enhanced_vm_enable_jit(EnhancedVM* vm, bool enable);
void enhanced_vm_enable_gc(EnhancedVM* vm, bool enable);
void enhanced_vm_enable_profiling(EnhancedVM* vm, bool enable);

// GC-aware allocation functions
void* vm_alloc(EnhancedVM* vm, size_t size);
void* vm_alloc_object(EnhancedVM* vm, size_t size, uint32_t type_id);
void vm_write_barrier(EnhancedVM* vm, void* old_obj, void* new_value);
void vm_gc_collect(EnhancedVM* vm);
void vm_register_root(EnhancedVM* vm, GCObject** root);
void vm_unregister_root(EnhancedVM* vm, GCObject** root);

// Statistics
void enhanced_vm_get_stats(
    const EnhancedVM* vm,
    uint64_t* total_executions,
    uint64_t* jit_executions,
    uint64_t* interpreter_executions
);

void enhanced_vm_get_gc_stats(
    const EnhancedVM* vm,
    uint64_t* gc_collections,
    uint64_t* gc_bytes_allocated,
    uint64_t* gc_bytes_freed,
    size_t* young_used,
    size_t* old_used
);

#endif // BCI_VM_ENHANCED_H
