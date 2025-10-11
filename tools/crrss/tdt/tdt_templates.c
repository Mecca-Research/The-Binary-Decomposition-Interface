
/**
 * @file tdt_templates.c
 * @brief Test Template Engine implementation
 * 
 * Phase 2 Stage 2 Implementation
 */

#include "tdt_templates.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ==================== Custom Framework Test Generation ====================

crrss_status_t tdt_template_generate_custom_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length)
{
    if (!ctx || !test_case || !output_code) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Generate test code in CRRSS custom format
    int written = snprintf(output_code, max_code_length,
        "static bool %s(void) {\n"
        "    // %s\n"
        "    ASSERT_NOT_NULL(%s);\n"
        "    return true;\n"
        "}\n",
        test_case->test_name,
        test_case->test_description,
        test_case->function_under_test
    );
    
    if (written < 0 || (size_t)written >= max_code_length) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_write_custom_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path)
{
    if (!ctx || !test_cases || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(output_path, "w");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Write header
    fprintf(file, "/**\n");
    fprintf(file, " * @file %s\n", output_path);
    fprintf(file, " * @brief Auto-generated test file by TDT\n");
    fprintf(file, " */\n\n");
    fprintf(file, "#include <stdio.h>\n");
    fprintf(file, "#include <stdbool.h>\n");
    fprintf(file, "#include <assert.h>\n\n");
    
    // Test utilities
    fprintf(file, "#define ASSERT_TRUE(expr) assert(expr)\n");
    fprintf(file, "#define ASSERT_NOT_NULL(ptr) assert((ptr) != NULL)\n\n");
    
    // Write test cases
    char test_code[4096];
    for (uint32_t i = 0; i < num_tests; i++) {
        if (tdt_template_generate_custom_test(ctx, &test_cases[i],
                                                test_code, sizeof(test_code)) == CRRSS_SUCCESS) {
            fprintf(file, "%s\n", test_code);
        }
    }
    
    // Write main function
    fprintf(file, "int main(void) {\n");
    fprintf(file, "    printf(\"Running %u tests...\\n\");\n", num_tests);
    for (uint32_t i = 0; i < num_tests; i++) {
        fprintf(file, "    if (%s()) printf(\"[PASS] %s\\n\");\n",
                test_cases[i].test_name, test_cases[i].test_name);
    }
    fprintf(file, "    return 0;\n");
    fprintf(file, "}\n");
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Unity Framework Test Generation ====================

crrss_status_t tdt_template_generate_unity_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length)
{
    if (!ctx || !test_case || !output_code) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Generate Unity test
    int written = snprintf(output_code, max_code_length,
        "void %s(void) {\n"
        "    // %s\n"
        "    TEST_ASSERT_NOT_NULL(%s);\n"
        "}\n",
        test_case->test_name,
        test_case->test_description,
        test_case->function_under_test
    );
    
    if (written < 0 || (size_t)written >= max_code_length) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_write_unity_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path)
{
    if (!ctx || !test_cases || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(output_path, "w");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Write header
    fprintf(file, "#include \"unity.h\"\n\n");
    
    // Write setup/teardown
    fprintf(file, "void setUp(void) {}\n");
    fprintf(file, "void tearDown(void) {}\n\n");
    
    // Write test cases
    char test_code[4096];
    for (uint32_t i = 0; i < num_tests; i++) {
        if (tdt_template_generate_unity_test(ctx, &test_cases[i],
                                               test_code, sizeof(test_code)) == CRRSS_SUCCESS) {
            fprintf(file, "%s\n", test_code);
        }
    }
    
    // Write main
    fprintf(file, "int main(void) {\n");
    fprintf(file, "    UNITY_BEGIN();\n");
    for (uint32_t i = 0; i < num_tests; i++) {
        fprintf(file, "    RUN_TEST(%s);\n", test_cases[i].test_name);
    }
    fprintf(file, "    return UNITY_END();\n");
    fprintf(file, "}\n");
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Check Framework Test Generation ====================

crrss_status_t tdt_template_generate_check_test(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_case,
    char* output_code,
    size_t max_code_length)
{
    if (!ctx || !test_case || !output_code) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Generate Check test
    int written = snprintf(output_code, max_code_length,
        "START_TEST(%s)\n"
        "{\n"
        "    // %s\n"
        "    ck_assert_ptr_nonnull(%s);\n"
        "}\n"
        "END_TEST\n",
        test_case->test_name,
        test_case->test_description,
        test_case->function_under_test
    );
    
    if (written < 0 || (size_t)written >= max_code_length) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_write_check_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    const char* output_path)
{
    if (!ctx || !test_cases || !output_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(output_path, "w");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Write header
    fprintf(file, "#include <check.h>\n\n");
    
    // Write test cases
    char test_code[4096];
    for (uint32_t i = 0; i < num_tests; i++) {
        if (tdt_template_generate_check_test(ctx, &test_cases[i],
                                               test_code, sizeof(test_code)) == CRRSS_SUCCESS) {
            fprintf(file, "%s\n", test_code);
        }
    }
    
    // Write suite
    fprintf(file, "Suite* test_suite(void) {\n");
    fprintf(file, "    Suite* s = suite_create(\"Test Suite\");\n");
    fprintf(file, "    TCase* tc_core = tcase_create(\"Core\");\n");
    for (uint32_t i = 0; i < num_tests; i++) {
        fprintf(file, "    tcase_add_test(tc_core, %s);\n", test_cases[i].test_name);
    }
    fprintf(file, "    suite_add_tcase(s, tc_core);\n");
    fprintf(file, "    return s;\n");
    fprintf(file, "}\n\n");
    
    // Write main
    fprintf(file, "int main(void) {\n");
    fprintf(file, "    Suite* s = test_suite();\n");
    fprintf(file, "    SRunner* sr = srunner_create(s);\n");
    fprintf(file, "    srunner_run_all(sr, CK_NORMAL);\n");
    fprintf(file, "    int failed = srunner_ntests_failed(sr);\n");
    fprintf(file, "    srunner_free(sr);\n");
    fprintf(file, "    return (failed == 0) ? 0 : 1;\n");
    fprintf(file, "}\n");
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Generic Test File Writing ====================

crrss_status_t tdt_template_write_test_file(
    tdt_context_t* ctx,
    const tdt_test_case_t* test_cases,
    uint32_t num_tests,
    tdt_framework_t framework,
    const char* output_path)
{
    switch (framework) {
        case TDT_FRAMEWORK_CUSTOM:
            return tdt_template_write_custom_test_file(ctx, test_cases, num_tests, output_path);
        case TDT_FRAMEWORK_UNITY:
            return tdt_template_write_unity_test_file(ctx, test_cases, num_tests, output_path);
        case TDT_FRAMEWORK_CHECK:
            return tdt_template_write_check_test_file(ctx, test_cases, num_tests, output_path);
        default:
            return CRRSS_ERROR_INVALID_PARAM;
    }
}

// ==================== Test Template Utilities ====================

crrss_status_t tdt_template_generate_header(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    const char* file_under_test,
    char* header,
    size_t max_header_length)
{
    if (!ctx || !header) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    const char* framework_include = "";
    switch (framework) {
        case TDT_FRAMEWORK_UNITY:
            framework_include = "#include \"unity.h\"\n";
            break;
        case TDT_FRAMEWORK_CHECK:
            framework_include = "#include <check.h>\n";
            break;
        default:
            framework_include = "#include <assert.h>\n";
            break;
    }
    
    snprintf(header, max_header_length,
        "/**\n"
        " * @file test_%s\n"
        " * @brief Auto-generated tests for %s\n"
        " */\n\n"
        "%s"
        "#include \"%s\"\n\n",
        file_under_test ? file_under_test : "unknown",
        file_under_test ? file_under_test : "unknown",
        framework_include,
        file_under_test ? file_under_test : "unknown.h"
    );
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_generate_footer(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* footer,
    size_t max_footer_length)
{
    if (!ctx || !footer) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Suppress unused parameter warnings
    (void)framework;
    
    snprintf(footer, max_footer_length,
        "\n// End of auto-generated test file\n"
    );
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_generate_setup(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* setup_code,
    size_t max_code_length)
{
    if (!ctx || !setup_code) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (framework == TDT_FRAMEWORK_UNITY) {
        snprintf(setup_code, max_code_length,
            "void setUp(void) {\n"
            "    // Setup code\n"
            "}\n"
        );
    } else {
        setup_code[0] = '\0';
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_template_generate_teardown(
    tdt_context_t* ctx,
    tdt_framework_t framework,
    char* teardown_code,
    size_t max_code_length)
{
    if (!ctx || !teardown_code) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (framework == TDT_FRAMEWORK_UNITY) {
        snprintf(teardown_code, max_code_length,
            "void tearDown(void) {\n"
            "    // Teardown code\n"
            "}\n"
        );
    } else {
        teardown_code[0] = '\0';
    }
    
    return CRRSS_SUCCESS;
}
