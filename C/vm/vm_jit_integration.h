
#ifndef VM_JIT_INTEGRATION_H
#define VM_JIT_INTEGRATION_H

#include "vm.h"
#include "jit/jit_compiler.h"
#include "jit/hot_path.h"
#include "jit/tiered_compilation.h"

// JIT integration configuration
typedef struct {
    bool enable_jit;
    bool enable_hotspot_detection;
    bool enable_tiered_compilation;
    uint64_t jit_threshold;
    uint64_t optimization_threshold;
    uint32_t max_compiled_functions;
} JITConfig;

// JIT code cache entry
typedef struct JITCacheEntry {
    uint32_t function_id;
    CompiledCode* compiled_code;
    uint64_t last_used;
    struct JITCacheEntry* next;
} JITCacheEntry;

// JIT code cache
typedef struct {
    JITCacheEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
    size_t max_entries;
    uint64_t cache_hits;
    uint64_t cache_misses;
} JITCodeCache;

// Enhanced VM with JIT integration
typedef struct {
    EnhancedVM* base_vm;
    
    // JIT components
    JITCompiler* jit_compiler;
    HotPathDetector* hot_path_detector;
    TieredCompilationManager* tiered_compilation;
    JITCodeCache* code_cache;
    
    // Configuration
    JITConfig config;
    
    // Execution statistics
    uint64_t function_executions;
    uint64_t jit_compilations;
    uint64_t jit_optimizations;
    uint64_t jit_execution_time_ns;
    uint64_t interpreter_execution_time_ns;
} JITIntegratedVM;

// JIT integrated VM API
JITIntegratedVM* jit_vm_create(size_t heap_size);
JITIntegratedVM* jit_vm_create_with_config(size_t heap_size, const JITConfig* config);
void jit_vm_destroy(JITIntegratedVM* vm);

// Execution with JIT integration
typedef struct {
    bool success;
    double result_value;
    bool used_jit;
    uint64_t execution_time_ns;
    uint64_t compilation_time_ns;
} JITVmResult;

JITVmResult jit_vm_execute(JITIntegratedVM* vm, const Chunk* chunk);
JITVmResult jit_vm_execute_function(JITIntegratedVM* vm, uint32_t function_id);

// JIT management
bool jit_vm_compile_function(JITIntegratedVM* vm, uint32_t function_id, const Chunk* chunk);
bool jit_vm_optimize_function(JITIntegratedVM* vm, uint32_t function_id, JITTier target_tier);
void jit_vm_invalidate_function(JITIntegratedVM* vm, uint32_t function_id);
void jit_vm_clear_cache(JITIntegratedVM* vm);

// Configuration
void jit_vm_set_config(JITIntegratedVM* vm, const JITConfig* config);
void jit_vm_get_config(const JITIntegratedVM* vm, JITConfig* config);

// Statistics
typedef struct {
    uint64_t total_executions;
    uint64_t jit_executions;
    uint64_t interpreter_executions;
    uint64_t jit_compilations;
    uint64_t jit_optimizations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t jit_execution_time_ns;
    uint64_t interpreter_execution_time_ns;
    uint64_t compilation_time_ns;
    double jit_speedup_ratio;
} JITVmStats;

void jit_vm_get_stats(const JITIntegratedVM* vm, JITVmStats* stats);
void jit_vm_reset_stats(JITIntegratedVM* vm);
void jit_vm_print_stats(const JITIntegratedVM* vm);

// Code cache management
JITCodeCache* jit_code_cache_create(size_t max_entries);
void jit_code_cache_destroy(JITCodeCache* cache);
CompiledCode* jit_code_cache_get(JITCodeCache* cache, uint32_t function_id);
bool jit_code_cache_put(JITCodeCache* cache, uint32_t function_id, CompiledCode* code);
void jit_code_cache_remove(JITCodeCache* cache, uint32_t function_id);
void jit_code_cache_clear(JITCodeCache* cache);

// Default configuration
JITConfig jit_vm_default_config(void);

#endif // VM_JIT_INTEGRATION_H
