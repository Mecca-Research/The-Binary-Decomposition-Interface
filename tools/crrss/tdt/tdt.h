
/**
 * @file tdt.h
 * @brief Test-Driven Timmy Profile - Comprehensive Test Generation and Coverage Analysis
 * 
 * The TDT (Test-Driven Timmy Profile) provides comprehensive test generation
 * and coverage analysis for the BDI kernel, including:
 * - Automatic test case generation (unit tests and integration tests)
 * - Multiple test generation strategies (pattern-based, coverage-driven, specification-based)
 * - Multi-framework test generation (custom, Unity, Check)
 * - Comprehensive coverage analysis (line, branch, function)
 * - Integration with MSM, STP, and BPME for targeted testing
 * 
 * Phase 2 Stage 2 Implementation
 */

#ifndef TDT_H
#define TDT_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== TDT Configuration ====================

/**
 * @brief Test generation strategies
 */
typedef enum {
    TDT_STRATEGY_PATTERN_BASED = 0,      // Analyze code patterns
    TDT_STRATEGY_COVERAGE_DRIVEN = 1,    // Maximize coverage
    TDT_STRATEGY_SPECIFICATION_BASED = 2, // Based on function specs
    TDT_STRATEGY_ALL = 3                 // Use all strategies
} tdt_generation_strategy_t;

/**
 * @brief Test framework types
 */
typedef enum {
    TDT_FRAMEWORK_CUSTOM = 0,   // Custom CRRSS test framework
    TDT_FRAMEWORK_UNITY = 1,    // Unity test framework
    TDT_FRAMEWORK_CHECK = 2,    // Check test framework
    TDT_FRAMEWORK_ALL = 3       // Generate for all frameworks
} tdt_framework_t;

/**
 * @brief Test types
 */
typedef enum {
    TDT_TEST_TYPE_UNIT = 0,        // Unit tests
    TDT_TEST_TYPE_INTEGRATION = 1,  // Integration tests
    TDT_TEST_TYPE_BOTH = 2          // Both unit and integration
} tdt_test_type_t;

/**
 * @brief TDT configuration structure
 */
typedef struct {
    // Test generation options
    tdt_generation_strategy_t strategy;
    tdt_framework_t framework;
    tdt_test_type_t test_type;
    bool auto_generate_tests;
    
    // Coverage options
    bool track_line_coverage;
    bool track_branch_coverage;
    bool track_function_coverage;
    double target_coverage_percentage;  // 0.0-100.0
    
    // Test generation parameters
    uint32_t max_tests_per_function;
    bool generate_edge_case_tests;
    bool generate_error_handling_tests;
    bool generate_boundary_tests;
    
    // Analysis options
    bool analyze_existing_tests;
    bool identify_coverage_gaps;
    bool suggest_test_improvements;
    
    // Output options
    const char* test_output_directory;
    bool generate_reports;
    bool verbose_output;
    
    // Integration options
    bool integrate_with_msm;
    bool integrate_with_stp;
    bool integrate_with_bpme;
} tdt_config_t;

// ==================== Test Generation Types ====================

/**
 * @brief Test case information
 */
typedef struct {
    const char* test_name;
    const char* function_under_test;
    const char* test_description;
    const char* test_code;
    
    // Test metadata
    tdt_test_type_t test_type;
    tdt_generation_strategy_t generation_strategy;
    bool is_edge_case;
    bool is_error_handling;
    bool is_boundary_test;
    
    // Expected results
    const char* expected_result;
    bool should_pass;
    
    // Source location
    const char* source_file;
    uint32_t source_line;
} tdt_test_case_t;

/**
 * @brief Function analysis for test generation
 */
typedef struct {
    const char* function_name;
    const char* file_path;
    uint32_t line_number;
    
    // Function signature
    const char* return_type;
    uint32_t parameter_count;
    const char** parameter_types;
    const char** parameter_names;
    
    // Function characteristics
    bool has_return_value;
    bool has_error_handling;
    bool has_loops;
    bool has_conditionals;
    bool has_pointer_operations;
    bool has_memory_allocation;
    
    // Complexity metrics
    uint32_t cyclomatic_complexity;
    uint32_t lines_of_code;
    uint32_t number_of_branches;
    
    // Suggested test count
    uint32_t recommended_test_count;
} tdt_function_analysis_t;

// ==================== Coverage Analysis Types ====================

/**
 * @brief Line coverage information
 */
