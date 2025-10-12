/**
 * @file test_fix.c
 * @brief Tests for Fix Suggestions
 */

#include "../fix_suggestions/fix_suggestions.h"
#include <stdio.h>
#include <assert.h>

void test_fix_initialization(void) {
    printf("Testing Fix initialization... ");
    
    fix_config_t config = {
        .enable_buffer_overflow_fixes = true,
        .min_confidence = 0.8
    };
    
    fix_context_t* ctx = fix_initialize(&config);
    assert(ctx != NULL);
    
    fix_shutdown(ctx);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Fix Suggestions Test Suite ===\n");
    
    test_fix_initialization();
    
    printf("\n✓ All Fix tests passed!\n");
    return 0;
}
