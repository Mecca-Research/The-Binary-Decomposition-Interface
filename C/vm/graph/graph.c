
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Default graph configuration
#define DEFAULT_MAX_NODES 1024
#define DEFAULT_MAX_EDGES 2048
#define DEFAULT_MEMORY_POOL_SIZE (1024 * 1024) // 1MB

// Helper macros
#define GRAPH_ALLOC(graph, size) graph_alloc(graph, size)
#define GRAPH_FREE(graph, ptr) graph_free(graph, ptr)

// Static helper functions
static uint32_t next_node_id = 1;
static uint32_t next_graph_id = 1;

static GraphNode* graph_node_create(uint32_t id, GraphNodeType type, const char* name) {
    GraphNode* node = (GraphNode*)calloc(1, sizeof(GraphNode));
    if (!node) return NULL;
    
    node->id = id;
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->max_inputs = 8;
    node->max_outputs = 8;
    node->inputs = (GraphEdge**)calloc(node->max_inputs, sizeof(GraphEdge*));
    node->outputs = (GraphEdge**)calloc(node->max_outputs, sizeof(GraphEdge*));
    node->output_values = (GraphValue*)calloc(node->max_outputs, sizeof(GraphValue));
    
    return node;
}

static void graph_node_destroy(GraphNode* node) {
    if (!node) return;
    
    free(node->name);
    free(node->inputs);
    free(node->outputs);
    free(node->output_values);
    free(node->custom_data);
    free(node);
}

static GraphEdge* graph_edge_create(GraphNode* source, uint32_t source_output,
                                   GraphNode* target, uint32_t target_input,
                                   GraphDataType data_type) {
    GraphEdge* edge = (GraphEdge*)calloc(1, sizeof(GraphEdge));
    if (!edge) return NULL;
    
    edge->source = source;
    edge->target = target;
    edge->source_output = source_output;
    edge->target_input = target_input;
    edge->data_type = data_type;
    
    return edge;
}

static void graph_edge_destroy(GraphEdge* edge) {
    if (!edge) return;
    free(edge);
}

// Core graph API implementation
Graph* graph_create(const char* name) {
    Graph* graph = (Graph*)calloc(1, sizeof(Graph));
    if (!graph) return NULL;
    
    graph->name = name ? strdup(name) : NULL;
    graph->id = next_graph_id++;
    graph->max_nodes = DEFAULT_MAX_NODES;
    graph->max_edges = DEFAULT_MAX_EDGES;
    
    graph->nodes = (GraphNode**)calloc(graph->max_nodes, sizeof(GraphNode*));
    graph->edges = (GraphEdge**)calloc(graph->max_edges, sizeof(GraphEdge*));
    graph->inputs = (GraphNode**)calloc(32, sizeof(GraphNode*));
    graph->outputs = (GraphNode**)calloc(32, sizeof(GraphNode*));
    
    // Initialize memory pool
    graph->memory_pool_size = DEFAULT_MEMORY_POOL_SIZE;
    graph->memory_pool = malloc(graph->memory_pool_size);
    graph->memory_used = 0;
    
    if (!graph->nodes || !graph->edges || !graph->inputs || 
        !graph->outputs || !graph->memory_pool) {
        graph_destroy(graph);
        return NULL;
    }
    
    return graph;
}

void graph_destroy(Graph* graph) {
    if (!graph) return;
    
    // Destroy all nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        graph_node_destroy(graph->nodes[i]);
    }
    
    // Destroy all edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        graph_edge_destroy(graph->edges[i]);
    }
    
    free(graph->name);
    free(graph->nodes);
    free(graph->edges);
    free(graph->inputs);
    free(graph->outputs);
    free(graph->memory_pool);
    free(graph);
}

