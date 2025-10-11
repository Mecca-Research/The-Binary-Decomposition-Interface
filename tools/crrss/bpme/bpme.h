
/**
 * @file bpme.h
 * @brief Bug Prior Mapping Engine - Analyzes code patterns and predicts bugs
 * 
 * The BPME analyzes code patterns using historical bug data from the
 * comprehensive bug analysis (PRs #1-165) to predict potential bugs
 * and assign priority levels.
 */

#ifndef BPME_H
#define BPME_H

#include "../common/crrss_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Configuration ====================
typedef struct {
    const char* knowledge_base_path;  // Path to historical bug database
    bool enable_ml_predictions;        // Use ML-based predictions
    bool enable_pattern_matching;      // Use pattern matching
    double confidence_threshold;       // Minimum confidence (0.0-1.0)
    uint32_t max_predictions;          // Max predictions per file
} bpme_config_t;

// ==================== BPME Context ====================
typedef struct bpme_context bpme_context_t;

// ==================== Initialization ====================
/**
 * @brief Initialize the Bug Prior Mapping Engine
 * @param config Configuration parameters
 * @return Initialized BPME context or NULL on failure
 */
bpme_context_t* bpme_initialize(const bpme_config_t* config);

/**
 * @brief Shutdown the BPME and free resources
 * @param ctx BPME context
 */
void bpme_shutdown(bpme_context_t* ctx);

// ==================== Analysis Functions ====================
/**
 * @brief Analyze a single file for bug patterns
 * @param ctx BPME context
 * @param file_path Path to file to analyze
 * @param predictions Output array for predictions
 * @param max_predictions Maximum predictions to return
 * @param num_predictions Number of predictions found
 * @return Status code
 */
crrss_status_t bpme_analyze_file(
    bpme_context_t* ctx,
    const char* file_path,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
);

/**
 * @brief Analyze an entire directory recursively
 * @param ctx BPME context
 * @param dir_path Path to directory
 * @param predictions Output array for predictions
 * @param max_predictions Maximum predictions to return
 * @param num_predictions Number of predictions found
 * @return Status code
 */
crrss_status_t bpme_analyze_directory(
    bpme_context_t* ctx,
    const char* dir_path,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
);

/**
 * @brief Analyze specific code snippet
 * @param ctx BPME context
 * @param code_snippet Code to analyze
 * @param snippet_length Length of code snippet
 * @param predictions Output array for predictions
 * @param max_predictions Maximum predictions to return
 * @param num_predictions Number of predictions found
 * @return Status code
 */
crrss_status_t bpme_analyze_snippet(
    bpme_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
);

// ==================== Risk Assessment ====================
/**
 * @brief Assess risk for code changes
 * @param ctx BPME context
 * @param file_path File being modified
 * @param diff Diff of changes
 * @param risk_level Output risk level
 * @return Status code
 */
crrss_status_t bpme_assess_change_risk(
    bpme_context_t* ctx,
    const char* file_path,
    const char* diff,
    risk_level_t* risk_level
);

/**
 * @brief Get risk assessment for a component
 * @param ctx BPME context
 * @param component Component to assess
 * @param risk_level Output risk level
 * @return Status code
 */
crrss_status_t bpme_get_component_risk(
    bpme_context_t* ctx,
    component_type_t component,
    risk_level_t* risk_level
);

// ==================== Pattern Database ====================
/**
 * @brief Load historical bug patterns from database
 * @param ctx BPME context
 * @param database_path Path to bug pattern database
 * @return Status code
 */
crrss_status_t bpme_load_pattern_database(
    bpme_context_t* ctx,
    const char* database_path
);

/**
 * @brief Get pattern information
 * @param ctx BPME context
 * @param pattern Pattern type
 * @param info Output pattern information
 * @return Status code
 */
crrss_status_t bpme_get_pattern_info(
    bpme_context_t* ctx,
    code_pattern_t pattern,
    bug_pattern_info_t* info
);

// ==================== Statistics ====================
/**
 * @brief Get BPME statistics
 * @param ctx BPME context
 * @param total_scans Total files scanned
 * @param bugs_predicted Total bugs predicted
 * @param accuracy Prediction accuracy if available
 * @return Status code
 */
crrss_status_t bpme_get_statistics(
    bpme_context_t* ctx,
    uint32_t* total_scans,
    uint32_t* bugs_predicted,
    double* accuracy
);

// ==================== Query Functions ====================
/**
 * @brief Query predictions by priority
 * @param ctx BPME context
 * @param priority Priority level to query
 * @param predictions Output array
 * @param max_predictions Max results
 * @param num_predictions Number of results
 * @return Status code
 */
crrss_status_t bpme_query_by_priority(
    bpme_context_t* ctx,
    bug_priority_t priority,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
);

/**
 * @brief Query predictions by category
 * @param ctx BPME context
 * @param category Category to query
 * @param predictions Output array
 * @param max_predictions Max results
 * @param num_predictions Number of results
 * @return Status code
 */
crrss_status_t bpme_query_by_category(
    bpme_context_t* ctx,
    bug_category_t category,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
);

#ifdef __cplusplus
}
#endif

#endif // BPME_H
