
/**
 * Regression Test Runner for BDI Kernel
 * 
 * Runs all regression test suites.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <strings.h>

// Test suite declarations
extern int test_regression_vm_main(void);
extern int test_regression_jit_main(void);
extern int test_regression_graph_main(void);
extern int test_regression_integration_main(void);

typedef struct {
    const char* name;
    int (*run)(void);
} RegressionTestSuite;

static RegressionTestSuite regression_suites[] = {
    {"VM", test_regression_vm_main},
    {"JIT", test_regression_jit_main},
    {"Graph", test_regression_graph_main},
    {"Integration", test_regression_integration_main},
};

static const int num_suites = sizeof(regression_suites) / sizeof(regression_suites[0]);

static void print_usage(const char* program_name) {
    printf("Usage: %s [suite_name]\n", program_name);
    printf("\nAvailable test suites:\n");
    printf("  all          - Run all regression tests (default)\n");
    printf("  vm           - VM regression tests\n");
    printf("  jit          - JIT regression tests\n");
    printf("  graph        - Graph regression tests\n");
    printf("  integration  - Integration regression tests\n");
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
    printf("║       BDI Kernel Regression Test Suite Runner             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    time_t start_time = time(NULL);
    int total_passed = 0;
    int total_failed = 0;
    
    if (strcmp(suite_name, "all") == 0) {
        // Run all suites
        for (int i = 0; i < num_suites; i++) {
            printf("\n--- Running %s Regression Tests ---\n", regression_suites[i].name);
            int result = regression_suites[i].run();
            if (result == 0) {
                total_passed++;
                printf("✓ %s regression tests PASSED\n", regression_suites[i].name);
            } else {
                total_failed++;
                printf("✗ %s regression tests FAILED\n", regression_suites[i].name);
            }
        }
    } else {
        // Run specific suite
        bool found = false;
        for (int i = 0; i < num_suites; i++) {
            if (strcasecmp(suite_name, regression_suites[i].name) == 0) {
                found = true;
                printf("\n--- Running %s Regression Tests ---\n", regression_suites[i].name);
                int result = regression_suites[i].run();
                if (result == 0) {
                    total_passed++;
                    printf("✓ %s regression tests PASSED\n", regression_suites[i].name);
                } else {
                    total_failed++;
                    printf("✗ %s regression tests FAILED\n", regression_suites[i].name);
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
