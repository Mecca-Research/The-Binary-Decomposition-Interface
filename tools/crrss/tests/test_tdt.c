/**
 * @file test_tdt.c
 * @brief Comprehensive test suite for Test-Driven Timmy Profile (TDT)
 * 
 * Phase 2 Stage 2 Implementation
 */

#include "../tdt/tdt.h"
#include "../tdt/tdt_generator.h"
#include "../tdt/tdt_coverage.h"
#include "../tdt/tdt_templates.h"
#include "../tdt/tdt_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// ==================== Test Utilities ====================

#define TEST_PASS "\033[32m[PASS]\033[0m"
#define TEST_FAIL "\033[31m[FAIL]\033[0m"
#define TEST_INFO "\033[34m[INFO]\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(test_func) do { \
    printf("\n%s Running: %s\n", TEST_INFO, #test_func); \
    if (test_func()) { \
        printf("%s %s\n", TEST_PASS, #test_func); \
        tests_passed++; \
    } else { \
        printf("%s %s\n", TEST_FAIL, #test_func); \
        tests_failed++; \
    } \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "  Assertion failed: %s at %s:%d\n", \
                #expr, __FILE__, __LINE__); \
        return false; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)
#define ASSERT_EQUAL(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NOT_EQUAL(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_SUCCESS(status) ASSERT_TRUE((status) == CRRSS_SUCCESS)

// ==================== Test File Creation ====================

static const char* create_test_file(const char* filename, const char* content) {
    static char path[256];
    snprintf(path, sizeof(path), "/tmp/%s", filename);
    
    FILE* file = fopen(path, "w");
    if (!file) {
        return NULL;
    }
    
    fputs(content, file);
    fclose(file);
    
    return path;
}

static void cleanup_test_file(const char* path) {
    if (path) {
        unlink(path);
    }
}

// ==================== Test Files Content ====================

static const char* SIMPLE_C_CODE = 
    "#include <stdint.h>\n"
    "#include <stdlib.h>\n"
    "\n"
    "int add(int a, int b) {\n"
    "    return a + b;\n"
    "}\n"
    "\n"
    "int* allocate_array(size_t size) {\n"
    "    int* arr = malloc(size * sizeof(int));\n"
    "    if (!arr) {\n"
    "        return NULL;\n"
    "    }\n"
    "    return arr;\n"
    "}\n"
    "\n"
    "void free_array(int* arr) {\n"
    "    if (arr) {\n"
    "        free(arr);\n"
    "    }\n"
    "}\n";

// ==================== Core TDT Tests ====================

static bool test_tdt_initialization(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_ALL,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_BOTH,
        .auto_generate_tests = true,
        .track_line_coverage = true,
        .track_branch_coverage = true,
        .track_function_coverage = true,
        .target_coverage_percentage = 80.0,
        .max_tests_per_function = 5,
        .generate_edge_case_tests = true,
        .generate_error_handling_tests = true,
        .generate_boundary_tests = true,
        .test_output_directory = "/tmp/tdt_tests",
        .generate_reports = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_null_config(void) {
    tdt_context_t* ctx = tdt_init(NULL);
    ASSERT_NULL(ctx);
    
    return true;
}

static bool test_tdt_reset(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = tdt_reset(ctx);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_configure(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_config_t new_config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_UNITY,
        .test_type = TDT_TEST_TYPE_INTEGRATION,
        .verbose_output = true
    };
    
    crrss_status_t status = tdt_configure(ctx, &new_config);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

// ==================== Test Generation Tests ====================

static bool test_tdt_analyze_file(void) {
    const char* test_file = create_test_file("test_code.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = tdt_analyze_file(ctx, test_file);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_generate_tests(void) {
    const char* test_file = create_test_file("test_code2.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_generation_result_t result;
    crrss_status_t status = tdt_generate_tests(ctx, test_file, &result);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(result.success);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_generate_function_tests(void) {
    const char* test_file = create_test_file("test_code3.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_SPECIFICATION_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .max_tests_per_function = 3,
        .generate_edge_case_tests = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_generation_result_t result;
    crrss_status_t status = tdt_generate_function_tests(ctx, test_file, "add", &result);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.tests_generated > 0);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

// ==================== Coverage Analysis Tests ====================

static bool test_tdt_analyze_coverage(void) {
    const char* test_file = create_test_file("test_code4.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .track_line_coverage = true,
        .track_branch_coverage = true,
        .track_function_coverage = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_file_coverage_t coverage;
    crrss_status_t status = tdt_analyze_coverage(ctx, test_file, &coverage);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(coverage.total_lines > 0);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_calculate_line_coverage(void) {
    const char* test_file = create_test_file("test_code5.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    double coverage_percent = 0.0;
    crrss_status_t status = tdt_calculate_line_coverage(ctx, test_file, &coverage_percent);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(coverage_percent >= 0.0 && coverage_percent <= 100.0);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_calculate_branch_coverage(void) {
    const char* test_file = create_test_file("test_code6.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    double coverage_percent = 0.0;
    crrss_status_t status = tdt_calculate_branch_coverage(ctx, test_file, &coverage_percent);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(coverage_percent >= 0.0 && coverage_percent <= 100.0);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_calculate_function_coverage(void) {
    const char* test_file = create_test_file("test_code7.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    double coverage_percent = 0.0;
    crrss_status_t status = tdt_calculate_function_coverage(ctx, test_file, &coverage_percent);
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(coverage_percent >= 0.0 && coverage_percent <= 100.0);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

static bool test_tdt_identify_coverage_gaps(void) {
    const char* test_file = create_test_file("test_code8.c", SIMPLE_C_CODE);
    ASSERT_NOT_NULL(test_file);
    
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .identify_coverage_gaps = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_coverage_gap_t gaps[100];
    uint32_t num_gaps = 0;
    crrss_status_t status = tdt_identify_coverage_gaps(ctx, test_file, gaps, 100, &num_gaps);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    cleanup_test_file(test_file);
    
    return true;
}

// ==================== Reporting Tests ====================

static bool test_tdt_generate_report(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_ALL,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_BOTH,
        .generate_reports = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_report_t report;
    crrss_status_t status = tdt_generate_report(ctx, &report);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_export_report(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_ALL,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_BOTH,
        .generate_reports = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_report_t report;
    tdt_generate_report(ctx, &report);
    
    const char* report_path = "/tmp/tdt_report.txt";
    crrss_status_t status = tdt_export_report(ctx, &report, report_path, "text");
    ASSERT_SUCCESS(status);
    
    // Verify file was created
    FILE* file = fopen(report_path, "r");
    ASSERT_NOT_NULL(file);
    fclose(file);
    unlink(report_path);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_get_statistics(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    tdt_statistics_t stats;
    crrss_status_t status = tdt_get_statistics(ctx, &stats);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

// ==================== String Conversion Tests ====================

static bool test_tdt_string_conversions(void) {
    const char* str;
    
    str = tdt_strategy_to_string(TDT_STRATEGY_PATTERN_BASED);
    ASSERT_NOT_NULL(str);
    
    str = tdt_framework_to_string(TDT_FRAMEWORK_UNITY);
    ASSERT_NOT_NULL(str);
    
    str = tdt_test_type_to_string(TDT_TEST_TYPE_UNIT);
    ASSERT_NOT_NULL(str);
    
    return true;
}

// ==================== Integration Tests ====================

static bool test_tdt_integrate_msm(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .integrate_with_msm = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = tdt_integrate_msm(ctx, NULL);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_integrate_stp(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .integrate_with_stp = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = tdt_integrate_stp(ctx, NULL);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

static bool test_tdt_integrate_bpme(void) {
    tdt_config_t config = {
        .strategy = TDT_STRATEGY_PATTERN_BASED,
        .framework = TDT_FRAMEWORK_CUSTOM,
        .test_type = TDT_TEST_TYPE_UNIT,
        .integrate_with_bpme = true,
        .verbose_output = false
    };
    
    tdt_context_t* ctx = tdt_init(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = tdt_integrate_bpme(ctx, NULL);
    ASSERT_SUCCESS(status);
    
    tdt_cleanup(ctx);
    
    return true;
}

// ==================== Main Test Runner ====================

int main(void) {
    printf("========================================\n");
    printf("TDT Test Suite\n");
    printf("========================================\n");
    
    // Core TDT Tests
    printf("\n=== Core TDT Tests ===\n");
    RUN_TEST(test_tdt_initialization);
    RUN_TEST(test_tdt_null_config);
    RUN_TEST(test_tdt_reset);
    RUN_TEST(test_tdt_configure);
    
    // Test Generation Tests
    printf("\n=== Test Generation Tests ===\n");
    RUN_TEST(test_tdt_analyze_file);
    RUN_TEST(test_tdt_generate_tests);
    RUN_TEST(test_tdt_generate_function_tests);
    
    // Coverage Analysis Tests
    printf("\n=== Coverage Analysis Tests ===\n");
    RUN_TEST(test_tdt_analyze_coverage);
    RUN_TEST(test_tdt_calculate_line_coverage);
    RUN_TEST(test_tdt_calculate_branch_coverage);
    RUN_TEST(test_tdt_calculate_function_coverage);
    RUN_TEST(test_tdt_identify_coverage_gaps);
    
    // Reporting Tests
    printf("\n=== Reporting Tests ===\n");
    RUN_TEST(test_tdt_generate_report);
    RUN_TEST(test_tdt_export_report);
    RUN_TEST(test_tdt_get_statistics);
    
    // String Conversion Tests
    printf("\n=== String Conversion Tests ===\n");
    RUN_TEST(test_tdt_string_conversions);
    
    // Integration Tests
    printf("\n=== Integration Tests ===\n");
    RUN_TEST(test_tdt_integrate_msm);
    RUN_TEST(test_tdt_integrate_stp);
    RUN_TEST(test_tdt_integrate_bpme);
    
    // Summary
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("========================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
