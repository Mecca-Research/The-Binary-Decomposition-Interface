
/**
 * @file bdi_parser.h
 * @brief BDI Dot/Filament Graph Parser with Constraints
 * 
 * Phase 2 Master Memory Manager - Complete Toolchain
 * BDI Dot/Filament graph parser with constraints (latency, uops/cycle, code-size, 
 * permitted instructions, memory layout, calling convention)
 */

#ifndef BDI_PARSER_H
#define BDI_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parser Constants
#define BDI_MAX_NODES           1024
#define BDI_MAX_EDGES           2048
#define BDI_MAX_CONSTRAINTS     256
#define BDI_MAX_FILAMENTS       64
#define BDI_MAX_NAME_LENGTH     128
#define BDI_MAX_ATTRIBUTES      32

// Node Types
typedef enum {
    BDI_NODE_OPERATION = 0,
    BDI_NODE_DATA,
    BDI_NODE_CONTROL,
    BDI_NODE_MEMORY,
    BDI_NODE_REGISTER,
    BDI_NODE_CONSTANT,
    BDI_NODE_FUNCTION,
    BDI_NODE_BRANCH,
    BDI_NODE_LOOP,
    BDI_NODE_BARRIER
} bdi_node_type_t;

// Edge Types
typedef enum {
    BDI_EDGE_DATA_FLOW = 0,
    BDI_EDGE_CONTROL_FLOW,
    BDI_EDGE_DEPENDENCY,
    BDI_EDGE_MEMORY_ORDER,
    BDI_EDGE_TIMING,
    BDI_EDGE_RESOURCE
} bdi_edge_type_t;

// Constraint Types
typedef enum {
    BDI_CONSTRAINT_LATENCY = 0,
    BDI_CONSTRAINT_THROUGHPUT,
    BDI_CONSTRAINT_CODE_SIZE,
    BDI_CONSTRAINT_MEMORY_USAGE,
    BDI_CONSTRAINT_REGISTER_USAGE,
    BDI_CONSTRAINT_INSTRUCTION_SET,
    BDI_CONSTRAINT_CALLING_CONVENTION,
    BDI_CONSTRAINT_ALIGNMENT,
    BDI_CONSTRAINT_POWER,
    BDI_CONSTRAINT_THERMAL
} bdi_constraint_type_t;

// Calling Conventions
typedef enum {
    BDI_CALLING_CONV_CDECL = 0,
    BDI_CALLING_CONV_STDCALL,
    BDI_CALLING_CONV_FASTCALL,
    BDI_CALLING_CONV_VECTORCALL,
    BDI_CALLING_CONV_SYSV_AMD64,
    BDI_CALLING_CONV_MS_X64,
    BDI_CALLING_CONV_CUSTOM
} bdi_calling_convention_t;

// Memory Layout Types
typedef enum {
    BDI_MEMORY_LAYOUT_LINEAR = 0,
    BDI_MEMORY_LAYOUT_SEGMENTED,
    BDI_MEMORY_LAYOUT_PAGED,
    BDI_MEMORY_LAYOUT_VIRTUAL,
    BDI_MEMORY_LAYOUT_NUMA,
    BDI_MEMORY_LAYOUT_CACHE_AWARE
} bdi_memory_layout_t;

/**
 * @brief BDI Node Attribute
 */
typedef struct {
    char name[64];
    char value[256];
    uint32_t flags;
} bdi_attribute_t;

/**
 * @brief BDI Graph Node
 */
typedef struct {
    uint32_t id;
    bdi_node_type_t type;
    char name[BDI_MAX_NAME_LENGTH];
    char operation[64];
    
    // Node properties
    uint32_t latency_cycles;
    uint32_t throughput_uops;
    uint32_t code_size_bytes;
    uint32_t memory_footprint;
    uint32_t register_pressure;
    
    // Attributes
    uint32_t attribute_count;
    bdi_attribute_t attributes[BDI_MAX_ATTRIBUTES];
    
    // Connectivity
    uint32_t input_count;
    uint32_t output_count;
    uint32_t input_edges[32];
    uint32_t output_edges[32];
    
    // Scheduling information
    uint32_t earliest_start_time;
    uint32_t latest_start_time;
    uint32_t critical_path_length;
    bool is_critical_path;
    
    // Resource requirements
    uint32_t execution_units;
    uint32_t memory_ports;
    uint32_t register_file_ports;
    
} bdi_node_t;

/**
 * @brief BDI Graph Edge
 */
