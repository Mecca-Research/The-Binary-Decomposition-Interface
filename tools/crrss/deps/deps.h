/**
 * @file deps.h
 * @brief Cross-Module Dependency Analysis - Phase 3 Component
 * 
 * Analyzes dependencies across BDI modules:
 * - Function call dependencies
 * - Data flow between modules
 * - Include file dependencies
 * - Circular dependency detection
 * - Dependency graph visualization
 * - High-risk coupling identification
 */

#ifndef CRRSS_DEPS_H
#define CRRSS_DEPS_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEPS_MAX_MODULES 200
#define DEPS_MAX_DEPENDENCIES 1000
#define DEPS_MAX_PATH_LEN 512
#define DEPS_MAX_MODULE_NAME 128
#define DEPS_MAX_FUNCTION_NAME 256

// Module information
typedef struct {
    char module_name[DEPS_MAX_MODULE_NAME];
    char module_path[DEPS_MAX_PATH_LEN];
    uint32_t line_count;
    uint32_t function_count;
    uint32_t incoming_deps;  // How many modules depend on this
    uint32_t outgoing_deps;  // How many modules this depends on
    double coupling_score;   // Measure of coupling (0.0 - 1.0)
    double cohesion_score;   // Measure of cohesion (0.0 - 1.0)
    bool is_critical;        // Is this a critical module?
} deps_module_info_t;

// Dependency relationship
typedef struct {
    char source_module[DEPS_MAX_MODULE_NAME];
    char target_module[DEPS_MAX_MODULE_NAME];
    char source_function[DEPS_MAX_FUNCTION_NAME];
    char target_function[DEPS_MAX_FUNCTION_NAME];
    uint32_t call_count;     // Number of times this dependency is used
    bool is_data_flow;       // Is this a data dependency?
    bool is_critical;        // Is this a critical dependency?
    double coupling_strength; // Strength of coupling (0.0 - 1.0)
} deps_relationship_t;

// Circular dependency
typedef struct {
    uint32_t cycle_length;
    char modules[10][DEPS_MAX_MODULE_NAME];  // Modules in the cycle
    double risk_score;       // Risk associated with this cycle
    const char* recommendation;
} deps_circular_dependency_t;

// Dependency graph
typedef struct {
    deps_module_info_t* modules;
    uint32_t module_count;
    uint32_t max_modules;
    
    deps_relationship_t* relationships;
    uint32_t relationship_count;
    uint32_t max_relationships;
    
    uint32_t* adjacency_matrix;  // For cycle detection
    uint32_t matrix_size;
} deps_graph_t;

// High-risk coupling point
typedef struct {
    char module1[DEPS_MAX_MODULE_NAME];
    char module2[DEPS_MAX_MODULE_NAME];
    double coupling_score;
    uint32_t dependency_count;
    const char* risk_reason;
    const char* mitigation;
} deps_coupling_point_t;

// Dependency statistics
typedef struct {
    uint32_t total_modules;
    uint32_t total_dependencies;
    uint32_t circular_dependencies;
    double avg_coupling;
    double avg_cohesion;
    uint32_t critical_modules;
    uint32_t high_risk_couplings;
} deps_statistics_t;

// Visualization format
typedef enum {
    DEPS_FORMAT_DOT = 0,     // GraphViz DOT format
    DEPS_FORMAT_ASCII = 1,    // ASCII art
    DEPS_FORMAT_JSON = 2,     // JSON graph data
    DEPS_FORMAT_HTML = 3      // HTML with embedded SVG
} deps_format_t;

// Configuration
typedef struct {
    const char* source_directory;
    bool analyze_includes;
    bool analyze_function_calls;
    bool analyze_data_flow;
    bool detect_circular_deps;
    bool calculate_coupling;
    uint32_t max_depth;          // Maximum dependency depth to analyze
    double coupling_threshold;    // Threshold for high coupling warning
    const char* exclude_pattern;  // Regex pattern for files to exclude
} deps_config_t;

// DEPS context
typedef struct deps_context deps_context_t;

/**
 * @brief Initialize DEPS system
 * @param config Configuration options
 * @return DEPS context or NULL on failure
 */
deps_context_t* deps_initialize(const deps_config_t* config);

/**
 * @brief Shutdown DEPS system
 * @param ctx DEPS context
 */
void deps_shutdown(deps_context_t* ctx);

