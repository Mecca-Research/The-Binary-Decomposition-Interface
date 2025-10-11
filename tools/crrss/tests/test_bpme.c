
/**
 * @file test_bpme.c
 * @brief Tests for Bug Prior Mapping Engine
 */

#include "../bpme/bpme.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_bpme_initialization() {
    printf("Testing BPME initialization...\n");
    
    bpme_config_t config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 100
    };
    
    bpme_context_t* ctx = bpme_initialize(&config);
    assert(ctx != NULL);
    
    bpme_shutdown(ctx);
    printf("  ✓ BPME initialization test passed\n");
}

void test_bpme_analyze_snippet() {
    printf("Testing BPME snippet analysis...\n");
    
    bpme_config_t config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 100
    };
    
    bpme_context_t* ctx = bpme_initialize(&config);
    assert(ctx != NULL);
    
    const char* code = "void* ptr = malloc(100);\n// Missing free()";
    bug_prediction_t predictions[10];
    uint32_t num_predictions = 0;
    
    crrss_status_t status = bpme_analyze_snippet(
        ctx, code, strlen(code), predictions, 10, &num_predictions
    );
    
    assert(status == CRRSS_SUCCESS);
    if (status != CRRSS_SUCCESS) {
        printf("  ERROR: bpme_analyze_snippet failed with status %d\n", status);
    }
    printf("  Found %u predictions\n", num_predictions);
    
    bpme_shutdown(ctx);
    printf("  ✓ BPME snippet analysis test passed\n");
}

void test_bpme_pattern_info() {
    printf("Testing BPME pattern information...\n");
    
    bpme_config_t config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 100
    };
    
    bpme_context_t* ctx = bpme_initialize(&config);
    assert(ctx != NULL);
    
    bug_pattern_info_t info;
    crrss_status_t status = bpme_get_pattern_info(ctx, PATTERN_MEMORY_LEAK, &info);
    
    assert(status == CRRSS_SUCCESS);
    if (status != CRRSS_SUCCESS) {
        printf("  ERROR: bpme_get_pattern_info failed with status %d\n", status);
    }
    assert(info.pattern_type == PATTERN_MEMORY_LEAK);
    printf("  Pattern: %s\n", info.pattern_name);
    
    bpme_shutdown(ctx);
    printf("  ✓ BPME pattern info test passed\n");
}

void test_bpme_statistics() {
    printf("Testing BPME statistics...\n");
    
    bpme_config_t config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 100
    };
    
    bpme_context_t* ctx = bpme_initialize(&config);
    assert(ctx != NULL);
    
    uint32_t total_scans = 0;
    uint32_t bugs_predicted = 0;
    double accuracy = 0.0;
    
    crrss_status_t status = bpme_get_statistics(ctx, &total_scans, &bugs_predicted, &accuracy);
    assert(status == CRRSS_SUCCESS);
    if (status != CRRSS_SUCCESS) {
        printf("  ERROR: bpme_get_statistics failed with status %d\n", status);
    }
    
    printf("  Total scans: %u\n", total_scans);
    printf("  Bugs predicted: %u\n", bugs_predicted);
    
    bpme_shutdown(ctx);
    printf("  ✓ BPME statistics test passed\n");
}

int main(void) {
    printf("=== Running BPME Tests ===\n\n");
    
    test_bpme_initialization();
    test_bpme_analyze_snippet();
    test_bpme_pattern_info();
    test_bpme_statistics();
    
    printf("\n=== All BPME Tests Passed ===\n");
    return 0;
}
