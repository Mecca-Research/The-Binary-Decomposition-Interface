
/**
 * @file test_msm.c
 * @brief Comprehensive test suite for Memory-Safety Maniac Profile (MSM)
 * 
 * Tests all MSM functionality including:
 * - Initialization and configuration
 * - Allocation tracking
 * - Pointer safety analysis
 * - NULL-check enforcement
 * - Buffer overflow detection
 * - Memory leak detection
 * - Use-after-free detection
 * - Double-free detection
 * - Static code analysis
 * - Integration with CRRSS components
 */

#include "../msm/msm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Macros for testing
#define TEST_START(name) \
    printf("\n[TEST] %s ... ", name); \
    fflush(stdout); \
    tests_run++;

#define TEST_PASS() \
    printf("✓ PASS\n"); \
    tests_passed++;

#define TEST_FAIL(msg) \
    printf("✗ FAIL: %s\n", msg); \
    tests_failed++;

#define ASSERT_TRUE(condition, msg) \
    if (!(condition)) { \
        TEST_FAIL(msg); \
        return; \
    }

#define ASSERT_FALSE(condition, msg) \
    if (condition) { \
        TEST_FAIL(msg); \
        return; \
    }

#define ASSERT_EQUAL(actual, expected, msg) \
    if ((actual) != (expected)) { \
        char fail_msg[256]; \
        snprintf(fail_msg, sizeof(fail_msg), "%s (expected: %d, actual: %d)", \
                 msg, (int)(expected), (int)(actual)); \
        TEST_FAIL(fail_msg); \
        return; \
    }

#define ASSERT_NOT_NULL(ptr, msg) \
    if ((ptr) == NULL) { \
        TEST_FAIL(msg); \
        return; \
    }

#define ASSERT_NULL(ptr, msg) \
    if ((ptr) != NULL) { \
        TEST_FAIL(msg); \
        return; \
    }

#define ASSERT_STATUS(status, expected, msg) \
    if ((status) != (expected)) { \
        char fail_msg[256]; \
        snprintf(fail_msg, sizeof(fail_msg), "%s (status: %d, expected: %d)", \
                 msg, (int)(status), (int)(expected)); \
        TEST_FAIL(fail_msg); \
        return; \
    }

// ==================== Helper Functions ====================

/**
 * @brief Create a default MSM configuration for testing
 */
static msm_config_t create_default_config(void) {
    msm_config_t config;
    memset(&config, 0, sizeof(msm_config_t));
    
    config.tracking_mode = MSM_TRACKING_DETAILED;
    config.enable_pointer_tracking = true;
    config.enable_allocation_tracking = true;
    config.enable_null_check_enforcement = true;
    config.enable_buffer_overflow_detection = true;
    config.enable_use_after_free_detection = true;
    config.enable_double_free_detection = true;
    config.enable_leak_detection = true;
    
    config.max_tracked_pointers = 1000;
    config.max_tracked_allocations = 1000;
    config.max_stack_depth = 10;
    
    config.generate_reports = true;
    config.track_allocation_sites = true;
    
    return config;
}

/**
 * @brief Create a test file with sample code
 */
static void create_test_file(const char* filename, const char* content) {
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fputs(content, fp);
        fclose(fp);
    }
}

// ==================== Initialization Tests ====================

static void test_msm_initialization(void) {
    TEST_START("MSM Initialization");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Verify we can get statistics
    msm_statistics_t stats;
    crrss_status_t status = msm_get_statistics(ctx, &stats);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to get statistics");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_msm_invalid_initialization(void) {
    TEST_START("MSM Invalid Initialization");
    
    msm_context_t* ctx = msm_initialize(NULL);
    ASSERT_NULL(ctx, "Should fail with NULL config");
    
    TEST_PASS();
}

