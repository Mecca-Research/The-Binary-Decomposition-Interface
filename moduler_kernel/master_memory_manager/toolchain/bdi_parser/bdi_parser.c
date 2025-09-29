
/**
 * @file bdi_parser.c
 * @brief BDI Dot/Filament Graph Parser Implementation
 * 
 * Phase 2 Master Memory Manager - Complete Toolchain
 * BDI Dot/Filament graph parser with constraints implementation
 */

#include "bdi_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// String constants for types
static const char* node_type_strings[] = {
    "operation", "data", "control", "memory", "register", 
    "constant", "function", "branch", "loop", "barrier"
};

static const char* edge_type_strings[] = {
    "data_flow", "control_flow", "dependency", "memory_order", "timing", "resource"
};

static const char* constraint_type_strings[] = {
    "latency", "throughput", "code_size", "memory_usage", "register_usage",
    "instruction_set", "calling_convention", "alignment", "power", "thermal"
};

static const char* calling_convention_strings[] = {
    "cdecl", "stdcall", "fastcall", "vectorcall", "sysv_amd64", "ms_x64", "custom"
};

static const char* memory_layout_strings[] = {
    "linear", "segmented", "paged", "virtual", "numa", "cache_aware"
};

/**
 * @brief Initialize parser context
 */
int bdi_parser_init(bdi_parser_context_t* context) {
    if (!context) return -1;
    
    memset(context, 0, sizeof(*context));
    context->current_line = 1;
    context->current_column = 1;
    
    return 0;
}

/**
 * @brief Cleanup parser context
 */
int bdi_parser_cleanup(bdi_parser_context_t* context) {
    if (!context) return -1;
    
    // Clear any allocated resources
    memset(context, 0, sizeof(*context));
    
    return 0;
}

/**
 * @brief Initialize BDI graph
 */
int bdi_graph_init(bdi_graph_t* graph, const char* name) {
    if (!graph) return -1;
    
    memset(graph, 0, sizeof(*graph));
    
    if (name) {
        strncpy(graph->name, name, sizeof(graph->name) - 1);
    }
    
    strcpy(graph->version, "1.0");
    graph->creation_time = time(NULL);
    graph->modification_time = graph->creation_time;
    
    return 0;
}

/**
 * @brief Cleanup BDI graph
 */
int bdi_graph_cleanup(bdi_graph_t* graph) {
    if (!graph) return -1;
    
    // Clear all data
    memset(graph, 0, sizeof(*graph));
    
    return 0;
}

/**
 * @brief Skip whitespace in input
 */
static void skip_whitespace(bdi_parser_context_t* context) {
    while (context->current_position < context->input_size) {
        char c = context->input_buffer[context->current_position];
        
        if (c == ' ' || c == '\t') {
            context->current_position++;
            context->current_column++;
        } else if (c == '\n') {
            context->current_position++;
            context->current_line++;
            context->current_column = 1;
        } else if (c == '\r') {
            context->current_position++;
            // Handle \r\n
            if (context->current_position < context->input_size && 
                context->input_buffer[context->current_position] == '\n') {
                context->current_position++;
            }
            context->current_line++;
            context->current_column = 1;
        } else {
            break;
        }
    }
}

/**
 * @brief Skip comments
 */
static void skip_comments(bdi_parser_context_t* context) {
    if (context->current_position < context->input_size) {
        char c = context->input_buffer[context->current_position];
        
        // Single line comment
        if (c == '#' || (c == '/' && context->current_position + 1 < context->input_size && 
                        context->input_buffer[context->current_position + 1] == '/')) {
            while (context->current_position < context->input_size && 
                   context->input_buffer[context->current_position] != '\n') {
                context->current_position++;
            }
        }
        // Multi-line comment
        else if (c == '/' && context->current_position + 1 < context->input_size && 
                 context->input_buffer[context->current_position + 1] == '*') {
            context->current_position += 2;
            while (context->current_position + 1 < context->input_size) {
                if (context->input_buffer[context->current_position] == '*' &&
                    context->input_buffer[context->current_position + 1] == '/') {
                    context->current_position += 2;
                    break;
                }
                if (context->input_buffer[context->current_position] == '\n') {
                    context->current_line++;
                    context->current_column = 1;
                } else {
                    context->current_column++;
                }
                context->current_position++;
            }
        }
    }
}

