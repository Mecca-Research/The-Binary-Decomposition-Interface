
#include "vm_graph_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

// Helper macros
#define GRAPH_CACHE_BUCKET_COUNT 64
#define GRAPH_CACHE_DEFAULT_SIZE 256
#define GRAPH_VM_DEFAULT_MEMORY_LIMIT (16 * 1024 * 1024) // 16MB

// Time measurement helper
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Hash function for cache
static uint32_t hash_function_id(uint32_t function_id) {
    // Simple hash function
    function_id ^= function_id >> 16;
    function_id *= 0x85ebca6b;
    function_id ^= function_id >> 13;
    function_id *= 0xc2b2ae35;
    function_id ^= function_id >> 16;
    return function_id % GRAPH_CACHE_BUCKET_COUNT;
}

// Graph cache implementation
GraphCache* graph_cache_create(size_t max_entries) {
    GraphCache* cache = (GraphCache*)calloc(1, sizeof(GraphCache));
    if (!cache) return NULL;
    
    cache->bucket_count = GRAPH_CACHE_BUCKET_COUNT;
    cache->max_entries = max_entries > 0 ? max_entries : GRAPH_CACHE_DEFAULT_SIZE;
    cache->buckets = (GraphCacheEntry**)calloc(cache->bucket_count, sizeof(GraphCacheEntry*));
    
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }
    
    return cache;
}

void graph_cache_destroy(GraphCache* cache) {
    if (!cache) return;
    
    // Free all entries
    for (size_t i = 0; i < cache->bucket_count; i++) {
        GraphCacheEntry* entry = cache->buckets[i];
        while (entry) {
            GraphCacheEntry* next = entry->next;
            graph_destroy(entry->graph);
            free(entry);
            entry = next;
        }
    }
    
    free(cache->buckets);
    free(cache);
}

Graph* graph_cache_get(GraphCache* cache, uint32_t function_id) {
    if (!cache) return NULL;
    
    uint32_t bucket = hash_function_id(function_id);
    GraphCacheEntry* entry = cache->buckets[bucket];
    
    while (entry) {
        if (entry->function_id == function_id) {
            entry->last_used = get_time_ns();
            cache->cache_hits++;
            return entry->graph;
        }
        entry = entry->next;
    }
    
    cache->cache_misses++;
    return NULL;
}

bool graph_cache_put(GraphCache* cache, uint32_t function_id, Graph* graph) {
    if (!cache || !graph) return false;
    
    // Check if entry already exists
    if (graph_cache_get(cache, function_id)) {
        return true; // Already cached
    }
    
    // Check if we need to evict
    if (cache->entry_count >= cache->max_entries) {
        graph_cache_evict_lru(cache);
    }
    
    // Create new entry
    GraphCacheEntry* entry = (GraphCacheEntry*)calloc(1, sizeof(GraphCacheEntry));
    if (!entry) return false;
    
    entry->function_id = function_id;
    entry->graph = graph;
    entry->creation_time = get_time_ns();
    entry->last_used = entry->creation_time;
    
    // Add to bucket
    uint32_t bucket = hash_function_id(function_id);
    entry->next = cache->buckets[bucket];
    cache->buckets[bucket] = entry;
    cache->entry_count++;
    
    return true;
}

void graph_cache_remove(GraphCache* cache, uint32_t function_id) {
    if (!cache) return;
    
    uint32_t bucket = hash_function_id(function_id);
    GraphCacheEntry** entry_ptr = &cache->buckets[bucket];
    
    while (*entry_ptr) {
        GraphCacheEntry* entry = *entry_ptr;
        if (entry->function_id == function_id) {
            *entry_ptr = entry->next;
            graph_destroy(entry->graph);
            free(entry);
            cache->entry_count--;
            return;
        }
        entry_ptr = &entry->next;
    }
}

