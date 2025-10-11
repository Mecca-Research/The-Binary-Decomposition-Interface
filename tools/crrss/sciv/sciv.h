
/**
 * @file sciv.h
 * @brief Self-Check Internal Validator - Validates code correctness and standards
 * 
 * The SCIV performs static analysis and validation of kernel code including:
 * - Coding standards compliance
 * - Memory management patterns
 * - Error handling validation
 * - API usage verification
 */

#ifndef SCIV_H
#define SCIV_H

#include "../common/crrss_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Validation Rules ====================
typedef enum {
    RULE_MEMORY_SAFETY = 0,
    RULE_ERROR_HANDLING = 1,
    RULE_NULL_CHECKS = 2,
    RULE_CODING_STYLE = 3,
    RULE_FUNCTION_COMPLEXITY = 4,
    RULE_COMMENT_QUALITY = 5,
    RULE_NAMING_CONVENTION = 6,
    RULE_API_USAGE = 7,
    RULE_CONCURRENCY_SAFETY = 8,
    RULE_RESOURCE_CLEANUP = 9,
    RULE_COUNT
} validation_rule_t;

// ==================== Configuration ====================
typedef struct {
    bool enable_strict_mode;
    bool enable_style_checks;
    bool enable_performance_checks;
    uint32_t max_function_complexity;
    uint32_t max_function_lines;
    uint32_t max_cyclomatic_complexity;
    const char* coding_standard;  // "kernel", "misra", "custom"
} sciv_config_t;

// ==================== SCIV Context ====================
typedef struct sciv_context sciv_context_t;

// ==================== Validation Report ====================
typedef struct {
    uint32_t total_files_validated;
    uint32_t total_issues_found;
    uint32_t errors;
    uint32_t warnings;
    uint32_t suggestions;
    double compliance_score;  // 0.0 to 1.0
    validation_issue_t* issues;
    uint32_t max_issues;
} validation_report_t;

// ==================== Initialization ====================
/**
 * @brief Initialize the Self-Check Internal Validator
 * @param config Configuration parameters
 * @return Initialized SCIV context or NULL on failure
 */
sciv_context_t* sciv_initialize(const sciv_config_t* config);

/**
 * @brief Shutdown the SCIV and free resources
 * @param ctx SCIV context
 */
void sciv_shutdown(sciv_context_t* ctx);

// ==================== Validation Functions ====================
/**
 * @brief Validate a single file
 * @param ctx SCIV context
 * @param file_path Path to file to validate
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t sciv_validate_file(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Validate entire directory recursively
 * @param ctx SCIV context
 * @param dir_path Path to directory
 * @param report Output validation report
 * @return Status code
 */
crrss_status_t sciv_validate_directory(
    sciv_context_t* ctx,
    const char* dir_path,
    validation_report_t* report
);

/**
 * @brief Validate code snippet
 * @param ctx SCIV context
 * @param code_snippet Code to validate
 * @param snippet_length Length of code
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t sciv_validate_snippet(
    sciv_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Rule Validation ====================
/**
 * @brief Check specific validation rule
 * @param ctx SCIV context
 * @param file_path File to check
 * @param rule Rule to validate
 * @param result Validation result
 * @return Status code
 */
crrss_status_t sciv_check_rule(
    sciv_context_t* ctx,
    const char* file_path,
    validation_rule_t rule,
    validation_result_t* result
);

/**
 * @brief Enable/disable specific rule
 * @param ctx SCIV context
 * @param rule Rule to configure
 * @param enabled Enable or disable
 * @return Status code
 */
crrss_status_t sciv_configure_rule(
    sciv_context_t* ctx,
    validation_rule_t rule,
    bool enabled
);

// ==================== Memory Validation ====================
/**
 * @brief Validate memory management patterns
 * @param ctx SCIV context
 * @param file_path File to validate
 * @param issues Output issues
 * @param max_issues Max issues
 * @param num_issues Number found
 * @return Status code
 */
crrss_status_t sciv_validate_memory_patterns(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Validate error handling patterns
 * @param ctx SCIV context
 * @param file_path File to validate
 * @param issues Output issues
 * @param max_issues Max issues
 * @param num_issues Number found
 * @return Status code
 */
crrss_status_t sciv_validate_error_handling(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Metrics ====================
/**
 * @brief Calculate code complexity metrics
 * @param ctx SCIV context
 * @param file_path File to analyze
 * @param complexity Output complexity score
 * @return Status code
 */
crrss_status_t sciv_calculate_complexity(
    sciv_context_t* ctx,
    const char* file_path,
    uint32_t* complexity
);

/**
 * @brief Get code quality score
 * @param ctx SCIV context
 * @param file_path File to score
 * @param score Output quality score (0.0-1.0)
 * @return Status code
 */
crrss_status_t sciv_get_quality_score(
    sciv_context_t* ctx,
    const char* file_path,
    double* score
);

// ==================== Statistics ====================
/**
 * @brief Get SCIV statistics
 * @param ctx SCIV context
 * @param total_validations Total validations performed
 * @param total_issues Total issues found
 * @param avg_compliance Average compliance score
 * @return Status code
 */
crrss_status_t sciv_get_statistics(
    sciv_context_t* ctx,
    uint32_t* total_validations,
    uint32_t* total_issues,
    double* avg_compliance
);

// ==================== Reporting ====================
/**
 * @brief Generate validation report
 * @param ctx SCIV context
 * @param output_path Path for report output
 * @param format Report format ("text", "json", "html")
 * @return Status code
 */
crrss_status_t sciv_generate_report(
    sciv_context_t* ctx,
    const char* output_path,
    const char* format
);

#ifdef __cplusplus
}
#endif

#endif // SCIV_H