/**
 * @brief Read identifier from input
 */
static int read_identifier(bdi_parser_context_t* context, char* buffer, size_t buffer_size) {
    size_t length = 0;
    
    while (context->current_position < context->input_size && length < buffer_size - 1) {
        char c = context->input_buffer[context->current_position];
        
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            buffer[length++] = c;
            context->current_position++;
            context->current_column++;
        } else {
            break;
        }
    }
    
    buffer[length] = '\0';
    return length > 0 ? 0 : -1;
}

/**
 * @brief Read string literal from input
 */
static int read_string_literal(bdi_parser_context_t* context, char* buffer, size_t buffer_size) {
    if (context->current_position >= context->input_size || 
        context->input_buffer[context->current_position] != '"') {
        return -1;
    }
    
    context->current_position++; // Skip opening quote
    context->current_column++;
    
    size_t length = 0;
    while (context->current_position < context->input_size && length < buffer_size - 1) {
        char c = context->input_buffer[context->current_position];
        
        if (c == '"') {
            context->current_position++; // Skip closing quote
            context->current_column++;
            break;
        } else if (c == '\\' && context->current_position + 1 < context->input_size) {
            // Handle escape sequences
            context->current_position++;
            context->current_column++;
            char next = context->input_buffer[context->current_position];
            
            switch (next) {
                case 'n': buffer[length++] = '\n'; break;
                case 't': buffer[length++] = '\t'; break;
                case 'r': buffer[length++] = '\r'; break;
                case '\\': buffer[length++] = '\\'; break;
                case '"': buffer[length++] = '"'; break;
                default: buffer[length++] = next; break;
            }
            
            context->current_position++;
            context->current_column++;
        } else {
            buffer[length++] = c;
            context->current_position++;
            if (c == '\n') {
                context->current_line++;
                context->current_column = 1;
            } else {
                context->current_column++;
            }
        }
    }
    
    buffer[length] = '\0';
    return 0;
}

/**
 * @brief Read number from input
 */
