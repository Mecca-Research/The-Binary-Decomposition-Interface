/**
 * @file test_cache_fix.c
 * @brief Test to verify the prediction caching fix
 */

#include <stdio.h>
#include <stdlib.h>
#include "bpme/bpme.h"

int main(void) {
    printf("=== Testing BPME Prediction Caching Fix ===\n\n");
    
    // Initialize BPME
    bpme_config_t config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 100
    };
    
    bpme_context_t* ctx = bpme_initialize(&config);
    if (!ctx) {
        printf("✗ Failed to initialize BPME\n");
        return 1;
    }
    printf("✓ BPME initialized\n");
    
    // Create a test file with some code patterns
    const char* test_file = "/tmp/test_cache_fix.c";
    FILE* fp = fopen(test_file, "w");
    if (!fp) {
        printf("✗ Failed to create test file\n");
        bpme_shutdown(ctx);
        return 1;
    }
    
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdlib.h>\n");
    fprintf(fp, "\n");
    fprintf(fp, "int main() {\n");
    fprintf(fp, "    char* ptr = malloc(100);\n");  // Unchecked malloc
    fprintf(fp, "    strcpy(ptr, \"test\");\n");      // Unsafe strcpy
    fprintf(fp, "    ptr->value = 10;\n");            // Potential NULL deref
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    printf("✓ Created test file: %s\n", test_file);
    
    // Analyze the file
    bug_prediction_t predictions[50];
    uint32_t num_predictions = 0;
    
    crrss_status_t status = bpme_analyze_file(
        ctx, test_file, predictions, 50, &num_predictions
    );
    
    if (status != CRRSS_SUCCESS) {
        printf("✗ File analysis failed with status: %d\n", status);
        bpme_shutdown(ctx);
        return 1;
    }
    
    printf("✓ File analyzed successfully\n");
    printf("  Found %u predictions\n", num_predictions);
    
    // Print the predictions from the analysis
    printf("\n--- Predictions from analysis ---\n");
    for (uint32_t i = 0; i < num_predictions; i++) {
        printf("Prediction %u:\n", i + 1);
        printf("  File: %s\n", predictions[i].file_path);
        printf("  Line: %u\n", predictions[i].line_number);
        printf("  Priority: %d\n", predictions[i].priority);
        printf("  Category: %d\n", predictions[i].category);
        printf("  Description: %s\n", predictions[i].description);
        printf("\n");
    }
    
    // Now query by priority P0 (CRITICAL)
    printf("\n--- Query Test: P0 (CRITICAL) bugs ---\n");
    bug_prediction_t query_results[50];
    uint32_t num_results = 0;
    
    status = bpme_query_by_priority(
        ctx, BUG_PRIORITY_P0_CRITICAL, query_results, 50, &num_results
    );
    
    if (status != CRRSS_SUCCESS) {
        printf("✗ Query failed with status: %d\n", status);
        bpme_shutdown(ctx);
        return 1;
    }
    
    printf("✓ Query executed successfully\n");
    printf("  Found %u P0 predictions in cache\n", num_results);
    
    // Print query results
    if (num_results > 0) {
        for (uint32_t i = 0; i < num_results; i++) {
            printf("Query Result %u:\n", i + 1);
            printf("  File: %s\n", query_results[i].file_path);
            printf("  Line: %u\n", query_results[i].line_number);
            printf("  Description: %s\n", query_results[i].description);
            printf("\n");
        }
    } else {
        printf("  (No P0 predictions found)\n");
    }
    
    // Query by priority P1 (HIGH)
    printf("\n--- Query Test: P1 (HIGH) bugs ---\n");
    num_results = 0;
    
    status = bpme_query_by_priority(
        ctx, BUG_PRIORITY_P1_HIGH, query_results, 50, &num_results
    );
    
    if (status != CRRSS_SUCCESS) {
        printf("✗ Query failed with status: %d\n", status);
        bpme_shutdown(ctx);
        return 1;
    }
    
    printf("✓ Query executed successfully\n");
    printf("  Found %u P1 predictions in cache\n", num_results);
    
    if (num_results > 0) {
        for (uint32_t i = 0; i < num_results; i++) {
            printf("Query Result %u:\n", i + 1);
            printf("  File: %s\n", query_results[i].file_path);
            printf("  Line: %u\n", query_results[i].line_number);
            printf("  Description: %s\n", query_results[i].description);
            printf("\n");
        }
    } else {
        printf("  (No P1 predictions found)\n");
    }
    
    // Query by category MEMORY
    printf("\n--- Query Test: MEMORY category bugs ---\n");
    num_results = 0;
    
    status = bpme_query_by_category(
        ctx, BUG_CATEGORY_MEMORY, query_results, 50, &num_results
    );
    
    if (status != CRRSS_SUCCESS) {
        printf("✗ Query failed with status: %d\n", status);
        bpme_shutdown(ctx);
        return 1;
    }
    
    printf("✓ Query executed successfully\n");
    printf("  Found %u MEMORY predictions in cache\n", num_results);
    
    if (num_results > 0) {
        for (uint32_t i = 0; i < num_results; i++) {
            printf("Query Result %u:\n", i + 1);
            printf("  File: %s\n", query_results[i].file_path);
            printf("  Line: %u\n", query_results[i].line_number);
            printf("  Description: %s\n", query_results[i].description);
            printf("\n");
        }
    }
    
    // Verify the fix worked
    printf("\n=== Verification Summary ===\n");
    if (num_predictions > 0 && num_results > 0) {
        printf("✓ CACHE FIX VERIFIED: Predictions are being cached!\n");
        printf("  - Analysis found %u predictions\n", num_predictions);
        printf("  - Query returned %u results from cache\n", num_results);
        printf("  - Fix is working correctly!\n");
    } else if (num_predictions > 0 && num_results == 0) {
        printf("✗ CACHE FIX FAILED: Predictions not cached!\n");
        printf("  - Analysis found %u predictions\n", num_predictions);
        printf("  - Query returned 0 results from cache\n");
        printf("  - Bug still present!\n");
    } else {
        printf("⚠ No predictions found in analysis (test may need adjustment)\n");
    }
    
    // Cleanup
    bpme_shutdown(ctx);
    remove(test_file);
    
    return 0;
}
