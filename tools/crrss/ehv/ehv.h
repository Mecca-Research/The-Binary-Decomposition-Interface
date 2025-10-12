
/**
 * @file ehv.h
 * @brief Error Heatmap Visualization - Phase 3 Component
 * 
 * Visualizes error patterns across the codebase with advanced analytics:
 * - Error frequency tracking by file/function/line
 * - Error severity distribution analysis
 * - Temporal pattern detection
 * - Error clustering and correlation
 */

#ifndef CRRSS_EHV_H
#define CRRSS_EHV_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EHV_MAX_HOTSPOTS 1000
#define EHV_MAX_CLUSTERS 50
#define EHV_MAX_PATH_LEN 512
#define EHV_MAX_FUNCTION_NAME 256

// Error location tracking
typedef struct {
    char file_path[EHV_MAX_PATH_LEN];
    char function_name[EHV_MAX_FUNCTION_NAME];
    uint32_t line_number;
    bug_category_t category;
    bug_priority_t priority;
    uint32_t frequency;          // How many times this error occurred
    time_t first_seen;           // First occurrence timestamp
    time_t last_seen;            // Last occurrence timestamp
    double severity_score;       // Computed severity (0.0 - 1.0)
    uint32_t cluster_id;         // Which cluster this belongs to
} ehv_error_location_t;

// Error hotspot (high-frequency error areas)
typedef struct {
    char location[EHV_MAX_PATH_LEN];  // File or function name
    uint32_t error_count;
    double heat_score;           // Normalized heat value (0.0 - 1.0)
    bug_category_t dominant_category;
    bug_priority_t dominant_priority;
} ehv_hotspot_t;

// Error cluster (related errors)
typedef struct {
    uint32_t cluster_id;
    uint32_t error_count;
    bug_category_t category;
    char description[256];
    double cluster_density;      // How closely related are errors
    ehv_error_location_t* locations;  // Array of locations in this cluster
    uint32_t location_count;
} ehv_cluster_t;

// Temporal pattern
typedef struct {
    char pattern_name[64];
    uint32_t occurrences[24];    // Hourly distribution
    uint32_t daily_trend[7];     // Day of week distribution
    double correlation_score;     // Correlation with other patterns
} ehv_temporal_pattern_t;

// Heatmap data
typedef struct {
    ehv_hotspot_t* hotspots;
    uint32_t hotspot_count;
    uint32_t max_hotspots;
    
    ehv_cluster_t* clusters;
    uint32_t cluster_count;
    uint32_t max_clusters;
    
    ehv_error_location_t* locations;
    uint32_t location_count;
    uint32_t max_locations;
    
    ehv_temporal_pattern_t* patterns;
    uint32_t pattern_count;
    
    // Statistics
    uint32_t total_errors;
    double avg_severity;
    double max_heat;
} ehv_heatmap_data_t;

// Visualization format
typedef enum {
    EHV_FORMAT_ASCII = 0,
    EHV_FORMAT_JSON = 1,
    EHV_FORMAT_HTML = 2,
    EHV_FORMAT_CSV = 3
} ehv_format_t;

// Configuration
typedef struct {
    bool enable_clustering;
    bool enable_temporal_analysis;
    bool track_severity_distribution;
    uint32_t max_hotspots;
    uint32_t max_clusters;
    uint32_t clustering_threshold;   // Minimum errors to form cluster
    double heat_threshold;           // Minimum heat to display (0.0-1.0)
    const char* output_directory;
} ehv_config_t;

// EHV context
typedef struct ehv_context ehv_context_t;

/**
 * @brief Initialize EHV system
 * @param config Configuration options
 * @return EHV context or NULL on failure
 */
ehv_context_t* ehv_initialize(const ehv_config_t* config);

/**
 * @brief Shutdown EHV system
 * @param ctx EHV context
 */
void ehv_shutdown(ehv_context_t* ctx);

/**
 * @brief Record an error occurrence
 * @param ctx EHV context
 * @param file_path Source file path
 * @param function_name Function name
 * @param line_number Line number
 * @param category Error category
 * @param priority Error priority
 * @return Status code
 */
crrss_status_t ehv_record_error(
    ehv_context_t* ctx,
    const char* file_path,
    const char* function_name,
    uint32_t line_number,
    bug_category_t category,
    bug_priority_t priority
);

/**
 * @brief Generate heatmap data
 * @param ctx EHV context
 * @param data Output heatmap data
 * @return Status code
 */
crrss_status_t ehv_generate_heatmap(
    ehv_context_t* ctx,
    ehv_heatmap_data_t* data
);

/**
 * @brief Identify error hotspots
 * @param ctx EHV context
 * @param hotspots Output hotspots array
 * @param max_hotspots Maximum hotspots to return
 * @param count Output hotspot count
 * @return Status code
 */
crrss_status_t ehv_identify_hotspots(
    ehv_context_t* ctx,
    ehv_hotspot_t* hotspots,
    uint32_t max_hotspots,
    uint32_t* count
);

/**
 * @brief Cluster related errors
 * @param ctx EHV context
 * @param clusters Output clusters array
 * @param max_clusters Maximum clusters to return
 * @param count Output cluster count
 * @return Status code
 */
crrss_status_t ehv_cluster_errors(
    ehv_context_t* ctx,
    ehv_cluster_t* clusters,
    uint32_t max_clusters,
    uint32_t* count
);

/**
 * @brief Analyze temporal patterns
 * @param ctx EHV context
 * @param patterns Output patterns array
 * @param max_patterns Maximum patterns to return
 * @param count Output pattern count
 * @return Status code
 */
crrss_status_t ehv_analyze_temporal_patterns(
    ehv_context_t* ctx,
    ehv_temporal_pattern_t* patterns,
    uint32_t max_patterns,
    uint32_t* count
);

/**
 * @brief Export heatmap visualization
 * @param ctx EHV context
 * @param format Output format
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t ehv_export_visualization(
    ehv_context_t* ctx,
    ehv_format_t format,
    const char* output_path
);

/**
 * @brief Get heatmap statistics
 * @param ctx EHV context
 * @param total_errors Output total errors
 * @param avg_severity Output average severity
 * @param max_heat Output maximum heat value
 * @return Status code
 */
crrss_status_t ehv_get_statistics(
    ehv_context_t* ctx,
    uint32_t* total_errors,
    double* avg_severity,
    double* max_heat
);

/**
 * @brief Clear heatmap data
 * @param ctx EHV context
 * @return Status code
 */
crrss_status_t ehv_clear_data(ehv_context_t* ctx);

/**
 * @brief Load heatmap data from file
 * @param ctx EHV context
 * @param file_path Input file path
 * @return Status code
 */
crrss_status_t ehv_load_data(ehv_context_t* ctx, const char* file_path);

/**
 * @brief Save heatmap data to file
 * @param ctx EHV context
 * @param file_path Output file path
 * @return Status code
 */
crrss_status_t ehv_save_data(ehv_context_t* ctx, const char* file_path);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_EHV_H
