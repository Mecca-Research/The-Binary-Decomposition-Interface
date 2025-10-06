
/**
 * @file redteam_harness.c
 * @brief Red-Team Test Harness Implementation
 */

#include "redteam_harness.h"
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

// ============================================================================
// Global State
// ============================================================================

static test_context_t g_context = {0};
static test_case_t *g_tests[1024] = {0};
static uint32_t g_test_count = 0;

// ============================================================================
// Signal Handlers
// ============================================================================

static void crash_signal_handler(int sig) {
    if (g_context.in_test) {
        redteam_log("Test crashed with signal %d", sig);
        longjmp(g_context.crash_handler, sig);
    }
    exit(1);
}

// ============================================================================
// Core Implementation
// ============================================================================

bool redteam_init(bool verbose, const char *log_file) {
    memset(&g_context, 0, sizeof(g_context));
    g_context.verbose = verbose;
    
    if (log_file) {
        g_context.log_file = fopen(log_file, "w");
        if (!g_context.log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", log_file);
            return false;
        }
    }
    
    // Install signal handlers
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGABRT, crash_signal_handler);
    signal(SIGFPE, crash_signal_handler);
    signal(SIGILL, crash_signal_handler);
    
    redteam_log("[REDTEAM] Test harness initialized");
    return true;
}

void redteam_cleanup(void) {
    if (g_context.log_file) {
        fclose(g_context.log_file);
        g_context.log_file = NULL;
    }
    
    // Restore default signal handlers
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
}

bool redteam_register_test(test_case_t *test) {
    if (g_test_count >= 1024) {
        fprintf(stderr, "Too many tests registered\n");
        return false;
    }
    
    g_tests[g_test_count++] = test;
    return true;
}

static test_result_t run_single_test(test_case_t *test) {
    g_context.current_test = test;
    g_context.in_test = true;
    g_context.test_start_time = redteam_get_timestamp_us();
    g_context.memory_at_start = redteam_get_memory_usage();
    
    test_result_t result = TEST_PASS;
    
    // Setup
    if (test->setup) {
        test->setup();
    }
    
    // Run test with crash protection
    if (setjmp(g_context.crash_handler) == 0) {
        result = test->func();
    } else {
        result = TEST_CRASH;
    }
    
    // Teardown
    if (test->teardown) {
        test->teardown();
    }
    
    // Check for leaks
    uint64_t memory_at_end = redteam_get_memory_usage();
    if (memory_at_end > g_context.memory_at_start) {
        uint64_t leaked = memory_at_end - g_context.memory_at_start;
        if (leaked > 1024) { // Ignore small leaks
            redteam_log("Memory leak detected: %lu bytes", leaked);
            result = TEST_LEAK;
        }
    }
    
    uint64_t elapsed = redteam_get_timestamp_us() - g_context.test_start_time;
    double elapsed_ms = elapsed / 1000.0;
    
    const char *result_str = "UNKNOWN";
    switch (result) {
        case TEST_PASS: result_str = "PASS"; g_context.stats.passed++; break;
        case TEST_FAIL: result_str = "FAIL"; g_context.stats.failed++; break;
        case TEST_SKIP: result_str = "SKIP"; g_context.stats.skipped++; break;
        case TEST_CRASH: result_str = "CRASH"; g_context.stats.crashed++; break;
        case TEST_TIMEOUT: result_str = "TIMEOUT"; g_context.stats.timeout++; break;
        case TEST_LEAK: result_str = "LEAK"; g_context.stats.leaked++; break;
    }
    
    redteam_log("[%s] %s::%s (%.3fms)", result_str, test->category, test->name, elapsed_ms);
    
    g_context.stats.total_time_ms += elapsed_ms;
    g_context.in_test = false;
    
    return result;
}

test_stats_t redteam_run_all_tests(void) {
    memset(&g_context.stats, 0, sizeof(g_context.stats));
    g_context.stats.total_tests = g_test_count;
    
    redteam_log("[REDTEAM] Running %u tests...", g_test_count);
    
    for (uint32_t i = 0; i < g_test_count; i++) {
        if (g_tests[i]->enabled) {
            run_single_test(g_tests[i]);
        } else {
            g_context.stats.skipped++;
        }
    }
    
    return g_context.stats;
}

test_stats_t redteam_run_category(const char *category) {
    memset(&g_context.stats, 0, sizeof(g_context.stats));
    
    redteam_log("[REDTEAM] Running tests in category: %s", category);
    
    for (uint32_t i = 0; i < g_test_count; i++) {
        if (g_tests[i]->enabled && strcmp(g_tests[i]->category, category) == 0) {
            g_context.stats.total_tests++;
            run_single_test(g_tests[i]);
        }
    }
    
    return g_context.stats;
}

test_result_t redteam_run_test(const char *name) {
    for (uint32_t i = 0; i < g_test_count; i++) {
        if (strcmp(g_tests[i]->name, name) == 0) {
            return run_single_test(g_tests[i]);
        }
    }
    
    redteam_log("Test not found: %s", name);
    return TEST_FAIL;
}

test_context_t *redteam_get_context(void) {
    return &g_context;
}

void redteam_print_stats(const test_stats_t *stats) {
    printf("\n");
    printf("========================================\n");
    printf("Red-Team Test Results\n");
    printf("========================================\n");
    printf("Total:    %u\n", stats->total_tests);
    printf("Passed:   %u (%.1f%%)\n", stats->passed, 
           100.0 * stats->passed / stats->total_tests);
    printf("Failed:   %u\n", stats->failed);
    printf("Skipped:  %u\n", stats->skipped);
    printf("Crashed:  %u\n", stats->crashed);
    printf("Timeout:  %u\n", stats->timeout);
    printf("Leaked:   %u\n", stats->leaked);
    printf("Time:     %.3f seconds\n", stats->total_time_ms / 1000.0);
    printf("========================================\n");
    
    if (stats->failed + stats->crashed + stats->leaked > 0) {
        printf("RESULT: FAILED\n");
    } else {
        printf("RESULT: PASSED\n");
    }
}

void redteam_log_failure(const char *file, int line, const char *func,
                        const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    fprintf(stderr, "[FAIL] %s:%d in %s: %s\n", file, line, func, buffer);
    if (g_context.log_file) {
        fprintf(g_context.log_file, "[FAIL] %s:%d in %s: %s\n", 
                file, line, func, buffer);
    }
}

void redteam_log(const char *format, ...) {
    if (!g_context.verbose && g_context.current_test) {
        return; // Suppress logs during tests unless verbose
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    printf("%s\n", buffer);
    if (g_context.log_file) {
        fprintf(g_context.log_file, "%s\n", buffer);
        fflush(g_context.log_file);
    }
}

uint64_t redteam_get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

uint64_t redteam_get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (uint64_t)usage.ru_maxrss * 1024; // Convert KB to bytes
}

bool redteam_check_leaks(void) {
    uint64_t current = redteam_get_memory_usage();
    return current > g_context.memory_at_start;
}
