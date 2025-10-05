
#ifndef BDI_GRAPH_EXECUTOR_H
#define BDI_GRAPH_EXECUTOR_H

#include "graph.h"
#include "../vm_jit_integration.h"

// Forward declarations
typedef struct GraphExecutor GraphExecutor;
typedef struct GraphScheduler GraphScheduler;

// Execution modes
typedef enum {
    GRAPH_EXEC_SEQUENTIAL,
    GRAPH_EXEC_PARALLEL,
    GRAPH_EXEC_ASYNC,
    GRAPH_EXEC_JIT_COMPILED
} GraphExecutionMode;

// Execution configuration
typedef struct {
    GraphExecutionMode mode;
    uint32_t max_threads;
    bool enable_jit;
    bool enable_optimization;
    bool enable_profiling;
    uint64_t jit_threshold;
    size_t memory_limit;
} GraphExecutorConfig;

// Execution context
typedef struct {
    Graph* graph;  // Reference to the graph being executed
    GraphValue* input_values;
    uint32_t input_count;
    GraphValue* output_values;
    uint32_t output_count;
    
    // Execution state
    bool* node_executed;
    GraphValue** node_outputs;
    uint32_t nodes_executed;
    
    // Memory management
    void* memory_pool;
    size_t memory_used;
    size_t memory_limit;
    
    // Error handling
    char* error_message;
    uint32_t error_node_id;
} GraphExecutionContext;

// Scheduler for managing execution order
struct GraphScheduler {
    Graph* graph;
    
    // Ready queue for nodes that can be executed
    uint32_t* ready_queue;
    uint32_t ready_queue_head;
    uint32_t ready_queue_tail;
    uint32_t ready_queue_size;
    
    // Dependency tracking
    uint32_t* remaining_deps;
    
    // Execution order
    uint32_t* execution_order;
    uint32_t execution_order_count;
    
    // Statistics
    uint64_t total_scheduled;
    uint64_t scheduling_time_ns;
};

// Graph executor
struct GraphExecutor {
    GraphExecutorConfig config;
    GraphScheduler* scheduler;
    JITIntegratedVM* jit_vm;
    
    // Execution statistics
    uint64_t total_executions;
    uint64_t successful_executions;
    uint64_t failed_executions;
    uint64_t total_execution_time_ns;
    uint64_t jit_compilation_time_ns;
    
    // Performance counters
    uint64_t nodes_executed;
    uint64_t edges_traversed;
    uint64_t memory_allocated;
    uint64_t memory_freed;
    
    // Thread pool (for parallel execution)
    void* thread_pool;
    uint32_t active_threads;
};

// Core executor API
GraphExecutor* graph_executor_create(void);
GraphExecutor* graph_executor_create_with_config(const GraphExecutorConfig* config);
void graph_executor_destroy(GraphExecutor* executor);

// Execution
GraphExecutionResult graph_executor_execute(GraphExecutor* executor, Graph* graph,
                                          GraphValue* inputs, uint32_t input_count);
GraphExecutionResult graph_executor_execute_async(GraphExecutor* executor, Graph* graph,
                                                 GraphValue* inputs, uint32_t input_count);

// Execution with context
GraphExecutionContext* graph_execution_context_create(Graph* graph);
void graph_execution_context_destroy(GraphExecutionContext* context);
bool graph_executor_execute_with_context(GraphExecutor* executor, Graph* graph,
                                        GraphExecutionContext* context);

// Scheduler API
GraphScheduler* graph_scheduler_create(Graph* graph);
void graph_scheduler_destroy(GraphScheduler* scheduler);
bool graph_scheduler_schedule(GraphScheduler* scheduler);
uint32_t graph_scheduler_get_next_ready(GraphScheduler* scheduler);
void graph_scheduler_mark_completed(GraphScheduler* scheduler, uint32_t node_id);
bool graph_scheduler_is_complete(const GraphScheduler* scheduler);
void graph_scheduler_reset(GraphScheduler* scheduler);

// Node execution
typedef bool (*GraphNodeExecutor)(GraphNode* node, GraphValue* inputs, 
                                 GraphValue* outputs, void* context);

bool graph_execute_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                       GraphExecutionContext* context);
bool graph_execute_constant_node(GraphNode* node, GraphValue* outputs);
bool graph_execute_arithmetic_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs);
bool graph_execute_logical_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs);
bool graph_execute_comparison_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs);
bool graph_execute_control_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                               GraphExecutionContext* context);
bool graph_execute_memory_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                              GraphExecutionContext* context);
bool graph_execute_custom_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                              GraphExecutionContext* context);

