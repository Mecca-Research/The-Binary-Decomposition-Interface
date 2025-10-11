/**
 * @file test_stp.c
 * @brief Comprehensive test suite for Strict Typist Profile (STP)
 * 
 * Phase 2 Stage 1 Implementation
 */

#include "../stp/stp.h"
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

static const char* TYPE_MISMATCH_CODE = 
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    int32_t x = 10;\n"
    "    int64_t y = (int64_t)x;  // Explicit cast\n"
    "    float z = x;  // Type mismatch\n"
    "}\n";

static const char* IMPLICIT_CONVERSION_CODE =
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    int x = 100;\n"
    "    long y = x;  // Implicit conversion\n"
    "    double d = 3.14;\n"
    "    int i = d;  // Implicit conversion with data loss\n"
    "}\n";

static const char* SIGNED_UNSIGNED_MIX_CODE =
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    int signed_val = -10;\n"
    "    unsigned int unsigned_val = 20;\n"
    "    \n"
    "    if (signed_val < unsigned_val) {  // Signed/unsigned comparison\n"
    "        // Potential issue\n"
    "    }\n"
    "}\n";

static const char* TYPE_PUNNING_CODE =
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    uint32_t x = 0x12345678;\n"
    "    float f = *(float*)&x;  // Type punning\n"
    "}\n";

static const char* STRUCT_PADDING_CODE =
    "#include <stdint.h>\n"
    "\n"
    "struct poorly_aligned {\n"
    "    char c;\n"
    "    int i;\n"
    "    char d;\n"
    "    long l;\n"
    "};\n"
    "\n"
    "struct well_aligned {\n"
    "    long l;\n"
    "    int i;\n"
    "    char c;\n"
    "    char d;\n"
    "};\n";

static const char* PACKED_STRUCT_CODE =
    "#include <stdint.h>\n"
    "\n"
    "struct packed_struct {\n"
    "    char c;\n"
    "    int i;\n"
    "    char d;\n"
    "} __attribute__((packed));\n";

static const char* UNSAFE_CAST_CODE =
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    long long big_val = 0x123456789ABCDEF0LL;\n"
    "    int small_val = (int)big_val;  // Unsafe narrowing cast\n"
    "}\n";

static const char* CONST_VIOLATION_CODE =
    "#include <stdint.h>\n"
    "\n"
    "void test_function() {\n"
    "    const int* const_ptr = NULL;\n"
    "    int* non_const = (int*)const_ptr;  // Removes const\n"
    "}\n";

// ==================== Basic Initialization Tests ====================

static bool test_stp_init_valid() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    config.check_type_mismatches = true;
    config.check_implicit_conversions = true;
    config.check_struct_padding = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_init_null_config() {
    stp_context_t* ctx = stp_initialize(NULL);
    ASSERT_NULL(ctx);
    return true;
}

static bool test_stp_shutdown_null() {
    stp_shutdown(NULL);  // Should not crash
    return true;
}

static bool test_stp_reset() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_mismatches = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = stp_reset(ctx);
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_configure() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_mismatches = true;
    config.check_implicit_conversions = true;
    
    crrss_status_t status = stp_configure(ctx, &config);
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_set_strictness_level() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_PERMISSIVE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    crrss_status_t status = stp_set_strictness_level(ctx, STP_STRICTNESS_PARANOID);
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Type Validation Tests ====================

