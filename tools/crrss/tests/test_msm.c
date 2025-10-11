
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

// ==================== MSM Reset Leak Counting Bug Test ====================

static void test_reset_clears_leak_counting_flags(void) {
    TEST_START("MSM Reset Clears Leak Counting Flags");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocations without freeing (simulating leaks)
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    void* addr3 = (void*)0x3000;
    
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_allocation(ctx, addr2, 200, "test.c", 20, "test_func");
    msm_track_allocation(ctx, addr3, 300, "test.c", 30, "test_func");
    
    // First leak detection - should count 3 leaks
    allocation_metadata_t leaks[10];
    uint32_t num_leaks = 0;
    crrss_status_t status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "First leak detection failed");
    ASSERT_EQUAL(num_leaks, 3, "Should detect 3 leaks");
    
    // Get statistics after first detection
    msm_statistics_t stats1;
    msm_get_statistics(ctx, &stats1);
    ASSERT_EQUAL(stats1.memory_leaks_detected, 3, "Should have 3 leaks counted");
    
    // Second leak detection WITHOUT reset - should NOT increment count
    num_leaks = 0;
    status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Second leak detection failed");
    ASSERT_EQUAL(num_leaks, 3, "Should still detect 3 leaks");
    
    msm_statistics_t stats2;
    msm_get_statistics(ctx, &stats2);
    ASSERT_EQUAL(stats2.memory_leaks_detected, 3, 
                 "Leak count should stay at 3 (no double counting)");
    
    // NOW RESET - this should clear counted_as_leak flags
    status = msm_reset(ctx);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to reset MSM");
    
    // Verify statistics are reset
    msm_statistics_t stats3;
    msm_get_statistics(ctx, &stats3);
    ASSERT_EQUAL(stats3.memory_leaks_detected, 0, "Statistics should be reset to 0");
    
    // Third leak detection AFTER reset - should count leaks again
    // BUG FIX VERIFICATION: This is where the bug was - leaks wouldn't be counted
    // because counted_as_leak flags weren't cleared by msm_reset()
    num_leaks = 0;
    status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Third leak detection (after reset) failed");
    ASSERT_EQUAL(num_leaks, 3, "Should still detect 3 leaks after reset");
    
    // Get statistics after third detection
    msm_statistics_t stats4;
    msm_get_statistics(ctx, &stats4);
    ASSERT_EQUAL(stats4.memory_leaks_detected, 3, 
                 "After reset, leaks should be counted again");
    
    // Fourth detection - should NOT increment again
    num_leaks = 0;
    status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Fourth leak detection failed");
    
    msm_statistics_t stats5;
    msm_get_statistics(ctx, &stats5);
    ASSERT_EQUAL(stats5.memory_leaks_detected, 3,
                 "Leak count should stay at 3 after fourth detection");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_reset_preserves_allocation_tracking(void) {
    TEST_START("MSM Reset Preserves Allocation Tracking");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocations
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_allocation(ctx, addr2, 200, "test.c", 20, "test_func");
    
    // Detect leaks before reset
    allocation_metadata_t leaks_before[10];
    uint32_t num_leaks_before = 0;
    msm_detect_leaks(ctx, leaks_before, 10, &num_leaks_before);
    ASSERT_EQUAL(num_leaks_before, 2, "Should detect 2 leaks before reset");
    
    // Reset
    crrss_status_t status = msm_reset(ctx);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Failed to reset MSM");
    
    // Allocations should still be tracked (not freed)
    allocation_metadata_t meta1, meta2;
    status = msm_get_allocation_metadata(ctx, addr1, &meta1);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Should still track addr1 after reset");
    
    status = msm_get_allocation_metadata(ctx, addr2, &meta2);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Should still track addr2 after reset");
    
    // Allocations should not be marked as freed
    ASSERT_FALSE(meta1.is_freed, "addr1 should not be marked as freed");
    ASSERT_FALSE(meta2.is_freed, "addr2 should not be marked as freed");
    
    // Detect leaks after reset - should work correctly
    allocation_metadata_t leaks_after[10];
    uint32_t num_leaks_after = 0;
    status = msm_detect_leaks(ctx, leaks_after, 10, &num_leaks_after);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Leak detection after reset should work");
    ASSERT_EQUAL(num_leaks_after, 2, "Should still detect 2 leaks after reset");
    
    // Verify leak statistics are correctly updated
    msm_statistics_t stats;
    msm_get_statistics(ctx, &stats);
    ASSERT_EQUAL(stats.memory_leaks_detected, 2, 
                 "Leak statistics should be correctly rebuilt after reset");
    
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

// ==================== Double-Counting Bug Test ====================

static void test_leak_double_counting_prevention(void) {
    TEST_START("Leak Double-Counting Prevention");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track multiple allocations but don't free them (simulating leaks)
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    void* addr3 = (void*)0x3000;
    
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_allocation(ctx, addr2, 200, "test.c", 20, "test_func");
    msm_track_allocation(ctx, addr3, 300, "test.c", 30, "test_func");
    
    // First leak detection
    allocation_metadata_t leaks[10];
    uint32_t num_leaks = 0;
    crrss_status_t status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "First leak detection failed");
    ASSERT_EQUAL(num_leaks, 3, "Should detect 3 leaks");
    
    // Get statistics after first detection
    msm_statistics_t stats1;
    msm_get_statistics(ctx, &stats1);
    uint32_t first_leak_count = stats1.memory_leaks_detected;
    ASSERT_EQUAL(first_leak_count, 3, "Should have 3 leaks counted");
    
    // Second leak detection (e.g., via report generation)
    num_leaks = 0;
    status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Second leak detection failed");
    ASSERT_EQUAL(num_leaks, 3, "Should still detect 3 leaks");
    
    // Get statistics after second detection
    msm_statistics_t stats2;
    msm_get_statistics(ctx, &stats2);
    uint32_t second_leak_count = stats2.memory_leaks_detected;
    
    // BUG FIX VERIFICATION: Count should NOT increase on repeated detection
    ASSERT_EQUAL(second_leak_count, first_leak_count, 
                 "Leak count should not increase on repeated detection");
    
    // Third detection to be extra sure
    num_leaks = 0;
    status = msm_detect_leaks(ctx, leaks, 10, &num_leaks);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Third leak detection failed");
    
    msm_statistics_t stats3;
    msm_get_statistics(ctx, &stats3);
    ASSERT_EQUAL(stats3.memory_leaks_detected, first_leak_count,
                 "Leak count should remain constant across multiple detections");
    
    msm_shutdown(ctx);
    
    TEST_PASS();
}