// JIT integration
bool graph_executor_compile_graph(GraphExecutor* executor, Graph* graph);
bool graph_executor_execute_compiled(GraphExecutor* executor, Graph* graph,
                                   GraphValue* inputs, uint32_t input_count,
                                   GraphValue* outputs, uint32_t* output_count);

// Configuration
void graph_executor_set_config(GraphExecutor* executor, const GraphExecutorConfig* config);
void graph_executor_get_config(const GraphExecutor* executor, GraphExecutorConfig* config);
GraphExecutorConfig graph_executor_default_config(void);

// Statistics and profiling
typedef struct {
    uint64_t total_executions;
    uint64_t successful_executions;
    uint64_t failed_executions;
    uint64_t total_execution_time_ns;
    uint64_t average_execution_time_ns;
    uint64_t jit_compilation_time_ns;
    uint64_t nodes_executed;
    uint64_t edges_traversed;
    uint64_t memory_allocated;
    uint64_t memory_freed;
    uint32_t active_threads;
    double throughput_nodes_per_second;
} GraphExecutorStats;

void graph_executor_get_stats(const GraphExecutor* executor, GraphExecutorStats* stats);
void graph_executor_reset_stats(GraphExecutor* executor);
void graph_executor_print_stats(const GraphExecutor* executor);

// Profiling
typedef struct {
    uint32_t node_id;
    char* node_name;
    GraphNodeType node_type;
    uint64_t execution_count;
    uint64_t total_time_ns;
    uint64_t average_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
} GraphNodeProfile;

typedef struct {
    GraphNodeProfile* node_profiles;
    uint32_t node_count;
    uint64_t total_profiling_time_ns;
} GraphExecutionProfile;

GraphExecutionProfile* graph_executor_get_profile(const GraphExecutor* executor, const Graph* graph);
void graph_execution_profile_destroy(GraphExecutionProfile* profile);
void graph_execution_profile_print(const GraphExecutionProfile* profile);

// Parallel execution support
typedef struct {
    GraphExecutor* executor;
    Graph* graph;
    GraphExecutionContext* context;
    uint32_t* node_batch;
    uint32_t batch_size;
    uint32_t thread_id;
} GraphExecutionTask;

bool graph_executor_execute_parallel(GraphExecutor* executor, Graph* graph,
                                    GraphExecutionContext* context);
void* graph_executor_worker_thread(void* arg);

// Memory management
void* graph_executor_alloc(GraphExecutionContext* context, size_t size);
void graph_executor_free(GraphExecutionContext* context, void* ptr);
bool graph_executor_check_memory_limit(const GraphExecutionContext* context, size_t additional);

// Error handling
const char* graph_execution_error_to_string(const GraphExecutionResult* result);
void graph_execution_context_set_error(GraphExecutionContext* context, 
                                      const char* message, uint32_t node_id);

// Optimization hints
typedef struct {
    bool can_parallelize;
    bool is_memory_bound;
    bool is_compute_bound;
    bool has_side_effects;
    uint32_t estimated_cycles;
    uint32_t memory_footprint;
} GraphNodeHints;

void graph_executor_analyze_node(GraphNode* node, GraphNodeHints* hints);
bool graph_executor_can_fuse_nodes(GraphNode* node1, GraphNode* node2);
uint32_t graph_executor_estimate_execution_time(const Graph* graph);

// Debugging and introspection
void graph_executor_dump_execution_state(const GraphExecutor* executor, 
                                        const GraphExecutionContext* context);
void graph_executor_trace_execution(GraphExecutor* executor, bool enable);
bool graph_executor_is_tracing(const GraphExecutor* executor);

// Utility functions
bool graph_value_is_valid(const GraphValue* value);
GraphValue graph_value_zero(GraphDataType type);
GraphValue graph_value_add(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_sub(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_mul(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_div(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_mod(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_and(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_or(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_xor(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_not(const GraphValue* a);
GraphValue graph_value_eq(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_ne(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_lt(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_le(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_gt(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_ge(const GraphValue* a, const GraphValue* b);
GraphValue graph_value_cast(const GraphValue* value, GraphDataType target_type);

#endif // BDI_GRAPH_EXECUTOR_H