static bool test_stp_detect_type_mismatches() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_mismatches = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("type_mismatch.c", TYPE_MISMATCH_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_type_mismatches(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u type mismatch issues\n", num_issues);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_detect_implicit_conversions() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_implicit_conversions = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("implicit_conv.c", IMPLICIT_CONVERSION_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_implicit_conversions(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u implicit conversion issues\n", num_issues);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_detect_signed_unsigned_mix() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_signed_unsigned_mix = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("signed_unsigned.c", SIGNED_UNSIGNED_MIX_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_signed_unsigned_mix(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u signed/unsigned mix issues\n", num_issues);
    ASSERT_TRUE(num_issues > 0);  // Should detect at least one issue
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_detect_type_punning() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_punning = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("type_punning.c", TYPE_PUNNING_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_type_punning(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u type punning issues\n", num_issues);
    ASSERT_TRUE(num_issues > 0);  // Should detect type punning
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_validate_type_compatibility() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    type_info_t int_type = {0};
    int_type.type_name = "int";
    int_type.type_size = 4;
    int_type.is_signed = true;
    
    type_info_t long_type = {0};
    long_type.type_name = "long";
    long_type.type_size = 8;
    long_type.is_signed = true;
    
    bool is_safe = false;
    crrss_status_t status = stp_validate_type_compatibility(
        ctx, &int_type, &long_type, &is_safe
    );
    
    ASSERT_SUCCESS(status);
    printf("  int -> long conversion is %s\n", is_safe ? "safe" : "unsafe");
    ASSERT_TRUE(is_safe);  // Widening conversion should be safe
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Struct Analysis Tests ====================

static bool test_stp_detect_struct_padding() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_struct_padding = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("struct_padding.c", STRUCT_PADDING_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_struct_padding_issues(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u struct padding issues\n", num_issues);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_detect_packing_issues() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_struct_packing = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("packed_struct.c", PACKED_STRUCT_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_packing_issues(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u struct packing issues\n", num_issues);
    ASSERT_TRUE(num_issues > 0);  // Should detect packed struct
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_analyze_struct_layout() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("struct_layout.c", STRUCT_PADDING_CODE);
    ASSERT_NOT_NULL(test_file);
    
    struct_layout_t layout;
    crrss_status_t status = stp_analyze_struct_layout(
        ctx, "poorly_aligned", test_file, &layout
    );
    
    ASSERT_SUCCESS(status);
    printf("  Struct layout analyzed: %s\n", layout.struct_name);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

// ==================== Type Casting Tests ====================

static bool test_stp_detect_unsafe_casts() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_unsafe_casts = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("unsafe_cast.c", UNSAFE_CAST_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_unsafe_casts(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u unsafe cast issues\n", num_issues);
    ASSERT_TRUE(num_issues > 0);  // Should detect unsafe cast
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_detect_const_violations() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_const_correctness = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("const_violation.c", CONST_VIOLATION_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_detect_const_violations(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Found %u const violation issues\n", num_issues);
    ASSERT_TRUE(num_issues > 0);  // Should detect const removal
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_analyze_cast_safety() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    type_info_t long_type = {0};
    long_type.type_name = "long long";
    long_type.type_size = 8;
    long_type.is_signed = true;
    
    type_info_t int_type = {0};
    int_type.type_name = "int";
    int_type.type_size = 4;
    int_type.is_signed = true;
    
    type_conversion_t conversion;
    crrss_status_t status = stp_analyze_cast_safety(
        ctx, &long_type, &int_type, &conversion
    );
    
    ASSERT_SUCCESS(status);
    printf("  Narrowing conversion: safe=%d, may_lose_data=%d\n",
           conversion.is_safe, conversion.may_lose_data);
    ASSERT_FALSE(conversion.is_safe);  // Narrowing should be unsafe
    ASSERT_TRUE(conversion.may_lose_data);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Comprehensive Analysis Tests ====================

static bool test_stp_analyze_file() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_PARANOID;
    config.check_type_mismatches = true;
    config.check_implicit_conversions = true;
    config.check_signed_unsigned_mix = true;
    config.check_type_punning = true;
    config.check_struct_padding = true;
    config.check_unsafe_casts = true;
    config.check_const_correctness = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("comprehensive.c", TYPE_PUNNING_CODE);
    ASSERT_NOT_NULL(test_file);
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_analyze_file(
        ctx, test_file, issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Comprehensive analysis found %u issues\n", num_issues);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_analyze_snippet() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_mismatches = true;
    config.check_unsafe_casts = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* snippet = 
        "void test() {\n"
        "    long long x = 100;\n"
        "    int y = (int)x;\n"
        "}\n";
    
    stp_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = stp_analyze_snippet(
        ctx, snippet, strlen(snippet), issues, 100, &num_issues
    );
    
    ASSERT_SUCCESS(status);
    printf("  Snippet analysis found %u issues\n", num_issues);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Statistics & Reporting Tests ====================

static bool test_stp_get_statistics() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    config.check_type_mismatches = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    stp_statistics_t stats;
    crrss_status_t status = stp_get_statistics(ctx, &stats);
    
    ASSERT_SUCCESS(status);
    printf("  Statistics retrieved: %u files analyzed\n", stats.files_analyzed);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_generate_report() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_mismatches = true;
    config.check_unsafe_casts = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    // Analyze a file first
    const char* test_file = create_test_file("report_test.c", UNSAFE_CAST_CODE);
    stp_issue_t issues[100];
    uint32_t num_issues;
    stp_analyze_file(ctx, test_file, issues, 100, &num_issues);
    
    stp_report_t report;
    crrss_status_t status = stp_generate_report(ctx, &report);
    
    ASSERT_SUCCESS(status);
    printf("  Report generated: score=%.2f, risk=%d\n",
           report.type_safety_score, report.overall_risk);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_export_report() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    stp_report_t report;
    stp_generate_report(ctx, &report);
    
    const char* output_path = "/tmp/stp_test_report.txt";
    crrss_status_t status = stp_export_report(ctx, &report, output_path, "text");
    
    ASSERT_SUCCESS(status);
    printf("  Report exported to %s\n", output_path);
    
    unlink(output_path);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_calculate_safety_score() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    double score = 0.0;
    crrss_status_t status = stp_calculate_safety_score(ctx, &score);
    
    ASSERT_SUCCESS(status);
    ASSERT_TRUE(score >= 0.0 && score <= 1.0);
    printf("  Safety score: %.2f\n", score);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Integration Tests ====================

static bool test_stp_integrate_bpme() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    config.integrate_with_bpme = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    void* mock_bpme_ctx = (void*)0x1234;
    crrss_status_t status = stp_integrate_bpme(ctx, mock_bpme_ctx);
    
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_integrate_sciv() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    config.integrate_with_sciv = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    void* mock_sciv_ctx = (void*)0x5678;
    crrss_status_t status = stp_integrate_sciv(ctx, mock_sciv_ctx);
    
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_integrate_msm() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    config.integrate_with_msm = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    void* mock_msm_ctx = (void*)0x9ABC;
    crrss_status_t status = stp_integrate_msm(ctx, mock_msm_ctx);
    
    ASSERT_SUCCESS(status);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Query Tests ====================

static bool test_stp_query_by_type() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_STRICT;
    config.check_type_punning = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("query_test.c", TYPE_PUNNING_CODE);
    stp_issue_t all_issues[100];
    uint32_t num_all;
    stp_analyze_file(ctx, test_file, all_issues, 100, &num_all);
    
    stp_issue_t query_issues[100];
    uint32_t num_query = 0;
    
    crrss_status_t status = stp_query_issues_by_type(
        ctx, STP_ISSUE_TYPE_PUNNING, query_issues, 100, &num_query
    );
    
    ASSERT_SUCCESS(status);
    printf("  Query found %u issues of type TYPE_PUNNING\n", num_query);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

static bool test_stp_query_by_priority() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_PARANOID;
    config.check_unsafe_casts = true;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    const char* test_file = create_test_file("priority_test.c", UNSAFE_CAST_CODE);
    stp_issue_t all_issues[100];
    uint32_t num_all;
    stp_analyze_file(ctx, test_file, all_issues, 100, &num_all);
    
    stp_issue_t query_issues[100];
    uint32_t num_query = 0;
    
    crrss_status_t status = stp_query_issues_by_priority(
        ctx, BUG_PRIORITY_P1_HIGH, query_issues, 100, &num_query
    );
    
    ASSERT_SUCCESS(status);
    printf("  Query found %u high priority issues\n", num_query);
    
    cleanup_test_file(test_file);
    stp_shutdown(ctx);
    return true;
}

// ==================== Utility Tests ====================

static bool test_stp_issue_type_to_string() {
    const char* str = stp_issue_type_to_string(STP_ISSUE_TYPE_MISMATCH);
    ASSERT_NOT_NULL(str);
    printf("  Issue type string: %s\n", str);
    return true;
}

static bool test_stp_strictness_level_to_string() {
    const char* str = stp_strictness_level_to_string(STP_STRICTNESS_PARANOID);
    ASSERT_NOT_NULL(str);
    printf("  Strictness level string: %s\n", str);
    return true;
}

static bool test_stp_get_type_info() {
    stp_config_t config = {0};
    config.strictness_level = STP_STRICTNESS_MODERATE;
    
    stp_context_t* ctx = stp_initialize(&config);
    ASSERT_NOT_NULL(ctx);
    
    type_info_t info;
    crrss_status_t status = stp_get_type_info(ctx, "int32_t", &info);
    
    ASSERT_SUCCESS(status);
    printf("  Type info for int32_t: size=%zu, alignment=%zu\n",
           info.type_size, info.type_alignment);
    
    stp_shutdown(ctx);
    return true;
}

// ==================== Main Test Runner ====================

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  STP Test Suite - Phase 2 Stage 1\n");
    printf("========================================\n");
    
    // Basic initialization tests
    printf("\n--- Initialization Tests ---\n");
    RUN_TEST(test_stp_init_valid);
    RUN_TEST(test_stp_init_null_config);
    RUN_TEST(test_stp_shutdown_null);
    RUN_TEST(test_stp_reset);
    RUN_TEST(test_stp_configure);
    RUN_TEST(test_stp_set_strictness_level);
    
    // Type validation tests
    printf("\n--- Type Validation Tests ---\n");
    RUN_TEST(test_stp_detect_type_mismatches);
    RUN_TEST(test_stp_detect_implicit_conversions);
    RUN_TEST(test_stp_detect_signed_unsigned_mix);
    RUN_TEST(test_stp_detect_type_punning);
    RUN_TEST(test_stp_validate_type_compatibility);
    
    // Struct analysis tests
    printf("\n--- Struct Analysis Tests ---\n");
    RUN_TEST(test_stp_detect_struct_padding);
    RUN_TEST(test_stp_detect_packing_issues);
    RUN_TEST(test_stp_analyze_struct_layout);
    
    // Type casting tests
    printf("\n--- Type Casting Tests ---\n");
    RUN_TEST(test_stp_detect_unsafe_casts);
    RUN_TEST(test_stp_detect_const_violations);
    RUN_TEST(test_stp_analyze_cast_safety);
    
    // Comprehensive analysis tests
    printf("\n--- Comprehensive Analysis Tests ---\n");
    RUN_TEST(test_stp_analyze_file);
    RUN_TEST(test_stp_analyze_snippet);
    
    // Statistics & reporting tests
    printf("\n--- Statistics & Reporting Tests ---\n");
    RUN_TEST(test_stp_get_statistics);
    RUN_TEST(test_stp_generate_report);
    RUN_TEST(test_stp_export_report);
    RUN_TEST(test_stp_calculate_safety_score);
    
    // Integration tests
    printf("\n--- Integration Tests ---\n");
    RUN_TEST(test_stp_integrate_bpme);
    RUN_TEST(test_stp_integrate_sciv);
    RUN_TEST(test_stp_integrate_msm);
    
    // Query tests
    printf("\n--- Query Tests ---\n");
    RUN_TEST(test_stp_query_by_type);
    RUN_TEST(test_stp_query_by_priority);
    
    // Utility tests
    printf("\n--- Utility Tests ---\n");
    RUN_TEST(test_stp_issue_type_to_string);
    RUN_TEST(test_stp_strictness_level_to_string);
    RUN_TEST(test_stp_get_type_info);
    
    // Print summary
    printf("\n========================================\n");
    printf("  Test Results Summary\n");
    printf("========================================\n");
    printf("  Tests Passed: %d\n", tests_passed);
    printf("  Tests Failed: %d\n", tests_failed);
    printf("  Total Tests:  %d\n", tests_passed + tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           100.0 * tests_passed / (tests_passed + tests_failed));
    printf("========================================\n\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