static void test_leak_counting_with_report_generation(void) {
    TEST_START("Leak Counting with Multiple Report Generations");
    
    msm_config_t config = create_default_config();
    msm_context_t* ctx = msm_initialize(&config);
    ASSERT_NOT_NULL(ctx, "Failed to initialize MSM context");
    
    // Track allocations without freeing
    void* addr1 = (void*)0x1000;
    void* addr2 = (void*)0x2000;
    
    msm_track_allocation(ctx, addr1, 100, "test.c", 10, "test_func");
    msm_track_allocation(ctx, addr2, 200, "test.c", 20, "test_func");
    
    // Generate first report - this will detect leaks and update statistics
    msm_report_t report1;
    crrss_status_t status = msm_generate_report(ctx, &report1);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "First report generation failed");
    ASSERT_EQUAL(report1.leak_count, 2, "Should detect 2 leaks in first report");
    
    // Get statistics AFTER first report generation
    msm_statistics_t stats1;
    msm_get_statistics(ctx, &stats1);
    uint32_t first_leak_count = stats1.memory_leaks_detected;
    ASSERT_EQUAL(first_leak_count, 2, "Should have 2 leaks counted after first report");
    
    // Generate second report (this would trigger the bug before the fix)
    msm_report_t report2;
    status = msm_generate_report(ctx, &report2);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Second report generation failed");
    ASSERT_EQUAL(report2.leak_count, 2, "Should still detect 2 leaks in second report");
    
    // Get statistics after second report
    msm_statistics_t stats2;
    msm_get_statistics(ctx, &stats2);
    uint32_t second_leak_count = stats2.memory_leaks_detected;
    
    // BUG FIX VERIFICATION: Count should NOT double
    ASSERT_EQUAL(second_leak_count, first_leak_count,
                 "Leak count should not inflate on repeated report generation");
    
    // Generate third report to be extra sure
    msm_report_t report3;
    status = msm_generate_report(ctx, &report3);
    ASSERT_STATUS(status, CRRSS_SUCCESS, "Third report generation failed");
    
    msm_statistics_t stats3;
    msm_get_statistics(ctx, &stats3);
    ASSERT_EQUAL(stats3.memory_leaks_detected, first_leak_count,
                 "Leak count should remain stable across all reports");
    
    // Cleanup
    if (report1.issues) free(report1.issues);
    if (report1.leak_records) free(report1.leak_records);
    if (report2.issues) free(report2.issues);
    if (report2.leak_records) free(report2.leak_records);
    if (report3.issues) free(report3.issues);
    if (report3.leak_records) free(report3.leak_records);
    
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
    
    // Double-counting bug tests
    test_leak_double_counting_prevention();
    test_leak_counting_with_report_generation();
    
    // MSM Reset leak counting bug tests
    test_reset_clears_leak_counting_flags();
    test_reset_preserves_allocation_tracking();
    
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
