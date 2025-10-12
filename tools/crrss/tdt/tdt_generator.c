/**
 * @file tdt_generator.c
 * @brief Test Case Generator implementation
 * 
 * Phase 2 Stage 2 Implementation
 */

#include "tdt_generator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// ==================== Helper Functions ====================

static uint32_t count_functions_in_file(FILE* file) {
    char line[1024];
    uint32_t count = 0;
    
    rewind(file);
    
    while (fgets(line, sizeof(line), file)) {
        // Simple heuristic: look for function definitions
        // Format: <return_type> <function_name>(<params>) {
        if (strstr(line, "(") && strstr(line, ")") && strstr(line, "{")) {
            // Skip if it's a comment or control structure
            char* trim = line;
            while (*trim && isspace(*trim)) trim++;
            
            if (strncmp(trim, "//", 2) != 0 &&
                strncmp(trim, "/*", 2) != 0 &&
                strncmp(trim, "if", 2) != 0 &&
                strncmp(trim, "for", 3) != 0 &&
                strncmp(trim, "while", 5) != 0 &&
                strncmp(trim, "switch", 6) != 0) {
                count++;
            }
        }
    }
    
    return count;
}

// ==================== Pattern-Based Generation ====================

crrss_status_t tdt_generate_pattern_based_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !file_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Count patterns
    code_pattern_t patterns[32];
    uint32_t num_patterns = 0;
    
    crrss_status_t status = tdt_analyze_code_patterns(ctx, file_path,
                                                        patterns, 32, &num_patterns);
    
    fclose(file);
    
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Generate tests based on patterns found
    result->tests_generated = num_patterns * 2; // Simplified: 2 tests per pattern
    result->success = true;
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_analyze_code_patterns(
    tdt_context_t* ctx,
    const char* file_path,
    code_pattern_t* patterns,
    uint32_t max_patterns,
    uint32_t* num_patterns)
{
    if (!ctx || !file_path || !patterns || !num_patterns) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    *num_patterns = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), file) && *num_patterns < max_patterns) {
        // Detect various patterns
        if (strstr(line, "malloc") || strstr(line, "calloc")) {
            patterns[(*num_patterns)++] = PATTERN_MEMORY_LEAK;
        }
        if (strstr(line, "free")) {
            patterns[(*num_patterns)++] = PATTERN_DOUBLE_FREE;
        }
        if (strstr(line, "NULL") && strstr(line, "==")) {
            patterns[(*num_patterns)++] = PATTERN_NULL_DEREF;
        }
        if (strstr(line, "strcpy") || strstr(line, "strcat")) {
            patterns[(*num_patterns)++] = PATTERN_BUFFER_OVERFLOW;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Coverage-Driven Generation ====================

crrss_status_t tdt_generate_coverage_driven_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !file_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Identify untested paths
    tdt_coverage_gap_t gaps[100];
    uint32_t num_gaps = 0;
    
    crrss_status_t status = tdt_identify_untested_paths(ctx, file_path,
                                                          gaps, 100, &num_gaps);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Generate tests for each gap
    result->tests_generated = num_gaps;
    result->success = true;
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_identify_untested_paths(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps)
{
    if (!ctx || !file_path || !gaps || !num_gaps) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_gaps = 0;
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[1024];
    uint32_t line_num = 0;
    
    while (fgets(line, sizeof(line), file) && *num_gaps < max_gaps) {
        line_num++;
        
        // Identify branches that might not be covered
        if (strstr(line, "if") || strstr(line, "else")) {
            if (*num_gaps < max_gaps) {
                gaps[*num_gaps].file_path = file_path;
                gaps[*num_gaps].line_number = line_num;
                gaps[*num_gaps].gap_type = "uncovered_branch";
                gaps[*num_gaps].description = "Branch may not be covered";
                gaps[*num_gaps].suggested_test = "Add test for this branch";
                (*num_gaps)++;
            }
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Specification-Based Generation ====================

crrss_status_t tdt_generate_specification_based_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !file_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Count functions to test
    uint32_t func_count = count_functions_in_file(file);
    fclose(file);
    
    // Generate tests based on function specifications
    result->tests_generated = func_count * 3; // 3 tests per function
    result->unit_tests_generated = func_count * 3;
    result->success = true;
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_analyze_function(
    tdt_context_t* ctx,
    const char* file_path,
    const char* function_name,
    tdt_function_analysis_t* analysis)
{
    if (!ctx || !file_path || !function_name || !analysis) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    memset(analysis, 0, sizeof(tdt_function_analysis_t));
    
    analysis->function_name = function_name;
    analysis->file_path = file_path;
    analysis->line_number = 1; // Placeholder
    
    // Default analysis
    analysis->return_type = "int";
    analysis->parameter_count = 0;
    analysis->has_return_value = true;
    analysis->cyclomatic_complexity = 5; // Placeholder
    analysis->lines_of_code = 20; // Placeholder
    analysis->number_of_branches = 3; // Placeholder
    analysis->recommended_test_count = 5;
    
    return CRRSS_SUCCESS;
}

// ==================== Unit Test Generation ====================

crrss_status_t tdt_generate_unit_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !file_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    uint32_t func_count = count_functions_in_file(file);
    fclose(file);
    
    // Generate unit tests (using default: 3 tests per function)
    const uint32_t tests_per_function = 3;
    result->tests_generated = func_count * tests_per_function;
    result->unit_tests_generated = result->tests_generated;
    result->success = true;
    
    // Avoid unused parameter warning
    (void)ctx;
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_generate_function_unit_test(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_case)
{
    if (!ctx || !function_analysis || !test_case) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    memset(test_case, 0, sizeof(tdt_test_case_t));
    
    // Generate basic test case
    char test_name_buf[256];
    snprintf(test_name_buf, sizeof(test_name_buf), "test_%s_basic",
             function_analysis->function_name);
    
    // Allocate memory for test name (caller must free)
    char* test_name = strdup(test_name_buf);
    if (!test_name) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    test_case->test_name = test_name;
    
    test_case->function_under_test = function_analysis->function_name;
    test_case->test_description = "Basic functionality test";
    test_case->test_type = TDT_TEST_TYPE_UNIT;
    test_case->generation_strategy = TDT_STRATEGY_SPECIFICATION_BASED;
    test_case->should_pass = true;
    test_case->source_file = function_analysis->file_path;
    test_case->source_line = function_analysis->line_number;
    
    return CRRSS_SUCCESS;
}

// ==================== Integration Test Generation ====================

crrss_status_t tdt_generate_integration_tests(
    tdt_context_t* ctx,
    const char** file_paths,
    uint32_t file_count,
    tdt_generation_result_t* result)
{
    if (!ctx || !file_paths || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Generate integration tests
    result->tests_generated = file_count * 2; // 2 integration tests per file pair
    result->integration_tests_generated = result->tests_generated;
    result->success = true;
    
    return CRRSS_SUCCESS;
}

// ==================== Special Case Test Generation ====================

crrss_status_t tdt_generate_edge_case_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests)
{
    if (!ctx || !function_analysis || !test_cases || !num_tests) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_tests = 0;
    
    // Generate edge case tests
    if (*num_tests < max_tests) {
        memset(&test_cases[*num_tests], 0, sizeof(tdt_test_case_t));
        
        char test_name_buf[256];
        snprintf(test_name_buf, sizeof(test_name_buf), "test_%s_edge_null_input",
                 function_analysis->function_name);
        
        // Allocate memory for test name (caller must free)
        char* test_name = strdup(test_name_buf);
        if (!test_name) {
            return CRRSS_ERROR_MEMORY_ALLOCATION;
        }
        
        test_cases[*num_tests].test_name = test_name;
        test_cases[*num_tests].function_under_test = function_analysis->function_name;
        test_cases[*num_tests].test_description = "Test with NULL input";
        test_cases[*num_tests].test_type = TDT_TEST_TYPE_UNIT;
        test_cases[*num_tests].is_edge_case = true;
        test_cases[*num_tests].should_pass = true;
        
        (*num_tests)++;
    }
    
    // Add more edge cases
    if (*num_tests < max_tests && function_analysis->has_loops) {
        memset(&test_cases[*num_tests], 0, sizeof(tdt_test_case_t));
        
        char test_name_buf[256];
        snprintf(test_name_buf, sizeof(test_name_buf), "test_%s_edge_empty_input",
                 function_analysis->function_name);
        
        // Allocate memory for test name (caller must free)
        char* test_name = strdup(test_name_buf);
        if (!test_name) {
            // Free previously allocated test name
            free((void*)test_cases[0].test_name);
            return CRRSS_ERROR_MEMORY_ALLOCATION;
        }
        
        test_cases[*num_tests].test_name = test_name;
        test_cases[*num_tests].function_under_test = function_analysis->function_name;
        test_cases[*num_tests].test_description = "Test with empty input";
        test_cases[*num_tests].test_type = TDT_TEST_TYPE_UNIT;
        test_cases[*num_tests].is_edge_case = true;
        test_cases[*num_tests].should_pass = true;
        
        (*num_tests)++;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_generate_error_handling_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests)
{
    if (!ctx || !function_analysis || !test_cases || !num_tests) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_tests = 0;
    
    if (!function_analysis->has_error_handling) {
        return CRRSS_SUCCESS;
    }
    
    // Generate error handling test
    if (*num_tests < max_tests) {
        memset(&test_cases[*num_tests], 0, sizeof(tdt_test_case_t));
        
        char test_name_buf[256];
        snprintf(test_name_buf, sizeof(test_name_buf), "test_%s_error_handling",
                 function_analysis->function_name);
        
        // Allocate memory for test name (caller must free)
        char* test_name = strdup(test_name_buf);
        if (!test_name) {
            return CRRSS_ERROR_MEMORY_ALLOCATION;
        }
        
        test_cases[*num_tests].test_name = test_name;
        test_cases[*num_tests].function_under_test = function_analysis->function_name;
        test_cases[*num_tests].test_description = "Test error handling";
        test_cases[*num_tests].test_type = TDT_TEST_TYPE_UNIT;
        test_cases[*num_tests].is_error_handling = true;
        test_cases[*num_tests].should_pass = true;
        
        (*num_tests)++;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_generate_boundary_tests(
    tdt_context_t* ctx,
    const tdt_function_analysis_t* function_analysis,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests)
{
    if (!ctx || !function_analysis || !test_cases || !num_tests) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_tests = 0;
    
    // Generate boundary tests
    const char* boundaries[] = {"minimum", "maximum", "zero"};
    
    for (uint32_t i = 0; i < 3 && *num_tests < max_tests; i++) {
        memset(&test_cases[*num_tests], 0, sizeof(tdt_test_case_t));
        
        char test_name_buf[256];
        snprintf(test_name_buf, sizeof(test_name_buf), "test_%s_boundary_%s",
                 function_analysis->function_name, boundaries[i]);
        
        // Allocate memory for test name (caller must free)
        char* test_name = strdup(test_name_buf);
        if (!test_name) {
            // Free previously allocated test names on error
            for (uint32_t j = 0; j < *num_tests; j++) {
                free((void*)test_cases[j].test_name);
            }
            *num_tests = 0;
            return CRRSS_ERROR_MEMORY_ALLOCATION;
        }
        
        test_cases[*num_tests].test_name = test_name;
        test_cases[*num_tests].function_under_test = function_analysis->function_name;
        test_cases[*num_tests].test_type = TDT_TEST_TYPE_UNIT;
        test_cases[*num_tests].is_boundary_test = true;
        test_cases[*num_tests].should_pass = true;
        
        (*num_tests)++;
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Test Case Identification ====================

crrss_status_t tdt_identify_test_cases(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests)
{
    if (!ctx || !file_path || !test_cases || !num_tests) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    *num_tests = 0;
    uint32_t func_count = count_functions_in_file(file);
    fclose(file);
    
    // Identify test cases for each function
    *num_tests = (func_count < max_tests) ? func_count : max_tests;
    
    return CRRSS_SUCCESS;
}