static int read_number(bdi_parser_context_t* context, uint32_t* value) {
    char buffer[32];
    size_t length = 0;
    
    while (context->current_position < context->input_size && length < sizeof(buffer) - 1) {
        char c = context->input_buffer[context->current_position];
        
        if (isdigit(c) || c == '.' || c == 'x' || c == 'X' || 
            (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            buffer[length++] = c;
            context->current_position++;
            context->current_column++;
        } else {
            break;
        }
    }
    
    buffer[length] = '\0';
    
    if (length > 0) {
        *value = strtoul(buffer, NULL, 0);
        return 0;
    }
    
    return -1;
}

/**
 * @brief Parse node definition
 */
static int parse_node(bdi_parser_context_t* context, bdi_graph_t* graph) {
    char name[BDI_MAX_NAME_LENGTH];
    char type_str[64];
    
    // Read node name
    if (read_identifier(context, name, sizeof(name)) != 0) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Expected node name at line %u", context->current_line);
        context->has_error = true;
        return -1;
    }
    
    skip_whitespace(context);
    skip_comments(context);
    
    // Expect '['
    if (context->current_position >= context->input_size || 
        context->input_buffer[context->current_position] != '[') {
        snprintf(context->error_message, sizeof(context->error_message),
                "Expected '[' after node name at line %u", context->current_line);
        context->has_error = true;
        return -1;
    }
    
    context->current_position++;
    context->current_column++;
    
    // Parse node attributes
    bdi_node_type_t node_type = BDI_NODE_OPERATION;
    uint32_t latency = 1;
    uint32_t throughput = 1;
    uint32_t code_size = 4;
    
    while (context->current_position < context->input_size) {
        skip_whitespace(context);
        skip_comments(context);
        
        if (context->current_position >= context->input_size) break;
        
        char c = context->input_buffer[context->current_position];
        if (c == ']') {
            context->current_position++;
            context->current_column++;
            break;
        }
        
        // Read attribute name
        char attr_name[64];
        if (read_identifier(context, attr_name, sizeof(attr_name)) != 0) {
            break;
        }
        
        skip_whitespace(context);
        
        // Expect '='
        if (context->current_position >= context->input_size || 
            context->input_buffer[context->current_position] != '=') {
            continue;
        }
        
        context->current_position++;
        context->current_column++;
        skip_whitespace(context);
        
        // Read attribute value
        char attr_value[256];
        if (context->current_position < context->input_size && 
            context->input_buffer[context->current_position] == '"') {
            read_string_literal(context, attr_value, sizeof(attr_value));
        } else {
            read_identifier(context, attr_value, sizeof(attr_value));
        }
        
        // Process known attributes
        if (strcmp(attr_name, "type") == 0) {
            for (int i = 0; i < 10; i++) {
                if (strcmp(attr_value, node_type_strings[i]) == 0) {
                    node_type = (bdi_node_type_t)i;
                    break;
                }
            }
        } else if (strcmp(attr_name, "latency") == 0) {
            latency = strtoul(attr_value, NULL, 0);
        } else if (strcmp(attr_name, "throughput") == 0) {
            throughput = strtoul(attr_value, NULL, 0);
        } else if (strcmp(attr_name, "code_size") == 0) {
            code_size = strtoul(attr_value, NULL, 0);
        }
        
        skip_whitespace(context);
        
        // Skip comma if present
        if (context->current_position < context->input_size && 
            context->input_buffer[context->current_position] == ',') {
            context->current_position++;
            context->current_column++;
        }
    }
    
    // Add node to graph
    uint32_t node_id = bdi_add_node(graph, node_type, name);
    if (node_id == UINT32_MAX) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Failed to add node '%s'", name);
        context->has_error = true;
        return -1;
    }
    
    bdi_node_t* node = bdi_get_node(graph, node_id);
    if (node) {
        node->latency_cycles = latency;
        node->throughput_uops = throughput;
        node->code_size_bytes = code_size;
    }
    
    // Add to symbol table
    if (context->symbol_count < 1024) {
        strncpy(context->symbols[context->symbol_count].name, name, 63);
        context->symbols[context->symbol_count].node_id = node_id;
        context->symbol_count++;
    }
    
    return 0;
}

/**
 * @brief Parse edge definition
 */
static int parse_edge(bdi_parser_context_t* context, bdi_graph_t* graph) {
    char source_name[BDI_MAX_NAME_LENGTH];
    char target_name[BDI_MAX_NAME_LENGTH];
    
    // Read source node name
    if (read_identifier(context, source_name, sizeof(source_name)) != 0) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Expected source node name at line %u", context->current_line);
        context->has_error = true;
        return -1;
    }
    
    skip_whitespace(context);
    
    // Expect '->'
    if (context->current_position + 1 >= context->input_size || 
        context->input_buffer[context->current_position] != '-' ||
        context->input_buffer[context->current_position + 1] != '>') {
        snprintf(context->error_message, sizeof(context->error_message),
                "Expected '->' after source node at line %u", context->current_line);
        context->has_error = true;
        return -1;
    }
    
    context->current_position += 2;
    context->current_column += 2;
    skip_whitespace(context);
    
    // Read target node name
    if (read_identifier(context, target_name, sizeof(target_name)) != 0) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Expected target node name at line %u", context->current_line);
        context->has_error = true;
        return -1;
    }
    
    // Find source and target nodes
    bdi_node_t* source_node = bdi_find_node_by_name(graph, source_name);
    bdi_node_t* target_node = bdi_find_node_by_name(graph, target_name);
    
    if (!source_node) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Source node '%s' not found at line %u", source_name, context->current_line);
        context->has_error = true;
        return -1;
    }
    
    if (!target_node) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Target node '%s' not found at line %u", target_name, context->current_line);
        context->has_error = true;
        return -1;
    }
    
    // Add edge
    uint32_t edge_id = bdi_add_edge(graph, BDI_EDGE_DATA_FLOW, source_node->id, target_node->id);
    if (edge_id == UINT32_MAX) {
        snprintf(context->error_message, sizeof(context->error_message),
                "Failed to add edge from '%s' to '%s'", source_name, target_name);
        context->has_error = true;
        return -1;
    }
    
    return 0;
}

