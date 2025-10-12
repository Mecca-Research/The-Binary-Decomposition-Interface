/**
 * @file test_deps.c
 * @brief Tests for Dependency Analysis
 */

#include "../deps/deps.h"
#include <stdio.h>
#include <assert.h>

void test_deps_initialization(void) {
    printf("Testing DEPS initialization... ");
    
    deps_config_t config = {
        .analyze_includes = true,
        .detect_circular_deps = true,
        .coupling_threshold = 0.7
    };
    
    deps_context_t* ctx = deps_initialize(&config);
    assert(ctx != NULL);
    
    deps_shutdown(ctx);
    printf("PASSED\n");
}

void test_deps_statistics(void) {
    printf("Testing DEPS statistics... ");
    
    deps_config_t config = {
        .analyze_includes = true,
        .detect_circular_deps = true
    };
    
    deps_context_t* ctx = deps_initialize(&config);
    
    deps_statistics_t stats;
    crrss_status_t status = deps_get_statistics(ctx, &stats);
    
    assert(status == CRRSS_SUCCESS);
    
    deps_shutdown(ctx);
    printf("PASSED\n");
}

int main(void) {
    printf("=== DEPS Test Suite ===\n");
    
    test_deps_initialization();
    test_deps_statistics();
    
    printf("\n✓ All DEPS tests passed!\n");
    return 0;
}