Graph* graph_clone(const Graph* graph) {
    if (!graph) return NULL;
    
    Graph* clone = graph_create(graph->name);
    if (!clone) return NULL;
    
    // Clone nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* original = graph->nodes[i];
        GraphNode* node = graph_add_node(clone, original->type, original->name);
        if (!node) {
            graph_destroy(clone);
            return NULL;
        }
        node->constant_value = original->constant_value;
        if (original->custom_data && original->custom_data_size > 0) {
            node->custom_data = malloc(original->custom_data_size);
            memcpy(node->custom_data, original->custom_data, original->custom_data_size);
            node->custom_data_size = original->custom_data_size;
        }
    }
    
    // Clone edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* original = graph->edges[i];
        graph_add_edge(clone, original->source->id, original->source_output,
                      original->target->id, original->target_input);
    }
    
    return clone;
}

GraphNode* graph_add_node(Graph* graph, GraphNodeType type, const char* name) {
    if (!graph || graph->node_count >= graph->max_nodes) return NULL;
    
    GraphNode* node = graph_node_create(next_node_id++, type, name);
    if (!node) return NULL;
    
    graph->nodes[graph->node_count++] = node;
    
    // Add to input/output lists if appropriate
    if (type == GRAPH_NODE_INPUT) {
        graph->inputs[graph->input_count++] = node;
    } else if (type == GRAPH_NODE_OUTPUT) {
        graph->outputs[graph->output_count++] = node;
    }
    
    return node;
}

GraphNode* graph_add_constant_node(Graph* graph, GraphValue value, const char* name) {
    GraphNode* node = graph_add_node(graph, GRAPH_NODE_CONSTANT, name);
    if (!node) return NULL;
    
    node->constant_value = value;
    node->is_constant = true;
    node->ready = true;
    node->output_values[0] = value;
    
    return node;
}

GraphNode* graph_add_input_node(Graph* graph, GraphDataType type, const char* name) {
    GraphNode* node = graph_add_node(graph, GRAPH_NODE_INPUT, name);
    if (!node) return NULL;
    
    node->output_values[0].type = type;
    return node;
}

GraphNode* graph_add_output_node(Graph* graph, GraphDataType type, const char* name) {
    GraphNode* node = graph_add_node(graph, GRAPH_NODE_OUTPUT, name);
    if (!node) return NULL;
    
    node->output_values[0].type = type;
    return node;
}

bool graph_remove_node(Graph* graph, uint32_t node_id) {
    if (!graph) return false;
    
    // Find node
    uint32_t node_index = UINT32_MAX;
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i]->id == node_id) {
            node_index = i;
            break;
        }
    }
    
    if (node_index == UINT32_MAX) return false;
    
    GraphNode* node = graph->nodes[node_index];
    
    // Remove all edges connected to this node
    for (uint32_t i = 0; i < graph->edge_count; ) {
        GraphEdge* edge = graph->edges[i];
        if (edge->source->id == node_id || edge->target->id == node_id) {
            graph_edge_destroy(edge);
            // Shift remaining edges
            for (uint32_t j = i; j < graph->edge_count - 1; j++) {
                graph->edges[j] = graph->edges[j + 1];
            }
            graph->edge_count--;
        } else {
            i++;
        }
    }
    
    // Remove from input/output lists
    if (node->type == GRAPH_NODE_INPUT) {
        for (uint32_t i = 0; i < graph->input_count; i++) {
            if (graph->inputs[i] == node) {
                for (uint32_t j = i; j < graph->input_count - 1; j++) {
                    graph->inputs[j] = graph->inputs[j + 1];
                }
                graph->input_count--;
                break;
            }
        }
    } else if (node->type == GRAPH_NODE_OUTPUT) {
        for (uint32_t i = 0; i < graph->output_count; i++) {
            if (graph->outputs[i] == node) {
                for (uint32_t j = i; j < graph->output_count - 1; j++) {
                    graph->outputs[j] = graph->outputs[j + 1];
                }
                graph->output_count--;
                break;
            }
        }
    }
    
    // Destroy node
    graph_node_destroy(node);
    
    // Shift remaining nodes
    for (uint32_t i = node_index; i < graph->node_count - 1; i++) {
        graph->nodes[i] = graph->nodes[i + 1];
    }
    graph->node_count--;
    
    return true;
}