/**
 * @brief Analyze dependencies in a module
 * @param ctx DEPS context
 * @param module_path Module path
 * @return Status code
 */
crrss_status_t deps_analyze_module(
    deps_context_t* ctx,
    const char* module_path
);

/**
 * @brief Analyze dependencies in a directory
 * @param ctx DEPS context
 * @param directory_path Directory path
 * @return Status code
 */
crrss_status_t deps_analyze_directory(
    deps_context_t* ctx,
    const char* directory_path
);

/**
 * @brief Build dependency graph
 * @param ctx DEPS context
 * @param graph Output dependency graph
 * @return Status code
 */
crrss_status_t deps_build_graph(
    deps_context_t* ctx,
    deps_graph_t* graph
);

/**
 * @brief Detect circular dependencies
 * @param ctx DEPS context
 * @param circular_deps Output circular dependencies array
 * @param max_circular Maximum circular dependencies to return
 * @param count Output count
 * @return Status code
 */
crrss_status_t deps_detect_circular(
    deps_context_t* ctx,
    deps_circular_dependency_t* circular_deps,
    uint32_t max_circular,
    uint32_t* count
);

/**
 * @brief Identify high-risk coupling points
 * @param ctx DEPS context
 * @param coupling_points Output coupling points array
 * @param max_points Maximum points to return
 * @param count Output count
 * @return Status code
 */
crrss_status_t deps_identify_coupling_points(
    deps_context_t* ctx,
    deps_coupling_point_t* coupling_points,
    uint32_t max_points,
    uint32_t* count
);

/**
 * @brief Calculate module coupling score
 * @param ctx DEPS context
 * @param module_name Module name
 * @param coupling_score Output coupling score
 * @return Status code
 */
crrss_status_t deps_calculate_coupling(
    deps_context_t* ctx,
    const char* module_name,
    double* coupling_score
);

/**
 * @brief Calculate module cohesion score
 * @param ctx DEPS context
 * @param module_name Module name
 * @param cohesion_score Output cohesion score
 * @return Status code
 */
crrss_status_t deps_calculate_cohesion(
    deps_context_t* ctx,
    const char* module_name,
    double* cohesion_score
);

/**
 * @brief Get module dependencies
 * @param ctx DEPS context
 * @param module_name Module name
 * @param dependencies Output dependencies array
 * @param max_dependencies Maximum dependencies to return
 * @param count Output count
 * @return Status code
 */
crrss_status_t deps_get_module_dependencies(
    deps_context_t* ctx,
    const char* module_name,
    deps_relationship_t* dependencies,
    uint32_t max_dependencies,
    uint32_t* count
);

/**
 * @brief Get modules that depend on a module (reverse dependencies)
 * @param ctx DEPS context
 * @param module_name Module name
 * @param dependents Output dependents array
 * @param max_dependents Maximum dependents to return
 * @param count Output count
 * @return Status code
 */
crrss_status_t deps_get_module_dependents(
    deps_context_t* ctx,
    const char* module_name,
    deps_relationship_t* dependents,
    uint32_t max_dependents,
    uint32_t* count
);

/**
 * @brief Export dependency visualization
 * @param ctx DEPS context
 * @param format Output format
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t deps_export_visualization(
    deps_context_t* ctx,
    deps_format_t format,
    const char* output_path
);

/**
 * @brief Get dependency statistics
 * @param ctx DEPS context
 * @param stats Output statistics
 * @return Status code
 */
crrss_status_t deps_get_statistics(
    deps_context_t* ctx,
    deps_statistics_t* stats
);

/**
 * @brief Find critical path between modules
 * @param ctx DEPS context
 * @param source_module Source module name
 * @param target_module Target module name
 * @param path Output path array (module names)
 * @param max_path_length Maximum path length
 * @param path_length Output path length
 * @return Status code
 */
crrss_status_t deps_find_critical_path(
    deps_context_t* ctx,
    const char* source_module,
    const char* target_module,
    char (*path)[DEPS_MAX_MODULE_NAME],
    uint32_t max_path_length,
    uint32_t* path_length
);

/**
 * @brief Generate dependency report
 * @param ctx DEPS context
 * @param output_path Output file path
 * @param format Report format (text, json, html)
 * @return Status code
 */
crrss_status_t deps_generate_report(
    deps_context_t* ctx,
    const char* output_path,
    const char* format
);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_DEPS_H
