

/*
 * BDI Kernel Test Runner
 * Unified test discovery, execution, and reporting infrastructure
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>

// Test categories
typedef enum {
    TEST_CATEGORY_UNIT,
    TEST_CATEGORY_INTEGRATION,
    TEST_CATEGORY_STRESS,
    TEST_CATEGORY_REGRESSION,
    TEST_CATEGORY_PERFORMANCE,
    TEST_CATEGORY_ALL
} test_category_t;

// Test result
typedef enum {
    TEST_RESULT_PASS,
    TEST_RESULT_FAIL,
    TEST_RESULT_SKIP,
    TEST_RESULT_ERROR
} test_result_t;

// Test function signature
typedef test_result_t (*test_func_t)(void);

// Test case structure
typedef struct {
    const char *name;
    const char *description;
    test_category_t category;
    test_func_t func;
    bool enabled;
} test_case_t;

// Test statistics
typedef struct {
    int total;
    int passed;
    int failed;
    int skipped;
    int errors;
    double duration_ms;
} test_stats_t;

// Configuration
typedef struct {
    test_category_t category;
    const char *filter;
    bool parallel;
    int duration_seconds;
    bool verbose;
    const char *output_format;
} test_config_t;

// Global test registry
static test_case_t *test_registry = NULL;
static size_t test_count = 0;
static size_t test_capacity = 0;

// Forward declarations
static void register_test(const char *name, const char *desc, test_category_t cat, test_func_t func);
static void discover_tests(void);
static void run_tests(test_config_t *config);
static void print_results(test_stats_t *stats);

// External test suite declarations
extern int run_memory_stress_test(int duration_sec);
extern int run_scheduler_stress_test(int duration_sec);
extern int run_process_stress_test(int duration_sec);
extern int run_regression_tests(void);

// Test registration macro
#define REGISTER_TEST(name, desc, cat, func) \
    __attribute__((constructor)) static void register_##func(void) { \
        register_test(name, desc, cat, func); \
    }

// Utility functions
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

static const char* category_to_string(test_category_t cat) {
    switch (cat) {
        case TEST_CATEGORY_UNIT: return "unit";
        case TEST_CATEGORY_INTEGRATION: return "integration";
        case TEST_CATEGORY_STRESS: return "stress";
        case TEST_CATEGORY_REGRESSION: return "regression";
        case TEST_CATEGORY_PERFORMANCE: return "performance";
        case TEST_CATEGORY_ALL: return "all";
        default: return "unknown";
    }
}

static const char* result_to_string(test_result_t result) {
    switch (result) {
        case TEST_RESULT_PASS: return "PASS";
        case TEST_RESULT_FAIL: return "FAIL";
        case TEST_RESULT_SKIP: return "SKIP";
        case TEST_RESULT_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Test registration
static void register_test(const char *name, const char *desc, test_category_t cat, test_func_t func) {
    if (test_count >= test_capacity) {
        test_capacity = test_capacity == 0 ? 64 : test_capacity * 2;
        test_registry = realloc(test_registry, test_capacity * sizeof(test_case_t));
        if (!test_registry) {
            fprintf(stderr, "Failed to allocate test registry\n");
            exit(1);
        }
    }
    
    test_registry[test_count].name = name;
    test_registry[test_count].description = desc;
    test_registry[test_count].category = cat;
    test_registry[test_count].func = func;
    test_registry[test_count].enabled = true;
    test_count++;
}

// Sample test functions (to be replaced with actual tests)
static test_result_t test_memory_allocation(void) {
    // TODO: Implement actual memory allocation test
    return TEST_RESULT_PASS;
}

static test_result_t test_scheduler_basic(void) {
    // TODO: Implement actual scheduler test
    return TEST_RESULT_PASS;
}

static test_result_t test_process_creation(void) {
    // TODO: Implement actual process creation test
    return TEST_RESULT_PASS;
}

// Wrapper functions for stress tests
static test_result_t test_memory_stress_wrapper(void) {
    // Get duration from global config (default 60 seconds)
    extern test_config_t g_test_config;
    int result = run_memory_stress_test(g_test_config.duration_seconds);
    return (result == 0) ? TEST_RESULT_PASS : TEST_RESULT_FAIL;
}

static test_result_t test_scheduler_stress_wrapper(void) {
    extern test_config_t g_test_config;
    int result = run_scheduler_stress_test(g_test_config.duration_seconds);
    return (result == 0) ? TEST_RESULT_PASS : TEST_RESULT_FAIL;
}

static test_result_t test_process_stress_wrapper(void) {
    extern test_config_t g_test_config;
    int result = run_process_stress_test(g_test_config.duration_seconds);
    return (result == 0) ? TEST_RESULT_PASS : TEST_RESULT_FAIL;
}

// Wrapper function for regression tests
static test_result_t test_regression_suite_wrapper(void) {
    int result = run_regression_tests();
    return (result == 0) ? TEST_RESULT_PASS : TEST_RESULT_FAIL;
}

// Register sample tests
REGISTER_TEST("memory_allocation", "Basic memory allocation test", TEST_CATEGORY_UNIT, test_memory_allocation)
REGISTER_TEST("scheduler_basic", "Basic scheduler functionality", TEST_CATEGORY_UNIT, test_scheduler_basic)
REGISTER_TEST("process_creation", "Process creation test", TEST_CATEGORY_INTEGRATION, test_process_creation)

// Register stress tests
REGISTER_TEST("memory_stress", "Memory allocation/deallocation stress test", TEST_CATEGORY_STRESS, test_memory_stress_wrapper)
REGISTER_TEST("scheduler_stress", "Scheduler work stealing and context switch stress", TEST_CATEGORY_STRESS, test_scheduler_stress_wrapper)
REGISTER_TEST("process_stress", "Process lifecycle and IPC stress test", TEST_CATEGORY_STRESS, test_process_stress_wrapper)

// Register regression tests
REGISTER_TEST("regression_suite", "Regression test suite for previously fixed bugs", TEST_CATEGORY_REGRESSION, test_regression_suite_wrapper)

// Test discovery
static void discover_tests(void) {
    printf("Discovered %zu tests:\n", test_count);
    for (size_t i = 0; i < test_count; i++) {
        printf("  [%s] %s - %s\n", 
               category_to_string(test_registry[i].category),
               test_registry[i].name,
               test_registry[i].description);
    }
}

// Global config for wrapper functions
test_config_t g_test_config;

// Test execution
static void run_tests(test_config_t *config) {
    // Store config globally for wrapper functions
    g_test_config = *config;
    
    test_stats_t stats = {0};
    double start_time = get_time_ms();
    
    printf("\n=== Running Tests ===\n");
    printf("Category: %s\n", category_to_string(config->category));
    if (config->filter) {
        printf("Filter: %s\n", config->filter);
    }
    printf("\n");
    
    for (size_t i = 0; i < test_count; i++) {
        test_case_t *test = &test_registry[i];
        
        // Apply category filter
        if (config->category != TEST_CATEGORY_ALL && 
            test->category != config->category) {
            continue;
        }
        
        // Apply name filter
        if (config->filter && !strstr(test->name, config->filter)) {
            continue;
        }
        
        if (!test->enabled) {
            stats.skipped++;
            continue;
        }
        
        stats.total++;
        
        if (config->verbose) {
            printf("Running: %s ... ", test->name);
            fflush(stdout);
        }
        
        double test_start = get_time_ms();
        test_result_t result = test->func();
        double test_duration = get_time_ms() - test_start;
        
        switch (result) {
            case TEST_RESULT_PASS:
                stats.passed++;
                if (config->verbose) {
                    printf("PASS (%.2f ms)\n", test_duration);
                }
                break;
            case TEST_RESULT_FAIL:
                stats.failed++;
                printf("FAIL: %s\n", test->name);
                break;
            case TEST_RESULT_SKIP:
                stats.skipped++;
                if (config->verbose) {
                    printf("SKIP\n");
                }
                break;
            case TEST_RESULT_ERROR:
                stats.errors++;
                printf("ERROR: %s\n", test->name);
                break;
        }
    }
    
    stats.duration_ms = get_time_ms() - start_time;
    print_results(&stats);
}

// Results printing
static void print_results(test_stats_t *stats) {
    printf("\n=== Test Results ===\n");
    printf("Total:   %d\n", stats->total);
    printf("Passed:  %d\n", stats->passed);
    printf("Failed:  %d\n", stats->failed);
    printf("Skipped: %d\n", stats->skipped);
    printf("Errors:  %d\n", stats->errors);
    printf("Duration: %.2f ms\n", stats->duration_ms);
    printf("\n");
    
    if (stats->failed > 0 || stats->errors > 0) {
        printf("Result: FAILED\n");
    } else {
        printf("Result: PASSED\n");
    }
}

// Usage
static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\nOptions:\n");
    printf("  -c, --category=CAT    Test category (unit|integration|stress|regression|performance|all)\n");
    printf("  -f, --filter=PATTERN  Filter tests by name pattern\n");
    printf("  -p, --parallel        Run tests in parallel\n");
    printf("  -d, --duration=SEC    Duration for stress tests (seconds)\n");
    printf("  -v, --verbose         Verbose output\n");
    printf("  -o, --output=FORMAT   Output format (text|json|xml)\n");
    printf("  -l, --list            List all tests\n");
    printf("  -h, --help            Show this help\n");
    printf("\nExamples:\n");
    printf("  %s --category=unit\n", prog);
    printf("  %s --filter=memory --verbose\n", prog);
    printf("  %s --category=stress --duration=3600\n", prog);
}

// Main
int main(int argc, char *argv[]) {
    test_config_t config = {
        .category = TEST_CATEGORY_ALL,
        .filter = NULL,
        .parallel = false,
        .duration_seconds = 60,
        .verbose = false,
        .output_format = "text"
    };
    
    bool list_only = false;
    
    static struct option long_options[] = {
        {"category", required_argument, 0, 'c'},
        {"filter", required_argument, 0, 'f'},
        {"parallel", no_argument, 0, 'p'},
        {"duration", required_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"output", required_argument, 0, 'o'},
        {"list", no_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "c:f:pd:vo:lh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                if (strcmp(optarg, "unit") == 0) config.category = TEST_CATEGORY_UNIT;
                else if (strcmp(optarg, "integration") == 0) config.category = TEST_CATEGORY_INTEGRATION;
                else if (strcmp(optarg, "stress") == 0) config.category = TEST_CATEGORY_STRESS;
                else if (strcmp(optarg, "regression") == 0) config.category = TEST_CATEGORY_REGRESSION;
                else if (strcmp(optarg, "performance") == 0) config.category = TEST_CATEGORY_PERFORMANCE;
                else if (strcmp(optarg, "all") == 0) config.category = TEST_CATEGORY_ALL;
                else {
                    fprintf(stderr, "Unknown category: %s\n", optarg);
                    return 1;
                }
                break;
            case 'f':
                config.filter = optarg;
                break;
            case 'p':
                config.parallel = true;
                break;
            case 'd':
                config.duration_seconds = atoi(optarg);
                break;
            case 'v':
                config.verbose = true;
                break;
            case 'o':
                config.output_format = optarg;
                break;
            case 'l':
                list_only = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    printf("BDI Kernel Test Runner\n");
    printf("======================\n\n");
    
    if (list_only) {
        discover_tests();
        return 0;
    }
    
    run_tests(&config);
    
    return 0;
}