GraphNode* graph_get_node(const Graph* graph, uint32_t node_id) {
    if (!graph) return NULL;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i]->id == node_id) {
            return graph->nodes[i];
        }
    }
    
    return NULL;
}

GraphNode* graph_find_node(const Graph* graph, const char* name) {
    if (!graph || !name) return NULL;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i]->name && strcmp(graph->nodes[i]->name, name) == 0) {
            return graph->nodes[i];
        }
    }
    
    return NULL;
}

GraphEdge* graph_add_edge(Graph* graph, uint32_t source_id, uint32_t source_output,
                         uint32_t target_id, uint32_t target_input) {
    if (!graph || graph->edge_count >= graph->max_edges) return NULL;
    
    GraphNode* source = graph_get_node(graph, source_id);
    GraphNode* target = graph_get_node(graph, target_id);
    
    if (!source || !target) return NULL;
    
    // Validate output/input indices
    if (source_output >= source->max_outputs || target_input >= target->max_inputs) {
        return NULL;
    }
    
    // Create edge
    GraphEdge* edge = graph_edge_create(source, source_output, target, target_input, 
                                       GRAPH_TYPE_F64); // Default type
    if (!edge) return NULL;
    
    // Add to graph
    graph->edges[graph->edge_count++] = edge;
    
    // Update node connections
    source->outputs[source_output] = edge;
    target->inputs[target_input] = edge;
    
    if (source_output >= source->output_count) {
        source->output_count = source_output + 1;
    }
    if (target_input >= target->input_count) {
        target->input_count = target_input + 1;
    }
    
    // Update dependency count
    target->dependency_count++;
    target->remaining_dependencies++;
    
    return edge;
}

bool graph_remove_edge(Graph* graph, uint32_t source_id, uint32_t source_output,
                      uint32_t target_id, uint32_t target_input) {
    if (!graph) return false;
    
    // Find edge
    uint32_t edge_index = UINT32_MAX;
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge->source->id == source_id && edge->source_output == source_output &&
            edge->target->id == target_id && edge->target_input == target_input) {
            edge_index = i;
            break;
        }
    }
    
    if (edge_index == UINT32_MAX) return false;
    
    GraphEdge* edge = graph->edges[edge_index];
    
    // Update node connections
    edge->source->outputs[edge->source_output] = NULL;
    edge->target->inputs[edge->target_input] = NULL;
    edge->target->dependency_count--;
    edge->target->remaining_dependencies--;
    
    // Destroy edge
    graph_edge_destroy(edge);
    
    // Shift remaining edges
    for (uint32_t i = edge_index; i < graph->edge_count - 1; i++) {
        graph->edges[i] = graph->edges[i + 1];
    }
    graph->edge_count--;
    
    return true;
}

bool graph_has_edge(const Graph* graph, uint32_t source_id, uint32_t target_id) {
    if (!graph) return false;
    
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge->source->id == source_id && edge->target->id == target_id) {
            return true;
        }
    }
    
    return false;
}

bool graph_validate(const Graph* graph, char** error_message) {
    if (!graph) {
        if (error_message) *error_message = strdup("Graph is NULL");
        return false;
    }
    
    // Check for cycles
    if (graph_has_cycles(graph)) {
        if (error_message) *error_message = strdup("Graph contains cycles");
        return false;
    }
    
    // Validate all edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (!edge->source || !edge->target) {
            if (error_message) *error_message = strdup("Edge has NULL source or target");
            return false;
        }
        
        if (edge->source_output >= edge->source->max_outputs ||
            edge->target_input >= edge->target->max_inputs) {
            if (error_message) *error_message = strdup("Edge has invalid input/output index");
            return false;
        }
    }
    
    return true;
}

