
/**
 * @file test_framework.h
 * @brief Test Framework API
 * @details This file provides the test framework functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

// Test framework macros and utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s:%d - %s\n", __FILE__, __LINE__, message); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s:%d - %s (expected: %ld, actual: %ld)\n", \
                   __FILE__, __LINE__, message, (long)(expected), (long)(actual)); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_NEQ(not_expected, actual, message) \
    do { \
        if ((not_expected) == (actual)) { \
            printf("FAIL: %s:%d - %s (should not equal: %ld)\n", \
                   __FILE__, __LINE__, message, (long)(not_expected)); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != NULL) { \
            printf("FAIL: %s:%d - %s (expected NULL, got %p)\n", \
                   __FILE__, __LINE__, message, (void*)(ptr)); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            printf("FAIL: %s:%d - %s (expected non-NULL)\n", \
                   __FILE__, __LINE__, message); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("FAIL: %s:%d - %s (expected: '%s', actual: '%s')\n", \
                   __FILE__, __LINE__, message, (expected), (actual)); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

#define TEST_ASSERT_MEM_EQ(expected, actual, size, message) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            printf("FAIL: %s:%d - %s (memory blocks differ)\n", \
                   __FILE__, __LINE__, message); \
            test_framework_fail_count++; \
            return false; \
        } \
        test_framework_pass_count++; \
    } while(0)

// Performance testing macros
#define TEST_BENCHMARK_START() \
    struct timeval benchmark_start, benchmark_end; \
    gettimeofday(&benchmark_start, NULL)

#define TEST_BENCHMARK_END(operation_name) \
    do { \
        gettimeofday(&benchmark_end, NULL); \
        double elapsed = (benchmark_end.tv_sec - benchmark_start.tv_sec) * 1000.0 + \
                        (benchmark_end.tv_usec - benchmark_start.tv_usec) / 1000.0; \
        printf("BENCHMARK: %s took %.3f ms\n", operation_name, elapsed); \
    } while(0)

// Memory leak detection helpers
#define TEST_MEMORY_CHECKPOINT() test_framework_memory_checkpoint()
#define TEST_MEMORY_VERIFY(message) test_framework_memory_verify(message)

// Test function type
typedef bool (*test_function_t)(void);

// Test suite structure
typedef struct {
    const char* name;
    test_function_t* tests;
    size_t test_count;
} test_suite_t;

// Global counters
extern int test_framework_pass_count;
extern int test_framework_fail_count;
extern size_t test_framework_memory_allocated;

// Framework functions
void test_framework_init(void);
void test_framework_cleanup(void);
bool test_framework_run_suite(const test_suite_t* suite);
void test_framework_print_summary(void);
void test_framework_memory_checkpoint(void);
bool test_framework_memory_verify(const char* message);

// Utility functions for testing
void* test_malloc(size_t size);
void test_free(void* ptr);
char* test_create_temp_file(const char* content);
void test_remove_temp_file(const char* filename);

#endif // TEST_FRAMEWORK_H
