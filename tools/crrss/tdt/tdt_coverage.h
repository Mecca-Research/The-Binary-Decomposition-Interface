
/**
 * @file tdt_coverage.h
 * @brief Coverage Analysis Engine - Line, branch, and function coverage tracking
 * 
 * Phase 2 Stage 2 Implementation
 */

#ifndef TDT_COVERAGE_H
#define TDT_COVERAGE_H

#include "tdt.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Coverage Analysis ====================

/**
 * @brief Analyze complete coverage for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage Output coverage information
 * @return Status code
 */
crrss_status_t tdt_coverage_analyze_file(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_file_coverage_t* coverage
);

// ==================== Line Coverage ====================

/**
 * @brief Calculate line coverage for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_coverage_calculate_line(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Get detailed line coverage information
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param line_coverage Output array for line coverage
 * @param max_lines Maximum lines to return
 * @param num_lines Number of lines returned
 * @return Status code
 */
crrss_status_t tdt_coverage_get_line_details(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_line_coverage_t* line_coverage,
    uint32_t max_lines,
    uint32_t* num_lines
);

/**
 * @brief Identify uncovered lines
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param uncovered_lines Output array for uncovered line numbers
 * @param max_lines Maximum lines to return
 * @param num_lines Number of uncovered lines
 * @return Status code
 */
crrss_status_t tdt_coverage_identify_uncovered_lines(
    tdt_context_t* ctx,
    const char* file_path,
    uint32_t* uncovered_lines,
    uint32_t max_lines,
    uint32_t* num_lines
);

// ==================== Branch Coverage ====================

/**
 * @brief Calculate branch coverage for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_coverage_calculate_branch(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Get detailed branch coverage information
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param branch_coverage Output array for branch coverage
 * @param max_branches Maximum branches to return
 * @param num_branches Number of branches returned
 * @return Status code
 */
crrss_status_t tdt_coverage_get_branch_details(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_branch_coverage_t* branch_coverage,
    uint32_t max_branches,
    uint32_t* num_branches
);

/**
 * @brief Identify uncovered branches
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param gaps Output array for branch gaps
 * @param max_gaps Maximum gaps to return
 * @param num_gaps Number of gaps found
 * @return Status code
 */
crrss_status_t tdt_coverage_identify_uncovered_branches(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps
);

// ==================== Function Coverage ====================

/**
 * @brief Calculate function coverage for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_coverage_calculate_function(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Get detailed function coverage information
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param function_coverage Output array for function coverage
 * @param max_functions Maximum functions to return
 * @param num_functions Number of functions returned
 * @return Status code
 */
crrss_status_t tdt_coverage_get_function_details(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_function_coverage_t* function_coverage,
    uint32_t max_functions,
    uint32_t* num_functions
);

/**
 * @brief Identify untested functions
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param function_names Output array for untested function names
 * @param max_functions Maximum functions to return
 * @param num_functions Number of untested functions
 * @return Status code
 */
crrss_status_t tdt_coverage_identify_untested_functions(
    tdt_context_t* ctx,
    const char* file_path,
    const char** function_names,
    uint32_t max_functions,
    uint32_t* num_functions
);

// ==================== Coverage Gap Analysis ====================

/**
 * @brief Identify all coverage gaps in a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param gaps Output array for gaps
 * @param max_gaps Maximum gaps to return
 * @param num_gaps Number of gaps found
 * @return Status code
 */
crrss_status_t tdt_coverage_identify_gaps(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps
);

/**
 * @brief Generate suggestions for improving coverage
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param gaps Coverage gaps
 * @param num_gaps Number of gaps
 * @return Status code
 */
crrss_status_t tdt_coverage_suggest_improvements(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t num_gaps
);

// ==================== Coverage Reporting ====================

/**
 * @brief Generate coverage report for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage Output coverage information
 * @return Status code
 */
crrss_status_t tdt_coverage_generate_report(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_file_coverage_t* coverage
);

/**
 * @brief Generate coverage report for a directory
 * @param ctx TDT context
 * @param dir_path Directory to analyze
 * @param coverage_array Output array for file coverage
 * @param max_files Maximum files to report
 * @param num_files Number of files analyzed
 * @return Status code
 */
crrss_status_t tdt_coverage_generate_directory_report(
    tdt_context_t* ctx,
    const char* dir_path,
    tdt_file_coverage_t* coverage_array,
    uint32_t max_files,
    uint32_t* num_files
);

#ifdef __cplusplus
}
#endif

#endif // TDT_COVERAGE_H
