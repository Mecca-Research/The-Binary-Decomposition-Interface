
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "vm_jit_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Default JIT configuration
JITConfig jit_vm_default_config(void) {
    JITConfig config = {
        .enable_jit = true,
        .enable_hotspot_detection = true,
        .enable_tiered_compilation = true,
        .jit_threshold = 100,
        .optimization_threshold = 1000,
        .max_compiled_functions = 1024
    };
    return config;
}

// JIT Code Cache Implementation
JITCodeCache* jit_code_cache_create(size_t max_entries) {
    JITCodeCache* cache = (JITCodeCache*)calloc(1, sizeof(JITCodeCache));
    if (!cache) return NULL;
    
    cache->bucket_count = max_entries / 4; // Load factor of ~4
    if (cache->bucket_count < 16) cache->bucket_count = 16;
    
    cache->buckets = (JITCacheEntry**)calloc(cache->bucket_count, sizeof(JITCacheEntry*));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }
    
    cache->max_entries = max_entries;
    cache->entry_count = 0;
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    
    return cache;
}

void jit_code_cache_destroy(JITCodeCache* cache) {
    if (!cache) return;
    
    // Free all entries
    for (size_t i = 0; i < cache->bucket_count; i++) {
        JITCacheEntry* entry = cache->buckets[i];
        while (entry) {
            JITCacheEntry* next = entry->next;
            if (entry->compiled_code) {
                compiled_code_destroy(entry->compiled_code);
            }
            free(entry);
            entry = next;
        }
    }
    
    free(cache->buckets);
    free(cache);
}

static size_t hash_function_id(uint32_t function_id, size_t bucket_count) {
    return function_id % bucket_count;
}

CompiledCode* jit_code_cache_get(JITCodeCache* cache, uint32_t function_id) {
    if (!cache) return NULL;
    
    size_t bucket = hash_function_id(function_id, cache->bucket_count);
    JITCacheEntry* entry = cache->buckets[bucket];
    
    while (entry) {
        if (entry->function_id == function_id) {
            cache->cache_hits++;
            entry->last_used = (uint64_t)time(NULL);
            return entry->compiled_code;
        }
        entry = entry->next;
    }
    
    cache->cache_misses++;
    return NULL;
}

bool jit_code_cache_put(JITCodeCache* cache, uint32_t function_id, CompiledCode* code) {
    if (!cache || !code) return false;
    
    // Check if already exists
    if (jit_code_cache_get(cache, function_id)) {
        return true; // Already cached
    }
    
    // Check capacity
    if (cache->entry_count >= cache->max_entries) {
        // TODO: Implement LRU eviction
        return false;
    }
    
    // Create new entry
    JITCacheEntry* entry = (JITCacheEntry*)malloc(sizeof(JITCacheEntry));
    if (!entry) return false;
    
    entry->function_id = function_id;
    entry->compiled_code = code;
    entry->last_used = (uint64_t)time(NULL);
    
    // Insert into hash table
    size_t bucket = hash_function_id(function_id, cache->bucket_count);
    entry->next = cache->buckets[bucket];
    cache->buckets[bucket] = entry;
    
    cache->entry_count++;
    return true;
}

void jit_code_cache_remove(JITCodeCache* cache, uint32_t function_id) {
    if (!cache) return;
    
    size_t bucket = hash_function_id(function_id, cache->bucket_count);
    JITCacheEntry** entry_ptr = &cache->buckets[bucket];
    
    while (*entry_ptr) {
        JITCacheEntry* entry = *entry_ptr;
        if (entry->function_id == function_id) {
            *entry_ptr = entry->next;
            if (entry->compiled_code) {
                compiled_code_destroy(entry->compiled_code);
            }
            free(entry);
            cache->entry_count--;
            return;
        }
        entry_ptr = &entry->next;
    }
}