/**
 * @brief Parse BDI graph from string
 */
int bdi_parse_string(const char* input, bdi_graph_t* graph) {
    if (!input || !graph) return -1;
    
    bdi_parser_context_t context;
    if (bdi_parser_init(&context) != 0) return -1;
    
    context.input_buffer = input;
    context.input_size = strlen(input);
    context.graph = graph;
    
    // Parse input
    while (context.current_position < context.input_size && !context.has_error) {
        skip_whitespace(&context);
        skip_comments(&context);
        
        if (context.current_position >= context.input_size) break;
        
        char keyword[64];
        size_t saved_pos = context.current_position;
        
        if (read_identifier(&context, keyword, sizeof(keyword)) == 0) {
            if (strcmp(keyword, "node") == 0) {
                skip_whitespace(&context);
                parse_node(&context, graph);
            } else if (strcmp(keyword, "edge") == 0) {
                skip_whitespace(&context);
                parse_edge(&context, graph);
            } else {
                // Try to parse as node definition (name without 'node' keyword)
                context.current_position = saved_pos;
                parse_node(&context, graph);
            }
        } else {
            // Skip unknown content
            context.current_position++;
            context.current_column++;
        }
    }
    
    int result = context.has_error ? -1 : 0;
    bdi_parser_cleanup(&context);
    
    return result;
}

/**
 * @brief Add node to graph
 */
uint32_t bdi_add_node(bdi_graph_t* graph, bdi_node_type_t type, const char* name) {
    if (!graph || graph->node_count >= BDI_MAX_NODES) return UINT32_MAX;
    
    uint32_t node_id = graph->node_count;
    bdi_node_t* node = &graph->nodes[node_id];
    
    memset(node, 0, sizeof(*node));
    node->id = node_id;
    node->type = type;
    
    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
    }
    
    // Set default values
    node->latency_cycles = 1;
    node->throughput_uops = 1;
    node->code_size_bytes = 4;
    node->memory_footprint = 0;
    node->register_pressure = 1;
    
    graph->node_count++;
    graph->modification_time = time(NULL);
    
    return node_id;
}

/**
 * @brief Add edge to graph
 */
uint32_t bdi_add_edge(bdi_graph_t* graph, bdi_edge_type_t type, uint32_t source, uint32_t target) {
    if (!graph || graph->edge_count >= BDI_MAX_EDGES) return UINT32_MAX;
    if (source >= graph->node_count || target >= graph->node_count) return UINT32_MAX;
    
    uint32_t edge_id = graph->edge_count;
    bdi_edge_t* edge = &graph->edges[edge_id];
    
    memset(edge, 0, sizeof(*edge));
    edge->id = edge_id;
    edge->type = type;
    edge->source_node = source;
    edge->target_node = target;
    edge->latency = 0;
    edge->bandwidth = 1;
    edge->weight = 1;
    
    // Update node connectivity
    bdi_node_t* source_node = &graph->nodes[source];
    bdi_node_t* target_node = &graph->nodes[target];
    
    if (source_node->output_count < 32) {
        source_node->output_edges[source_node->output_count++] = edge_id;
    }
    
    if (target_node->input_count < 32) {
        target_node->input_edges[target_node->input_count++] = edge_id;
    }
    
    graph->edge_count++;
    graph->modification_time = time(NULL);
    
    return edge_id;
}

