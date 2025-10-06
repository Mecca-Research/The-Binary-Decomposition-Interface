
/**
 * @file redteam_harness.h
 * @brief Red-Team Test Harness for BDI Kernel Memory Tests
 * @details Provides comprehensive test infrastructure including test registration,
 *          execution, reporting, timing, and memory leak detection.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#ifndef REDTEAM_HARNESS_H
#define REDTEAM_HARNESS_H

#include "../../../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>

// ============================================================================
// Test Result Types
// ============================================================================

typedef enum {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
    TEST_CRASH,
    TEST_TIMEOUT,
    TEST_LEAK
} test_result_t;

// ============================================================================
// Test Function Types
// ============================================================================

typedef test_result_t (*test_func_t)(void);
typedef void (*setup_func_t)(void);
typedef void (*teardown_func_t)(void);

// ============================================================================
// Test Case Structure
// ============================================================================

typedef struct {
    const char *name;
    const char *category;
    const char *description;
    test_func_t func;
    setup_func_t setup;
    teardown_func_t teardown;
    uint32_t timeout_ms;
    bool enabled;
} test_case_t;

// ============================================================================
// Test Statistics
// ============================================================================

typedef struct {
    uint32_t total_tests;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
    uint32_t crashed;
    uint32_t timeout;
    uint32_t leaked;
    double total_time_ms;
    uint64_t total_memory_allocated;
    uint64_t total_memory_freed;
    uint64_t peak_memory_usage;
} test_stats_t;

// ============================================================================
// Test Context
// ============================================================================

typedef struct {
    test_case_t *current_test;
    test_stats_t stats;
    jmp_buf crash_handler;
    bool in_test;
    uint64_t test_start_time;
    uint64_t memory_at_start;
    FILE *log_file;
    bool verbose;
} test_context_t;

// ============================================================================
// Test Registration Macros
// ============================================================================

#define REDTEAM_TEST(name, category, description) \
    static test_result_t test_##name(void); \
    static test_case_t test_case_##name = { \
        .name = #name, \
        .category = category, \
        .description = description, \
        .func = test_##name, \
        .setup = NULL, \
        .teardown = NULL, \
        .timeout_ms = 30000, \
        .enabled = true \
    }; \
    static test_result_t test_##name(void)

#define REDTEAM_TEST_WITH_SETUP(name, category, description, setup_fn, teardown_fn) \
    static test_result_t test_##name(void); \
    static test_case_t test_case_##name = { \
        .name = #name, \
        .category = category, \
        .description = description, \
        .func = test_##name, \
        .setup = setup_fn, \
        .teardown = teardown_fn, \
        .timeout_ms = 30000, \
        .enabled = true \
    }; \
    static test_result_t test_##name(void)

// ============================================================================
// Assertion Macros
// ============================================================================

#define REDTEAM_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            redteam_log_failure(__FILE__, __LINE__, __func__, \
                              "Assertion failed: " message); \
            return TEST_FAIL; \
        } \
    } while(0)

#define REDTEAM_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            redteam_log_failure(__FILE__, __LINE__, __func__, \
                              "Expected %ld, got %ld: " message, \
                              (long)(expected), (long)(actual)); \
            return TEST_FAIL; \
        } \
    } while(0)

#define REDTEAM_ASSERT_NEQ(not_expected, actual, message) \
    do { \
        if ((not_expected) == (actual)) { \
            redteam_log_failure(__FILE__, __LINE__, __func__, \
                              "Should not equal %ld: " message, \
                              (long)(not_expected)); \
            return TEST_FAIL; \
        } \
    } while(0)

#define REDTEAM_ASSERT_NULL(ptr, message) \
    REDTEAM_ASSERT((ptr) == NULL, message)

#define REDTEAM_ASSERT_NOT_NULL(ptr, message) \
    REDTEAM_ASSERT((ptr) != NULL, message)

#define REDTEAM_ASSERT_PTR_ALIGNED(ptr, alignment, message) \
    REDTEAM_ASSERT(((uintptr_t)(ptr) & ((alignment) - 1)) == 0, message)

#define REDTEAM_ASSERT_IN_RANGE(value, min, max, message) \
    REDTEAM_ASSERT((value) >= (min) && (value) <= (max), message)

#define REDTEAM_EXPECT_CRASH(code) \
    do { \
        if (setjmp(redteam_get_context()->crash_handler) == 0) { \
            code; \
            redteam_log_failure(__FILE__, __LINE__, __func__, \
                              "Expected crash did not occur"); \
            return TEST_FAIL; \
        } \
    } while(0)

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize the test harness
 * @param verbose Enable verbose output
 * @param log_file Optional log file path (NULL for stdout only)
 * @return true on success
 */
bool redteam_init(bool verbose, const char *log_file);

/**
 * @brief Cleanup and finalize the test harness
 */
void redteam_cleanup(void);

/**
 * @brief Register a test case
 * @param test Test case to register
 * @return true on success
 */
bool redteam_register_test(test_case_t *test);

/**
 * @brief Run all registered tests
 * @return Test statistics
 */
test_stats_t redteam_run_all_tests(void);

/**
 * @brief Run tests in a specific category
 * @param category Category name
 * @return Test statistics
 */
test_stats_t redteam_run_category(const char *category);

/**
 * @brief Run a single test by name
 * @param name Test name
 * @return Test result
 */
test_result_t redteam_run_test(const char *name);

/**
 * @brief Get current test context
 * @return Test context pointer
 */
test_context_t *redteam_get_context(void);

/**
 * @brief Print test statistics
 * @param stats Statistics to print
 */
void redteam_print_stats(const test_stats_t *stats);

/**
 * @brief Log a test failure
 * @param file Source file
 * @param line Line number
 * @param func Function name
 * @param format Printf-style format string
 */
void redteam_log_failure(const char *file, int line, const char *func,
                        const char *format, ...);

/**
 * @brief Log a test message
 * @param format Printf-style format string
 */
void redteam_log(const char *format, ...);

/**
 * @brief Get current timestamp in microseconds
 * @return Timestamp
 */
uint64_t redteam_get_timestamp_us(void);

/**
 * @brief Get current memory usage
 * @return Memory usage in bytes
 */
uint64_t redteam_get_memory_usage(void);

/**
 * @brief Check for memory leaks
 * @return true if leaks detected
 */
bool redteam_check_leaks(void);

#endif // REDTEAM_HARNESS_H