bool graph_has_cycles(const Graph* graph) {
    if (!graph) return false;
    
    // Use DFS to detect cycles
    enum { WHITE, GRAY, BLACK };
    uint8_t* colors = (uint8_t*)calloc(graph->node_count, sizeof(uint8_t));
    if (!colors) return false;
    
    bool has_cycle = false;
    
    // DFS helper function (implemented as iterative to avoid stack overflow)
    for (uint32_t start = 0; start < graph->node_count && !has_cycle; start++) {
        if (colors[start] != WHITE) continue;
        
        // Stack for DFS
        uint32_t* stack = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
        int stack_top = 0;
        
        stack[stack_top++] = start;
        colors[start] = GRAY;
        
        while (stack_top > 0 && !has_cycle) {
            uint32_t current_idx = stack[--stack_top];
            GraphNode* current = graph->nodes[current_idx];
            
            // Check all outgoing edges
            for (uint32_t i = 0; i < current->output_count; i++) {
                GraphEdge* edge = current->outputs[i];
                if (!edge) continue;
                
                // Find target node index
                uint32_t target_idx = UINT32_MAX;
                for (uint32_t j = 0; j < graph->node_count; j++) {
                    if (graph->nodes[j] == edge->target) {
                        target_idx = j;
                        break;
                    }
                }
                
                if (target_idx == UINT32_MAX) continue;
                
                if (colors[target_idx] == GRAY) {
                    has_cycle = true;
                    break;
                } else if (colors[target_idx] == WHITE) {
                    colors[target_idx] = GRAY;
                    stack[stack_top++] = target_idx;
                }
            }
            
            colors[current_idx] = BLACK;
        }
        
        free(stack);
    }
    
    free(colors);
    return has_cycle;
}

bool graph_is_dag(const Graph* graph) {
    return !graph_has_cycles(graph);
}

uint32_t* graph_topological_sort(const Graph* graph, uint32_t* count) {
    if (!graph || !count) return NULL;
    
    *count = 0;
    if (graph->node_count == 0) return NULL;
    
    // Kahn's algorithm
    uint32_t* in_degree = (uint32_t*)calloc(graph->node_count, sizeof(uint32_t));
    uint32_t* result = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
    uint32_t* queue = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
    
    if (!in_degree || !result || !queue) {
        free(in_degree);
        free(result);
        free(queue);
        return NULL;
    }
    
    // Calculate in-degrees
    for (uint32_t i = 0; i < graph->node_count; i++) {
        in_degree[i] = graph->nodes[i]->dependency_count;
    }
    
    // Find nodes with no incoming edges
    uint32_t queue_front = 0, queue_back = 0;
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (in_degree[i] == 0) {
            queue[queue_back++] = i;
        }
    }
    
    // Process queue
    while (queue_front < queue_back) {
        uint32_t current_idx = queue[queue_front++];
        result[(*count)++] = graph->nodes[current_idx]->id;
        
        GraphNode* current = graph->nodes[current_idx];
        
        // Reduce in-degree of neighbors
        for (uint32_t i = 0; i < current->output_count; i++) {
            GraphEdge* edge = current->outputs[i];
            if (!edge) continue;
            
            // Find target node index
            uint32_t target_idx = UINT32_MAX;
            for (uint32_t j = 0; j < graph->node_count; j++) {
                if (graph->nodes[j] == edge->target) {
                    target_idx = j;
                    break;
                }
            }
            
            if (target_idx != UINT32_MAX) {
                in_degree[target_idx]--;
                if (in_degree[target_idx] == 0) {
                    queue[queue_back++] = target_idx;
                }
            }
        }
    }
    
    free(in_degree);
    free(queue);
    
    // Check if all nodes were processed (no cycles)
    if (*count != graph->node_count) {
        free(result);
        *count = 0;
        return NULL;
    }
    
    return result;
}

