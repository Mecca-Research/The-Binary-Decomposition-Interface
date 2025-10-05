
/**
 * Test Utilities for BDI Kernel Tests
 * 
 * Provides utility functions for stress and regression testing.
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

// Memory tracking
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_allocated;
    size_t peak_allocated;
    uint64_t allocation_count;
    uint64_t free_count;
} MemoryStats;

// Performance measurement
typedef struct {
    struct timeval start_time;
    struct timeval end_time;
    double elapsed_ms;
} PerformanceTimer;

// Memory tracking functions
void test_utils_init_memory_tracking(void);
void test_utils_reset_memory_tracking(void);
MemoryStats test_utils_get_memory_stats(void);
void test_utils_print_memory_stats(void);
bool test_utils_check_memory_leaks(void);

// Performance measurement functions
void test_utils_timer_start(PerformanceTimer* timer);
void test_utils_timer_stop(PerformanceTimer* timer);
double test_utils_timer_elapsed_ms(const PerformanceTimer* timer);
void test_utils_timer_print(const PerformanceTimer* timer, const char* label);

// Stress test helpers
void test_utils_sleep_ms(uint32_t milliseconds);
uint64_t test_utils_get_timestamp_ns(void);
double test_utils_calculate_ops_per_second(uint64_t operations, double elapsed_ms);

// Random number generation for testing
void test_utils_seed_random(uint32_t seed);
uint32_t test_utils_random_uint32(void);
double test_utils_random_double(void);
uint32_t test_utils_random_range(uint32_t min, uint32_t max);

// String utilities
char* test_utils_format_bytes(size_t bytes);
char* test_utils_format_number(uint64_t number);

// Assertion helpers for stress tests
#define STRESS_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "STRESS ASSERTION FAILED: %s:%d - %s\n", \
                    __FILE__, __LINE__, message); \
            return false; \
        } \
    } while(0)

#define STRESS_ASSERT_RANGE(value, min, max, message) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            fprintf(stderr, "STRESS ASSERTION FAILED: %s:%d - %s (value: %ld, range: [%ld, %ld])\n", \
                    __FILE__, __LINE__, message, (long)(value), (long)(min), (long)(max)); \
            return false; \
        } \
    } while(0)

#endif // TEST_UTILS_H