/**
 * @brief Get node by ID
 */
bdi_node_t* bdi_get_node(bdi_graph_t* graph, uint32_t node_id) {
    if (!graph || node_id >= graph->node_count) return NULL;
    return &graph->nodes[node_id];
}

/**
 * @brief Find node by name
 */
bdi_node_t* bdi_find_node_by_name(bdi_graph_t* graph, const char* name) {
    if (!graph || !name) return NULL;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (strcmp(graph->nodes[i].name, name) == 0) {
            return &graph->nodes[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Convert node type to string
 */
const char* bdi_node_type_to_string(bdi_node_type_t type) {
    if (type < sizeof(node_type_strings) / sizeof(node_type_strings[0])) {
        return node_type_strings[type];
    }
    return "unknown";
}

/**
 * @brief Convert edge type to string
 */
const char* bdi_edge_type_to_string(bdi_edge_type_t type) {
    if (type < sizeof(edge_type_strings) / sizeof(edge_type_strings[0])) {
        return edge_type_strings[type];
    }
    return "unknown";
}

/**
 * @brief Print graph summary
 */
void bdi_print_graph_summary(const bdi_graph_t* graph) {
    if (!graph) return;
    
    printf("BDI Graph: %s (v%s)\n", graph->name, graph->version);
    printf("  Nodes: %u\n", graph->node_count);
    printf("  Edges: %u\n", graph->edge_count);
    printf("  Constraints: %u\n", graph->constraint_count);
    printf("  Filaments: %u\n", graph->filament_count);
    printf("  Total Latency: %u cycles\n", graph->total_latency);
    printf("  Total Throughput: %.2f uops/cycle\n", graph->total_throughput);
    printf("  Total Code Size: %u bytes\n", graph->total_code_size);
    printf("  Total Memory Usage: %u bytes\n", graph->total_memory_usage);
    
    if (graph->description[0]) {
        printf("  Description: %s\n", graph->description);
    }
}

/**
 * @brief Validate graph structure
 */
int bdi_graph_validate(const bdi_graph_t* graph) {
    if (!graph) return -1;
    
    // Check for cycles in data flow
    // Check node connectivity
    // Validate constraints
    // Check filament consistency
    
    // For now, basic validation
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        const bdi_edge_t* edge = &graph->edges[i];
        if (edge->source_node >= graph->node_count || 
            edge->target_node >= graph->node_count) {
            return -1; // Invalid edge
        }
    }
    
    return 0;
}

/**
 * @brief Export graph to DOT format
 */
int bdi_export_to_dot(const bdi_graph_t* graph, const char* filename) {
    if (!graph || !filename) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) return -1;
    
    fprintf(file, "digraph \"%s\" {\n", graph->name);
    fprintf(file, "  rankdir=TB;\n");
    fprintf(file, "  node [shape=box];\n\n");
    
    // Write nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        const bdi_node_t* node = &graph->nodes[i];
        fprintf(file, "  \"%s\" [label=\"%s\\nType: %s\\nLatency: %u\"];\n",
                node->name, node->name, bdi_node_type_to_string(node->type),
                node->latency_cycles);
    }
    
    fprintf(file, "\n");
    
    // Write edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        const bdi_edge_t* edge = &graph->edges[i];
        const bdi_node_t* source = &graph->nodes[edge->source_node];
        const bdi_node_t* target = &graph->nodes[edge->target_node];
        
        fprintf(file, "  \"%s\" -> \"%s\" [label=\"%s\"];\n",
                source->name, target->name, bdi_edge_type_to_string(edge->type));
    }
    
    fprintf(file, "}\n");
    fclose(file);
    
    return 0;
}
