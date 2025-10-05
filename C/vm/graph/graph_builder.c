
#include "graph_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Graph builder structure
struct GraphBuilder {
    Graph* graph;
    GraphBuilderConfig config;
    
    // Node ID mapping for validation
    uint32_t* node_ids;
    uint32_t node_id_count;
    uint32_t node_id_capacity;
    
    // Error tracking
    char* last_error;
    bool has_error;
};

// Expression builder structure
struct GraphExprBuilder {
    GraphBuilder* graph_builder;
    
    // Variable mapping
    struct {
        char* name;
        uint32_t node_id;
        GraphDataType type;
    }* variables;
    uint32_t variable_count;
    uint32_t variable_capacity;
};

// Helper functions
static bool builder_add_node_id(GraphBuilder* builder, uint32_t node_id) {
    if (builder->node_id_count >= builder->node_id_capacity) {
        builder->node_id_capacity = builder->node_id_capacity ? builder->node_id_capacity * 2 : 16;
        builder->node_ids = (uint32_t*)realloc(builder->node_ids, 
                                              builder->node_id_capacity * sizeof(uint32_t));
        if (!builder->node_ids) return false;
    }
    
    builder->node_ids[builder->node_id_count++] = node_id;
    return true;
}

static bool builder_has_node_id(const GraphBuilder* builder, uint32_t node_id) {
    for (uint32_t i = 0; i < builder->node_id_count; i++) {
        if (builder->node_ids[i] == node_id) return true;
    }
    return false;
}

static void builder_set_error(GraphBuilder* builder, const char* error) {
    if (builder->last_error) {
        free(builder->last_error);
    }
    builder->last_error = error ? strdup(error) : NULL;
    builder->has_error = (error != NULL);
}

static uint32_t builder_add_binary_op(GraphBuilder* builder, GraphNodeType type,
                                     uint32_t left, uint32_t right, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    // Validate inputs
    if (!builder_has_node_id(builder, left) || !builder_has_node_id(builder, right)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, type, name);
    if (!node) {
        builder_set_error(builder, "Failed to create node");
        return 0;
    }
    
    // Connect inputs
    if (!graph_add_edge(builder->graph, left, 0, node->id, 0) ||
        !graph_add_edge(builder->graph, right, 0, node->id, 1)) {
        builder_set_error(builder, "Failed to connect inputs");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

static uint32_t builder_add_unary_op(GraphBuilder* builder, GraphNodeType type,
                                    uint32_t operand, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    // Validate input
    if (!builder_has_node_id(builder, operand)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, type, name);
    if (!node) {
        builder_set_error(builder, "Failed to create node");
        return 0;
    }
    
    // Connect input
    if (!graph_add_edge(builder->graph, operand, 0, node->id, 0)) {
        builder_set_error(builder, "Failed to connect input");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Graph builder API implementation
GraphBuilder* graph_builder_create(const char* graph_name) {
    GraphBuilderConfig config = graph_builder_default_config();
    return graph_builder_create_with_config(graph_name, &config);
}

GraphBuilder* graph_builder_create_with_config(const char* graph_name, 
                                              const GraphBuilderConfig* config) {
    GraphBuilder* builder = (GraphBuilder*)calloc(1, sizeof(GraphBuilder));
    if (!builder) return NULL;
    
    builder->graph = graph_create(graph_name);
    if (!builder->graph) {
        free(builder);
        return NULL;
    }
    
    if (config) {
        builder->config = *config;
    } else {
        builder->config = graph_builder_default_config();
    }
    
    builder->node_id_capacity = builder->config.initial_node_capacity;
    builder->node_ids = (uint32_t*)malloc(builder->node_id_capacity * sizeof(uint32_t));
    if (!builder->node_ids) {
        graph_destroy(builder->graph);
        free(builder);
        return NULL;
    }
    
    return builder;
}

void graph_builder_destroy(GraphBuilder* builder) {
    if (!builder) return;
    
    graph_destroy(builder->graph);
    free(builder->node_ids);
    free(builder->last_error);
    free(builder);
}

uint32_t graph_builder_add_constant(GraphBuilder* builder, GraphValue value, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    GraphNode* node = graph_add_constant_node(builder->graph, value, name);
    if (!node) {
        builder_set_error(builder, "Failed to create constant node");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_add_input(GraphBuilder* builder, GraphDataType type, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    GraphNode* node = graph_add_input_node(builder->graph, type, name);
    if (!node) {
        builder_set_error(builder, "Failed to create input node");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_add_output(GraphBuilder* builder, GraphDataType type, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    GraphNode* node = graph_add_output_node(builder->graph, type, name);
    if (!node) {
        builder_set_error(builder, "Failed to create output node");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Arithmetic operations
uint32_t graph_builder_add(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_ADD, left, right, name);
}

uint32_t graph_builder_sub(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_SUB, left, right, name);
}

uint32_t graph_builder_mul(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_MUL, left, right, name);
}

uint32_t graph_builder_div(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_DIV, left, right, name);
}

uint32_t graph_builder_mod(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_MOD, left, right, name);
}

// Logical operations
uint32_t graph_builder_and(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_AND, left, right, name);
}

uint32_t graph_builder_or(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_OR, left, right, name);
}

uint32_t graph_builder_xor(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_XOR, left, right, name);
}

uint32_t graph_builder_not(GraphBuilder* builder, uint32_t operand, const char* name) {
    return builder_add_unary_op(builder, GRAPH_NODE_NOT, operand, name);
}

// Comparison operations
uint32_t graph_builder_eq(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_EQ, left, right, name);
}

uint32_t graph_builder_ne(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_NE, left, right, name);
}