typedef struct {
    uint32_t id;
    bdi_edge_type_t type;
    uint32_t source_node;
    uint32_t target_node;
    uint32_t source_port;
    uint32_t target_port;
    
    // Edge properties
    uint32_t latency;
    uint32_t bandwidth;
    uint32_t weight;
    bool is_critical;
    
    // Data type information
    char data_type[32];
    uint32_t data_width;
    uint32_t data_alignment;
    
    // Attributes
    uint32_t attribute_count;
    bdi_attribute_t attributes[BDI_MAX_ATTRIBUTES];
    
} bdi_edge_t;

/**
 * @brief BDI Constraint
 */
typedef struct {
    uint32_t id;
    bdi_constraint_type_t type;
    char name[BDI_MAX_NAME_LENGTH];
    
    // Constraint parameters
    union {
        struct {
            uint32_t max_cycles;
            uint32_t target_cycles;
            float weight;
        } latency;
        
        struct {
            float min_uops_per_cycle;
            float target_uops_per_cycle;
            float weight;
        } throughput;
        
        struct {
            uint32_t max_bytes;
            uint32_t target_bytes;
            float weight;
        } code_size;
        
        struct {
            uint32_t max_bytes;
            uint32_t alignment;
            bdi_memory_layout_t layout;
        } memory;
        
        struct {
            uint32_t max_registers;
            uint32_t register_classes;
            bool allow_spilling;
        } registers;
        
        struct {
            uint32_t permitted_instructions[64];
            uint32_t forbidden_instructions[64];
            uint32_t instruction_count;
        } instruction_set;
        
        struct {
            bdi_calling_convention_t convention;
            bool preserve_registers[32];
            uint32_t stack_alignment;
            bool red_zone_allowed;
        } calling_conv;
        
        struct {
            uint32_t alignment;
            bool natural_alignment;
            bool packed_structs;
        } alignment;
    } params;
    
    // Constraint scope
    uint32_t node_count;
    uint32_t affected_nodes[256];
    
    // Priority and enforcement
    float priority;
    bool is_hard_constraint;
    bool is_violated;
    
} bdi_constraint_t;

/**
 * @brief BDI Filament (execution thread/path)
 */
typedef struct {
    uint32_t id;
    char name[BDI_MAX_NAME_LENGTH];
    
    // Filament properties
    uint32_t node_count;
    uint32_t nodes[256];
    uint32_t execution_order[256];
    
    // Performance characteristics
    uint32_t total_latency;
    float average_throughput;
    uint32_t code_size;
    uint32_t memory_usage;
    
    // Resource usage
    uint32_t register_usage[8]; // Per register class
    uint32_t execution_unit_usage[16];
    uint32_t memory_port_usage;
    
    // Parallelism information
    uint32_t parallel_filaments[16];
    uint32_t parallel_count;
    uint32_t synchronization_points[32];
    uint32_t sync_point_count;
    
} bdi_filament_t;

/**
 * @brief BDI Graph Structure
 */
typedef struct {
    char name[BDI_MAX_NAME_LENGTH];
    char version[32];
    
    // Graph components
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t constraint_count;
    uint32_t filament_count;
    
    bdi_node_t nodes[BDI_MAX_NODES];
    bdi_edge_t edges[BDI_MAX_EDGES];
    bdi_constraint_t constraints[BDI_MAX_CONSTRAINTS];
    bdi_filament_t filaments[BDI_MAX_FILAMENTS];
    
    // Global properties
    uint32_t total_latency;
    float total_throughput;
    uint32_t total_code_size;
    uint32_t total_memory_usage;
    
    // Optimization targets
    bool optimize_for_latency;
    bool optimize_for_throughput;
    bool optimize_for_code_size;
    bool optimize_for_power;
    
    // Metadata
    uint64_t creation_time;
    uint64_t modification_time;
    char author[128];
    char description[512];
    
} bdi_graph_t;

/**
 * @brief Parser Context
 */
typedef struct {
    // Input handling
    const char* input_buffer;
    size_t input_size;
    size_t current_position;
    uint32_t current_line;
    uint32_t current_column;
    
    // Parser state
    bdi_graph_t* graph;
    bool parsing_nodes;
    bool parsing_edges;
    bool parsing_constraints;
    bool parsing_filaments;
    
    // Error handling
    bool has_error;
    char error_message[512];
    uint32_t error_line;
    uint32_t error_column;
    
    // Symbol table
    struct {
        char name[64];
        uint32_t node_id;
    } symbols[1024];
    uint32_t symbol_count;
    
} bdi_parser_context_t;

// Core Parser Functions
int bdi_parser_init(bdi_parser_context_t* context);
int bdi_parser_cleanup(bdi_parser_context_t* context);
int bdi_parse_graph(bdi_parser_context_t* context, const char* input, bdi_graph_t* graph);
int bdi_parse_file(const char* filename, bdi_graph_t* graph);
int bdi_parse_string(const char* input, bdi_graph_t* graph);