static void test_msm_reset(void) {
    TEST_START("MSM Reset");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track some allocations
    void* test_addr = (void*)0x1000;
    msm_track_allocation(ctx, test_addr, 100, "test.c", 10, "test_func");
    
    // Reset
    crrss_status_t status = msm_reset(ctx);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to reset MSM");
    
    // Verify statistics are reset
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_EQUAL(stats.total_allocations_tracked, 0, "Statistics not reset");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Allocation Tracking Tests ====================

static void test_allocation_tracking(void) {
    TEST_START("Allocation Tracking");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocation
    void* test_addr = (void*)0x1000;
    crrss_status_t status = msm_track_allocation(ctx, test_addr, 256, 
                                                  "test.c", 10, "test_func");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to track allocation");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_EQUAL(stats.total_allocations_tracked, 1, "Allocation not tracked");
    ASSERT_EQUAL(stats.current_allocations, 1, "Current allocations incorrect");
    ASSERT_EQUAL(stats.total_memory_tracked, 256, "Total memory incorrect");
    
    // Get metadata
    allocation_metadata_t meta;
    status = msm_get_allocation_metadata(ctx, test_addr, &meta);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to get allocation metadata");
    ASSERT_EQUAL(meta.size, 256, "Metadata size incorrect");
    ASSERT_FALSE(meta.is_freed, "Metadata should not be freed");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_deallocation_tracking(void) {
    TEST_START("Deallocation Tracking");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocation
    void* test_addr = (void*)0x1000;
    msm_track_allocation(ctx, test_addr, 256, "test.c", 10, "test_func");
    
    // Track deallocation
    crrss_status_t status = msm_track_deallocation(ctx, test_addr, 
                                                    "test.c", 20, "test_func");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to track deallocation");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_EQUAL(stats.total_deallocations_tracked, 1, "Deallocation not tracked");
    ASSERT_EQUAL(stats.current_allocations, 0, "Current allocations should be 0");
    
    // Verify metadata
    allocation_metadata_t meta;
    status = msm_get_allocation_metadata(ctx, test_addr, &meta);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to get allocation metadata");
    ASSERT_TRUE(meta.is_freed, "Metadata should be marked as freed");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_double_free_detection(void) {
    TEST_START("Double-Free Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocation
    void* test_addr = (void*)0x1000;
    msm_track_allocation(ctx, test_addr, 256, "test.c", 10, "test_func");
    
    // First free - should succeed
    crrss_status_t status = msm_track_deallocation(ctx, test_addr, 
                                                    "test.c", 20, "test_func");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "First deallocation should succeed");
    
    // Second free - should detect double-free
    status = msm_track_deallocation(ctx, test_addr, "test.c", 30, "test_func");
    ASSERT_STATUS(status, CRRSS_ERROR_VALIDATION_FAILED, 
                  "Should detect double-free");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_TRUE(stats.double_free_detected > 0, "Double-free not detected");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Pointer Tracking Tests ====================

static void test_pointer_tracking(void) {
    TEST_START("Pointer Tracking");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track pointer
    void* ptr_addr = (void*)0x2000;
    void* points_to = (void*)0x3000;
    crrss_status_t status = msm_track_pointer(ctx, ptr_addr, points_to, 
                                              "test.c", 10, "test_func");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to track pointer");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_TRUE(stats.total_pointers_tracked > 0, "Pointer not tracked");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_pointer_validation(void) {
    TEST_START("Pointer Validation");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocation
    void* test_addr = (void*)0x1000;
    msm_track_allocation(ctx, test_addr, 256, "test.c", 10, "test_func");
    
    // Validate pointer to allocated memory
    bool is_valid = false;
    crrss_status_t status = msm_validate_pointer(ctx, test_addr, &is_valid);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to validate pointer");
    ASSERT_TRUE(is_valid, "Pointer to allocated memory should be valid");
    
    // Free the memory
    msm_track_deallocation(ctx, test_addr, "test.c", 20, "test_func");
    
    // Validate again - should be invalid
    status = msm_validate_pointer(ctx, test_addr, &is_valid);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to validate pointer");
    ASSERT_FALSE(is_valid, "Pointer to freed memory should be invalid");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_use_after_free_detection(void) {
    TEST_START("Use-After-Free Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocation
    void* test_addr = (void*)0x1000;
    msm_track_allocation(ctx, test_addr, 256, "test.c", 10, "test_func");
    
    // Track pointer to allocation
    void* ptr_addr = (void*)0x2000;
    msm_track_pointer(ctx, ptr_addr, test_addr, "test.c", 15, "test_func");
    
    // Free the memory
    msm_track_deallocation(ctx, test_addr, "test.c", 20, "test_func");
    
    // Access through pointer - should detect use-after-free
    crrss_status_t status = msm_track_pointer_access(ctx, ptr_addr, 
                                                      POINTER_ACCESS_READ,
                                                      "test.c", 25);
    ASSERT_STATUS(status, CRRSS_ERROR_VALIDATION_FAILED, 
                  "Should detect use-after-free");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_TRUE(stats.use_after_free_detected > 0, "Use-after-free not detected");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Static Analysis Tests ====================

static void test_use_after_free_static_detection(void) {
    TEST_START("Use-After-Free Static Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with use-after-free
    const char* test_code = 
        "void test_function() {\n"
        "    char* ptr = malloc(100);\n"
        "    free(ptr);\n"
        "    ptr[0] = 'A';  // Use after free!\n"
        "}\n";
    
    create_test_file("/tmp/test_uaf.c", test_code);
    
    // Detect use-after-free
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_detect_use_after_free(ctx, "/tmp/test_uaf.c",
                                                       issues, 10, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to detect use-after-free");
    ASSERT_TRUE(num_issues > 0, "Should detect use-after-free in code");
    
    // Cleanup
    unlink("/tmp/test_uaf.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_double_free_static_detection(void) {
    TEST_START("Double-Free Static Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with double-free
    const char* test_code = 
        "void test_function() {\n"
        "    char* ptr = malloc(100);\n"
        "    free(ptr);\n"
        "    free(ptr);  // Double free!\n"
        "}\n";
    
    create_test_file("/tmp/test_df.c", test_code);
    
    // Detect double-free
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_detect_double_free(ctx, "/tmp/test_df.c",
                                                    issues, 10, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to detect double-free");
    ASSERT_TRUE(num_issues > 0, "Should detect double-free in code");
    
    // Cleanup
    unlink("/tmp/test_df.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_null_check_analysis(void) {
    TEST_START("NULL-Check Analysis");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with missing NULL check
    const char* test_code = 
        "void test_function(char* ptr) {\n"
        "    ptr->field = 10;  // Missing NULL check!\n"
        "}\n";
    
    create_test_file("/tmp/test_null.c", test_code);
    
    // Analyze NULL checks
    null_check_result_t results[10];
    uint32_t num_results = 0;
    crrss_status_t status = msm_analyze_null_checks(ctx, "/tmp/test_null.c",
                                                     results, 10, &num_results);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to analyze NULL checks");
    ASSERT_TRUE(num_results > 0, "Should detect missing NULL check");
    
    // Cleanup
    unlink("/tmp/test_null.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_buffer_overflow_detection(void) {
    TEST_START("Buffer Overflow Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with unsafe functions
    const char* test_code = 
        "void test_function() {\n"
        "    char buf[10];\n"
        "    strcpy(buf, input);  // Unsafe!\n"
        "    strcat(buf, more);   // Unsafe!\n"
        "}\n";
    
    create_test_file("/tmp/test_overflow.c", test_code);
    
    // Detect buffer overflow
    buffer_analysis_result_t results[10];
    uint32_t num_results = 0;
    crrss_status_t status = msm_detect_buffer_overflow(ctx, "/tmp/test_overflow.c",
                                                        results, 10, &num_results);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to detect buffer overflow");
    ASSERT_TRUE(num_results > 0, "Should detect unsafe functions");
    
    // Cleanup
    unlink("/tmp/test_overflow.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_buffer_access_checking(void) {
    TEST_START("Buffer Access Checking");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    char buffer[100];
    
    // Valid access - should succeed
    crrss_status_t status = msm_check_buffer_access(ctx, buffer, 100, 0, 50,
                                                     "test.c", 10);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Valid access should succeed");
    
    // Overflow access - should fail
    status = msm_check_buffer_access(ctx, buffer, 100, 90, 20, "test.c", 20);
    ASSERT_STATUS(status, CRRSS_ERROR_VALIDATION_FAILED, 
                  "Should detect buffer overflow");
    
    // Verify statistics
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_TRUE(stats.buffer_overflow_detected > 0, "Buffer overflow not detected");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Memory Leak Tests ====================

static void test_memory_leak_detection(void) {
    TEST_START("Memory Leak Detection");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track multiple allocations
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    void* addr3 = (void*)0x3000;
    
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_allocation(ctx, addr2, 200, "test.c", 20, "test_func");
    msm_track_allocation(ctx, addr3, 300, "test.c", 30, "test_func");
    
    // Free only one
    msm_track_deallocation(ctx, addr2, "test.c", 40, "test_func");
    
    // Detect leaks
    allocation_metadata_t leaks[10];
    uint32_t num_leaks = 0;
    crrss_status_t status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to detect leaks");
    ASSERT_EQUAL(num_leaks, 2, "Should detect 2 leaks");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_memory_leak_static_analysis(void) {
    TEST_START("Memory Leak Static Analysis");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with memory leak
    const char* test_code = 
        "void test_function() {\n"
        "    char* ptr = malloc(100);\n"
        "    if (error) {\n"
        "        return;  // Leak!\n"
        "    }\n"
        "    free(ptr);\n"
        "}\n";
    
    create_test_file("/tmp/test_leak.c", test_code);
    
    // Analyze for leaks
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_analyze_memory_leaks(ctx, "/tmp/test_leak.c",
                                                      issues, 10, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to analyze memory leaks");
    ASSERT_TRUE(num_issues > 0, "Should detect memory leak");
    
    // Cleanup
    unlink("/tmp/test_leak.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Comprehensive Analysis Tests ====================

static void test_file_analysis(void) {
    TEST_START("Comprehensive File Analysis");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file with multiple issues
    const char* test_code = 
        "void test_function() {\n"
        "    char* ptr = malloc(100);\n"
        "    strcpy(ptr, input);  // Unsafe\n"
        "    free(ptr);\n"
        "    ptr[0] = 'A';  // Use after free\n"
        "}\n";
    
    create_test_file("/tmp/test_comprehensive.c", test_code);
    
    // Analyze file
    msm_issue_t issues[20];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_analyze_file(ctx, "/tmp/test_comprehensive.c",
                                             issues, 20, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to analyze file");
    ASSERT_TRUE(num_issues > 0, "Should detect multiple issues");
    
    // Cleanup
    unlink("/tmp/test_comprehensive.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_snippet_analysis(void) {
    TEST_START("Code Snippet Analysis");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    const char* snippet = 
        "char* ptr = malloc(100);\n"
        "free(ptr);\n"
        "free(ptr);  // Double free!\n";
    
    // Analyze snippet
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_analyze_snippet(ctx, snippet, strlen(snippet),
                                                 issues, 10, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to analyze snippet");
    ASSERT_TRUE(num_issues > 0, "Should detect issues in snippet");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Reporting Tests ====================

static void test_statistics_generation(void) {
    TEST_START("Statistics Generation");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track some operations
    void* addr1 = (void*)0x1000;
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 20, "test_func");
    
    // Get statistics
    msm_statistics_t stats;
    crrss_status_t status = msm_get_statistics(ctx, &stats);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to get statistics");
    
    ASSERT_EQUAL(stats.total_allocations_tracked, 1, "Incorrect allocation count");
    ASSERT_EQUAL(stats.total_deallocations_tracked, 1, "Incorrect deallocation count");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_report_generation(void) {
    TEST_START("Report Generation");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track some issues
    void* addr1 = (void*)0x1000;
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 20, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 30, "test_func");  // Double-free
    
    // Generate report
    msm_report_t report;
    crrss_status_t status = msm_generate_report(ctx, &report);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to generate report");
    
    ASSERT_TRUE(report.issue_count > 0, "Report should contain issues");
    ASSERT_TRUE(report.safety_score >= 0.0 && report.safety_score <= 1.0,
                "Safety score out of range");
    
    // Export report
    status = msm_export_report(ctx, &report, "/tmp/msm_test_report.txt", "text");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to export text report");
    
    status = msm_export_report(ctx, &report, "/tmp/msm_test_report.json", "json");
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to export JSON report");
    
    // Cleanup
    if (report.issues) free(report.issues);
    if (report.leak_records) free(report.leak_records);
    unlink("/tmp/msm_test_report.txt");
    unlink("/tmp/msm_test_report.json");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_safety_score_calculation(void) {
    TEST_START("Safety Score Calculation");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Calculate score with no issues
    double score = 0.0;
    crrss_status_t status = msm_calculate_safety_score(ctx, &score);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to calculate safety score");
    ASSERT_TRUE(score == 1.0, "Score should be 1.0 with no issues");
    
    // Add some issues and recalculate
    void* addr1 = (void*)0x1000;
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 20, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 30, "test_func");  // Double-free
    
    status = msm_calculate_safety_score(ctx, &score);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to calculate safety score");
    ASSERT_TRUE(score < 1.0, "Score should be less than 1.0 with issues");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Query Tests ====================

static void test_issue_query_by_type(void) {
    TEST_START("Issue Query by Type");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create test file and analyze
    const char* test_code = 
        "void test() {\n"
        "    char* p = malloc(100);\n"
        "    free(p);\n"
        "    free(p);\n"
        "}\n";
    create_test_file("/tmp/test_query.c", test_code);
    
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    msm_analyze_file(ctx, "/tmp/test_query.c", issues, 10, &num_issues);
    
    // Query by type
    msm_issue_t queried[10];
    uint32_t num_queried = 0;
    crrss_status_t status = msm_query_issues_by_type(ctx, MSM_ISSUE_DOUBLE_FREE,
                                                      queried, 10, &num_queried);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to query by type");
    
    // Cleanup
    unlink("/tmp/test_query.c");
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_issue_query_by_priority(void) {
    TEST_START("Issue Query by Priority");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Create issues with different priorities
    void* addr1 = (void*)0x1000;
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 20, "test_func");
    msm_track_deallocation(ctx, addr1, "test.c", 30, "test_func");  // Critical
    
    // Query by priority
    msm_issue_t issues[10];
    uint32_t num_issues = 0;
    crrss_status_t status = msm_query_issues_by_priority(ctx, BUG_PRIORITY_P0_CRITICAL,
                                                          issues, 10, &num_issues);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to query by priority");
    ASSERT_TRUE(num_issues > 0, "Should find critical issues");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Integration Tests ====================

static void test_integration_setup(void) {
    TEST_START("Integration Setup");
    
    msm_config_t config = create_default_config();
    config.integrate_with_bpme = true;
    config.integrate_with_sciv = true;
    config.integrate_with_memory_layer = true;
    
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Simulate integration
    void* dummy_bpme = (void*)0x1000;
    void* dummy_sciv = (void*)0x2000;
    void* dummy_memory = (void*)0x3000;
    
    crrss_status_t status = msm_integrate_bpme(ctx, dummy_bpme);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to integrate BPME");
    
    status = msm_integrate_sciv(ctx, dummy_sciv);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to integrate SCIV");
    
    status = msm_integrate_memory_layer(ctx, dummy_memory);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to integrate Memory Layer");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

// ==================== Utility Function Tests ====================

static void test_utility_functions(void) {
    TEST_START("Utility Functions");
    
    // Test string conversion functions
    const char* issue_str = msm_issue_type_to_string(MSM_ISSUE_USE_AFTER_FREE);
    ASSERT_TRUE(strcmp(issue_str, "Use-After-Free") == 0, 
                "Incorrect issue type string");
    
    const char* state_str = msm_pointer_state_to_string(POINTER_STATE_VALID);
    ASSERT_TRUE(strcmp(state_str, "Valid") == 0, "Incorrect pointer state string");
    
    const char* mode_str = msm_tracking_mode_to_string(MSM_TRACKING_DETAILED);
    ASSERT_TRUE(strcmp(mode_str, "Detailed") == 0, "Incorrect tracking mode string");
    
    TEST_PASS();
}

// ==================== Main Test Runner ====================

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  MSM Test Suite\n");
    printf("========================================\n");
    
    // Initialization tests
    test_msm_initialization();
    test_msm_invalid_initialization();
    test_msm_reset();
    
    // Allocation tracking tests
    test_allocation_tracking();
    test_deallocation_tracking();
    test_double_free_detection();
    
    // Pointer tracking tests
    test_pointer_tracking();
    test_pointer_validation();
    test_use_after_free_detection();
    
    // Static analysis tests
    test_use_after_free_static_detection();
    test_double_free_static_detection();
    test_null_check_analysis();
    test_buffer_overflow_detection();
    test_buffer_access_checking();
    
    // Memory leak tests
    test_memory_leak_detection();
    test_memory_leak_static_analysis();
    
    // Comprehensive analysis tests
    test_file_analysis();
    test_snippet_analysis();
    
    // Reporting tests
    test_statistics_generation();
    test_report_generation();
    test_safety_score_calculation();
    
    // Query tests
    test_issue_query_by_type();
    test_issue_query_by_priority();
    
    // Integration tests
    test_integration_setup();
    
    // Utility tests
    test_utility_functions();
    
    // Print summary
    printf("\n");
    printf("========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("  Total Tests:  %d\n", tests_run);
    printf("  Passed:       %d (%.1f%%)\n", tests_passed, 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("  Failed:       %d\n", tests_failed);
    printf("========================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