uint32_t graph_builder_lt(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_LT, left, right, name);
}

uint32_t graph_builder_le(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_LE, left, right, name);
}

uint32_t graph_builder_gt(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_GT, left, right, name);
}

uint32_t graph_builder_ge(GraphBuilder* builder, uint32_t left, uint32_t right, const char* name) {
    return builder_add_binary_op(builder, GRAPH_NODE_GE, left, right, name);
}

// Control flow operations
uint32_t graph_builder_branch(GraphBuilder* builder, uint32_t condition, 
                             uint32_t true_target, uint32_t false_target, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    // Validate inputs
    if (!builder_has_node_id(builder, condition) || 
        !builder_has_node_id(builder, true_target) ||
        !builder_has_node_id(builder, false_target)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_BRANCH, name);
    if (!node) {
        builder_set_error(builder, "Failed to create branch node");
        return 0;
    }
    
    // Connect inputs
    if (!graph_add_edge(builder->graph, condition, 0, node->id, 0) ||
        !graph_add_edge(builder->graph, true_target, 0, node->id, 1) ||
        !graph_add_edge(builder->graph, false_target, 0, node->id, 2)) {
        builder_set_error(builder, "Failed to connect branch inputs");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_phi(GraphBuilder* builder, uint32_t* inputs, uint32_t input_count, const char* name) {
    if (!builder || !builder->graph || !inputs || input_count == 0) return 0;
    
    // Validate all inputs
    for (uint32_t i = 0; i < input_count; i++) {
        if (!builder_has_node_id(builder, inputs[i])) {
            builder_set_error(builder, "Invalid input node ID in phi");
            return 0;
        }
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_PHI, name);
    if (!node) {
        builder_set_error(builder, "Failed to create phi node");
        return 0;
    }
    
    // Connect all inputs
    for (uint32_t i = 0; i < input_count; i++) {
        if (!graph_add_edge(builder->graph, inputs[i], 0, node->id, i)) {
            builder_set_error(builder, "Failed to connect phi input");
            return 0;
        }
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_select(GraphBuilder* builder, uint32_t condition, 
                             uint32_t true_value, uint32_t false_value, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    // Validate inputs
    if (!builder_has_node_id(builder, condition) || 
        !builder_has_node_id(builder, true_value) ||
        !builder_has_node_id(builder, false_value)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_SELECT, name);
    if (!node) {
        builder_set_error(builder, "Failed to create select node");
        return 0;
    }
    
    // Connect inputs
    if (!graph_add_edge(builder->graph, condition, 0, node->id, 0) ||
        !graph_add_edge(builder->graph, true_value, 0, node->id, 1) ||
        !graph_add_edge(builder->graph, false_value, 0, node->id, 2)) {
        builder_set_error(builder, "Failed to connect select inputs");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Memory operations
uint32_t graph_builder_load(GraphBuilder* builder, uint32_t address, GraphDataType type, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (!builder_has_node_id(builder, address)) {
        builder_set_error(builder, "Invalid address node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_LOAD, name);
    if (!node) {
        builder_set_error(builder, "Failed to create load node");
        return 0;
    }
    
    // Set output type
    node->output_values[0].type = type;
    
    // Connect address input
    if (!graph_add_edge(builder->graph, address, 0, node->id, 0)) {
        builder_set_error(builder, "Failed to connect load address");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_store(GraphBuilder* builder, uint32_t address, uint32_t value, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (!builder_has_node_id(builder, address) || !builder_has_node_id(builder, value)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_STORE, name);
    if (!node) {
        builder_set_error(builder, "Failed to create store node");
        return 0;
    }
    
    // Connect inputs
    if (!graph_add_edge(builder->graph, address, 0, node->id, 0) ||
        !graph_add_edge(builder->graph, value, 0, node->id, 1)) {
        builder_set_error(builder, "Failed to connect store inputs");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_alloc(GraphBuilder* builder, uint32_t size, GraphDataType type, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (!builder_has_node_id(builder, size)) {
        builder_set_error(builder, "Invalid size node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_ALLOC, name);
    if (!node) {
        builder_set_error(builder, "Failed to create alloc node");
        return 0;
    }
    
    // Set output type to pointer
    node->output_values[0].type = GRAPH_TYPE_PTR;
    
    // Connect size input
    if (!graph_add_edge(builder->graph, size, 0, node->id, 0)) {
        builder_set_error(builder, "Failed to connect alloc size");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Type operations
uint32_t graph_builder_cast(GraphBuilder* builder, uint32_t value, 
                           GraphDataType from_type, GraphDataType to_type, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (!builder_has_node_id(builder, value)) {
        builder_set_error(builder, "Invalid value node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_CAST, name);
    if (!node) {
        builder_set_error(builder, "Failed to create cast node");
        return 0;
    }
    
    // Set output type
    node->output_values[0].type = to_type;
    
    // Store type information in custom data
    struct {
        GraphDataType from_type;
        GraphDataType to_type;
    } cast_data = { from_type, to_type };
    
    node->custom_data = malloc(sizeof(cast_data));
    if (node->custom_data) {
        memcpy(node->custom_data, &cast_data, sizeof(cast_data));
        node->custom_data_size = sizeof(cast_data);
    }
    
    // Connect input
    if (!graph_add_edge(builder->graph, value, 0, node->id, 0)) {
        builder_set_error(builder, "Failed to connect cast input");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Function operations
uint32_t graph_builder_call(GraphBuilder* builder, const char* function_name, 
                           uint32_t* args, uint32_t arg_count, const char* name) {
    if (!builder || !builder->graph || !function_name) return 0;
    
    // Validate all arguments
    for (uint32_t i = 0; i < arg_count; i++) {
        if (!builder_has_node_id(builder, args[i])) {
            builder_set_error(builder, "Invalid argument node ID");
            return 0;
        }
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_CALL, name);
    if (!node) {
        builder_set_error(builder, "Failed to create call node");
        return 0;
    }
    
    // Store function name in custom data
    size_t name_len = strlen(function_name) + 1;
    node->custom_data = malloc(name_len);
    if (node->custom_data) {
        memcpy(node->custom_data, function_name, name_len);
        node->custom_data_size = name_len;
    }
    
    // Connect arguments
    for (uint32_t i = 0; i < arg_count; i++) {
        if (!graph_add_edge(builder->graph, args[i], 0, node->id, i)) {
            builder_set_error(builder, "Failed to connect call argument");
            return 0;
        }
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_return(GraphBuilder* builder, uint32_t value, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (value != 0 && !builder_has_node_id(builder, value)) {
        builder_set_error(builder, "Invalid return value node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_RETURN, name);
    if (!node) {
        builder_set_error(builder, "Failed to create return node");
        return 0;
    }
    
    // Connect return value if provided
    if (value != 0) {
        if (!graph_add_edge(builder->graph, value, 0, node->id, 0)) {
            builder_set_error(builder, "Failed to connect return value");
            return 0;
        }
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Loop operations
uint32_t graph_builder_loop_begin(GraphBuilder* builder, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_LOOP_BEGIN, name);
    if (!node) {
        builder_set_error(builder, "Failed to create loop begin node");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

uint32_t graph_builder_loop_end(GraphBuilder* builder, uint32_t loop_begin, 
                                uint32_t condition, const char* name) {
    if (!builder || !builder->graph) return 0;
    
    if (!builder_has_node_id(builder, loop_begin) || !builder_has_node_id(builder, condition)) {
        builder_set_error(builder, "Invalid input node ID");
        return 0;
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_LOOP_END, name);
    if (!node) {
        builder_set_error(builder, "Failed to create loop end node");
        return 0;
    }
    
    // Connect inputs
    if (!graph_add_edge(builder->graph, loop_begin, 0, node->id, 0) ||
        !graph_add_edge(builder->graph, condition, 0, node->id, 1)) {
        builder_set_error(builder, "Failed to connect loop end inputs");
        return 0;
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Custom operations
uint32_t graph_builder_custom(GraphBuilder* builder, const char* operation_name,
                             uint32_t* inputs, uint32_t input_count,
                             GraphDataType* output_types, uint32_t output_count,
                             void* custom_data, size_t custom_data_size,
                             const char* name) {
    if (!builder || !builder->graph || !operation_name) return 0;
    
    // Validate all inputs
    for (uint32_t i = 0; i < input_count; i++) {
        if (!builder_has_node_id(builder, inputs[i])) {
            builder_set_error(builder, "Invalid input node ID");
            return 0;
        }
    }
    
    GraphNode* node = graph_add_node(builder->graph, GRAPH_NODE_CUSTOM, name);
    if (!node) {
        builder_set_error(builder, "Failed to create custom node");
        return 0;
    }
    
    // Store custom data
    if (custom_data && custom_data_size > 0) {
        node->custom_data = malloc(custom_data_size);
        if (node->custom_data) {
            memcpy(node->custom_data, custom_data, custom_data_size);
            node->custom_data_size = custom_data_size;
        }
    }
    
    // Set output types
    for (uint32_t i = 0; i < output_count && i < node->max_outputs; i++) {
        node->output_values[i].type = output_types[i];
    }
    
    // Connect inputs
    for (uint32_t i = 0; i < input_count; i++) {
        if (!graph_add_edge(builder->graph, inputs[i], 0, node->id, i)) {
            builder_set_error(builder, "Failed to connect custom node input");
            return 0;
        }
    }
    
    builder_add_node_id(builder, node->id);
    return node->id;
}

// Connection management
bool graph_builder_connect(GraphBuilder* builder, uint32_t source, uint32_t source_output,
                          uint32_t target, uint32_t target_input) {
    if (!builder || !builder->graph) return false;
    
    if (!builder_has_node_id(builder, source) || !builder_has_node_id(builder, target)) {
        builder_set_error(builder, "Invalid node ID for connection");
        return false;
    }
    
    GraphEdge* edge = graph_add_edge(builder->graph, source, source_output, target, target_input);
    if (!edge) {
        builder_set_error(builder, "Failed to create connection");
        return false;
    }
    
    return true;
}

bool graph_builder_disconnect(GraphBuilder* builder, uint32_t source, uint32_t target) {
    if (!builder || !builder->graph) return false;
    
    // Find and remove all edges between source and target
    bool removed_any = false;
    for (uint32_t i = 0; i < builder->graph->edge_count; ) {
        GraphEdge* edge = builder->graph->edges[i];
        if (edge->source->id == source && edge->target->id == target) {
            if (graph_remove_edge(builder->graph, source, edge->source_output, 
                                 target, edge->target_input)) {
                removed_any = true;
                // Don't increment i since we removed an edge
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    
    return removed_any;
}

// Builder state management
bool graph_builder_validate(GraphBuilder* builder, char** error_message) {
    if (!builder || !builder->graph) {
        if (error_message) *error_message = strdup("Invalid builder");
        return false;
    }
    
    return graph_validate(builder->graph, error_message);
}

Graph* graph_builder_build(GraphBuilder* builder) {
    if (!builder || !builder->graph) return NULL;
    
    if (builder->config.validate_on_build) {
        char* error = NULL;
        if (!graph_builder_validate(builder, &error)) {
            builder_set_error(builder, error);
            free(error);
            return NULL;
        }
    }
    
    // Clone the graph to return
    Graph* result = graph_clone(builder->graph);
    return result;
}

Graph* graph_builder_build_and_optimize(GraphBuilder* builder) {
    Graph* graph = graph_builder_build(builder);
    if (!graph) return NULL;
    
    // TODO: Apply optimizations
    // For now, just return the unoptimized graph
    return graph;
}

// Builder introspection
uint32_t graph_builder_get_node_count(const GraphBuilder* builder) {
    return builder ? builder->node_id_count : 0;
}

uint32_t graph_builder_get_edge_count(const GraphBuilder* builder) {
    return builder && builder->graph ? builder->graph->edge_count : 0;
}

bool graph_builder_has_node(const GraphBuilder* builder, uint32_t node_id) {
    return builder_has_node_id(builder, node_id);
}

bool graph_builder_has_edge(const GraphBuilder* builder, uint32_t source, uint32_t target) {
    return builder && builder->graph ? graph_has_edge(builder->graph, source, target) : false;
}

// Configuration
void graph_builder_set_config(GraphBuilder* builder, const GraphBuilderConfig* config) {
    if (builder && config) {
        builder->config = *config;
    }
}

void graph_builder_get_config(const GraphBuilder* builder, GraphBuilderConfig* config) {
    if (builder && config) {
        *config = builder->config;
    }
}

GraphBuilderConfig graph_builder_default_config(void) {
    GraphBuilderConfig config = {
        .auto_optimize = false,
        .validate_on_build = true,
        .enable_type_checking = true,
        .initial_node_capacity = 64,
        .initial_edge_capacity = 128
    };
    return config;
}

// Convenience functions for common patterns
GraphNodeList graph_builder_create_linear_chain(GraphBuilder* builder, 
                                               GraphNodeType* operations, uint32_t count,
                                               uint32_t input_node, const char* name_prefix) {
    GraphNodeList result = {0};
    
    if (!builder || !operations || count == 0) return result;
    
    result.node_ids = (uint32_t*)malloc(count * sizeof(uint32_t));
    if (!result.node_ids) return result;
    
    uint32_t current_input = input_node;
    
    for (uint32_t i = 0; i < count; i++) {
        char node_name[64];
        snprintf(node_name, sizeof(node_name), "%s_%u", 
                name_prefix ? name_prefix : "chain", i);
        
        uint32_t node_id = 0;
        
        // Create node based on type
        switch (operations[i]) {
            case GRAPH_NODE_ADD:
                // For simplicity, add with itself (could be parameterized)
                node_id = graph_builder_add(builder, current_input, current_input, node_name);
                break;
            case GRAPH_NODE_SUB:
                node_id = graph_builder_sub(builder, current_input, current_input, node_name);
                break;
            case GRAPH_NODE_MUL:
                node_id = graph_builder_mul(builder, current_input, current_input, node_name);
                break;
            case GRAPH_NODE_NOT:
                node_id = graph_builder_not(builder, current_input, node_name);
                break;
            default:
                // Create generic node
                node_id = 0; // Skip unsupported operations
                break;
        }
        
        if (node_id == 0) {
            // Failed to create node, cleanup and return partial result
            result.count = i;
            return result;
        }
        
        result.node_ids[i] = node_id;
        current_input = node_id;
    }
    
    result.count = count;
    return result;
}

GraphNodeList graph_builder_create_parallel_ops(GraphBuilder* builder,
                                               GraphNodeType operation, uint32_t count,
                                               uint32_t* inputs, uint32_t input_count,
                                               const char* name_prefix) {
    GraphNodeList result = {0};
    
    if (!builder || count == 0 || !inputs || input_count == 0) return result;
    
    result.node_ids = (uint32_t*)malloc(count * sizeof(uint32_t));
    if (!result.node_ids) return result;
    
    for (uint32_t i = 0; i < count; i++) {
        char node_name[64];
        snprintf(node_name, sizeof(node_name), "%s_%u", 
                name_prefix ? name_prefix : "parallel", i);
        
        // Use inputs in round-robin fashion
        uint32_t input1 = inputs[i % input_count];
        uint32_t input2 = inputs[(i + 1) % input_count];
        
        uint32_t node_id = 0;
        
        switch (operation) {
            case GRAPH_NODE_ADD:
                node_id = graph_builder_add(builder, input1, input2, node_name);
                break;
            case GRAPH_NODE_SUB:
                node_id = graph_builder_sub(builder, input1, input2, node_name);
                break;
            case GRAPH_NODE_MUL:
                node_id = graph_builder_mul(builder, input1, input2, node_name);
                break;
            case GRAPH_NODE_DIV:
                node_id = graph_builder_div(builder, input1, input2, node_name);
                break;
            default:
                node_id = 0;
                break;
        }
        
        if (node_id == 0) {
            result.count = i;
            return result;
        }
        
        result.node_ids[i] = node_id;
    }
    
    result.count = count;
    return result;
}

uint32_t graph_builder_create_reduction(GraphBuilder* builder, GraphNodeType operation,
                                       uint32_t* inputs, uint32_t input_count,
                                       const char* name) {
    if (!builder || !inputs || input_count == 0) return 0;
    
    if (input_count == 1) return inputs[0];
    
    // Create binary tree reduction
    uint32_t* current_level = (uint32_t*)malloc(input_count * sizeof(uint32_t));
    if (!current_level) return 0;
    
    memcpy(current_level, inputs, input_count * sizeof(uint32_t));
    uint32_t current_count = input_count;
    uint32_t level = 0;
    
    while (current_count > 1) {
        uint32_t next_count = (current_count + 1) / 2;
        uint32_t* next_level = (uint32_t*)malloc(next_count * sizeof(uint32_t));
        if (!next_level) {
            free(current_level);
            return 0;
        }
        
        for (uint32_t i = 0; i < next_count; i++) {
            uint32_t left = current_level[i * 2];
            uint32_t right = (i * 2 + 1 < current_count) ? current_level[i * 2 + 1] : left;
            
            char node_name[64];
            snprintf(node_name, sizeof(node_name), "%s_reduce_l%u_n%u", 
                    name ? name : "reduction", level, i);
            
            uint32_t node_id = 0;
            switch (operation) {
                case GRAPH_NODE_ADD:
                    node_id = graph_builder_add(builder, left, right, node_name);
                    break;
                case GRAPH_NODE_MUL:
                    node_id = graph_builder_mul(builder, left, right, node_name);
                    break;
                case GRAPH_NODE_AND:
                    node_id = graph_builder_and(builder, left, right, node_name);
                    break;
                case GRAPH_NODE_OR:
                    node_id = graph_builder_or(builder, left, right, node_name);
                    break;
                default:
                    free(current_level);
                    free(next_level);
                    return 0;
            }
            
            if (node_id == 0) {
                free(current_level);
                free(next_level);
                return 0;
            }
            
            next_level[i] = node_id;
        }
        
        free(current_level);
        current_level = next_level;
        current_count = next_count;
        level++;
    }
    
    uint32_t result = current_level[0];
    free(current_level);
    return result;
}

// Expression builder implementation (basic)
GraphExprBuilder* graph_expr_builder_create(GraphBuilder* graph_builder) {
    if (!graph_builder) return NULL;
    
    GraphExprBuilder* expr_builder = (GraphExprBuilder*)calloc(1, sizeof(GraphExprBuilder));
    if (!expr_builder) return NULL;
    
    expr_builder->graph_builder = graph_builder;
    expr_builder->variable_capacity = 16;
    expr_builder->variables = malloc(expr_builder->variable_capacity * 
                                   sizeof(*expr_builder->variables));
    
    if (!expr_builder->variables) {
        free(expr_builder);
        return NULL;
    }
    
    return expr_builder;
}

void graph_expr_builder_destroy(GraphExprBuilder* expr_builder) {
    if (!expr_builder) return;
    
    for (uint32_t i = 0; i < expr_builder->variable_count; i++) {
        free(expr_builder->variables[i].name);
    }
    free(expr_builder->variables);
    free(expr_builder);
}

uint32_t graph_expr_parse(GraphExprBuilder* expr_builder, const char* expression) {
    // Very basic expression parser - just handles simple cases
    if (!expr_builder || !expression) return 0;
    
    // For now, just return 0 (not implemented)
    // A full implementation would parse the expression string and build the graph
    return 0;
}

uint32_t graph_expr_variable(GraphExprBuilder* expr_builder, const char* name, GraphDataType type) {
    if (!expr_builder || !name) return 0;
    
    // Check if variable already exists
    for (uint32_t i = 0; i < expr_builder->variable_count; i++) {
        if (strcmp(expr_builder->variables[i].name, name) == 0) {
            return expr_builder->variables[i].node_id;
        }
    }
    
    // Create new variable
    if (expr_builder->variable_count >= expr_builder->variable_capacity) {
        expr_builder->variable_capacity *= 2;
        expr_builder->variables = realloc(expr_builder->variables,
                                        expr_builder->variable_capacity * 
                                        sizeof(*expr_builder->variables));
        if (!expr_builder->variables) return 0;
    }
    
    uint32_t node_id = graph_builder_add_input(expr_builder->graph_builder, type, name);
    if (node_id == 0) return 0;
    
    expr_builder->variables[expr_builder->variable_count].name = strdup(name);
    expr_builder->variables[expr_builder->variable_count].node_id = node_id;
    expr_builder->variables[expr_builder->variable_count].type = type;
    expr_builder->variable_count++;
    
    return node_id;
}

uint32_t graph_expr_constant(GraphExprBuilder* expr_builder, GraphValue value) {
    if (!expr_builder) return 0;
    
    return graph_builder_add_constant(expr_builder->graph_builder, value, NULL);
}

// Cleanup
void graph_node_list_destroy(GraphNodeList* list) {
    if (!list) return;
    
    free(list->node_ids);
    list->node_ids = NULL;
    list->count = 0;
}