// Graph Construction
int bdi_graph_init(bdi_graph_t* graph, const char* name);
int bdi_graph_cleanup(bdi_graph_t* graph);
int bdi_graph_validate(const bdi_graph_t* graph);
int bdi_graph_optimize(bdi_graph_t* graph);

// Node Management
uint32_t bdi_add_node(bdi_graph_t* graph, bdi_node_type_t type, const char* name);
int bdi_remove_node(bdi_graph_t* graph, uint32_t node_id);
bdi_node_t* bdi_get_node(bdi_graph_t* graph, uint32_t node_id);
bdi_node_t* bdi_find_node_by_name(bdi_graph_t* graph, const char* name);
int bdi_set_node_attribute(bdi_node_t* node, const char* name, const char* value);
const char* bdi_get_node_attribute(const bdi_node_t* node, const char* name);

// Edge Management
uint32_t bdi_add_edge(bdi_graph_t* graph, bdi_edge_type_t type, uint32_t source, uint32_t target);
int bdi_remove_edge(bdi_graph_t* graph, uint32_t edge_id);
bdi_edge_t* bdi_get_edge(bdi_graph_t* graph, uint32_t edge_id);
int bdi_set_edge_attribute(bdi_edge_t* edge, const char* name, const char* value);

// Constraint Management
uint32_t bdi_add_constraint(bdi_graph_t* graph, bdi_constraint_type_t type, const char* name);
int bdi_remove_constraint(bdi_graph_t* graph, uint32_t constraint_id);
bdi_constraint_t* bdi_get_constraint(bdi_graph_t* graph, uint32_t constraint_id);
int bdi_set_latency_constraint(bdi_constraint_t* constraint, uint32_t max_cycles, uint32_t target_cycles);
int bdi_set_throughput_constraint(bdi_constraint_t* constraint, float min_uops, float target_uops);
int bdi_set_code_size_constraint(bdi_constraint_t* constraint, uint32_t max_bytes, uint32_t target_bytes);
int bdi_set_calling_convention_constraint(bdi_constraint_t* constraint, bdi_calling_convention_t conv);
int bdi_set_memory_layout_constraint(bdi_constraint_t* constraint, bdi_memory_layout_t layout);

// Filament Management
uint32_t bdi_add_filament(bdi_graph_t* graph, const char* name);
int bdi_remove_filament(bdi_graph_t* graph, uint32_t filament_id);
bdi_filament_t* bdi_get_filament(bdi_graph_t* graph, uint32_t filament_id);
int bdi_add_node_to_filament(bdi_filament_t* filament, uint32_t node_id);
int bdi_set_filament_execution_order(bdi_filament_t* filament, const uint32_t* order, uint32_t count);

// Graph Analysis
int bdi_analyze_critical_path(bdi_graph_t* graph);
int bdi_analyze_resource_usage(bdi_graph_t* graph);
int bdi_analyze_parallelism(bdi_graph_t* graph);
int bdi_check_constraints(bdi_graph_t* graph);
int bdi_estimate_performance(bdi_graph_t* graph);

// Serialization
int bdi_serialize_graph(const bdi_graph_t* graph, char* buffer, size_t buffer_size);
int bdi_deserialize_graph(bdi_graph_t* graph, const char* buffer, size_t buffer_size);
int bdi_save_graph_to_file(const bdi_graph_t* graph, const char* filename);
int bdi_load_graph_from_file(bdi_graph_t* graph, const char* filename);

// Visualization and Export
int bdi_export_to_dot(const bdi_graph_t* graph, const char* filename);
int bdi_export_to_json(const bdi_graph_t* graph, const char* filename);
int bdi_export_to_xml(const bdi_graph_t* graph, const char* filename);
int bdi_generate_report(const bdi_graph_t* graph, const char* filename);

// Utility Functions
const char* bdi_node_type_to_string(bdi_node_type_t type);
const char* bdi_edge_type_to_string(bdi_edge_type_t type);
const char* bdi_constraint_type_to_string(bdi_constraint_type_t type);
const char* bdi_calling_convention_to_string(bdi_calling_convention_t conv);
const char* bdi_memory_layout_to_string(bdi_memory_layout_t layout);

// Error Handling
const char* bdi_get_last_error(const bdi_parser_context_t* context);
void bdi_clear_error(bdi_parser_context_t* context);
bool bdi_has_error(const bdi_parser_context_t* context);

// Debugging
void bdi_print_graph_summary(const bdi_graph_t* graph);
void bdi_print_node_details(const bdi_node_t* node);
void bdi_print_edge_details(const bdi_edge_t* edge);
void bdi_print_constraint_details(const bdi_constraint_t* constraint);
void bdi_print_filament_details(const bdi_filament_t* filament);

#ifdef __cplusplus
}
#endif

#endif // BDI_PARSER_H
