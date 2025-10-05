
/**
 * Test Utilities Implementation for BDI Kernel Tests
 */

#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global memory tracking state
static MemoryStats g_memory_stats = {0};
static bool g_memory_tracking_enabled = false;

// Memory tracking functions
void test_utils_init_memory_tracking(void) {
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_memory_tracking_enabled = true;
}

void test_utils_reset_memory_tracking(void) {
    g_memory_stats.total_allocated = 0;
    g_memory_stats.total_freed = 0;
    g_memory_stats.current_allocated = 0;
    g_memory_stats.peak_allocated = 0;
    g_memory_stats.allocation_count = 0;
    g_memory_stats.free_count = 0;
}

MemoryStats test_utils_get_memory_stats(void) {
    return g_memory_stats;
}

void test_utils_print_memory_stats(void) {
    printf("\n=== Memory Statistics ===\n");
    printf("Total Allocated:   %zu bytes\n", g_memory_stats.total_allocated);
    printf("Total Freed:       %zu bytes\n", g_memory_stats.total_freed);
    printf("Current Allocated: %zu bytes\n", g_memory_stats.current_allocated);
    printf("Peak Allocated:    %zu bytes\n", g_memory_stats.peak_allocated);
    printf("Allocation Count:  %lu\n", g_memory_stats.allocation_count);
    printf("Free Count:        %lu\n", g_memory_stats.free_count);
    printf("========================\n\n");
}

bool test_utils_check_memory_leaks(void) {
    if (!g_memory_tracking_enabled) {
        return true;
    }
    
    if (g_memory_stats.current_allocated > 0) {
        printf("WARNING: Memory leak detected! %zu bytes still allocated\n", 
               g_memory_stats.current_allocated);
        return false;
    }
    
    return true;
}

// Performance measurement functions
void test_utils_timer_start(PerformanceTimer* timer) {
    gettimeofday(&timer->start_time, NULL);
}

void test_utils_timer_stop(PerformanceTimer* timer) {
    gettimeofday(&timer->end_time, NULL);
    timer->elapsed_ms = (timer->end_time.tv_sec - timer->start_time.tv_sec) * 1000.0 +
                        (timer->end_time.tv_usec - timer->start_time.tv_usec) / 1000.0;
}

double test_utils_timer_elapsed_ms(const PerformanceTimer* timer) {
    return timer->elapsed_ms;
}

void test_utils_timer_print(const PerformanceTimer* timer, const char* label) {
    printf("TIMER [%s]: %.3f ms\n", label, timer->elapsed_ms);
}

// Stress test helpers
void test_utils_sleep_ms(uint32_t milliseconds) {
    usleep(milliseconds * 1000);
}

uint64_t test_utils_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

double test_utils_calculate_ops_per_second(uint64_t operations, double elapsed_ms) {
    if (elapsed_ms <= 0.0) {
        return 0.0;
    }
    return (double)operations / (elapsed_ms / 1000.0);
}

// Random number generation
static uint32_t g_random_state = 0;

void test_utils_seed_random(uint32_t seed) {
    g_random_state = seed;
}

uint32_t test_utils_random_uint32(void) {
    // Simple LCG random number generator
    g_random_state = g_random_state * 1664525 + 1013904223;
    return g_random_state;
}

double test_utils_random_double(void) {
    return (double)test_utils_random_uint32() / (double)UINT32_MAX;
}

uint32_t test_utils_random_range(uint32_t min, uint32_t max) {
    if (min >= max) {
        return min;
    }
    return min + (test_utils_random_uint32() % (max - min + 1));
}

// String utilities
char* test_utils_format_bytes(size_t bytes) {
    static char buffer[64];
    
    if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
    
    return buffer;
}

char* test_utils_format_number(uint64_t number) {
    static char buffer[64];
    
    if (number < 1000) {
        snprintf(buffer, sizeof(buffer), "%lu", number);
    } else if (number < 1000000) {
        snprintf(buffer, sizeof(buffer), "%.2fK", number / 1000.0);
    } else if (number < 1000000000) {
        snprintf(buffer, sizeof(buffer), "%.2fM", number / 1000000.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2fB", number / 1000000000.0);
    }
    
    return buffer;
}
