
/**
 * @file tdt_templates.h
 * @brief Test Template Engine - Multi-framework test generation
 * 
 * Phase 2 Stage 2 Implementation
 */

#ifndef TDT_TEMPLATES_H
#define TDT_TEMPLATES_H

#include "tdt.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Custom Framework Test Generation ====================

/**
 * @brief Generate test using custom CRRSS framework
 * @param ctx TDT context
 * @param test_case Test case to generate
 * @param output_code Output generated test code
 * @param max_code_length Maximum code length
 * @return Status code
 */
crrss_status_t tdt_template_generate_custom_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length
);

/**
 * @brief Write custom framework test to file
 * @param ctx TDT context
 * @param test_cases Array of test cases
 * @param num_tests Number of test cases
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t tdt_template_write_custom_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path
);

// ==================== Unity Framework Test Generation ====================

/**
 * @brief Generate test using Unity framework
 * @param ctx TDT context
 * @param test_case Test case to generate
 * @param output_code Output generated test code
 * @param max_code_length Maximum code length
 * @return Status code
 */
crrss_status_t tdt_template_generate_unity_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length
);

/**
 * @brief Write Unity framework test to file
 * @param ctx TDT context
 * @param test_cases Array of test cases
 * @param num_tests Number of test cases
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t tdt_template_write_unity_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path
);

// ==================== Check Framework Test Generation ====================

/**
 * @brief Generate test using Check framework
 * @param ctx TDT context
 * @param test_case Test case to generate
 * @param output_code Output generated test code
 * @param max_code_length Maximum code length
 * @return Status code
 */
crrss_status_t tdt_template_generate_check_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length
);

/**
 * @brief Write Check framework test to file
 * @param ctx TDT context
 * @param test_cases Array of test cases
 * @param num_tests Number of test cases
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t tdt_template_write_check_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path
);

// ==================== Generic Test File Writing ====================

/**
 * @brief Write test file using specified framework
 * @param ctx TDT context
 * @param test_cases Array of test cases
 * @param num_tests Number of test cases
 * @param framework Test framework to use
 * @param output_path Output file path
 * @return Status code
 */
crrss_status_t tdt_template_write_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    tdt_framework_t framework,
    const char* output_path
);

// ==================== Test Template Utilities ====================

/**
 * @brief Generate test file header
 * @param ctx TDT context
 * @param framework Test framework
 * @param file_under_test File being tested
 * @param header Output header code
 * @param max_header_length Maximum header length
 * @return Status code
 */
crrss_status_t tdt_template_generate_header(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    const char* file_under_test,
    char* header,
    size_t max_header_length
);

/**
 * @brief Generate test file footer
 * @param ctx TDT context
 * @param framework Test framework
 * @param footer Output footer code
 * @param max_footer_length Maximum footer length
 * @return Status code
 */
crrss_status_t tdt_template_generate_footer(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* footer,
    size_t max_footer_length
);

/**
 * @brief Generate setup function
 * @param ctx TDT context
 * @param framework Test framework
 * @param setup_code Output setup code
 * @param max_code_length Maximum code length
 * @return Status code
 */
crrss_status_t tdt_template_generate_setup(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* setup_code,
    size_t max_code_length
);

/**
 * @brief Generate teardown function
 * @param ctx TDT context
 * @param framework Test framework
 * @param teardown_code Output teardown code
 * @param max_code_length Maximum code length
 * @return Status code
 */
crrss_status_t tdt_template_generate_teardown(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* teardown_code,
    size_t max_code_length
);

#ifdef __cplusplus
}
#endif

#endif // TDT_TEMPLATES_H
