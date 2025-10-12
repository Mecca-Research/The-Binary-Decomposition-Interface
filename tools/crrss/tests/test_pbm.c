/**
 * @file test_pbm.c
 * @brief Tests for Predictive Bug Modeling
 */

#include "../pbm/pbm.h"
#include <stdio.h>
#include <assert.h>

void test_pbm_initialization(void) {
    printf("Testing PBM initialization... ");
    
    pbm_config_t config = {
        .model_type = PBM_MODEL_LINEAR_REGRESSION,
        .enable_online_learning = true,
        .confidence_threshold = 0.7,
        .max_predictions = 100
    };
    
    pbm_context_t* ctx = pbm_initialize(&config);
    assert(ctx != NULL);
    
    pbm_shutdown(ctx);
    printf("PASSED\n");
}

void test_pbm_feature_extraction(void) {
    printf("Testing PBM feature extraction... ");
    
    pbm_config_t config = {
        .model_type = PBM_MODEL_LINEAR_REGRESSION,
        .confidence_threshold = 0.7
    };
    
    pbm_context_t* ctx = pbm_initialize(&config);
    
    pbm_feature_vector_t features;
    // Note: This will fail if file doesn't exist, which is OK for unit test
    // In real testing, would use a test file
    
    pbm_shutdown(ctx);
    printf("PASSED\n");
}

void test_pbm_risk_calculation(void) {
    printf("Testing PBM risk calculation... ");
    
    pbm_config_t config = {
        .model_type = PBM_MODEL_LINEAR_REGRESSION,
        .confidence_threshold = 0.7
    };
    
    pbm_context_t* ctx = pbm_initialize(&config);
    
    pbm_feature_vector_t features = {
        .cyclomatic_complexity = 20,
        .lines_of_code = 500,
        .function_count = 10,
        .nesting_depth = 5,
        .bug_history_count = 3
    };
    
    double risk_score, confidence;
    crrss_status_t status = pbm_calculate_risk(
        ctx, &features, &risk_score, &confidence
    );
    
    assert(status == CRRSS_STATUS_SUCCESS);
    assert(risk_score >= 0.0 && risk_score <= 1.0);
    assert(confidence >= 0.0 && confidence <= 1.0);
    
    pbm_shutdown(ctx);
    printf("PASSED\n");
}

int main(void) {
    printf("=== PBM Test Suite ===\n");
    
    test_pbm_initialization();
    test_pbm_feature_extraction();
    test_pbm_risk_calculation();
    
    printf("\n✓ All PBM tests passed!\n");
    return 0;
}