uint32_t graph_get_depth(const Graph* graph) {
    if (!graph) return 0;
    
    uint32_t max_depth = 0;
    
    // Calculate depth for each node using BFS
    for (uint32_t i = 0; i < graph->input_count; i++) {
        GraphNode* input = graph->inputs[i];
        
        // BFS from this input
        uint32_t* depths = (uint32_t*)calloc(graph->node_count, sizeof(uint32_t));
        bool* visited = (bool*)calloc(graph->node_count, sizeof(bool));
        uint32_t* queue = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
        
        if (!depths || !visited || !queue) {
            free(depths);
            free(visited);
            free(queue);
            continue;
        }
        
        uint32_t queue_front = 0, queue_back = 0;
        
        // Find input node index
        uint32_t input_idx = UINT32_MAX;
        for (uint32_t j = 0; j < graph->node_count; j++) {
            if (graph->nodes[j] == input) {
                input_idx = j;
                break;
            }
        }
        
        if (input_idx == UINT32_MAX) {
            free(depths);
            free(visited);
            free(queue);
            continue;
        }
        
        queue[queue_back++] = input_idx;
        visited[input_idx] = true;
        depths[input_idx] = 0;
        
        while (queue_front < queue_back) {
            uint32_t current_idx = queue[queue_front++];
            GraphNode* current = graph->nodes[current_idx];
            
            for (uint32_t j = 0; j < current->output_count; j++) {
                GraphEdge* edge = current->outputs[j];
                if (!edge) continue;
                
                // Find target node index
                uint32_t target_idx = UINT32_MAX;
                for (uint32_t k = 0; k < graph->node_count; k++) {
                    if (graph->nodes[k] == edge->target) {
                        target_idx = k;
                        break;
                    }
                }
                
                if (target_idx != UINT32_MAX && !visited[target_idx]) {
                    visited[target_idx] = true;
                    depths[target_idx] = depths[current_idx] + 1;
                    queue[queue_back++] = target_idx;
                    
                    if (depths[target_idx] > max_depth) {
                        max_depth = depths[target_idx];
                    }
                }
            }
        }
        
        free(depths);
        free(visited);
        free(queue);
    }
    
    return max_depth + 1;
}

uint32_t graph_get_width(const Graph* graph) {
    if (!graph) return 0;
    
    // Calculate maximum number of nodes at any depth level
    uint32_t depth = graph_get_depth(graph);
    if (depth == 0) return 0;
    
    uint32_t* level_counts = (uint32_t*)calloc(depth, sizeof(uint32_t));
    if (!level_counts) return 0;
    
    // Count nodes at each level (simplified approach)
    for (uint32_t i = 0; i < graph->node_count; i++) {
        // For simplicity, distribute nodes evenly across levels
        uint32_t level = i % depth;
        level_counts[level]++;
    }
    
    uint32_t max_width = 0;
    for (uint32_t i = 0; i < depth; i++) {
        if (level_counts[i] > max_width) {
            max_width = level_counts[i];
        }
    }
    
    free(level_counts);
    return max_width;
}

void graph_get_stats(const Graph* graph, GraphStats* stats) {
    if (!graph || !stats) return;
    
    memset(stats, 0, sizeof(GraphStats));
    
    stats->total_nodes = graph->node_count;
    stats->total_edges = graph->edge_count;
    stats->input_nodes = graph->input_count;
    stats->output_nodes = graph->output_count;
    stats->total_executions = graph->execution_count;
    stats->total_execution_time_ns = graph->total_execution_time_ns;
    
    if (stats->total_executions > 0) {
        stats->average_execution_time_ns = 
            (double)stats->total_execution_time_ns / stats->total_executions;
    }
    
    // Count node types
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        
        switch (node->type) {
            case GRAPH_NODE_CONSTANT:
                stats->constant_nodes++;
                break;
            case GRAPH_NODE_INPUT:
            case GRAPH_NODE_OUTPUT:
                // Already counted above
                break;
            case GRAPH_NODE_BRANCH:
            case GRAPH_NODE_PHI:
            case GRAPH_NODE_MERGE:
            case GRAPH_NODE_LOOP_BEGIN:
            case GRAPH_NODE_LOOP_END:
                stats->control_nodes++;
                break;
            default:
                stats->operation_nodes++;
                break;
        }
        
        if (node->is_dead) {
            stats->dead_nodes++;
        }
    }
}