void jit_code_cache_clear(JITCodeCache* cache) {
    if (!cache) return;
    
    for (size_t i = 0; i < cache->bucket_count; i++) {
        JITCacheEntry* entry = cache->buckets[i];
        while (entry) {
            JITCacheEntry* next = entry->next;
            if (entry->compiled_code) {
                compiled_code_destroy(entry->compiled_code);
            }
            free(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }
    
    cache->entry_count = 0;
}

// JIT Integrated VM Implementation
JITIntegratedVM* jit_vm_create(size_t heap_size) {
    JITConfig config = jit_vm_default_config();
    return jit_vm_create_with_config(heap_size, &config);
}

JITIntegratedVM* jit_vm_create_with_config(size_t heap_size, const JITConfig* config) {
    JITIntegratedVM* vm = (JITIntegratedVM*)calloc(1, sizeof(JITIntegratedVM));
    if (!vm) return NULL;
    
    // Create base enhanced VM
    vm->base_vm = enhanced_vm_create(heap_size);
    if (!vm->base_vm) {
        free(vm);
        return NULL;
    }
    
    // Copy configuration
    if (config) {
        vm->config = *config;
    } else {
        vm->config = jit_vm_default_config();
    }
    
    // Initialize JIT components if enabled
    if (vm->config.enable_jit) {
        // Create JIT compiler
        vm->jit_compiler = jit_compiler_create();
        if (!vm->jit_compiler) {
            enhanced_vm_destroy(vm->base_vm);
            free(vm);
            return NULL;
        }
        
        if (jit_compiler_init(vm->jit_compiler) != JIT_STATUS_SUCCESS) {
            jit_compiler_destroy(vm->jit_compiler);
            enhanced_vm_destroy(vm->base_vm);
            free(vm);
            return NULL;
        }
        
        // Create hot path detector if enabled
        if (vm->config.enable_hotspot_detection) {
            vm->hot_path_detector = hot_path_detector_create();
            if (vm->hot_path_detector) {
                hot_path_detector_set_thresholds(
                    vm->hot_path_detector,
                    vm->config.jit_threshold,
                    vm->config.optimization_threshold
                );
            }
        }
        
        // Create tiered compilation manager if enabled
        if (vm->config.enable_tiered_compilation && vm->jit_compiler && vm->hot_path_detector) {
            vm->tiered_compilation = tiered_compilation_create(vm->jit_compiler, vm->hot_path_detector);
            if (vm->tiered_compilation) {
                tiered_compilation_set_thresholds(
                    vm->tiered_compilation,
                    vm->config.jit_threshold,
                    vm->config.optimization_threshold
                );
            }
        }
        
        // Create code cache
        vm->code_cache = jit_code_cache_create(vm->config.max_compiled_functions);
    }
    
    // Initialize statistics
    vm->function_executions = 0;
    vm->jit_compilations = 0;
    vm->jit_optimizations = 0;
    vm->jit_execution_time_ns = 0;
    vm->interpreter_execution_time_ns = 0;
    
    return vm;
}

void jit_vm_destroy(JITIntegratedVM* vm) {
    if (!vm) return;
    
    // Destroy JIT components
    if (vm->code_cache) {
        jit_code_cache_destroy(vm->code_cache);
    }
    
    if (vm->tiered_compilation) {
        tiered_compilation_destroy(vm->tiered_compilation);
    }
    
    if (vm->hot_path_detector) {
        hot_path_detector_destroy(vm->hot_path_detector);
    }
    
    if (vm->jit_compiler) {
        jit_compiler_destroy(vm->jit_compiler);
    }
    
    // Destroy base VM
    if (vm->base_vm) {
        enhanced_vm_destroy(vm->base_vm);
    }
    
    free(vm);
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

JITVmResult jit_vm_execute(JITIntegratedVM* vm, const Chunk* chunk) {
    JITVmResult result = {0};
    
    if (!vm || !chunk) {
        result.success = false;
        return result;
    }
    
    vm->function_executions++;
    
    // For simplicity, use function_id = 0 for chunk execution
    uint32_t function_id = 0;
    
    uint64_t start_time = get_time_ns();
    
    // Check if JIT is enabled and function is compiled
    CompiledCode* compiled_code = NULL;
    if (vm->config.enable_jit && vm->code_cache) {
        compiled_code = jit_code_cache_get(vm->code_cache, function_id);
    }
    
    if (compiled_code) {
        // Execute JIT-compiled code
        int64_t jit_result = 0;
        int64_t args[] = {0}; // Mock arguments
        
        JITStatus status = jit_compiler_execute(
            vm->jit_compiler,
            compiled_code,
            vm->base_vm,
            args,
            0,
            &jit_result
        );
        
        uint64_t end_time = get_time_ns();
        result.execution_time_ns = end_time - start_time;
        vm->jit_execution_time_ns += result.execution_time_ns;
        
        result.success = (status == JIT_STATUS_SUCCESS);
        result.result_value = (double)jit_result;
        result.used_jit = true;
        result.compilation_time_ns = 0;
        
        // Record execution for hot path detection
        if (vm->hot_path_detector) {
            hot_path_detector_record_execution(
                vm->hot_path_detector,
                function_id,
                0, // basic_block_id
                result.execution_time_ns
            );
        }
    } else {
        // Execute with interpreter
        EnhancedVmResult vm_result = enhanced_vm_execute_with_result(vm->base_vm, chunk);
        
        uint64_t end_time = get_time_ns();
        result.execution_time_ns = end_time - start_time;
        vm->interpreter_execution_time_ns += result.execution_time_ns;
        
        result.success = vm_result.success;
        result.result_value = vm_result.result_value;
        result.used_jit = false;
        result.compilation_time_ns = 0;
        
        // Record execution for hot path detection
        if (vm->hot_path_detector) {
            hot_path_detector_record_execution(
                vm->hot_path_detector,
                function_id,
                0,
                result.execution_time_ns
            );
            
            // Check if function should be compiled
            if (hot_path_detector_is_hot(vm->hot_path_detector, function_id, 0) && 
                !compiled_code && vm->jit_compiler) {
                
                uint64_t compile_start = get_time_ns();
                if (jit_vm_compile_function(vm, function_id, chunk)) {
                    uint64_t compile_end = get_time_ns();
                    result.compilation_time_ns = compile_end - compile_start;
                    vm->jit_compilations++;
                }
            }
        }
    }
    
    return result;
}

bool jit_vm_compile_function(JITIntegratedVM* vm, uint32_t function_id, const Chunk* chunk) {
    if (!vm || !vm->jit_compiler || !chunk) return false;
    
    CompiledCode* compiled_code = NULL;
    JITStatus status = jit_compiler_compile_function(
        vm->jit_compiler,
        (const Chunk*)chunk, // Cast for compatibility
        function_id,
        JIT_TIER_BASELINE,
        &compiled_code
    );
    
    if (status == JIT_STATUS_SUCCESS && compiled_code) {
        if (vm->code_cache) {
            return jit_code_cache_put(vm->code_cache, function_id, compiled_code);
        }
        return true;
    }
    
    return false;
}

bool jit_vm_optimize_function(JITIntegratedVM* vm, uint32_t function_id, JITTier target_tier) {
    if (!vm || !vm->jit_compiler || !vm->code_cache) return false;
    
    CompiledCode* compiled_code = jit_code_cache_get(vm->code_cache, function_id);
    if (!compiled_code) return false;
    
    JITStatus status = jit_compiler_optimize(vm->jit_compiler, compiled_code, target_tier);
    if (status == JIT_STATUS_SUCCESS) {
        vm->jit_optimizations++;
        return true;
    }
    
    return false;
}

void jit_vm_invalidate_function(JITIntegratedVM* vm, uint32_t function_id) {
    if (!vm || !vm->code_cache) return;
    jit_code_cache_remove(vm->code_cache, function_id);
}

void jit_vm_clear_cache(JITIntegratedVM* vm) {
    if (!vm || !vm->code_cache) return;
    jit_code_cache_clear(vm->code_cache);
}

// Configuration
void jit_vm_set_config(JITIntegratedVM* vm, const JITConfig* config) {
    if (!vm || !config) return;
    vm->config = *config;
    
    // Update component configurations
    if (vm->hot_path_detector) {
        hot_path_detector_set_thresholds(
            vm->hot_path_detector,
            config->jit_threshold,
            config->optimization_threshold
        );
    }
    
    if (vm->tiered_compilation) {
        tiered_compilation_set_thresholds(
            vm->tiered_compilation,
            config->jit_threshold,
            config->optimization_threshold
        );
    }
}

void jit_vm_get_config(const JITIntegratedVM* vm, JITConfig* config) {
    if (!vm || !config) return;
    *config = vm->config;
}

// Statistics
void jit_vm_get_stats(const JITIntegratedVM* vm, JITVmStats* stats) {
    if (!vm || !stats) return;
    
    memset(stats, 0, sizeof(JITVmStats));
    
    stats->total_executions = vm->function_executions;
    stats->jit_compilations = vm->jit_compilations;
    stats->jit_optimizations = vm->jit_optimizations;
    stats->jit_execution_time_ns = vm->jit_execution_time_ns;
    stats->interpreter_execution_time_ns = vm->interpreter_execution_time_ns;
    
    // Get base VM stats
    uint64_t total_exec, jit_exec, interp_exec;
    enhanced_vm_get_stats(vm->base_vm, &total_exec, &jit_exec, &interp_exec);
    
    stats->jit_executions = jit_exec;
    stats->interpreter_executions = interp_exec;
    
    // Get cache stats
    if (vm->code_cache) {
        stats->cache_hits = vm->code_cache->cache_hits;
        stats->cache_misses = vm->code_cache->cache_misses;
    }
    
    // Calculate speedup ratio
    if (stats->interpreter_execution_time_ns > 0 && stats->jit_execution_time_ns > 0) {
        stats->jit_speedup_ratio = (double)stats->interpreter_execution_time_ns / 
                                  (double)stats->jit_execution_time_ns;
    } else {
        stats->jit_speedup_ratio = 1.0;
    }
    
    // Get compilation time from JIT compiler
    if (vm->jit_compiler) {
        uint64_t functions_compiled, compilation_time, optimization_time;
        jit_compiler_get_stats(vm->jit_compiler, &functions_compiled, &compilation_time, &optimization_time);
        stats->compilation_time_ns = compilation_time + optimization_time;
    }
}

void jit_vm_reset_stats(JITIntegratedVM* vm) {
    if (!vm) return;
    
    vm->function_executions = 0;
    vm->jit_compilations = 0;
    vm->jit_optimizations = 0;
    vm->jit_execution_time_ns = 0;
    vm->interpreter_execution_time_ns = 0;
    
    if (vm->jit_compiler) {
        jit_compiler_reset_stats(vm->jit_compiler);
    }
    
    if (vm->hot_path_detector) {
        hot_path_detector_reset_stats(vm->hot_path_detector);
    }
    
    if (vm->tiered_compilation) {
        tiered_compilation_reset_stats(vm->tiered_compilation);
    }
    
    if (vm->code_cache) {
        vm->code_cache->cache_hits = 0;
        vm->code_cache->cache_misses = 0;
    }
}

void jit_vm_print_stats(const JITIntegratedVM* vm) {
    if (!vm) return;
    
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    
    printf("\n=== JIT VM Statistics ===\n");
    printf("Total Executions: %lu\n", stats.total_executions);
    printf("JIT Executions: %lu\n", stats.jit_executions);
    printf("Interpreter Executions: %lu\n", stats.interpreter_executions);
    printf("JIT Compilations: %lu\n", stats.jit_compilations);
    printf("JIT Optimizations: %lu\n", stats.jit_optimizations);
    printf("Cache Hits: %lu\n", stats.cache_hits);
    printf("Cache Misses: %lu\n", stats.cache_misses);
    printf("JIT Execution Time: %.2f ms\n", stats.jit_execution_time_ns / 1000000.0);
    printf("Interpreter Execution Time: %.2f ms\n", stats.interpreter_execution_time_ns / 1000000.0);
    printf("Compilation Time: %.2f ms\n", stats.compilation_time_ns / 1000000.0);
    printf("JIT Speedup Ratio: %.2fx\n", stats.jit_speedup_ratio);
    printf("========================\n\n");
}

// Stub implementation for function execution
JITVmResult jit_vm_execute_function(JITIntegratedVM* vm, uint32_t function_id) {
    JITVmResult result = {0};
    
    if (!vm) {
        result.success = false;
        return result;
    }
    
    // TODO: Implement function-specific execution
    // For now, return a mock result
    result.success = true;
    result.result_value = (double)function_id;
    result.used_jit = false;
    result.execution_time_ns = 1000;
    result.compilation_time_ns = 0;
    
    return result;
}
