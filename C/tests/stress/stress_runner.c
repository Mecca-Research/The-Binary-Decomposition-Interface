
/**
 * Stress Test Runner for BDI Kernel
 * 
 * Runs all stress test suites with configurable parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <strings.h>

// Test suite declarations
extern int test_stress_memory_main(void);
extern int test_stress_stack_main(void);
extern int test_stress_cpu_main(void);
extern int test_stress_concurrency_main(void);

typedef struct {
    const char* name;
    int (*run)(void);
} StressTestSuite;

static StressTestSuite stress_suites[] = {
    {"Memory", test_stress_memory_main},
    {"Stack", test_stress_stack_main},
    {"CPU", test_stress_cpu_main},
    {"Concurrency", test_stress_concurrency_main},
};

static const int num_suites = sizeof(stress_suites) / sizeof(stress_suites[0]);

static void print_usage(const char* program_name) {
    printf("Usage: %s [suite_name]\n", program_name);
    printf("\nAvailable test suites:\n");
    printf("  all          - Run all stress tests (default)\n");
    printf("  memory       - Memory stress tests\n");
    printf("  stack        - Stack stress tests\n");
    printf("  cpu          - CPU stress tests\n");
    printf("  concurrency  - Concurrency stress tests\n");
}

int main(int argc, char* argv[]) {
    const char* suite_name = "all";
    
    if (argc > 1) {
        suite_name = argv[1];
    }
    
    if (strcmp(suite_name, "help") == 0 || strcmp(suite_name, "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         BDI Kernel Stress Test Suite Runner               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    time_t start_time = time(NULL);
    int total_passed = 0;
    int total_failed = 0;
    
    if (strcmp(suite_name, "all") == 0) {
        // Run all suites
        for (int i = 0; i < num_suites; i++) {
            printf("\n--- Running %s Stress Tests ---\n", stress_suites[i].name);
            int result = stress_suites[i].run();
            if (result == 0) {
                total_passed++;
                printf("✓ %s stress tests PASSED\n", stress_suites[i].name);
            } else {
                total_failed++;
                printf("✗ %s stress tests FAILED\n", stress_suites[i].name);
            }
        }
    } else {
        // Run specific suite
        bool found = false;
        for (int i = 0; i < num_suites; i++) {
            if (strcasecmp(suite_name, stress_suites[i].name) == 0) {
                found = true;
                printf("\n--- Running %s Stress Tests ---\n", stress_suites[i].name);
                int result = stress_suites[i].run();
                if (result == 0) {
                    total_passed++;
                    printf("✓ %s stress tests PASSED\n", stress_suites[i].name);
                } else {
                    total_failed++;
                    printf("✗ %s stress tests FAILED\n", stress_suites[i].name);
                }
                break;
            }
        }
        
        if (!found) {
            printf("Error: Unknown test suite '%s'\n", suite_name);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                            ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Suites Passed: %-3d                                       ║\n", total_passed);
    printf("║  Suites Failed: %-3d                                       ║\n", total_failed);
    printf("║  Total Time:    %.0f seconds                              ║\n", elapsed);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return (total_failed == 0) ? 0 : 1;
}