// Utility functions
const char* graph_node_type_to_string(GraphNodeType type) {
    switch (type) {
        case GRAPH_NODE_CONSTANT: return "CONSTANT";
        case GRAPH_NODE_INPUT: return "INPUT";
        case GRAPH_NODE_OUTPUT: return "OUTPUT";
        case GRAPH_NODE_ADD: return "ADD";
        case GRAPH_NODE_SUB: return "SUB";
        case GRAPH_NODE_MUL: return "MUL";
        case GRAPH_NODE_DIV: return "DIV";
        case GRAPH_NODE_MOD: return "MOD";
        case GRAPH_NODE_AND: return "AND";
        case GRAPH_NODE_OR: return "OR";
        case GRAPH_NODE_XOR: return "XOR";
        case GRAPH_NODE_NOT: return "NOT";
        case GRAPH_NODE_EQ: return "EQ";
        case GRAPH_NODE_NE: return "NE";
        case GRAPH_NODE_LT: return "LT";
        case GRAPH_NODE_LE: return "LE";
        case GRAPH_NODE_GT: return "GT";
        case GRAPH_NODE_GE: return "GE";
        case GRAPH_NODE_BRANCH: return "BRANCH";
        case GRAPH_NODE_PHI: return "PHI";
        case GRAPH_NODE_CALL: return "CALL";
        case GRAPH_NODE_LOAD: return "LOAD";
        case GRAPH_NODE_STORE: return "STORE";
        case GRAPH_NODE_ALLOC: return "ALLOC";
        case GRAPH_NODE_CAST: return "CAST";
        case GRAPH_NODE_SELECT: return "SELECT";
        case GRAPH_NODE_MERGE: return "MERGE";
        case GRAPH_NODE_LOOP_BEGIN: return "LOOP_BEGIN";
        case GRAPH_NODE_LOOP_END: return "LOOP_END";
        case GRAPH_NODE_RETURN: return "RETURN";
        case GRAPH_NODE_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

const char* graph_data_type_to_string(GraphDataType type) {
    switch (type) {
        case GRAPH_TYPE_VOID: return "void";
        case GRAPH_TYPE_I8: return "i8";
        case GRAPH_TYPE_I16: return "i16";
        case GRAPH_TYPE_I32: return "i32";
        case GRAPH_TYPE_I64: return "i64";
        case GRAPH_TYPE_F32: return "f32";
        case GRAPH_TYPE_F64: return "f64";
        case GRAPH_TYPE_PTR: return "ptr";
        case GRAPH_TYPE_BOOL: return "bool";
        case GRAPH_TYPE_CONTROL: return "control";
        default: return "unknown";
    }
}

GraphValue graph_value_create(GraphDataType type, const void* data) {
    GraphValue value = {0};
    value.type = type;
    
    if (!data) return value;
    
    switch (type) {
        case GRAPH_TYPE_I8:
            value.data.i8 = *(const int8_t*)data;
            break;
        case GRAPH_TYPE_I16:
            value.data.i16 = *(const int16_t*)data;
            break;
        case GRAPH_TYPE_I32:
            value.data.i32 = *(const int32_t*)data;
            break;
        case GRAPH_TYPE_I64:
            value.data.i64 = *(const int64_t*)data;
            break;
        case GRAPH_TYPE_F32:
            value.data.f32 = *(const float*)data;
            break;
        case GRAPH_TYPE_F64:
            value.data.f64 = *(const double*)data;
            break;
        case GRAPH_TYPE_PTR:
            value.data.ptr = *(void* const*)data;
            break;
        case GRAPH_TYPE_BOOL:
            value.data.boolean = *(const bool*)data;
            break;
        default:
            break;
    }
    
    return value;
}

bool graph_value_equals(const GraphValue* a, const GraphValue* b) {
    if (!a || !b || a->type != b->type) return false;
    
    switch (a->type) {
        case GRAPH_TYPE_I8: return a->data.i8 == b->data.i8;
        case GRAPH_TYPE_I16: return a->data.i16 == b->data.i16;
        case GRAPH_TYPE_I32: return a->data.i32 == b->data.i32;
        case GRAPH_TYPE_I64: return a->data.i64 == b->data.i64;
        case GRAPH_TYPE_F32: return a->data.f32 == b->data.f32;
        case GRAPH_TYPE_F64: return a->data.f64 == b->data.f64;
        case GRAPH_TYPE_PTR: return a->data.ptr == b->data.ptr;
        case GRAPH_TYPE_BOOL: return a->data.boolean == b->data.boolean;
        default: return true; // void and control types
    }
}

char* graph_value_to_string(const GraphValue* value) {
    if (!value) return strdup("null");
    
    char* result = (char*)malloc(64);
    if (!result) return NULL;
    
    switch (value->type) {
        case GRAPH_TYPE_I8:
            snprintf(result, 64, "%d", value->data.i8);
            break;
        case GRAPH_TYPE_I16:
            snprintf(result, 64, "%d", value->data.i16);
            break;
        case GRAPH_TYPE_I32:
            snprintf(result, 64, "%d", value->data.i32);
            break;
        case GRAPH_TYPE_I64:
            snprintf(result, 64, "%ld", value->data.i64);
            break;
        case GRAPH_TYPE_F32:
            snprintf(result, 64, "%.6f", value->data.f32);
            break;
        case GRAPH_TYPE_F64:
            snprintf(result, 64, "%.6f", value->data.f64);
            break;
        case GRAPH_TYPE_PTR:
            snprintf(result, 64, "%p", value->data.ptr);
            break;
        case GRAPH_TYPE_BOOL:
            snprintf(result, 64, "%s", value->data.boolean ? "true" : "false");
            break;
        case GRAPH_TYPE_VOID:
            snprintf(result, 64, "void");
            break;
        case GRAPH_TYPE_CONTROL:
            snprintf(result, 64, "control");
            break;
        default:
            snprintf(result, 64, "unknown");
            break;
    }
    
    return result;
}

// Memory management helpers
void* graph_alloc(Graph* graph, size_t size) {
    if (!graph || !graph->memory_pool) return malloc(size);
    
    // Align to 8-byte boundary
    size = (size + 7) & ~7;
    
    if (graph->memory_used + size > graph->memory_pool_size) {
        // Fall back to regular malloc if pool is full
        return malloc(size);
    }
    
    void* ptr = (char*)graph->memory_pool + graph->memory_used;
    graph->memory_used += size;
    return ptr;
}

void graph_free(Graph* graph, void* ptr) {
    if (!graph || !ptr) return;
    
    // Check if pointer is in memory pool
    if (ptr >= graph->memory_pool && 
        ptr < (char*)graph->memory_pool + graph->memory_pool_size) {
        // Memory pool allocation - no individual free
        return;
    }
    
    // Regular malloc allocation
    free(ptr);
}

void graph_reset_memory_pool(Graph* graph) {
    if (!graph) return;
    graph->memory_used = 0;
}

// Serialization stubs (basic implementation)
bool graph_save_to_file(const Graph* graph, const char* filename) {
    if (!graph || !filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "# BDI Graph: %s\n", graph->name ? graph->name : "unnamed");
    fprintf(file, "nodes: %u\n", graph->node_count);
    fprintf(file, "edges: %u\n", graph->edge_count);
    
    // Write nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        fprintf(file, "node %u %s %s\n", node->id, 
                graph_node_type_to_string(node->type),
                node->name ? node->name : "unnamed");
    }
    
    // Write edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        fprintf(file, "edge %u %u %u %u\n", 
                edge->source->id, edge->source_output,
                edge->target->id, edge->target_input);
    }
    
    fclose(file);
    return true;
}

