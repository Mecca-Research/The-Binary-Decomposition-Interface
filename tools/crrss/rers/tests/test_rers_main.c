
/**
 * @file test_rers_main.c
 * @brief Comprehensive test suite for entire RERS system
 */

#include "../rers.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test: Initialize and shutdown RERS system */
static void test_system_init_shutdown(void) {
    printf("  [TEST] System initialize and shutdown... ");
    
    rers_config_t config = {
        .enable_replay = true,
        .enable_learning = true,
        .enable_patterns = true,
        .enable_integration = true,
        .max_patterns = 1024,
        .max_replay_depth = 10,
        .learning_threshold = 5
    };
    
    rers_system_t *system = NULL;
    rers_error_t err = rers_init(&config, &system);
    assert(err == RERS_SUCCESS);
    assert(system != NULL);
    
    rers_shutdown(system);
    
    printf("PASS\n");
}

/* Test: Initialize with NULL config (use defaults) */
static void test_system_init_defaults(void) {
    printf("  [TEST] System initialize with defaults... ");
    
    rers_system_t *system = NULL;
    rers_error_t err = rers_init(NULL, &system);
    assert(err == RERS_SUCCESS);
    assert(system != NULL);
    
    rers_shutdown(system);
    
    printf("PASS\n");
}

/* Test: Get and reset statistics */
static void test_statistics(void) {
    printf("  [TEST] Get and reset statistics... ");
    
    rers_system_t *system = NULL;
    rers_init(NULL, &system);
    
    rers_stats_t stats;
    rers_error_t err = rers_get_stats(system, &stats);
    assert(err == RERS_SUCCESS);
    
    err = rers_reset_stats(system);
    assert(err == RERS_SUCCESS);
    
    err = rers_get_stats(system, &stats);
    assert(err == RERS_SUCCESS);
    assert(stats.errors_detected == 0);
    assert(stats.errors_replayed == 0);
    assert(stats.patterns_learned == 0);
    
    rers_shutdown(system);
    
    printf("PASS\n");
}

/* Test: Get version string */
static void test_get_version(void) {
    printf("  [TEST] Get version string... ");
    
    const char *version = rers_get_version();
    assert(version != NULL);
    assert(strlen(version) > 0);
    printf("[%s] ", version);
    
    printf("PASS\n");
}

/* Test: Get error string */
static void test_get_error_string(void) {
    printf("  [TEST] Get error string... ");
    
    const char *str = rers_get_error_string(RERS_SUCCESS);
    assert(str != NULL);
    assert(strcmp(str, "Success") == 0);
    
    str = rers_get_error_string(RERS_ERROR_INVALID_PARAM);
    assert(str != NULL);
    assert(strlen(str) > 0);
    
    printf("PASS\n");
}

/* Test: Selective component initialization */
static void test_selective_components(void) {
    printf("  [TEST] Selective component initialization... ");
    
    /* Enable only replay and patterns */
    rers_config_t config = {
        .enable_replay = true,
        .enable_learning = false,
        .enable_patterns = true,
        .enable_integration = false,
        .max_patterns = 512,
        .max_replay_depth = 5,
        .learning_threshold = 3
    };
    
    rers_system_t *system = NULL;
    rers_error_t err = rers_init(&config, &system);
    assert(err == RERS_SUCCESS);
    
    rers_shutdown(system);
    
    printf("PASS\n");
}

/* Test: Invalid parameter handling */
static void test_invalid_parameters(void) {
    printf("  [TEST] Invalid parameter handling... ");
    
    rers_error_t err = rers_init(NULL, NULL);
    assert(err == RERS_ERROR_INVALID_PARAM);
    
    err = rers_get_stats(NULL, NULL);
    assert(err == RERS_ERROR_INVALID_PARAM);
    
    err = rers_reset_stats(NULL);
    assert(err == RERS_ERROR_INVALID_PARAM);
    
    printf("PASS\n");
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RERS (Runtime Error Replay System) - Comprehensive Tests\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    printf("=== RERS Main System Tests ===\n\n");
    
    test_system_init_shutdown();
    test_system_init_defaults();
    test_statistics();
    test_get_version();
    test_get_error_string();
    test_selective_components();
    test_invalid_parameters();
    
    printf("\n=== All Main System Tests Passed ===\n");
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✓ All RERS Tests Completed Successfully\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    return 0;
}