typedef struct {
    uint32_t line_number;
    bool is_executable;
    bool is_covered;
    uint32_t execution_count;
} tdt_line_coverage_t;

/**
 * @brief Branch coverage information
 */
typedef struct {
    uint32_t line_number;
    const char* branch_type;  // "if", "else", "switch", "loop"
    bool true_branch_covered;
    bool false_branch_covered;
    uint32_t true_branch_count;
    uint32_t false_branch_count;
} tdt_branch_coverage_t;

/**
 * @brief Function coverage information
 */
typedef struct {
    const char* function_name;
    bool is_called;
    uint32_t call_count;
    double line_coverage_percent;
    double branch_coverage_percent;
} tdt_function_coverage_t;

/**
 * @brief File coverage information
 */
typedef struct {
    const char* file_path;
    
    // Line coverage
    uint32_t total_lines;
    uint32_t executable_lines;
    uint32_t covered_lines;
    double line_coverage_percent;
    
    // Branch coverage
    uint32_t total_branches;
    uint32_t covered_branches;
    double branch_coverage_percent;
    
    // Function coverage
    uint32_t total_functions;
    uint32_t covered_functions;
    double function_coverage_percent;
    
    // Detailed coverage
    tdt_line_coverage_t* line_coverage;
    uint32_t line_coverage_count;
    tdt_branch_coverage_t* branch_coverage;
    uint32_t branch_coverage_count;
    tdt_function_coverage_t* function_coverage;
    uint32_t function_coverage_count;
} tdt_file_coverage_t;

/**
 * @brief Coverage gap information
 */
typedef struct {
    const char* file_path;
    uint32_t line_number;
    const char* function_name;
    const char* gap_type;  // "uncovered_line", "uncovered_branch", "untested_function"
    const char* description;
    const char* suggested_test;
} tdt_coverage_gap_t;

// ==================== Test Generation Results ====================

/**
 * @brief Test generation result
 */
typedef struct {
    bool success;
    uint32_t tests_generated;
    uint32_t tests_failed_to_generate;
    
    // Generated tests
    tdt_test_case_t* test_cases;
    uint32_t test_case_count;
    uint32_t max_test_cases;
    
    // Test files generated
    const char** generated_test_files;
    uint32_t generated_file_count;
    
    // Generation statistics
    uint32_t unit_tests_generated;
    uint32_t integration_tests_generated;
    uint32_t edge_case_tests_generated;
    uint32_t error_handling_tests_generated;
    
    // Estimated coverage improvement
    double estimated_coverage_improvement;
} tdt_generation_result_t;

// ==================== TDT Statistics ====================

/**
 * @brief TDT statistics
 */
typedef struct {
    // Test generation statistics
    uint32_t total_tests_generated;
    uint32_t unit_tests_generated;
    uint32_t integration_tests_generated;
    uint32_t functions_analyzed;
    uint32_t files_analyzed;
    
    // Coverage statistics
    double average_line_coverage;
    double average_branch_coverage;
    double average_function_coverage;
    uint32_t coverage_gaps_identified;
    
    // Quality metrics
    uint32_t potential_bugs_found;
    uint32_t missing_error_checks_found;
    uint32_t missing_null_checks_found;
    
    // Performance metrics
    double analysis_time_seconds;
    double generation_time_seconds;
} tdt_statistics_t;

/**
 * @brief TDT comprehensive report
 */
typedef struct {
    tdt_statistics_t statistics;
    
    // Test generation results
    tdt_generation_result_t generation_result;
    
    // Coverage analysis
    tdt_file_coverage_t* file_coverage;
    uint32_t file_coverage_count;
    
    // Coverage gaps
    tdt_coverage_gap_t* coverage_gaps;
    uint32_t coverage_gap_count;
    
    // Function analyses
    tdt_function_analysis_t* function_analyses;
    uint32_t function_analysis_count;
    
    // Overall assessment
    double overall_test_quality_score;  // 0.0-1.0
    double overall_coverage_score;      // 0.0-1.0
    const char* summary;
    const char* recommendations;
} tdt_report_t;

// ==================== TDT Context ====================

typedef struct tdt_context tdt_context_t;

// ==================== Initialization & Shutdown ====================

/**
 * @brief Initialize the TDT system
 * @param config Configuration parameters
 * @return Initialized TDT context or NULL on failure
 */
tdt_context_t* tdt_init(const tdt_config_t* config);

/**
 * @brief Shutdown the TDT system and free resources
 * @param ctx TDT context
 */
