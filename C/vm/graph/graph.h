
#ifndef BDI_GRAPH_H
#define BDI_GRAPH_H

#include "../c23_compat.h"
#include "../bci_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct GraphNode GraphNode;
typedef struct GraphEdge GraphEdge;
typedef struct Graph Graph;
typedef struct GraphExecutor GraphExecutor;

// Graph node types
typedef enum {
    GRAPH_NODE_CONSTANT,
    GRAPH_NODE_INPUT,
    GRAPH_NODE_OUTPUT,
    GRAPH_NODE_ADD,
    GRAPH_NODE_SUB,
    GRAPH_NODE_MUL,
    GRAPH_NODE_DIV,
    GRAPH_NODE_MOD,
    GRAPH_NODE_AND,
    GRAPH_NODE_OR,
    GRAPH_NODE_XOR,
    GRAPH_NODE_NOT,
    GRAPH_NODE_EQ,
    GRAPH_NODE_NE,
    GRAPH_NODE_LT,
    GRAPH_NODE_LE,
    GRAPH_NODE_GT,
    GRAPH_NODE_GE,
    GRAPH_NODE_BRANCH,
    GRAPH_NODE_PHI,
    GRAPH_NODE_CALL,
    GRAPH_NODE_LOAD,
    GRAPH_NODE_STORE,
    GRAPH_NODE_ALLOC,
    GRAPH_NODE_CAST,
    GRAPH_NODE_SELECT,
    GRAPH_NODE_MERGE,
    GRAPH_NODE_LOOP_BEGIN,
    GRAPH_NODE_LOOP_END,
    GRAPH_NODE_RETURN,
    GRAPH_NODE_CUSTOM
} GraphNodeType;

// Data types for graph values
typedef enum {
    GRAPH_TYPE_VOID,
    GRAPH_TYPE_I8,
    GRAPH_TYPE_I16,
    GRAPH_TYPE_I32,
    GRAPH_TYPE_I64,
    GRAPH_TYPE_F32,
    GRAPH_TYPE_F64,
    GRAPH_TYPE_PTR,
    GRAPH_TYPE_BOOL,
    GRAPH_TYPE_CONTROL
} GraphDataType;

// Graph value representation
typedef struct {
    GraphDataType type;
    union {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        void* ptr;
        bool boolean;
    } data;
} GraphValue;

// Graph edge representing data flow
struct GraphEdge {
    GraphNode* source;
    GraphNode* target;
    uint32_t source_output;
    uint32_t target_input;
    GraphDataType data_type;
    GraphEdge* next_source;
    GraphEdge* next_target;
};

// Graph node
struct GraphNode {
    uint32_t id;
    GraphNodeType type;
    char* name;
    
    // Node inputs and outputs
    GraphEdge** inputs;
    GraphEdge** outputs;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t max_inputs;
    uint32_t max_outputs;
    
    // Node data
    GraphValue constant_value;
    void* custom_data;
    size_t custom_data_size;
    
    // Execution state
    bool executed;
    bool ready;
    GraphValue* output_values;
    
    // Scheduling information
    uint32_t dependency_count;
    uint32_t remaining_dependencies;
    
    // Optimization metadata
    bool is_dead;
    bool is_constant;
    uint32_t execution_count;
    uint64_t execution_time_ns;
    
    // Memory management
    struct GraphNode* next;
};

// Graph structure
struct Graph {
    char* name;
    uint32_t id;
    
    // Nodes
    GraphNode** nodes;
    uint32_t node_count;
    uint32_t max_nodes;
    
    // Edges
    GraphEdge** edges;
    uint32_t edge_count;
    uint32_t max_edges;
    
    // Entry and exit points
    GraphNode** inputs;
    GraphNode** outputs;
    uint32_t input_count;
    uint32_t output_count;
    
    // Execution metadata
    bool is_compiled;
    bool is_optimized;
    uint32_t execution_count;
    uint64_t total_execution_time_ns;
    
    // Memory management
    void* memory_pool;
    size_t memory_pool_size;
    size_t memory_used;
};

// Graph execution result
typedef struct {
    bool success;
    GraphValue* output_values;
    uint32_t output_count;
    uint64_t execution_time_ns;
    uint32_t nodes_executed;
    char* error_message;
} GraphExecutionResult;

// Graph statistics
typedef struct {
    uint32_t total_nodes;
    uint32_t total_edges;
    uint32_t input_nodes;
    uint32_t output_nodes;
    uint32_t constant_nodes;
    uint32_t operation_nodes;
    uint32_t control_nodes;
    uint32_t dead_nodes;
    uint64_t total_executions;
    uint64_t total_execution_time_ns;
    double average_execution_time_ns;
} GraphStats;

// Core graph API
Graph* graph_create(const char* name);
void graph_destroy(Graph* graph);
Graph* graph_clone(const Graph* graph);

// Node management
GraphNode* graph_add_node(Graph* graph, GraphNodeType type, const char* name);
GraphNode* graph_add_constant_node(Graph* graph, GraphValue value, const char* name);
GraphNode* graph_add_input_node(Graph* graph, GraphDataType type, const char* name);
GraphNode* graph_add_output_node(Graph* graph, GraphDataType type, const char* name);
bool graph_remove_node(Graph* graph, uint32_t node_id);
GraphNode* graph_get_node(const Graph* graph, uint32_t node_id);
GraphNode* graph_find_node(const Graph* graph, const char* name);

// Edge management
GraphEdge* graph_add_edge(Graph* graph, uint32_t source_id, uint32_t source_output,
                         uint32_t target_id, uint32_t target_input);
bool graph_remove_edge(Graph* graph, uint32_t source_id, uint32_t source_output,
                      uint32_t target_id, uint32_t target_input);
bool graph_has_edge(const Graph* graph, uint32_t source_id, uint32_t target_id);

// Graph validation
bool graph_validate(const Graph* graph, char** error_message);
bool graph_has_cycles(const Graph* graph);
bool graph_is_dag(const Graph* graph);

// Graph analysis
uint32_t* graph_topological_sort(const Graph* graph, uint32_t* count);
uint32_t graph_get_depth(const Graph* graph);
uint32_t graph_get_width(const Graph* graph);
void graph_get_stats(const Graph* graph, GraphStats* stats);

// Graph serialization
bool graph_save_to_file(const Graph* graph, const char* filename);
Graph* graph_load_from_file(const char* filename);
char* graph_to_dot(const Graph* graph);
char* graph_to_json(const Graph* graph);

// Utility functions
const char* graph_node_type_to_string(GraphNodeType type);
const char* graph_data_type_to_string(GraphDataType type);
GraphValue graph_value_create(GraphDataType type, const void* data);
bool graph_value_equals(const GraphValue* a, const GraphValue* b);
char* graph_value_to_string(const GraphValue* value);

// Memory management helpers
void* graph_alloc(Graph* graph, size_t size);
void graph_free(Graph* graph, void* ptr);
void graph_reset_memory_pool(Graph* graph);

#endif // BDI_GRAPH_H

