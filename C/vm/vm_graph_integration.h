
#ifndef VM_GRAPH_INTEGRATION_H
#define VM_GRAPH_INTEGRATION_H

#include "vm_jit_integration.h"
#include "graph/graph.h"
#include "graph/graph_builder.h"
#include "graph/graph_executor.h"
#include "graph/graph_optimizer.h"

// Forward declarations
typedef struct GraphIntegratedVM GraphIntegratedVM;
typedef struct GraphVMConfig GraphVMConfig;

// Graph-VM integration modes
typedef enum {
    GRAPH_VM_MODE_HYBRID,          // Mix bytecode and graph execution
    GRAPH_VM_MODE_GRAPH_ONLY,      // Graph execution only
    GRAPH_VM_MODE_BYTECODE_ONLY,   // Traditional bytecode execution
    GRAPH_VM_MODE_ADAPTIVE         // Dynamically choose best mode
} GraphVMMode;

// Graph-VM configuration
typedef struct GraphVMConfig {
    GraphVMMode mode;
    
    // Graph execution settings
    GraphExecutorConfig executor_config;
    GraphOptimizerConfig optimizer_config;
    
    // Integration settings
    bool enable_graph_compilation;
    bool enable_graph_optimization;
    bool enable_hybrid_execution;
    uint64_t graph_compilation_threshold;
    uint64_t bytecode_to_graph_threshold;
    
    // Performance settings
    size_t graph_cache_size;
    uint32_t max_concurrent_graphs;
    bool enable_graph_profiling;
    
    // JIT integration
    bool enable_graph_jit;
    uint64_t graph_jit_threshold;
} GraphVMConfig;

// Graph cache entry
typedef struct GraphCacheEntry {
    uint32_t function_id;
    Graph* graph;
    bool is_optimized;
    bool is_compiled;
    uint64_t creation_time;
    uint64_t last_used;
    uint64_t execution_count;
    uint64_t total_execution_time_ns;
    struct GraphCacheEntry* next;
} GraphCacheEntry;

// Graph cache
typedef struct {
    GraphCacheEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
    size_t max_entries;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t evictions;
} GraphCache;

// Graph-integrated VM
struct GraphIntegratedVM {
    JITIntegratedVM* base_vm;
    
    // Graph components
    GraphExecutor* graph_executor;
    GraphOptimizer* graph_optimizer;
    GraphCache* graph_cache;
    
    // Configuration
    GraphVMConfig config;
    
    // Execution statistics
    uint64_t graph_executions;
    uint64_t bytecode_executions;
    uint64_t hybrid_executions;
    uint64_t graph_compilations;
    uint64_t graph_optimizations;
    uint64_t graph_execution_time_ns;
    uint64_t bytecode_execution_time_ns;
    
    // Performance monitoring
    uint64_t total_functions_executed;
    uint64_t functions_converted_to_graph;
    double graph_speedup_ratio;
    
    // Memory management
    size_t graph_memory_used;
    size_t graph_memory_limit;
};

// Graph execution result with VM integration
typedef struct {
    bool success;
    GraphValue* output_values;
    uint32_t output_count;
    bool used_graph_execution;
    bool used_jit_compilation;
    uint64_t execution_time_ns;
    uint64_t compilation_time_ns;
    uint32_t nodes_executed;
    char* error_message;
} GraphVMResult;

// Core Graph-VM API
GraphIntegratedVM* graph_vm_create(size_t heap_size);
GraphIntegratedVM* graph_vm_create_with_config(size_t heap_size, const GraphVMConfig* config);
void graph_vm_destroy(GraphIntegratedVM* vm);

// Execution with graph integration
GraphVMResult graph_vm_execute(GraphIntegratedVM* vm, const Chunk* chunk);
GraphVMResult graph_vm_execute_function(GraphIntegratedVM* vm, uint32_t function_id);
GraphVMResult graph_vm_execute_graph(GraphIntegratedVM* vm, Graph* graph, 
                                    GraphValue* inputs, uint32_t input_count);

// Graph compilation and optimization
bool graph_vm_compile_function_to_graph(GraphIntegratedVM* vm, uint32_t function_id, 
                                       const Chunk* chunk);
bool graph_vm_optimize_graph(GraphIntegratedVM* vm, uint32_t function_id);
Graph* graph_vm_get_function_graph(GraphIntegratedVM* vm, uint32_t function_id);

// Bytecode to graph conversion
Graph* graph_vm_convert_bytecode_to_graph(GraphIntegratedVM* vm, const Chunk* chunk);
bool graph_vm_convert_function_to_graph(GraphIntegratedVM* vm, uint32_t function_id);

