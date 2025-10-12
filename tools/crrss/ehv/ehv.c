
/**
 * @file ehv.c
 * @brief Error Heatmap Visualization Implementation
 */

#include "ehv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct ehv_context {
    ehv_config_t config;
    ehv_error_location_t* locations;
    uint32_t location_count;
    uint32_t location_capacity;
    bool initialized;
};

// ==================== Initialization ====================

ehv_context_t* ehv_initialize(const ehv_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    ehv_context_t* ctx = (ehv_context_t*)calloc(1, sizeof(ehv_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->config = *config;
    ctx->location_capacity = config->max_hotspots > 0 ? config->max_hotspots : EHV_MAX_HOTSPOTS;
    ctx->locations = (ehv_error_location_t*)calloc(ctx->location_capacity, sizeof(ehv_error_location_t));
    
    if (!ctx->locations) {
        free(ctx);
        return NULL;
    }
    
    ctx->location_count = 0;
    ctx->initialized = true;
    
    return ctx;
}

void ehv_shutdown(ehv_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->locations) {
        free(ctx->locations);
    }
    
    free(ctx);
}

// ==================== Error Recording ====================

crrss_status_t ehv_record_error(
    ehv_context_t* ctx,
    const char* file_path,
    const char* function_name,
    uint32_t line_number,
    bug_category_t category,
    bug_priority_t priority
) {
    if (!ctx || !ctx->initialized || !file_path || !function_name) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Check if this location already exists
    for (uint32_t i = 0; i < ctx->location_count; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        if (strcmp(loc->file_path, file_path) == 0 &&
            strcmp(loc->function_name, function_name) == 0 &&
            loc->line_number == line_number &&
            loc->category == category) {
            // Update existing location
            loc->frequency++;
            loc->last_seen = time(NULL);
            return CRRSS_SUCCESS;
        }
    }
    
    // Add new location
    if (ctx->location_count >= ctx->location_capacity) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    ehv_error_location_t* loc = &ctx->locations[ctx->location_count];
    strncpy(loc->file_path, file_path, EHV_MAX_PATH_LEN - 1);
    strncpy(loc->function_name, function_name, EHV_MAX_FUNCTION_NAME - 1);
    loc->line_number = line_number;
    loc->category = category;
    loc->priority = priority;
    loc->frequency = 1;
    loc->first_seen = time(NULL);
    loc->last_seen = time(NULL);
    
    // Calculate severity score based on priority
    switch (priority) {
        case BUG_PRIORITY_P0_CRITICAL: loc->severity_score = 1.0; break;
        case BUG_PRIORITY_P1_HIGH: loc->severity_score = 0.75; break;
        case BUG_PRIORITY_P2_MEDIUM: loc->severity_score = 0.5; break;
        case BUG_PRIORITY_P3_LOW: loc->severity_score = 0.25; break;
        default: loc->severity_score = 0.1; break;
    }
    
    ctx->location_count++;
    return CRRSS_SUCCESS;
}

// ==================== Hotspot Identification ====================

static double calculate_heat_score(const ehv_error_location_t* loc) {
    // Heat score based on frequency and severity
    return (double)loc->frequency * loc->severity_score;
}

crrss_status_t ehv_identify_hotspots(
    ehv_context_t* ctx,
    ehv_hotspot_t* hotspots,
    uint32_t max_hotspots,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !hotspots || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Calculate heat scores for all locations
    for (uint32_t i = 0; i < ctx->location_count && *count < max_hotspots; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        double heat = calculate_heat_score(loc);
        
        if (heat >= ctx->config.heat_threshold) {
            ehv_hotspot_t* hotspot = &hotspots[*count];
            snprintf(hotspot->location, EHV_MAX_PATH_LEN, "%s:%s:%u", 
                     loc->file_path, loc->function_name, loc->line_number);
            hotspot->error_count = loc->frequency;
            hotspot->heat_score = heat;
            hotspot->dominant_category = loc->category;
            hotspot->dominant_priority = loc->priority;
            (*count)++;
        }
    }
    
    // Sort by heat score (bubble sort for simplicity)
    for (uint32_t i = 0; i < *count - 1; i++) {
        for (uint32_t j = 0; j < *count - i - 1; j++) {
            if (hotspots[j].heat_score < hotspots[j + 1].heat_score) {
                ehv_hotspot_t temp = hotspots[j];
                hotspots[j] = hotspots[j + 1];
                hotspots[j + 1] = temp;
            }
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Error Clustering ====================

crrss_status_t ehv_cluster_errors(
    ehv_context_t* ctx,
    ehv_cluster_t* clusters,
    uint32_t max_clusters,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !clusters || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.enable_clustering) {
        *count = 0;
        return CRRSS_SUCCESS;
    }
    
    *count = 0;
    
    // Simple clustering by file and category
    for (uint32_t i = 0; i < ctx->location_count && *count < max_clusters; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        
        // Check if this location belongs to an existing cluster
        bool found = false;
        for (uint32_t j = 0; j < *count; j++) {
            if (clusters[j].category == loc->category) {
                // Add to existing cluster
                clusters[j].error_count++;
                found = true;
                break;
            }
        }
        
        // Create new cluster
        if (!found && *count < max_clusters) {
            ehv_cluster_t* cluster = &clusters[*count];
            cluster->cluster_id = *count;
            cluster->error_count = 1;
            cluster->category = loc->category;
            
            // Set description based on category
            const char* cat_name = "Unknown";
            switch (loc->category) {
                case BUG_CATEGORY_MEMORY: cat_name = "Memory Issues"; break;
                case BUG_CATEGORY_CONCURRENCY: cat_name = "Concurrency Issues"; break;
                case BUG_CATEGORY_LOGIC: cat_name = "Logic Errors"; break;
                case BUG_CATEGORY_PERFORMANCE: cat_name = "Performance Issues"; break;
                case BUG_CATEGORY_SECURITY: cat_name = "Security Vulnerabilities"; break;
                default: break;
            }
            strncpy(cluster->description, cat_name, 255);
            
            cluster->cluster_density = 0.5; // Placeholder
            cluster->locations = NULL;  // Would be populated in full implementation
            cluster->location_count = 0;
            
            (*count)++;
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Temporal Analysis ====================

crrss_status_t ehv_analyze_temporal_patterns(
    ehv_context_t* ctx,
    ehv_temporal_pattern_t* patterns,
    uint32_t max_patterns,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !patterns || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.enable_temporal_analysis) {
        *count = 0;
        return CRRSS_SUCCESS;
    }
    
    // Initialize pattern
    if (max_patterns > 0) {
        ehv_temporal_pattern_t* pattern = &patterns[0];
        strncpy(pattern->pattern_name, "Overall Distribution", 63);
        memset(pattern->occurrences, 0, sizeof(pattern->occurrences));
        memset(pattern->daily_trend, 0, sizeof(pattern->daily_trend));
        
        // Analyze timestamps
        for (uint32_t i = 0; i < ctx->location_count; i++) {
            ehv_error_location_t* loc = &ctx->locations[i];
            struct tm* tm_info = localtime(&loc->last_seen);
            if (tm_info) {
                pattern->occurrences[tm_info->tm_hour]++;
                pattern->daily_trend[tm_info->tm_wday]++;
            }
        }
        
        pattern->correlation_score = 0.75; // Placeholder
        *count = 1;
    } else {
        *count = 0;
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Heatmap Generation ====================

crrss_status_t ehv_generate_heatmap(
    ehv_context_t* ctx,
    ehv_heatmap_data_t* data
) {
    if (!ctx || !ctx->initialized || !data) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Allocate memory for heatmap data
    data->max_hotspots = ctx->config.max_hotspots;
    data->hotspots = (ehv_hotspot_t*)calloc(data->max_hotspots, sizeof(ehv_hotspot_t));
    
    data->max_clusters = ctx->config.max_clusters;
    data->clusters = (ehv_cluster_t*)calloc(data->max_clusters, sizeof(ehv_cluster_t));
    
    data->max_locations = ctx->location_count;
    data->locations = (ehv_error_location_t*)calloc(data->max_locations, sizeof(ehv_error_location_t));
    
    if (!data->hotspots || !data->clusters || !data->locations) {
        if (data->hotspots) free(data->hotspots);
        if (data->clusters) free(data->clusters);
        if (data->locations) free(data->locations);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Copy locations
    memcpy(data->locations, ctx->locations, ctx->location_count * sizeof(ehv_error_location_t));
    data->location_count = ctx->location_count;
    
    // Identify hotspots
    ehv_identify_hotspots(ctx, data->hotspots, data->max_hotspots, &data->hotspot_count);
    
    // Cluster errors
    ehv_cluster_errors(ctx, data->clusters, data->max_clusters, &data->cluster_count);
    
    // Calculate statistics
    data->total_errors = ctx->location_count;
    double total_severity = 0.0;
    data->max_heat = 0.0;
    
    for (uint32_t i = 0; i < ctx->location_count; i++) {
        total_severity += ctx->locations[i].severity_score;
        double heat = calculate_heat_score(&ctx->locations[i]);
        if (heat > data->max_heat) {
            data->max_heat = heat;
        }
    }
    
    data->avg_severity = ctx->location_count > 0 ? total_severity / ctx->location_count : 0.0;
    
    return CRRSS_SUCCESS;
}

// ==================== Visualization Export ====================

static crrss_status_t export_ascii(ehv_context_t* ctx, const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(fp, "║           ERROR HEATMAP VISUALIZATION (ASCII)             ║\n");
    fprintf(fp, "╚════════════════════════════════════════════════════════════╝\n\n");
    
    fprintf(fp, "Total Errors: %u\n", ctx->location_count);
    fprintf(fp, "═══════════════════════════════════════════════════════════\n\n");
    
    // Show hotspots
    ehv_hotspot_t hotspots[50];
    uint32_t hotspot_count = 0;
    ehv_identify_hotspots(ctx, hotspots, 50, &hotspot_count);
    
    fprintf(fp, "TOP HOTSPOTS:\n");
    fprintf(fp, "─────────────────────────────────────────────────────────\n");
    for (uint32_t i = 0; i < hotspot_count && i < 20; i++) {
        int bar_len = (int)(hotspots[i].heat_score * 40.0);
        fprintf(fp, "%3u. %-40s [", i + 1, hotspots[i].location);
        for (int j = 0; j < bar_len && j < 40; j++) {
            fprintf(fp, "█");
        }
        fprintf(fp, "] %.2f\n", hotspots[i].heat_score);
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

static crrss_status_t export_json(ehv_context_t* ctx, const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"total_errors\": %u,\n", ctx->location_count);
    fprintf(fp, "  \"locations\": [\n");
    
    for (uint32_t i = 0; i < ctx->location_count; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"file\": \"%s\",\n", loc->file_path);
        fprintf(fp, "      \"function\": \"%s\",\n", loc->function_name);
        fprintf(fp, "      \"line\": %u,\n", loc->line_number);
        fprintf(fp, "      \"category\": %d,\n", loc->category);
        fprintf(fp, "      \"priority\": %d,\n", loc->priority);
        fprintf(fp, "      \"frequency\": %u,\n", loc->frequency);
        fprintf(fp, "      \"severity_score\": %.2f,\n", loc->severity_score);
        fprintf(fp, "      \"heat_score\": %.2f\n", calculate_heat_score(loc));
        fprintf(fp, "    }%s\n", i < ctx->location_count - 1 ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

static crrss_status_t export_html(ehv_context_t* ctx, const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>CRRSS Error Heatmap</title>\n");
    fprintf(fp, "<style>\n");
    fprintf(fp, "body { font-family: Arial, sans-serif; margin: 20px; }\n");
    fprintf(fp, "h1 { color: #333; }\n");
    fprintf(fp, ".hotspot { padding: 10px; margin: 5px 0; background: #f0f0f0; border-radius: 5px; }\n");
    fprintf(fp, ".heat-high { background: #ff4444; color: white; }\n");
    fprintf(fp, ".heat-medium { background: #ffaa44; }\n");
    fprintf(fp, ".heat-low { background: #ffff44; }\n");
    fprintf(fp, "</style>\n</head>\n<body>\n");
    
    fprintf(fp, "<h1>Error Heatmap Visualization</h1>\n");
    fprintf(fp, "<p><strong>Total Errors:</strong> %u</p>\n", ctx->location_count);
    fprintf(fp, "<h2>Top Hotspots</h2>\n");
    
    ehv_hotspot_t hotspots[50];
    uint32_t hotspot_count = 0;
    ehv_identify_hotspots(ctx, hotspots, 50, &hotspot_count);
    
    for (uint32_t i = 0; i < hotspot_count && i < 20; i++) {
        const char* heat_class = "heat-low";
        if (hotspots[i].heat_score > 10.0) heat_class = "heat-high";
        else if (hotspots[i].heat_score > 5.0) heat_class = "heat-medium";
        
        fprintf(fp, "<div class='hotspot %s'>\n", heat_class);
        fprintf(fp, "<strong>#%u</strong> %s<br/>\n", i + 1, hotspots[i].location);
        fprintf(fp, "Heat: %.2f | Errors: %u\n", hotspots[i].heat_score, hotspots[i].error_count);
        fprintf(fp, "</div>\n");
    }
    
    fprintf(fp, "</body>\n</html>\n");
    fclose(fp);
    return CRRSS_SUCCESS;
}

static crrss_status_t export_csv(ehv_context_t* ctx, const char* output_path) {
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(fp, "File,Function,Line,Category,Priority,Frequency,Severity,Heat\n");
    
    for (uint32_t i = 0; i < ctx->location_count; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        fprintf(fp, "%s,%s,%u,%d,%d,%u,%.2f,%.2f\n",
                loc->file_path, loc->function_name, loc->line_number,
                loc->category, loc->priority, loc->frequency,
                loc->severity_score, calculate_heat_score(loc));
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

crrss_status_t ehv_export_visualization(
    ehv_context_t* ctx,
    ehv_format_t format,
    const char* output_path
) {
    if (!ctx || !ctx->initialized || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    switch (format) {
        case EHV_FORMAT_ASCII:
            return export_ascii(ctx, output_path);
        case EHV_FORMAT_JSON:
            return export_json(ctx, output_path);
        case EHV_FORMAT_HTML:
            return export_html(ctx, output_path);
        case EHV_FORMAT_CSV:
            return export_csv(ctx, output_path);
        default:
            return CRRSS_ERROR_INVALID_PARAM;
    }
}

// ==================== Statistics ====================

crrss_status_t ehv_get_statistics(
    ehv_context_t* ctx,
    uint32_t* total_errors,
    double* avg_severity,
    double* max_heat
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (total_errors) {
        *total_errors = ctx->location_count;
    }
    
    if (avg_severity) {
        double total = 0.0;
        for (uint32_t i = 0; i < ctx->location_count; i++) {
            total += ctx->locations[i].severity_score;
        }
        *avg_severity = ctx->location_count > 0 ? total / ctx->location_count : 0.0;
    }
    
    if (max_heat) {
        *max_heat = 0.0;
        for (uint32_t i = 0; i < ctx->location_count; i++) {
            double heat = calculate_heat_score(&ctx->locations[i]);
            if (heat > *max_heat) {
                *max_heat = heat;
            }
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Data Management ====================

crrss_status_t ehv_clear_data(ehv_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    ctx->location_count = 0;
    memset(ctx->locations, 0, ctx->location_capacity * sizeof(ehv_error_location_t));
    
    return CRRSS_SUCCESS;
}

crrss_status_t ehv_load_data(ehv_context_t* ctx, const char* file_path) {
    if (!ctx || !ctx->initialized || !file_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Read location count
    uint32_t count = 0;
    if (fscanf(fp, "%u\n", &count) != 1) {
        fclose(fp);
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Clear existing data
    ehv_clear_data(ctx);
    
    // Read locations
    for (uint32_t i = 0; i < count && i < ctx->location_capacity; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        if (fscanf(fp, "%511s %255s %u %d %d %u %ld %ld %lf\n",
                   loc->file_path, loc->function_name, &loc->line_number,
                   (int*)&loc->category, (int*)&loc->priority, &loc->frequency,
                   &loc->first_seen, &loc->last_seen, &loc->severity_score) == 9) {
            ctx->location_count++;
        }
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

crrss_status_t ehv_save_data(ehv_context_t* ctx, const char* file_path) {
    if (!ctx || !ctx->initialized || !file_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(file_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Write location count
    fprintf(fp, "%u\n", ctx->location_count);
    
    // Write locations
    for (uint32_t i = 0; i < ctx->location_count; i++) {
        ehv_error_location_t* loc = &ctx->locations[i];
        fprintf(fp, "%s %s %u %d %d %u %ld %ld %f\n",
                loc->file_path, loc->function_name, loc->line_number,
                loc->category, loc->priority, loc->frequency,
                loc->first_seen, loc->last_seen, loc->severity_score);
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}
