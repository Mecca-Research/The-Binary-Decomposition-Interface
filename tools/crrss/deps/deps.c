/**
 * @file deps.c
 * @brief Cross-Module Dependency Analysis Implementation
 */

#include "deps.h"
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

struct deps_context {
    deps_config_t config;
    deps_module_info_t* modules;
    uint32_t module_count;
    uint32_t module_capacity;
    
    deps_relationship_t* relationships;
    uint32_t relationship_count;
    uint32_t relationship_capacity;
    
    bool initialized;
};

// ==================== Initialization ====================

deps_context_t* deps_initialize(const deps_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    deps_context_t* ctx = (deps_context_t*)calloc(1, sizeof(deps_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->config = *config;
    ctx->module_capacity = DEPS_MAX_MODULES;
    ctx->relationship_capacity = DEPS_MAX_DEPENDENCIES;
    
    ctx->modules = (deps_module_info_t*)calloc(ctx->module_capacity, sizeof(deps_module_info_t));
    ctx->relationships = (deps_relationship_t*)calloc(ctx->relationship_capacity, sizeof(deps_relationship_t));
    
    if (!ctx->modules || !ctx->relationships) {
        if (ctx->modules) free(ctx->modules);
        if (ctx->relationships) free(ctx->relationships);
        free(ctx);
        return NULL;
    }
    
    ctx->module_count = 0;
    ctx->relationship_count = 0;
    ctx->initialized = true;
    
    return ctx;
}

void deps_shutdown(deps_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->modules) {
        free(ctx->modules);
    }
    if (ctx->relationships) {
        free(ctx->relationships);
    }
    
    free(ctx);
}

// ==================== Module Analysis ====================

static void extract_module_name(const char* path, char* module_name) {
    const char* last_slash = strrchr(path, '/');
    const char* filename = last_slash ? last_slash + 1 : path;
    const char* dot = strrchr(filename, '.');
    
    if (dot) {
        size_t len = dot - filename;
        if (len >= DEPS_MAX_MODULE_NAME) len = DEPS_MAX_MODULE_NAME - 1;
        strncpy(module_name, filename, len);
        module_name[len] = '\0';
    } else {
        strncpy(module_name, filename, DEPS_MAX_MODULE_NAME - 1);
    }
}

static void analyze_includes(const char* file_path, deps_context_t* ctx, const char* current_module) {
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Look for #include directives
        if (strstr(line, "#include")) {
            char* start = strchr(line, '"');
            if (!start) start = strchr(line, '<');
            if (start) {
                start++;
                char* end = strchr(start, '"');
                if (!end) end = strchr(start, '>');
                if (end) {
                    char included_file[DEPS_MAX_PATH_LEN];
                    size_t len = end - start;
                    if (len >= DEPS_MAX_PATH_LEN) len = DEPS_MAX_PATH_LEN - 1;
                    strncpy(included_file, start, len);
                    included_file[len] = '\0';
                    
                    // Extract module name from included file
                    char target_module[DEPS_MAX_MODULE_NAME];
                    extract_module_name(included_file, target_module);
                    
                    // Add relationship if we have space
                    if (ctx->relationship_count < ctx->relationship_capacity) {
                        deps_relationship_t* rel = &ctx->relationships[ctx->relationship_count];
                        strncpy(rel->source_module, current_module, DEPS_MAX_MODULE_NAME - 1);
                        strncpy(rel->target_module, target_module, DEPS_MAX_MODULE_NAME - 1);
                        rel->call_count = 1;
                        rel->is_data_flow = false;
                        rel->is_critical = false;
                        rel->coupling_strength = 0.5;
                        ctx->relationship_count++;
                    }
                }
            }
        }
    }
    
    fclose(fp);
}