Graph* graph_load_from_file(const char* filename) {
    if (!filename) return NULL;
    
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;
    
    Graph* graph = graph_create("loaded");
    if (!graph) {
        fclose(file);
        return NULL;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue; // Comment
        
        if (strncmp(line, "node", 4) == 0) {
            uint32_t id;
            char type_str[32], name[64];
            if (sscanf(line, "node %u %s %s", &id, type_str, name) == 3) {
                // Find node type
                GraphNodeType type = GRAPH_NODE_CUSTOM;
                for (int t = 0; t <= GRAPH_NODE_CUSTOM; t++) {
                    if (strcmp(graph_node_type_to_string(t), type_str) == 0) {
                        type = t;
                        break;
                    }
                }
                graph_add_node(graph, type, name);
            }
        } else if (strncmp(line, "edge", 4) == 0) {
            uint32_t src_id, src_out, tgt_id, tgt_in;
            if (sscanf(line, "edge %u %u %u %u", &src_id, &src_out, &tgt_id, &tgt_in) == 4) {
                graph_add_edge(graph, src_id, src_out, tgt_id, tgt_in);
            }
        }
    }
    
    fclose(file);
    return graph;
}

char* graph_to_dot(const Graph* graph) {
    if (!graph) return NULL;
    
    size_t buffer_size = 4096;
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t pos = 0;
    pos += snprintf(buffer + pos, buffer_size - pos, 
                   "digraph \"%s\" {\n", graph->name ? graph->name : "graph");
    
    // Write nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        pos += snprintf(buffer + pos, buffer_size - pos,
                       "  n%u [label=\"%s\\n%s\"];\n",
                       node->id, node->name ? node->name : "",
                       graph_node_type_to_string(node->type));
        
        if (pos >= buffer_size - 256) {
            buffer_size *= 2;
            buffer = (char*)realloc(buffer, buffer_size);
            if (!buffer) return NULL;
        }
    }
    
    // Write edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        pos += snprintf(buffer + pos, buffer_size - pos,
                       "  n%u -> n%u;\n",
                       edge->source->id, edge->target->id);
        
        if (pos >= buffer_size - 256) {
            buffer_size *= 2;
            buffer = (char*)realloc(buffer, buffer_size);
            if (!buffer) return NULL;
        }
    }
    
    pos += snprintf(buffer + pos, buffer_size - pos, "}\n");
    
    return buffer;
}

