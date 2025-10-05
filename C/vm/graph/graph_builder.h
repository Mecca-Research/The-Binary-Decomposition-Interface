
#ifndef BDI_GRAPH_BUILDER_H
#define BDI_GRAPH_BUILDER_H

#include "graph.h"

// Graph builder for programmatic construction
typedef struct GraphBuilder GraphBuilder;

// Builder configuration
typedef struct {
    bool auto_optimize;
    bool validate_on_build;
    bool enable_type_checking;
    size_t initial_node_capacity;
    size_t initial_edge_capacity;
} GraphBuilderConfig;

// Graph builder API
GraphBuilder* graph_builder_create(const char* graph_name);
GraphBuilder* graph_builder_create_with_config(const char* graph_name, 
                                              const GraphBuilderConfig* config);
void graph_builder_destroy(GraphBuilder* builder);

// Node creation with fluent API
uint32_t graph_builder_add_constant(GraphBuilder* builder, GraphValue value, const char* name);
uint32_t graph_builder_add_input(GraphBuilder* builder, GraphDataType type, const char* name);
uint32_t graph_builder_add_output(GraphBuilder* builder, GraphDataType type, const char* name);

// Arithmetic operations
uint32_t graph_builder_add(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_sub(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_mul(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_div(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_mod(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);

// Logical operations
uint32_t graph_builder_and(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_or(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_xor(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_not(GraphBuilder* builder, uint32_t operand, const char* name);

// Comparison operations
uint32_t graph_builder_eq(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_ne(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_lt(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_le(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_gt(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);
uint32_t graph_builder_ge(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name);

// Control flow operations
uint32_t graph_builder_branch(GraphBuilder* builder, uint32_t condition, 
                             uint32_t true_target, uint32_t false_target, const char* name);
uint32_t graph_builder_phi(GraphBuilder* builder, uint32_t* inputs, uint32_t input_count, const char* name);
uint32_t graph_builder_select(GraphBuilder* builder, uint32_t condition, 
                             uint32_t true_value, uint32_t false_value, const char* name);

// Memory operations
uint32_t graph_builder_load(GraphBuilder* builder, uint32_t address, GraphDataType type, const char* name);
uint32_t graph_builder_store(GraphBuilder* builder, uint32_t address, uint32_t value, const char* name);
uint32_t graph_builder_alloc(GraphBuilder* builder, uint32_t size, GraphDataType type, const char* name);

// Type operations
uint32_t graph_builder_cast(GraphBuilder* builder, uint32_t value, 
                           GraphDataType from_type, GraphDataType to_type, const char* name);

// Function operations
uint32_t graph_builder_call(GraphBuilder* builder, const char* function_name, 
                           uint32_t* args, uint32_t arg_count, const char* name);
uint32_t graph_builder_return(GraphBuilder* builder, uint32_t value, const char* name);

// Loop operations
uint32_t graph_builder_loop_begin(GraphBuilder* builder, const char* name);
uint32_t graph_builder_loop_end(GraphBuilder* builder, uint32_t loop_begin, 
                                uint32_t condition, const char* name);

// Custom operations
uint32_t graph_builder_custom(GraphBuilder* builder, const char* operation_name,
                             uint32_t* inputs, uint32_t input_count,
                             GraphDataType* output_types, uint32_t output_count,
                             void* custom_data, size_t custom_data_size,
                             const char* name);

// Connection management
bool graph_builder_connect(GraphBuilder* builder, uint32_t source, uint32_t source_output,
                          uint32_t target, uint32_t target_input);
bool graph_builder_disconnect(GraphBuilder* builder, uint32_t source, uint32_t target);

// Builder state management
bool graph_builder_validate(GraphBuilder* builder, char** error_message);
Graph* graph_builder_build(GraphBuilder* builder);
Graph* graph_builder_build_and_optimize(GraphBuilder* builder);

// Builder introspection
uint32_t graph_builder_get_node_count(const GraphBuilder* builder);
uint32_t graph_builder_get_edge_count(const GraphBuilder* builder);
bool graph_builder_has_node(const GraphBuilder* builder, uint32_t node_id);
bool graph_builder_has_edge(const GraphBuilder* builder, uint32_t source, uint32_t target);

// Configuration
void graph_builder_set_config(GraphBuilder* builder, const GraphBuilderConfig* config);
void graph_builder_get_config(const GraphBuilder* builder, GraphBuilderConfig* config);
GraphBuilderConfig graph_builder_default_config(void);

// Convenience functions for common patterns
typedef struct {
    uint32_t* node_ids;
    uint32_t count;
} GraphNodeList;

// Create common graph patterns
GraphNodeList graph_builder_create_linear_chain(GraphBuilder* builder, 
                                               GraphNodeType* operations, uint32_t count,
                                               uint32_t input_node, const char* name_prefix);
GraphNodeList graph_builder_create_parallel_ops(GraphBuilder* builder,
                                               GraphNodeType operation, uint32_t count,
                                               uint32_t* inputs, uint32_t input_count,
                                               const char* name_prefix);
uint32_t graph_builder_create_reduction(GraphBuilder* builder, GraphNodeType operation,
                                       uint32_t* inputs, uint32_t input_count,
                                       const char* name);

// Expression builder (for mathematical expressions)
typedef struct GraphExprBuilder GraphExprBuilder;

GraphExprBuilder* graph_expr_builder_create(GraphBuilder* graph_builder);
void graph_expr_builder_destroy(GraphExprBuilder* expr_builder);

uint32_t graph_expr_parse(GraphExprBuilder* expr_builder, const char* expression);
uint32_t graph_expr_variable(GraphExprBuilder* expr_builder, const char* name, GraphDataType type);
uint32_t graph_expr_constant(GraphExprBuilder* expr_builder, GraphValue value);

// Cleanup
void graph_node_list_destroy(GraphNodeList* list);

#endif // BDI_GRAPH_BUILDER_H