crrss_status_t deps_analyze_module(
    deps_context_t* ctx,
    const char* module_path
) {
    if (!ctx || !ctx->initialized || !module_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->module_count >= ctx->module_capacity) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Extract module name
    char module_name[DEPS_MAX_MODULE_NAME];
    extract_module_name(module_path, module_name);
    
    // Check if module already exists
    for (uint32_t i = 0; i < ctx->module_count; i++) {
        if (strcmp(ctx->modules[i].module_name, module_name) == 0) {
            return CRRSS_SUCCESS;  // Already analyzed
        }
    }
    
    // Add new module
    deps_module_info_t* module = &ctx->modules[ctx->module_count];
    strncpy(module->module_name, module_name, DEPS_MAX_MODULE_NAME - 1);
    strncpy(module->module_path, module_path, DEPS_MAX_PATH_LEN - 1);
    
    // Count lines
    FILE* fp = fopen(module_path, "r");
    if (fp) {
        char line[1024];
        uint32_t line_count = 0;
        while (fgets(line, sizeof(line), fp)) {
            line_count++;
        }
        module->line_count = line_count;
        fclose(fp);
    }
    
    module->function_count = 0;  // Would need proper parsing
    module->incoming_deps = 0;
    module->outgoing_deps = 0;
    module->coupling_score = 0.5;
    module->cohesion_score = 0.7;
    module->is_critical = false;
    
    ctx->module_count++;
    
    // Analyze includes if enabled
    if (ctx->config.analyze_includes) {
        analyze_includes(module_path, ctx, module_name);
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t deps_analyze_directory(
    deps_context_t* ctx,
    const char* directory_path
) {
    if (!ctx || !ctx->initialized || !directory_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    DIR* dir = opendir(directory_path);
    if (!dir) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == 8) { // DT_REG = 8
            // Check if it's a C file
            const char* ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0)) {
                char full_path[DEPS_MAX_PATH_LEN];
                snprintf(full_path, DEPS_MAX_PATH_LEN, "%s/%s", directory_path, entry->d_name);
                deps_analyze_module(ctx, full_path);
            }
        }
    }
    
    closedir(dir);
    return CRRSS_SUCCESS;
}

// ==================== Dependency Graph ====================