void graph_cache_clear(GraphCache* cache) {
    if (!cache) return;
    
    for (size_t i = 0; i < cache->bucket_count; i++) {
        GraphCacheEntry* entry = cache->buckets[i];
        while (entry) {
            GraphCacheEntry* next = entry->next;
            graph_destroy(entry->graph);
            free(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }
    
    cache->entry_count = 0;
}

void graph_cache_evict_lru(GraphCache* cache) {
    if (!cache || cache->entry_count == 0) return;
    
    // Find LRU entry
    GraphCacheEntry* lru_entry = NULL;
    GraphCacheEntry** lru_entry_ptr = NULL;
    uint64_t oldest_time = UINT64_MAX;
    
    for (size_t i = 0; i < cache->bucket_count; i++) {
        GraphCacheEntry** entry_ptr = &cache->buckets[i];
        while (*entry_ptr) {
            GraphCacheEntry* entry = *entry_ptr;
            if (entry->last_used < oldest_time) {
                oldest_time = entry->last_used;
                lru_entry = entry;
                lru_entry_ptr = entry_ptr;
            }
            entry_ptr = &entry->next;
        }
    }
    
    // Remove LRU entry
    if (lru_entry && lru_entry_ptr) {
        *lru_entry_ptr = lru_entry->next;
        graph_destroy(lru_entry->graph);
        free(lru_entry);
        cache->entry_count--;
        cache->evictions++;
    }
}

// Core Graph-VM implementation
GraphIntegratedVM* graph_vm_create(size_t heap_size) {
    GraphVMConfig config = graph_vm_default_config();
    return graph_vm_create_with_config(heap_size, &config);
}

GraphIntegratedVM* graph_vm_create_with_config(size_t heap_size, const GraphVMConfig* config) {
    GraphIntegratedVM* vm = (GraphIntegratedVM*)calloc(1, sizeof(GraphIntegratedVM));
    if (!vm) return NULL;
    
    // Create base JIT-integrated VM
    if (config && config->enable_graph_jit) {
        JITConfig jit_config = jit_vm_default_config();
        jit_config.enable_jit = true;
        jit_config.jit_threshold = config->graph_jit_threshold;
        vm->base_vm = jit_vm_create_with_config(heap_size, &jit_config);
    } else {
        vm->base_vm = jit_vm_create(heap_size);
    }
    
    if (!vm->base_vm) {
        free(vm);
        return NULL;
    }
    
    // Set configuration
    if (config) {
        vm->config = *config;
    } else {
        vm->config = graph_vm_default_config();
    }
    
    // Create graph components
    vm->graph_executor = graph_executor_create_with_config(&vm->config.executor_config);
    vm->graph_optimizer = graph_optimizer_create_with_config(&vm->config.optimizer_config);
    vm->graph_cache = graph_cache_create(vm->config.graph_cache_size);
    
    if (!vm->graph_executor || !vm->graph_optimizer || !vm->graph_cache) {
        graph_vm_destroy(vm);
        return NULL;
    }
    
    // Initialize memory management
    vm->graph_memory_limit = GRAPH_VM_DEFAULT_MEMORY_LIMIT;
    
    return vm;
}

void graph_vm_destroy(GraphIntegratedVM* vm) {
    if (!vm) return;
    
    jit_vm_destroy(vm->base_vm);
    graph_executor_destroy(vm->graph_executor);
    graph_optimizer_destroy(vm->graph_optimizer);
    graph_cache_destroy(vm->graph_cache);
    free(vm);
}

GraphVMResult graph_vm_execute(GraphIntegratedVM* vm, const Chunk* chunk) {
    GraphVMResult result = {0};
    
    if (!vm || !chunk) {
        result.error_message = strdup("Invalid VM or chunk");
        return result;
    }
    
    uint64_t start_time = get_time_ns();
    
    // Decide execution strategy based on configuration
    switch (vm->config.mode) {
        case GRAPH_VM_MODE_GRAPH_ONLY: {
            // Convert to graph and execute
            Graph* graph = graph_vm_convert_bytecode_to_graph(vm, chunk);
            if (graph) {
                result = graph_vm_execute_graph(vm, graph, NULL, 0);
                result.used_graph_execution = true;
                graph_destroy(graph);
            } else {
                result.error_message = strdup("Failed to convert bytecode to graph");
            }
            break;
        }
        
        case GRAPH_VM_MODE_BYTECODE_ONLY: {
            // Execute using base JIT VM
            JITVmResult jit_result = jit_vm_execute(vm->base_vm, chunk);
            result.success = jit_result.success;
            result.used_jit_compilation = jit_result.used_jit;
            result.execution_time_ns = jit_result.execution_time_ns;
            result.compilation_time_ns = jit_result.compilation_time_ns;
            
            if (result.success) {
                result.output_count = 1;
                result.output_values = (GraphValue*)malloc(sizeof(GraphValue));
                if (result.output_values) {
                    result.output_values[0].type = GRAPH_TYPE_F64;
                    result.output_values[0].data.f64 = jit_result.result_value;
                }
            } else {
                result.error_message = strdup("Bytecode execution failed");
            }
            break;
        }
        
        case GRAPH_VM_MODE_HYBRID:
        case GRAPH_VM_MODE_ADAPTIVE:
        default: {
            // Try graph execution first, fall back to bytecode
            Graph* graph = graph_vm_convert_bytecode_to_graph(vm, chunk);
            if (graph) {
                result = graph_vm_execute_graph(vm, graph, NULL, 0);
                result.used_graph_execution = true;
                
                if (!result.success) {
                    // Fall back to bytecode execution
                    JITVmResult jit_result = jit_vm_execute(vm->base_vm, chunk);
                    result.success = jit_result.success;
                    result.used_jit_compilation = jit_result.used_jit;
                    result.used_graph_execution = false;
                    
                    if (result.success && !result.output_values) {
                        result.output_count = 1;
                        result.output_values = (GraphValue*)malloc(sizeof(GraphValue));
                        if (result.output_values) {
                            result.output_values[0].type = GRAPH_TYPE_F64;
                            result.output_values[0].data.f64 = jit_result.result_value;
                        }
                    }
                }
                
                graph_destroy(graph);
            } else {
                // Execute using base JIT VM
                JITVmResult jit_result = jit_vm_execute(vm->base_vm, chunk);
                result.success = jit_result.success;
                result.used_jit_compilation = jit_result.used_jit;
                result.execution_time_ns = jit_result.execution_time_ns;
                result.compilation_time_ns = jit_result.compilation_time_ns;
                
                if (result.success) {
                    result.output_count = 1;
                    result.output_values = (GraphValue*)malloc(sizeof(GraphValue));
                    if (result.output_values) {
                        result.output_values[0].type = GRAPH_TYPE_F64;
                        result.output_values[0].data.f64 = jit_result.result_value;
                    }
                }
            }
            break;
        }
    }
    
    // Update statistics
    vm->total_functions_executed++;
    if (result.used_graph_execution) {
        vm->graph_executions++;
        vm->graph_execution_time_ns += result.execution_time_ns;
    } else {
        vm->bytecode_executions++;
        vm->bytecode_execution_time_ns += result.execution_time_ns;
    }
    
    if (!result.execution_time_ns) {
        result.execution_time_ns = get_time_ns() - start_time;
    }
    
    return result;
}

GraphVMResult graph_vm_execute_function(GraphIntegratedVM* vm, uint32_t function_id) {
    GraphVMResult result = {0};
    
    if (!vm) {
        result.error_message = strdup("Invalid VM");
        return result;
    }
    
    uint64_t start_time = get_time_ns();
    
    // Check if we have a cached graph for this function
    Graph* cached_graph = graph_cache_get(vm->graph_cache, function_id);
    
    if (cached_graph) {
        // Execute cached graph
        result = graph_vm_execute_graph(vm, cached_graph, NULL, 0);
        result.used_graph_execution = true;
    } else {
        // Execute using base JIT VM
        JITVmResult jit_result = jit_vm_execute_function(vm->base_vm, function_id);
        result.success = jit_result.success;
        result.used_jit_compilation = jit_result.used_jit;
        result.execution_time_ns = jit_result.execution_time_ns;
        result.compilation_time_ns = jit_result.compilation_time_ns;
        
        if (result.success) {
            result.output_count = 1;
            result.output_values = (GraphValue*)malloc(sizeof(GraphValue));
            if (result.output_values) {
                result.output_values[0].type = GRAPH_TYPE_F64;
                result.output_values[0].data.f64 = jit_result.result_value;
            }
        }
    }
    
    // Update statistics
    vm->total_functions_executed++;
    if (result.used_graph_execution) {
        vm->graph_executions++;
        vm->graph_execution_time_ns += result.execution_time_ns;
    } else {
        vm->bytecode_executions++;
        vm->bytecode_execution_time_ns += result.execution_time_ns;
    }
    
    if (!result.execution_time_ns) {
        result.execution_time_ns = get_time_ns() - start_time;
    }
    
    return result;
}

GraphVMResult graph_vm_execute_graph(GraphIntegratedVM* vm, Graph* graph, 
                                    GraphValue* inputs, uint32_t input_count) {
    GraphVMResult result = {0};
    
    if (!vm || !graph) {
        result.error_message = strdup("Invalid VM or graph");
        return result;
    }
    
    uint64_t start_time = get_time_ns();
    
    // Optimize graph if enabled
    if (vm->config.enable_graph_optimization && !graph->is_optimized) {
        uint64_t opt_start = get_time_ns();
        graph_optimizer_optimize(vm->graph_optimizer, graph);
        vm->graph_optimization_time_ns += get_time_ns() - opt_start;
        vm->graph_optimizations++;
    }
    
    // Execute graph
    GraphExecutionResult exec_result = graph_executor_execute(vm->graph_executor, graph, 
                                                            inputs, input_count);
    
    result.success = exec_result.success;
    result.output_values = exec_result.output_values;
    result.output_count = exec_result.output_count;
    result.execution_time_ns = exec_result.execution_time_ns;
    result.nodes_executed = exec_result.nodes_executed;
    result.used_graph_execution = true;
    
    if (!result.success && exec_result.error_message) {
        result.error_message = strdup(exec_result.error_message);
    }
    
    // Don't free exec_result.output_values as we're transferring ownership
    free(exec_result.error_message);
    
    if (!result.execution_time_ns) {
        result.execution_time_ns = get_time_ns() - start_time;
    }
    
    return result;
}

// Bytecode to graph conversion (simplified implementation)
Graph* graph_vm_convert_bytecode_to_graph(GraphIntegratedVM* vm, const Chunk* chunk) {
    if (!vm || !chunk) return NULL;
    
    // Create graph builder
    GraphBuilder* builder = graph_builder_create("bytecode_graph");
    if (!builder) return NULL;
    
    // Simple conversion: create a linear chain of operations
    // This is a very basic implementation - a full implementation would
    // parse the bytecode and create appropriate graph nodes
    
    uint32_t input_node = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t current_node = input_node;
    
    // Process bytecode instructions (simplified)
    for (int i = 0; i < chunk->count; i++) {
        uint8_t instruction = chunk->code[i];
        
        switch (instruction) {
            case OP_CONSTANT: {
                if (i + 1 < chunk->count) {
                    uint8_t constant_index = chunk->code[++i];
                    if (constant_index < chunk->constants.count) {
                        double value = chunk->constants.values[constant_index];
                        GraphValue const_val = {0};
                        const_val.type = GRAPH_TYPE_F64;
                        const_val.data.f64 = value;
                        current_node = graph_builder_add_constant(builder, const_val, "constant");
                    }
                }
                break;
            }
            
            case OP_ADD: {
                // For simplicity, add with previous node
                uint32_t add_node = graph_builder_add(builder, current_node, current_node, "add");
                if (add_node) current_node = add_node;
                break;
            }
            
            case OP_SUBTRACT: {
                uint32_t sub_node = graph_builder_sub(builder, current_node, current_node, "sub");
                if (sub_node) current_node = sub_node;
                break;
            }
            
            case OP_MULTIPLY: {
                uint32_t mul_node = graph_builder_mul(builder, current_node, current_node, "mul");
                if (mul_node) current_node = mul_node;
                break;
            }
            
            case OP_DIVIDE: {
                uint32_t div_node = graph_builder_div(builder, current_node, current_node, "div");
                if (div_node) current_node = div_node;
                break;
            }
            
            default:
                // Skip unknown instructions
                break;
        }
    }
    
    // Add output node
    uint32_t output_node = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    if (output_node && current_node) {
        graph_builder_connect(builder, current_node, 0, output_node, 0);
    }
    
    // Build the graph
    Graph* graph = graph_builder_build(builder);
    graph_builder_destroy(builder);
    
    return graph;
}

bool graph_vm_convert_function_to_graph(GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return false;
    
    // Check if already cached
    if (graph_cache_get(vm->graph_cache, function_id)) {
        return true;
    }
    
    // For now, create a simple placeholder graph
    Graph* graph = graph_create("function_graph");
    if (!graph) return false;
    
    // Add basic input/output nodes
    GraphNode* input = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    GraphNode* output = graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    
    if (input && output) {
        graph_add_edge(graph, input->id, 0, output->id, 0);
    }
    
    // Cache the graph
    bool success = graph_cache_put(vm->graph_cache, function_id, graph);
    if (success) {
        vm->functions_converted_to_graph++;
    } else {
        graph_destroy(graph);
    }
    
    return success;
}

// Configuration
GraphVMConfig graph_vm_default_config(void) {
    GraphVMConfig config = {
        .mode = GRAPH_VM_MODE_HYBRID,
        .executor_config = graph_executor_default_config(),
        .optimizer_config = graph_optimizer_default_config(),
        .enable_graph_compilation = true,
        .enable_graph_optimization = true,
        .enable_hybrid_execution = true,
        .graph_compilation_threshold = 10,
        .bytecode_to_graph_threshold = 5,
        .graph_cache_size = GRAPH_CACHE_DEFAULT_SIZE,
        .max_concurrent_graphs = 4,
        .enable_graph_profiling = false,
        .enable_graph_jit = false,
        .graph_jit_threshold = 100
    };
    return config;
}

void graph_vm_set_config(GraphIntegratedVM* vm, const GraphVMConfig* config) {
    if (vm && config) {
        vm->config = *config;
        
        // Update component configurations
        if (vm->graph_executor) {
            graph_executor_set_config(vm->graph_executor, &config->executor_config);
        }
        if (vm->graph_optimizer) {
            graph_optimizer_set_config(vm->graph_optimizer, &config->optimizer_config);
        }
    }
}

void graph_vm_get_config(const GraphIntegratedVM* vm, GraphVMConfig* config) {
    if (vm && config) {
        *config = vm->config;
    }
}

// Statistics
void graph_vm_get_stats(const GraphIntegratedVM* vm, GraphVMStats* stats) {
    if (!vm || !stats) return;
    
    memset(stats, 0, sizeof(GraphVMStats));
    
    stats->total_executions = vm->total_functions_executed;
    stats->graph_executions = vm->graph_executions;
    stats->bytecode_executions = vm->bytecode_executions;
    stats->hybrid_executions = vm->hybrid_executions;
    stats->graph_compilations = vm->graph_compilations;
    stats->graph_optimizations = vm->graph_optimizations;
    stats->graph_execution_time_ns = vm->graph_execution_time_ns;
    stats->bytecode_execution_time_ns = vm->bytecode_execution_time_ns;
    stats->graph_memory_used = vm->graph_memory_used;
    
    if (vm->graph_cache) {
        stats->cache_hits = vm->graph_cache->cache_hits;
        stats->cache_misses = vm->graph_cache->cache_misses;
        stats->cache_evictions = vm->graph_cache->evictions;
        stats->active_graphs = vm->graph_cache->entry_count;
    }
    
    // Calculate speedup ratio
    if (vm->bytecode_execution_time_ns > 0 && vm->graph_execution_time_ns > 0) {
        stats->graph_speedup_ratio = 
            (double)vm->bytecode_execution_time_ns / vm->graph_execution_time_ns;
    }
}

void graph_vm_reset_stats(GraphIntegratedVM* vm) {
    if (!vm) return;
    
    vm->graph_executions = 0;
    vm->bytecode_executions = 0;
    vm->hybrid_executions = 0;
    vm->graph_compilations = 0;
    vm->graph_optimizations = 0;
    vm->graph_execution_time_ns = 0;
    vm->bytecode_execution_time_ns = 0;
    vm->total_functions_executed = 0;
    vm->functions_converted_to_graph = 0;
    vm->graph_speedup_ratio = 0.0;
    
    if (vm->graph_cache) {
        vm->graph_cache->cache_hits = 0;
        vm->graph_cache->cache_misses = 0;
        vm->graph_cache->evictions = 0;
    }
    
    if (vm->graph_executor) {
        graph_executor_reset_stats(vm->graph_executor);
    }
    
    if (vm->graph_optimizer) {
        graph_optimizer_reset_stats(vm->graph_optimizer);
    }
}

void graph_vm_print_stats(const GraphIntegratedVM* vm) {
    if (!vm) return;
    
    GraphVMStats stats;
    graph_vm_get_stats(vm, &stats);
    
    printf("Graph-Integrated VM Statistics:\n");
    printf("  Total Executions: %lu\n", stats.total_executions);
    printf("  Graph Executions: %lu (%.1f%%)\n", stats.graph_executions,
           stats.total_executions > 0 ? 
           100.0 * stats.graph_executions / stats.total_executions : 0.0);
    printf("  Bytecode Executions: %lu (%.1f%%)\n", stats.bytecode_executions,
           stats.total_executions > 0 ? 
           100.0 * stats.bytecode_executions / stats.total_executions : 0.0);
    printf("  Graph Compilations: %lu\n", stats.graph_compilations);
    printf("  Graph Optimizations: %lu\n", stats.graph_optimizations);
    printf("  Graph Execution Time: %.3f ms\n", 
           (double)stats.graph_execution_time_ns / 1000000.0);
    printf("  Bytecode Execution Time: %.3f ms\n", 
           (double)stats.bytecode_execution_time_ns / 1000000.0);
    printf("  Graph Speedup Ratio: %.2fx\n", stats.graph_speedup_ratio);
    printf("  Cache Hits: %lu\n", stats.cache_hits);
    printf("  Cache Misses: %lu\n", stats.cache_misses);
    printf("  Cache Hit Rate: %.1f%%\n", 
           (stats.cache_hits + stats.cache_misses) > 0 ?
           100.0 * stats.cache_hits / (stats.cache_hits + stats.cache_misses) : 0.0);
    printf("  Active Graphs: %u\n", stats.active_graphs);
    printf("  Graph Memory Used: %zu bytes\n", stats.graph_memory_used);
}

// Adaptive execution helpers
bool graph_vm_should_use_graph_execution(GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return false;
    
    // Simple heuristic: use graph execution if we have a cached graph
    return graph_cache_get(vm->graph_cache, function_id) != NULL;
}

bool graph_vm_should_compile_to_graph(GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return false;
    
    // Simple heuristic: compile if function has been executed enough times
    // In a real implementation, this would track per-function execution counts
    return vm->total_functions_executed > vm->config.bytecode_to_graph_threshold;
}

void graph_vm_update_execution_profile(GraphIntegratedVM* vm, uint32_t function_id,
                                      uint64_t execution_time_ns, bool used_graph) {
    if (!vm) return;
    
    // Update global statistics
    if (used_graph) {
        vm->graph_execution_time_ns += execution_time_ns;
    } else {
        vm->bytecode_execution_time_ns += execution_time_ns;
    }
    
    // Update speedup ratio
    if (vm->bytecode_execution_time_ns > 0 && vm->graph_execution_time_ns > 0) {
        vm->graph_speedup_ratio = 
            (double)vm->bytecode_execution_time_ns / vm->graph_execution_time_ns;
    }
}

// Error handling
const char* graph_vm_error_to_string(const GraphVMResult* result) {
    if (!result) return "Invalid result";
    
    if (result->success) return "Success";
    
    return result->error_message ? result->error_message : "Unknown error";
}

void graph_vm_clear_error(GraphIntegratedVM* vm) {
    // No persistent error state to clear in current implementation
    (void)vm;
}

// Memory management
void* graph_vm_alloc_graph_memory(GraphIntegratedVM* vm, size_t size) {
    if (!vm) return NULL;
    
    if (vm->graph_memory_used + size > vm->graph_memory_limit) {
        return NULL; // Out of memory
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        vm->graph_memory_used += size;
    }
    
    return ptr;
}

void graph_vm_free_graph_memory(GraphIntegratedVM* vm, void* ptr) {
    if (!vm || !ptr) return;
    
    // In a real implementation, we'd track allocation sizes
    free(ptr);
}

bool graph_vm_check_graph_memory_limit(const GraphIntegratedVM* vm, size_t additional) {
    if (!vm) return false;
    
    return vm->graph_memory_used + additional <= vm->graph_memory_limit;
}

// Utility functions
bool graph_vm_validate_integration(const GraphIntegratedVM* vm) {
    if (!vm) return false;
    
    return vm->base_vm && vm->graph_executor && 
           vm->graph_optimizer && vm->graph_cache;
}

uint32_t graph_vm_estimate_graph_benefit(const GraphIntegratedVM* vm, const Chunk* chunk) {
    if (!vm || !chunk) return 0;
    
    // Simple heuristic: benefit increases with chunk size
    return chunk->count * 10;
}

bool graph_vm_can_convert_to_graph(const Chunk* chunk) {
    if (!chunk) return false;
    
    // Simple check: can convert if chunk has basic arithmetic operations
    for (int i = 0; i < chunk->count; i++) {
        uint8_t instruction = chunk->code[i];
        switch (instruction) {
            case OP_CONSTANT:
            case OP_ADD:
            case OP_SUBTRACT:
            case OP_MULTIPLY:
            case OP_DIVIDE:
                return true; // Has convertible operations
            default:
                break;
        }
    }
    
    return false;
}

// Stub implementations for remaining functions
Graph* graph_vm_get_function_graph(GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return NULL;
    return graph_cache_get(vm->graph_cache, function_id);
}

bool graph_vm_compile_function_to_graph(GraphIntegratedVM* vm, uint32_t function_id, 
                                       const Chunk* chunk) {
    if (!vm || !chunk) return false;
    
    Graph* graph = graph_vm_convert_bytecode_to_graph(vm, chunk);
    if (!graph) return false;
    
    bool success = graph_cache_put(vm->graph_cache, function_id, graph);
    if (success) {
        vm->graph_compilations++;
    } else {
        graph_destroy(graph);
    }
    
    return success;
}

bool graph_vm_optimize_graph(GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return false;
    
    Graph* graph = graph_cache_get(vm->graph_cache, function_id);
    if (!graph) return false;
    
    return graph_optimizer_optimize(vm->graph_optimizer, graph);
}

GraphBuilder* graph_vm_create_builder(GraphIntegratedVM* vm, const char* name) {
    if (!vm) return NULL;
    return graph_builder_create(name);
}

bool graph_vm_register_graph(GraphIntegratedVM* vm, uint32_t function_id, Graph* graph) {
    if (!vm || !graph) return false;
    return graph_cache_put(vm->graph_cache, function_id, graph);
}

// Debugging stubs
void graph_vm_dump_function_graph(const GraphIntegratedVM* vm, uint32_t function_id) {
    if (!vm) return;
    
    Graph* graph = graph_cache_get(vm->graph_cache, function_id);
    if (graph) {
        char* dot = graph_to_dot(graph);
        if (dot) {
            printf("Graph for function %u:\n%s\n", function_id, dot);
            free(dot);
        }
    } else {
        printf("No graph found for function %u\n", function_id);
    }
}

void graph_vm_dump_cache_state(const GraphIntegratedVM* vm) {
    if (!vm || !vm->graph_cache) return;
    
    printf("Graph Cache State:\n");
    printf("  Entries: %zu/%zu\n", vm->graph_cache->entry_count, vm->graph_cache->max_entries);
    printf("  Hits: %lu\n", vm->graph_cache->cache_hits);
    printf("  Misses: %lu\n", vm->graph_cache->cache_misses);
    printf("  Evictions: %lu\n", vm->graph_cache->evictions);
}

void graph_vm_trace_execution(GraphIntegratedVM* vm, bool enable) {
    // Stub implementation
    (void)vm;
    (void)enable;
}

bool graph_vm_is_tracing(const GraphIntegratedVM* vm) {
    // Stub implementation
    (void)vm;
    return false;
}

// Advanced features stubs
GraphVMHotspotAnalysis* graph_vm_analyze_hotspots(GraphIntegratedVM* vm) {
    if (!vm) return NULL;
    
    GraphVMHotspotAnalysis* analysis = (GraphVMHotspotAnalysis*)calloc(1, sizeof(GraphVMHotspotAnalysis));
    if (!analysis) return NULL;
    
    analysis->hot_function_count = 0;
    analysis->analysis_time_ns = get_time_ns();
    
    return analysis;
}

void graph_vm_hotspot_analysis_destroy(GraphVMHotspotAnalysis* analysis) {
    if (!analysis) return;
    
    free(analysis->hot_functions);
    free(analysis);
}

bool graph_vm_optimize_hotspots(GraphIntegratedVM* vm, const GraphVMHotspotAnalysis* analysis) {
    // Stub implementation
    (void)vm;
    (void)analysis;
    return false;
}

bool graph_vm_enable_parallel_execution(GraphIntegratedVM* vm, uint32_t thread_count) {
    // Stub implementation
    (void)vm;
    (void)thread_count;
    return false;
}

void graph_vm_disable_parallel_execution(GraphIntegratedVM* vm) {
    // Stub implementation
    (void)vm;
}

bool graph_vm_save_graph_cache(const GraphIntegratedVM* vm, const char* filename) {
    // Stub implementation
    (void)vm;
    (void)filename;
    return false;
}

bool graph_vm_load_graph_cache(GraphIntegratedVM* vm, const char* filename) {
    // Stub implementation
    (void)vm;
    (void)filename;
    return false;
}

GraphVMFunctionProfile* graph_vm_get_function_profiles(const GraphIntegratedVM* vm, 
                                                      uint32_t* profile_count) {
    if (!vm || !profile_count) return NULL;
    
    *profile_count = 0;
    return NULL;
}

void graph_vm_function_profiles_destroy(GraphVMFunctionProfile* profiles) {
    free(profiles);
}
