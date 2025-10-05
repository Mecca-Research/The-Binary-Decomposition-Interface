
/**
 * Stress Test Runner Header for BDI Kernel
 */

#ifndef STRESS_RUNNER_H
#define STRESS_RUNNER_H

#include <stdbool.h>
#include <stdint.h>

// Stress test configuration
typedef struct {
    uint32_t duration_seconds;
    uint32_t iterations;
    const char* intensity;  // "low", "medium", "high"
    bool verbose;
} StressTestConfig;

// Stress test result
typedef struct {
    bool passed;
    uint64_t operations_completed;
    double elapsed_time_seconds;
    const char* error_message;
} StressTestResult;

// Initialize stress test configuration with defaults
void stress_config_init(StressTestConfig* config);

// Run stress test with configuration
StressTestResult stress_run_test(const char* test_name, StressTestConfig* config);

#endif // STRESS_RUNNER_H