crrss_status_t deps_build_graph(
    deps_context_t* ctx,
    deps_graph_t* graph
) {
    if (!ctx || !ctx->initialized || !graph) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Allocate graph structures
    graph->max_modules = ctx->module_count;
    graph->max_relationships = ctx->relationship_count;
    
    graph->modules = (deps_module_info_t*)calloc(graph->max_modules, sizeof(deps_module_info_t));
    graph->relationships = (deps_relationship_t*)calloc(graph->max_relationships, sizeof(deps_relationship_t));
    
    if (!graph->modules || !graph->relationships) {
        if (graph->modules) free(graph->modules);
        if (graph->relationships) free(graph->relationships);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Copy data
    memcpy(graph->modules, ctx->modules, ctx->module_count * sizeof(deps_module_info_t));
    memcpy(graph->relationships, ctx->relationships, ctx->relationship_count * sizeof(deps_relationship_t));
    
    graph->module_count = ctx->module_count;
    graph->relationship_count = ctx->relationship_count;
    
    // Build adjacency matrix
    graph->matrix_size = ctx->module_count;
    graph->adjacency_matrix = (uint32_t*)calloc(graph->matrix_size * graph->matrix_size, sizeof(uint32_t));
    
    if (!graph->adjacency_matrix) {
        free(graph->modules);
        free(graph->relationships);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Populate adjacency matrix
    for (uint32_t i = 0; i < ctx->relationship_count; i++) {
        deps_relationship_t* rel = &ctx->relationships[i];
        
        // Find source and target indices
        int source_idx = -1, target_idx = -1;
        for (uint32_t j = 0; j < ctx->module_count; j++) {
            if (strcmp(ctx->modules[j].module_name, rel->source_module) == 0) {
                source_idx = j;
            }
            if (strcmp(ctx->modules[j].module_name, rel->target_module) == 0) {
                target_idx = j;
            }
        }
        
        if (source_idx >= 0 && target_idx >= 0) {
            graph->adjacency_matrix[source_idx * graph->matrix_size + target_idx] = 1;
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Circular Dependency Detection ====================

static bool has_cycle_util(uint32_t* matrix, uint32_t size, uint32_t node, 
                          bool* visited, bool* rec_stack) {
    visited[node] = true;
    rec_stack[node] = true;
    
    for (uint32_t i = 0; i < size; i++) {
        if (matrix[node * size + i]) {
            if (!visited[i]) {
                if (has_cycle_util(matrix, size, i, visited, rec_stack)) {
                    return true;
                }
            } else if (rec_stack[i]) {
                return true;
            }
        }
    }
    
    rec_stack[node] = false;
    return false;
}

crrss_status_t deps_detect_circular(
    deps_context_t* ctx,
    deps_circular_dependency_t* circular_deps,
    uint32_t max_circular,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !circular_deps || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    if (!ctx->config.detect_circular_deps) {
        return CRRSS_SUCCESS;
    }
    
    // Build graph
    deps_graph_t graph;
    crrss_status_t status = deps_build_graph(ctx, &graph);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Detect cycles using DFS
    bool* visited = (bool*)calloc(graph.matrix_size, sizeof(bool));
    bool* rec_stack = (bool*)calloc(graph.matrix_size, sizeof(bool));
    
    if (!visited || !rec_stack) {
        if (visited) free(visited);
        if (rec_stack) free(rec_stack);
        free(graph.modules);
        free(graph.relationships);
        free(graph.adjacency_matrix);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    for (uint32_t i = 0; i < graph.matrix_size && *count < max_circular; i++) {
        if (!visited[i]) {
            if (has_cycle_util(graph.adjacency_matrix, graph.matrix_size, i, visited, rec_stack)) {
                // Found a cycle
                deps_circular_dependency_t* circ = &circular_deps[*count];
                circ->cycle_length = 2;  // Simplified
                strncpy(circ->modules[0], graph.modules[i].module_name, DEPS_MAX_MODULE_NAME - 1);
                circ->risk_score = 0.7;
                circ->recommendation = "Break circular dependency by introducing an interface";
                (*count)++;
            }
        }
    }
    
    free(visited);
    free(rec_stack);
    free(graph.modules);
    free(graph.relationships);
    free(graph.adjacency_matrix);
    
    return CRRSS_SUCCESS;
}

// ==================== Coupling Analysis ====================

crrss_status_t deps_calculate_coupling(
    deps_context_t* ctx,
    const char* module_name,
    double* coupling_score
) {
    if (!ctx || !ctx->initialized || !module_name || !coupling_score) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    uint32_t dependencies = 0;
    for (uint32_t i = 0; i < ctx->relationship_count; i++) {
        if (strcmp(ctx->relationships[i].source_module, module_name) == 0) {
            dependencies++;
        }
    }
    
    // Coupling score based on number of dependencies
    *coupling_score = (double)dependencies / 10.0;
    if (*coupling_score > 1.0) *coupling_score = 1.0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t deps_identify_coupling_points(
    deps_context_t* ctx,
    deps_coupling_point_t* coupling_points,
    uint32_t max_points,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !coupling_points || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Find module pairs with high coupling
    for (uint32_t i = 0; i < ctx->module_count && *count < max_points; i++) {
        for (uint32_t j = i + 1; j < ctx->module_count && *count < max_points; j++) {
            uint32_t dep_count = 0;
            
            // Count dependencies between these two modules
            for (uint32_t k = 0; k < ctx->relationship_count; k++) {
                if ((strcmp(ctx->relationships[k].source_module, ctx->modules[i].module_name) == 0 &&
                     strcmp(ctx->relationships[k].target_module, ctx->modules[j].module_name) == 0) ||
                    (strcmp(ctx->relationships[k].source_module, ctx->modules[j].module_name) == 0 &&
                     strcmp(ctx->relationships[k].target_module, ctx->modules[i].module_name) == 0)) {
                    dep_count++;
                }
            }
            
            if (dep_count > 3) {  // Threshold for high coupling
                deps_coupling_point_t* point = &coupling_points[*count];
                strncpy(point->module1, ctx->modules[i].module_name, DEPS_MAX_MODULE_NAME - 1);
                strncpy(point->module2, ctx->modules[j].module_name, DEPS_MAX_MODULE_NAME - 1);
                point->coupling_score = (double)dep_count / 10.0;
                point->dependency_count = dep_count;
                point->risk_reason = "High number of dependencies between modules";
                point->mitigation = "Consider introducing a facade or adapter pattern";
                (*count)++;
            }
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Statistics ====================

crrss_status_t deps_get_statistics(
    deps_context_t* ctx,
    deps_statistics_t* stats
) {
    if (!ctx || !ctx->initialized || !stats) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    stats->total_modules = ctx->module_count;
    stats->total_dependencies = ctx->relationship_count;
    
    // Detect circular dependencies
    deps_circular_dependency_t circulars[10];
    uint32_t circular_count = 0;
    deps_detect_circular(ctx, circulars, 10, &circular_count);
    stats->circular_dependencies = circular_count;
    
    // Calculate average coupling and cohesion
    double total_coupling = 0.0;
    double total_cohesion = 0.0;
    for (uint32_t i = 0; i < ctx->module_count; i++) {
        total_coupling += ctx->modules[i].coupling_score;
        total_cohesion += ctx->modules[i].cohesion_score;
    }
    stats->avg_coupling = ctx->module_count > 0 ? total_coupling / ctx->module_count : 0.0;
    stats->avg_cohesion = ctx->module_count > 0 ? total_cohesion / ctx->module_count : 0.0;
    
    // Count critical modules
    stats->critical_modules = 0;
    for (uint32_t i = 0; i < ctx->module_count; i++) {
        if (ctx->modules[i].is_critical) {
            stats->critical_modules++;
        }
    }
    
    // Count high-risk couplings
    deps_coupling_point_t points[20];
    uint32_t point_count = 0;
    deps_identify_coupling_points(ctx, points, 20, &point_count);
    stats->high_risk_couplings = point_count;
    
    return CRRSS_SUCCESS;
}

// ==================== Visualization Export ====================

static crrss_status_t export_dot(deps_context_t* ctx, const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "digraph dependencies {\n");
    fprintf(fp, "  rankdir=LR;\n");
    fprintf(fp, "  node [shape=box, style=rounded];\n\n");
    
    // Add nodes
    for (uint32_t i = 0; i < ctx->module_count; i++) {
        fprintf(fp, "  \"%s\";\n", ctx->modules[i].module_name);
    }
    
    fprintf(fp, "\n");
    
    // Add edges
    for (uint32_t i = 0; i < ctx->relationship_count; i++) {
        fprintf(fp, "  \"%s\" -> \"%s\";\n",
                ctx->relationships[i].source_module,
                ctx->relationships[i].target_module);
    }
    
    fprintf(fp, "}\n");
    fclose(fp);
    return CRRSS_SUCCESS;
}

crrss_status_t deps_export_visualization(
    deps_context_t* ctx,
    deps_format_t format,
    const char* output_path
) {
    if (!ctx || !ctx->initialized || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    switch (format) {
        case DEPS_FORMAT_DOT:
            return export_dot(ctx, output_path);
        // Other formats would be implemented similarly
        default:
            return CRRSS_ERROR_INVALID_PARAM;
    }
}

crrss_status_t deps_generate_report(
    deps_context_t* ctx,
    const char* output_path,
    const char* format __attribute__((unused))
) {
    if (!ctx || !ctx->initialized || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "DEPENDENCY ANALYSIS REPORT\n");
    fprintf(fp, "==========================\n\n");
    
    deps_statistics_t stats;
    deps_get_statistics(ctx, &stats);
    
    fprintf(fp, "Total Modules: %u\n", stats.total_modules);
    fprintf(fp, "Total Dependencies: %u\n", stats.total_dependencies);
    fprintf(fp, "Circular Dependencies: %u\n", stats.circular_dependencies);
    fprintf(fp, "Average Coupling: %.2f\n", stats.avg_coupling);
    fprintf(fp, "Average Cohesion: %.2f\n", stats.avg_cohesion);
    fprintf(fp, "Critical Modules: %u\n", stats.critical_modules);
    fprintf(fp, "High-Risk Couplings: %u\n\n", stats.high_risk_couplings);
    
    fprintf(fp, "MODULES:\n");
    fprintf(fp, "--------\n");
    for (uint32_t i = 0; i < ctx->module_count; i++) {
        fprintf(fp, "%s: %u lines, %u functions\n",
                ctx->modules[i].module_name,
                ctx->modules[i].line_count,
                ctx->modules[i].function_count);
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}