// Cache management
GraphCache* graph_cache_create(size_t max_entries);
void graph_cache_destroy(GraphCache* cache);
Graph* graph_cache_get(GraphCache* cache, uint32_t function_id);
bool graph_cache_put(GraphCache* cache, uint32_t function_id, Graph* graph);
void graph_cache_remove(GraphCache* cache, uint32_t function_id);
void graph_cache_clear(GraphCache* cache);
void graph_cache_evict_lru(GraphCache* cache);

// Configuration
void graph_vm_set_config(GraphIntegratedVM* vm, const GraphVMConfig* config);
void graph_vm_get_config(const GraphIntegratedVM* vm, GraphVMConfig* config);
GraphVMConfig graph_vm_default_config(void);

// Statistics and profiling
typedef struct {
    uint64_t total_executions;
    uint64_t graph_executions;
    uint64_t bytecode_executions;
    uint64_t hybrid_executions;
    uint64_t graph_compilations;
    uint64_t graph_optimizations;
    uint64_t graph_execution_time_ns;
    uint64_t bytecode_execution_time_ns;
    uint64_t graph_compilation_time_ns;
    uint64_t graph_optimization_time_ns;
    double graph_speedup_ratio;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t cache_evictions;
    size_t graph_memory_used;
    uint32_t active_graphs;
    uint32_t optimized_graphs;
} GraphVMStats;

void graph_vm_get_stats(const GraphIntegratedVM* vm, GraphVMStats* stats);
void graph_vm_reset_stats(GraphIntegratedVM* vm);
void graph_vm_print_stats(const GraphIntegratedVM* vm);

// Performance analysis
typedef struct {
    uint32_t function_id;
    uint64_t bytecode_time_ns;
    uint64_t graph_time_ns;
    uint64_t compilation_time_ns;
    uint64_t optimization_time_ns;
    double speedup_ratio;
    uint32_t execution_count;
    bool should_use_graph;
} GraphVMFunctionProfile;

GraphVMFunctionProfile* graph_vm_get_function_profiles(const GraphIntegratedVM* vm, 
                                                      uint32_t* profile_count);
void graph_vm_function_profiles_destroy(GraphVMFunctionProfile* profiles);

// Adaptive execution
bool graph_vm_should_use_graph_execution(GraphIntegratedVM* vm, uint32_t function_id);
bool graph_vm_should_compile_to_graph(GraphIntegratedVM* vm, uint32_t function_id);
void graph_vm_update_execution_profile(GraphIntegratedVM* vm, uint32_t function_id,
                                      uint64_t execution_time_ns, bool used_graph);

// Debugging and introspection
void graph_vm_dump_function_graph(const GraphIntegratedVM* vm, uint32_t function_id);
void graph_vm_dump_cache_state(const GraphIntegratedVM* vm);
void graph_vm_trace_execution(GraphIntegratedVM* vm, bool enable);
bool graph_vm_is_tracing(const GraphIntegratedVM* vm);

// Graph builder integration
GraphBuilder* graph_vm_create_builder(GraphIntegratedVM* vm, const char* name);
bool graph_vm_register_graph(GraphIntegratedVM* vm, uint32_t function_id, Graph* graph);

// Error handling
const char* graph_vm_error_to_string(const GraphVMResult* result);
void graph_vm_clear_error(GraphIntegratedVM* vm);

// Memory management
void* graph_vm_alloc_graph_memory(GraphIntegratedVM* vm, size_t size);
void graph_vm_free_graph_memory(GraphIntegratedVM* vm, void* ptr);
bool graph_vm_check_graph_memory_limit(const GraphIntegratedVM* vm, size_t additional);

// Utility functions
bool graph_vm_validate_integration(const GraphIntegratedVM* vm);
uint32_t graph_vm_estimate_graph_benefit(const GraphIntegratedVM* vm, const Chunk* chunk);
bool graph_vm_can_convert_to_graph(const Chunk* chunk);

// Advanced features
typedef struct {
    uint32_t* hot_functions;
    uint32_t hot_function_count;
    uint64_t analysis_time_ns;
} GraphVMHotspotAnalysis;

GraphVMHotspotAnalysis* graph_vm_analyze_hotspots(GraphIntegratedVM* vm);
void graph_vm_hotspot_analysis_destroy(GraphVMHotspotAnalysis* analysis);
bool graph_vm_optimize_hotspots(GraphIntegratedVM* vm, const GraphVMHotspotAnalysis* analysis);

// Parallel graph execution
bool graph_vm_enable_parallel_execution(GraphIntegratedVM* vm, uint32_t thread_count);
void graph_vm_disable_parallel_execution(GraphIntegratedVM* vm);

// Graph serialization for VM
bool graph_vm_save_graph_cache(const GraphIntegratedVM* vm, const char* filename);
bool graph_vm_load_graph_cache(GraphIntegratedVM* vm, const char* filename);

#endif // VM_GRAPH_INTEGRATION_H

