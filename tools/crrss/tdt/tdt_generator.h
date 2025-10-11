
/**
 * @file tdt_generator.h
 * @brief Test Case Generator - Multiple test generation strategies
 * 
 * Phase 2 Stage 2 Implementation
 */

#ifndef TDT_GENERATOR_H
#define TDT_GENERATOR_H

#include "tdt.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Pattern-Based Generation ====================

/**
 * @brief Generate tests based on code patterns
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_pattern_based_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Analyze code patterns in file
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param patterns Output array for detected patterns
 * @param max_patterns Maximum patterns to return
 * @param num_patterns Number of patterns found
 * @return Status code
 */
crrss_status_t tdt_analyze_code_patterns(
    tdt_context_t* ctx,
    const char* file_path,
    code_pattern_t* patterns,
    uint32_t max_patterns,
    uint32_t* num_patterns
);

// ==================== Coverage-Driven Generation ====================

/**
 * @brief Generate tests to maximize coverage
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_coverage_driven_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Identify untested code paths
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param gaps Output array for untested paths
 * @param max_gaps Maximum gaps to return
 * @param num_gaps Number of gaps found
 * @return Status code
 */
crrss_status_t tdt_identify_untested_paths(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps
);

// ==================== Specification-Based Generation ====================

/**
 * @brief Generate tests based on function specifications
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_specification_based_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Analyze function for test generation
 * @param ctx TDT context
 * @param file_path File containing function
 * @param function_name Function to analyze
 * @param analysis Output function analysis
 * @return Status code
 */
crrss_status_t tdt_analyze_function(
    tdt_context_t* ctx,
    const char* file_path,
    const char* function_name,
    tdt_function_analysis_t* analysis
);

// ==================== Unit Test Generation ====================

/**
 * @brief Generate unit tests for functions
 * @param ctx TDT context
 * @param file_path File to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_unit_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Generate unit test for specific function
 * @param ctx TDT context
 * @param function_analysis Function analysis
 * @param test_case Output test case
 * @return Status code
 */
crrss_status_t tdt_generate_function_unit_test(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_case
);

// ==================== Integration Test Generation ====================

/**
 * @brief Generate integration tests for modules
 * @param ctx TDT context
 * @param file_paths Array of files to test together
 * @param file_count Number of files
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_generate_integration_tests(
    tdt_context_t* ctx,
    const char** file_paths,
    uint32_t file_count,
    tdt_generation_result_t* result
);

// ==================== Special Case Test Generation ====================

/**
 * @brief Generate edge case tests
 * @param ctx TDT context
 * @param function_analysis Function analysis
 * @param test_cases Output array for test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_generate_edge_case_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate error handling tests
 * @param ctx TDT context
 * @param function_analysis Function analysis
 * @param test_cases Output array for test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_generate_error_handling_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate boundary tests
 * @param ctx TDT context
 * @param function_analysis Function analysis
 * @param test_cases Output array for test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_generate_boundary_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

// ==================== Test Case Identification ====================

/**
 * @brief Identify test cases from code analysis
 * @param ctx TDT context
 * @param file_path File to analyze
 * @param test_cases Output array for identified test cases
 * @param max_tests Maximum test cases to identify
 * @param num_tests Number of test cases identified
 * @return Status code
 */
crrss_status_t tdt_identify_test_cases(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

#ifdef __cplusplus
}
#endif

#endif // TDT_GENERATOR_H
