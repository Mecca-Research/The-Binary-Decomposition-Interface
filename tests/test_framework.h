
// BDI Test Framework - Simple C Testing Framework
// Provides basic unit testing capabilities for C23 modernization

#ifndef BDI_TEST_FRAMEWORK_H
#define BDI_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Test statistics
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} TestStats;

// Global test statistics
extern TestStats g_test_stats;

// Color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

// Test macros
#define TEST_INIT() \
    do { \
        g_test_stats.total_tests = 0; \
        g_test_stats.passed_tests = 0; \
        g_test_stats.failed_tests = 0; \
        g_test_stats.skipped_tests = 0; \
    } while(0)

#define TEST_BEGIN(name) \
    do { \
        printf(COLOR_BLUE "[ RUN      ] %s" COLOR_RESET "\n", name); \
        g_test_stats.total_tests++; \
    } while(0)

#define TEST_END(name) \
    do { \
        printf(COLOR_GREEN "[       OK ] %s" COLOR_RESET "\n", name); \
        g_test_stats.passed_tests++; \
    } while(0)

#define TEST_FAIL(name, msg) \
    do { \
        printf(COLOR_RED "[  FAILED  ] %s: %s" COLOR_RESET "\n", name, msg); \
        g_test_stats.failed_tests++; \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "  Assertion failed: %s" COLOR_RESET "\n", msg); \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, msg) \
    ASSERT_TRUE(!(condition), msg)

#define ASSERT_EQ(a, b, msg) \
    ASSERT_TRUE((a) == (b), msg)

#define ASSERT_NEQ(a, b, msg) \
    ASSERT_TRUE((a) != (b), msg)

#define ASSERT_NULL(ptr, msg) \
    ASSERT_TRUE((ptr) == nullptr, msg)

#define ASSERT_NOT_NULL(ptr, msg) \
    ASSERT_TRUE((ptr) != nullptr, msg)

#define ASSERT_STR_EQ(a, b, msg) \
    ASSERT_TRUE(strcmp(a, b) == 0, msg)

#define TEST_SUMMARY() \
    do { \
        printf("\n" COLOR_BLUE "==================== TEST SUMMARY ====================" COLOR_RESET "\n"); \
        printf("Total tests:  %d\n", g_test_stats.total_tests); \
        printf(COLOR_GREEN "Passed:       %d" COLOR_RESET "\n", g_test_stats.passed_tests); \
        if (g_test_stats.failed_tests > 0) { \
            printf(COLOR_RED "Failed:       %d" COLOR_RESET "\n", g_test_stats.failed_tests); \
        } else { \
            printf("Failed:       %d\n", g_test_stats.failed_tests); \
        } \
        printf("Skipped:      %d\n", g_test_stats.skipped_tests); \
        printf(COLOR_BLUE "======================================================" COLOR_RESET "\n"); \
        if (g_test_stats.failed_tests > 0) { \
            return EXIT_FAILURE; \
        } \
        return EXIT_SUCCESS; \
    } while(0)

// Test runner function type
typedef bool (*TestFunction)(void);

// Run a single test
static inline bool run_test(const char* name, TestFunction test_func) {
    TEST_BEGIN(name);
    if (test_func()) {
        TEST_END(name);
        return true;
    } else {
        TEST_FAIL(name, "Test function returned false");
        return false;
    }
}

#endif // BDI_TEST_FRAMEWORK_H

