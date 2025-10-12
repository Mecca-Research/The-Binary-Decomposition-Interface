
/**
 * @file test_rers_patterns.c
 * @brief Unit tests for RERS Pattern Database
 */

#include "../rers_patterns.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test fixture */
static rers_pattern_db_t *db = NULL;

/* Setup function */
static void setup(void) {
    rers_pattern_config_t config = {
        .max_patterns = 1024,
        .enable_fuzzy_match = true
    };
    
    rers_error_t err = rers_pattern_init(&config, &db);
    assert(err == RERS_SUCCESS);
    assert(db != NULL);
}

/* Teardown function */
static void teardown(void) {
    if (db) {
        rers_pattern_shutdown(db);
        db = NULL;
    }
}

/* Test: Initialize and shutdown */
static void test_init_shutdown(void) {
    printf("  [TEST] Initialize and shutdown... ");
    
    rers_pattern_config_t config = {
        .max_patterns = 512,
        .enable_fuzzy_match = false
    };
    
    rers_pattern_db_t *test_db = NULL;
    rers_error_t err = rers_pattern_init(&config, &test_db);
    assert(err == RERS_SUCCESS);
    assert(test_db != NULL);
    
    rers_pattern_shutdown(test_db);
    
    printf("PASS\n");
}

/* Test: Add pattern */
static void test_add_pattern(void) {
    printf("  [TEST] Add pattern... ");
    
    setup();
    
    rers_pattern_t pattern = {
        .pattern_id = 0,
        .error_type = RERS_ERROR_TYPE_SEGFAULT,
        .signature = "null_pointer_dereference",
        .description = "NULL pointer dereference detected",
        .fix_suggestion = "Check pointer for NULL before dereferencing",
        .match_count = 0,
        .created_at = 0
    };
    
    uint64_t pattern_id;
    rers_error_t err = rers_pattern_add(db, &pattern, &pattern_id);
    assert(err == RERS_SUCCESS);
    assert(pattern_id > 0);
    
    size_t count = rers_pattern_get_count(db);
    assert(count == 1);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Add multiple patterns */
static void test_add_multiple_patterns(void) {
    printf("  [TEST] Add multiple patterns... ");
    
    setup();
    
    const char *signatures[] = {
        "buffer_overflow",
        "use_after_free",
        "double_free",
        "memory_leak"
    };
    
    for (size_t i = 0; i < sizeof(signatures) / sizeof(signatures[0]); i++) {
        rers_pattern_t pattern = {
            .pattern_id = 0,
            .error_type = RERS_ERROR_TYPE_MEMORY_LEAK,
            .signature = signatures[i],
            .description = "Memory error",
            .fix_suggestion = "Fix memory issue",
            .match_count = 0,
            .created_at = 0
        };
        
        uint64_t pattern_id;
        rers_error_t err = rers_pattern_add(db, &pattern, &pattern_id);
        assert(err == RERS_SUCCESS);
    }
    
    size_t count = rers_pattern_get_count(db);
    assert(count == 4);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Match pattern */
static void test_match_pattern(void) {
    printf("  [TEST] Match pattern... ");
    
    setup();
    
    /* Add pattern */
    rers_pattern_t pattern = {
        .pattern_id = 0,
        .error_type = RERS_ERROR_TYPE_ASSERTION,
        .signature = "assertion_failure",
        .description = "Assertion failed",
        .fix_suggestion = "Review assertion condition",
        .match_count = 0,
        .created_at = 0
    };
    
    uint64_t pattern_id;
    rers_pattern_add(db, &pattern, &pattern_id);
    
    /* Create error context to match */
    rers_error_context_t context = {
        .type = RERS_ERROR_TYPE_ASSERTION,
        .file = "test.c",
        .line = 42,
        .function = "assertion_failure",
        .message = "Assertion failed: x > 0",
        .context_data = NULL,
        .context_size = 0
    };
    
    /* Match pattern */
    rers_match_result_t result;
    rers_error_t err = rers_pattern_match(db, &context, &result);
    assert(err == RERS_SUCCESS);
    assert(result.pattern_id == pattern_id);
    assert(result.confidence != RERS_MATCH_NONE);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get pattern by ID */
static void test_get_pattern(void) {
    printf("  [TEST] Get pattern by ID... ");
    
    setup();
    
    rers_pattern_t pattern = {
        .pattern_id = 0,
        .error_type = RERS_ERROR_TYPE_LOGIC_ERROR,
        .signature = "logic_error",
        .description = "Logic error detected",
        .fix_suggestion = "Review logic flow",
        .match_count = 0,
        .created_at = 0
    };
    
    uint64_t pattern_id;
    rers_pattern_add(db, &pattern, &pattern_id);
    
    /* Retrieve pattern */
    rers_pattern_t retrieved;
    rers_error_t err = rers_pattern_get(db, pattern_id, &retrieved);
    assert(err == RERS_SUCCESS);
    assert(retrieved.pattern_id == pattern_id);
    assert(retrieved.error_type == RERS_ERROR_TYPE_LOGIC_ERROR);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Clear patterns */
static void test_clear_patterns(void) {
    printf("  [TEST] Clear patterns... ");
    
    setup();
    
    /* Add some patterns */
    for (int i = 0; i < 5; i++) {
        rers_pattern_t pattern = {
            .pattern_id = 0,
            .error_type = RERS_ERROR_TYPE_SEGFAULT,
            .signature = "test_pattern",
            .description = "Test",
            .fix_suggestion = "Fix it",
            .match_count = 0,
            .created_at = 0
        };
        
        rers_pattern_add(db, &pattern, NULL);
    }
    
    assert(rers_pattern_get_count(db) == 5);
    
    /* Clear patterns */
    rers_error_t err = rers_pattern_clear(db);
    assert(err == RERS_SUCCESS);
    assert(rers_pattern_get_count(db) == 0);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get confidence name */
static void test_get_confidence_name(void) {
    printf("  [TEST] Get confidence name... ");
    
    const char *name = rers_pattern_get_confidence_name(RERS_MATCH_EXACT);
    assert(name != NULL);
    assert(strcmp(name, "Exact") == 0);
    
    name = rers_pattern_get_confidence_name(RERS_MATCH_HIGH);
    assert(name != NULL);
    assert(strcmp(name, "High") == 0);
    
    printf("PASS\n");
}

/* Main test runner */
int main(void) {
    printf("\n=== RERS Pattern Database Tests ===\n\n");
    
    test_init_shutdown();
    test_add_pattern();
    test_add_multiple_patterns();
    test_match_pattern();
    test_get_pattern();
    test_clear_patterns();
    test_get_confidence_name();
    
    printf("\n=== All Pattern Database Tests Passed ===\n\n");
    return 0;
}
