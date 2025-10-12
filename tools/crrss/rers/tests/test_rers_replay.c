
/**
 * @file test_rers_replay.c
 * @brief Unit tests for RERS Replay Engine
 */

#include "../rers_replay.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test fixture */
static rers_replay_engine_t *engine = NULL;

/* Setup function */
static void setup(void) {
    rers_replay_config_t config = {
        .max_depth = 10,
        .enable_segfault = true,
        .enable_assertion = true,
        .enable_memory_leak = true,
        .enable_logic_error = true
    };
    
    rers_error_t err = rers_replay_init(&config, &engine);
    assert(err == RERS_SUCCESS);
    assert(engine != NULL);
}

/* Teardown function */
static void teardown(void) {
    if (engine) {
        rers_replay_shutdown(engine);
        engine = NULL;
    }
}

/* Test: Initialize and shutdown */
static void test_init_shutdown(void) {
    printf("  [TEST] Initialize and shutdown... ");
    
    rers_replay_config_t config = {
        .max_depth = 5,
        .enable_segfault = true,
        .enable_assertion = true,
        .enable_memory_leak = false,
        .enable_logic_error = true
    };
    
    rers_replay_engine_t *test_engine = NULL;
    rers_error_t err = rers_replay_init(&config, &test_engine);
    assert(err == RERS_SUCCESS);
    assert(test_engine != NULL);
    
    rers_replay_shutdown(test_engine);
    
    printf("PASS\n");
}

/* Test: Record segfault error */
static void test_record_segfault(void) {
    printf("  [TEST] Record segfault error... ");
    
    setup();
    
    rers_error_context_t context = {
        .type = RERS_ERROR_TYPE_SEGFAULT,
        .file = "test.c",
        .line = 42,
        .function = "test_function",
        .message = "Segmentation fault at address 0x0",
        .context_data = NULL,
        .context_size = 0
    };
    
    rers_error_t err = rers_replay_record(engine, &context);
    assert(err == RERS_SUCCESS);
    
    size_t count = rers_replay_get_count(engine);
    assert(count == 1);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Record multiple error types */
static void test_record_multiple_errors(void) {
    printf("  [TEST] Record multiple error types... ");
    
    setup();
    
    rers_error_type_t types[] = {
        RERS_ERROR_TYPE_SEGFAULT,
        RERS_ERROR_TYPE_ASSERTION,
        RERS_ERROR_TYPE_MEMORY_LEAK,
        RERS_ERROR_TYPE_LOGIC_ERROR
    };
    
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        rers_error_context_t context = {
            .type = types[i],
            .file = "test.c",
            .line = (int)(100 + i),
            .function = "test_function",
            .message = "Test error",
            .context_data = NULL,
            .context_size = 0
        };
        
        rers_error_t err = rers_replay_record(engine, &context);
        assert(err == RERS_SUCCESS);
    }
    
    size_t count = rers_replay_get_count(engine);
    assert(count == 4);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Replay recorded error */
static void test_replay_error(void) {
    printf("  [TEST] Replay recorded error... ");
    
    setup();
    
    rers_error_context_t context = {
        .type = RERS_ERROR_TYPE_ASSERTION,
        .file = "test.c",
        .line = 100,
        .function = "test_assertion",
        .message = "Assertion failed: x > 0",
        .context_data = NULL,
        .context_size = 0
    };
    
    rers_replay_record(engine, &context);
    
    rers_error_t err = rers_replay_execute(engine, 1);
    assert(err == RERS_SUCCESS);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get error type name */
static void test_get_type_name(void) {
    printf("  [TEST] Get error type name... ");
    
    const char *name = rers_replay_get_type_name(RERS_ERROR_TYPE_SEGFAULT);
    assert(name != NULL);
    assert(strcmp(name, "Segmentation Fault") == 0);
    
    name = rers_replay_get_type_name(RERS_ERROR_TYPE_ASSERTION);
    assert(name != NULL);
    assert(strcmp(name, "Assertion Failure") == 0);
    
    printf("PASS\n");
}

/* Main test runner */
int main(void) {
    printf("\n=== RERS Replay Engine Tests ===\n\n");
    
    test_init_shutdown();
    test_record_segfault();
    test_record_multiple_errors();
    test_replay_error();
    test_get_type_name();
    
    printf("\n=== All Replay Engine Tests Passed ===\n\n");
    return 0;
}
