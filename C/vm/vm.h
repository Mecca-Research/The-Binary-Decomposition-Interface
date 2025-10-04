
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
    BCIVM* base_vm;
    
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
    
    // Statistics
    uint64_t total_executions;
    uint64_t jit_executions;
    uint64_t interpreter_executions;
} EnhancedVM;

// Enhanced VM API
EnhancedVM* enhanced_vm_create(size_t heap_size);
void enhanced_vm_destroy(EnhancedVM* vm);

bool enhanced_vm_execute(EnhancedVM* vm, const BCIChunk* chunk);
bool enhanced_vm_execute_function(EnhancedVM* vm, uint32_t function_id);

// Configuration
void enhanced_vm_enable_jit(EnhancedVM* vm, bool enable);
void enhanced_vm_enable_gc(EnhancedVM* vm, bool enable);
void enhanced_vm_enable_profiling(EnhancedVM* vm, bool enable);

// Statistics
void enhanced_vm_get_stats(
    const EnhancedVM* vm,
    uint64_t* total_executions,
    uint64_t* jit_executions,
    uint64_t* interpreter_executions
);

#endif // BCI_VM_ENHANCED_H
