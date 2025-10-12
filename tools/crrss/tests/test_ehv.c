/**
 * @file test_ehv.c
 * @brief Tests for Error Heatmap Visualization
 */

#include "../ehv/ehv.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_ehv_initialization(void) {
    printf("Testing EHV initialization... ");
    
    ehv_config_t config = {
        .enable_clustering = true,
        .enable_temporal_analysis = true,
        .max_hotspots = 100,
        .max_clusters = 50,
        .clustering_threshold = 3,
        .heat_threshold = 0.3
    };
    
    ehv_context_t* ctx = ehv_initialize(&config);
    assert(ctx != NULL);
    
    ehv_shutdown(ctx);
    printf("PASSED\n");
}

void test_ehv_record_error(void) {
    printf("Testing EHV error recording... ");
    
    ehv_config_t config = {
        .enable_clustering = true,
        .max_hotspots = 100,
        .heat_threshold = 0.0
    };
    
    ehv_context_t* ctx = ehv_initialize(&config);
    assert(ctx != NULL);
    
    crrss_status_t status = ehv_record_error(
        ctx, "test.c", "test_func", 10,
        BUG_CATEGORY_MEMORY, BUG_PRIORITY_P0_CRITICAL
    );
    assert(status == CRRSS_SUCCESS);
    
    // Record same error again (should increment frequency)
    status = ehv_record_error(
        ctx, "test.c", "test_func", 10,
        BUG_CATEGORY_MEMORY, BUG_PRIORITY_P0_CRITICAL
    );
    assert(status == CRRSS_SUCCESS);
    
    ehv_shutdown(ctx);
    printf("PASSED\n");
}

void test_ehv_hotspots(void) {
    printf("Testing EHV hotspot identification... ");
    
    ehv_config_t config = {
        .enable_clustering = true,
        .max_hotspots = 100,
        .heat_threshold = 0.0
    };
    
    ehv_context_t* ctx = ehv_initialize(&config);
    
    // Record multiple errors
    for (int i = 0; i < 5; i++) {
        ehv_record_error(ctx, "hotspot.c", "hot_func", 45,
                        BUG_CATEGORY_MEMORY, BUG_PRIORITY_P0_CRITICAL);
    }
    
    ehv_hotspot_t hotspots[10];
    uint32_t count = 0;
    crrss_status_t status = ehv_identify_hotspots(ctx, hotspots, 10, &count);
    
    assert(status == CRRSS_SUCCESS);
    assert(count > 0);
    
    ehv_shutdown(ctx);
    printf("PASSED\n");
}

void test_ehv_export(void) {
    printf("Testing EHV visualization export... ");
    
    ehv_config_t config = {
        .enable_clustering = true,
        .max_hotspots = 100,
        .heat_threshold = 0.0
    };
    
    ehv_context_t* ctx = ehv_initialize(&config);
    
    ehv_record_error(ctx, "test.c", "func", 10,
                    BUG_CATEGORY_MEMORY, BUG_PRIORITY_P1_HIGH);
    
    crrss_status_t status = ehv_export_visualization(
        ctx, EHV_FORMAT_JSON, "/tmp/ehv_test.json"
    );
    assert(status == CRRSS_SUCCESS);
    
    ehv_shutdown(ctx);
    printf("PASSED\n");
}

int main(void) {
    printf("=== EHV Test Suite ===\n");
    
    test_ehv_initialization();
    test_ehv_record_error();
    test_ehv_hotspots();
    test_ehv_export();
    
    printf("\n✓ All EHV tests passed!\n");
    return 0;
}
