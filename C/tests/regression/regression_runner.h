
/**
 * Regression Test Runner Header for BDI Kernel
 */

#ifndef REGRESSION_RUNNER_H
#define REGRESSION_RUNNER_H

#include <stdbool.h>
#include <stdint.h>

// Regression test configuration
typedef struct {
    const char* baseline_path;
    bool compare_with_baseline;
    bool verbose;
} RegressionTestConfig;

// Regression test result
typedef struct {
    bool passed;
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    const char* error_message;
} RegressionTestResult;

// Initialize regression test configuration with defaults
void regression_config_init(RegressionTestConfig* config);

// Run regression test with configuration
RegressionTestResult regression_run_test(const char* test_name, RegressionTestConfig* config);

#endif // REGRESSION_RUNNER_H