char* graph_to_json(const Graph* graph) {
    if (!graph) return NULL;
    
    size_t buffer_size = 8192;
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t pos = 0;
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "{\n  \"name\": \"%s\",\n  \"id\": %u,\n",
                   graph->name ? graph->name : "unnamed", graph->id);
    
    // Write nodes
    pos += snprintf(buffer + pos, buffer_size - pos, "  \"nodes\": [\n");
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        pos += snprintf(buffer + pos, buffer_size - pos,
                       "    {\"id\": %u, \"type\": \"%s\", \"name\": \"%s\"}%s\n",
                       node->id, graph_node_type_to_string(node->type),
                       node->name ? node->name : "",
                       i < graph->node_count - 1 ? "," : "");
        
        if (pos >= buffer_size - 512) {
            buffer_size *= 2;
            buffer = (char*)realloc(buffer, buffer_size);
            if (!buffer) return NULL;
        }
    }
    pos += snprintf(buffer + pos, buffer_size - pos, "  ],\n");
    
    // Write edges
    pos += snprintf(buffer + pos, buffer_size - pos, "  \"edges\": [\n");
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        pos += snprintf(buffer + pos, buffer_size - pos,
                       "    {\"source\": %u, \"target\": %u, \"source_output\": %u, \"target_input\": %u}%s\n",
                       edge->source->id, edge->target->id,
                       edge->source_output, edge->target_input,
                       i < graph->edge_count - 1 ? "," : "");
        
        if (pos >= buffer_size - 512) {
            buffer_size *= 2;
            buffer = (char*)realloc(buffer, buffer_size);
            if (!buffer) return NULL;
        }
    }
    pos += snprintf(buffer + pos, buffer_size - pos, "  ]\n}\n");
    
    return buffer;
}