void tdt_cleanup(tdt_context_t* ctx);

/**
 * @brief Reset TDT state
 * @param ctx TDT context
 * @return Status code
 */
crrss_status_t tdt_reset(tdt_context_t* ctx);

/**
 * @brief Configure TDT system
 * @param ctx TDT context
 * @param config New configuration
 * @return Status code
 */
crrss_status_t tdt_configure(tdt_context_t* ctx, const tdt_config_t* config);

// ==================== Test Generation ====================

/**
 * @brief Analyze a file for test generation
 * @param ctx TDT context
 * @param file_path File to analyze
 * @return Status code
 */
crrss_status_t tdt_analyze_file(tdt_context_t* ctx, const char* file_path);

/**
 * @brief Generate tests for a file
 * @param ctx TDT context
 * @param file_path File to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Generate tests for a specific function
 * @param ctx TDT context
 * @param file_path File containing function
 * @param function_name Function to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_function_tests(
    tdt_context_t* ctx,
    const char* file_path,
    const char* function_name,
    tdt_generation_result_t* result
);

/**
 * @brief Generate tests for an entire directory
 * @param ctx TDT context
 * @param dir_path Directory to analyze
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_directory_tests(
    tdt_context_t* ctx,
    const char* dir_path,
    tdt_generation_result_t* result
);

// ==================== Coverage Analysis ====================

/**
 * @brief Analyze coverage for a file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage Output coverage information
 * @return Status code
 */
crrss_status_t tdt_analyze_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_file_coverage_t* coverage
);

/**
 * @brief Calculate line coverage
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_calculate_line_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Calculate branch coverage
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_calculate_branch_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Calculate function coverage
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param coverage_percent Output coverage percentage
 * @return Status code
 */
crrss_status_t tdt_calculate_function_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent
);

/**
 * @brief Identify coverage gaps
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param gaps Output array for gaps
 * @param max_gaps Maximum gaps to return
 * @param num_gaps Number of gaps found
 * @return Status code
 */
crrss_status_t tdt_identify_coverage_gaps(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps
);

// ==================== Reporting ====================

/**
 * @brief Generate comprehensive TDT report
 * @param ctx TDT context
 * @param report Output report
 * @return Status code
 */
crrss_status_t tdt_generate_report(tdt_context_t* ctx, tdt_report_t* report);

/**
 * @brief Export report to file
 * @param ctx TDT context
 * @param report Report to export
 * @param output_path Output file path
 * @param format Format ("text", "json", "html")
 * @return Status code
 */
crrss_status_t tdt_export_report(
    tdt_context_t* ctx,
    const tdt_report_t* report,
    const char* output_path,
    const char* format
);

/**
 * @brief Get TDT statistics
 * @param ctx TDT context
 * @param stats Output statistics
 * @return Status code
 */
crrss_status_t tdt_get_statistics(tdt_context_t* ctx, tdt_statistics_t* stats);

// ==================== CRRSS Integration ====================

/**
 * @brief Integrate TDT with MSM for memory safety test generation
 * @param ctx TDT context
 * @param msm_ctx MSM context
 * @return Status code
 */
crrss_status_t tdt_integrate_msm(tdt_context_t* ctx, void* msm_ctx);

/**
 * @brief Integrate TDT with STP for type safety test generation
 * @param ctx TDT context
 * @param stp_ctx STP context
 * @return Status code
 */
crrss_status_t tdt_integrate_stp(tdt_context_t* ctx, void* stp_ctx);

/**
 * @brief Integrate TDT with BPME for bug prediction test generation
 * @param ctx TDT context
 * @param bpme_ctx BPME context
 * @return Status code
 */
crrss_status_t tdt_integrate_bpme(tdt_context_t* ctx, void* bpme_ctx);

// ==================== Utility Functions ====================

/**
 * @brief Get string representation of generation strategy
 * @param strategy Generation strategy
 * @return String representation
 */
const char* tdt_strategy_to_string(tdt_generation_strategy_t strategy);

/**
 * @brief Get string representation of framework
 * @param framework Test framework
 * @return String representation
 */
const char* tdt_framework_to_string(tdt_framework_t framework);

/**
 * @brief Get string representation of test type
 * @param test_type Test type
 * @return String representation
 */
const char* tdt_test_type_to_string(tdt_test_type_t test_type);

#ifdef __cplusplus
}
#endif

#endif // TDT_H
