
/**
 * @file test_rers_integration.c
 * @brief Unit tests for RERS Integration Layer
 */

#include "../rers_integration.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test fixture */
static rers_integration_layer_t *layer = NULL;

/* Setup function */
static void setup(void) {
    rers_integration_config_t config = {
        .max_profiles = 4,
        .enable_coordination = true
    };
    
    rers_error_t err = rers_integration_init(&config, &layer);
    assert(err == RERS_SUCCESS);
    assert(layer != NULL);
}

/* Teardown function */
static void teardown(void) {
    if (layer) {
        rers_integration_shutdown(layer);
        layer = NULL;
    }
}

/* Test: Initialize and shutdown */
static void test_init_shutdown(void) {
    printf("  [TEST] Initialize and shutdown... ");
    
    rers_integration_config_t config = {
        .max_profiles = 4,
        .enable_coordination = true
    };
    
    rers_integration_layer_t *test_layer = NULL;
    rers_error_t err = rers_integration_init(&config, &test_layer);
    assert(err == RERS_SUCCESS);
    assert(test_layer != NULL);
    
    rers_integration_shutdown(test_layer);
    
    printf("PASS\n");
}

/* Test: Submit profile output */
static void test_submit_output(void) {
    printf("  [TEST] Submit profile output... ");
    
    setup();
    
    rers_profile_output_t output = {
        .profile = RERS_PROFILE_MSM,
        .task = RERS_TASK_ERROR_ANALYSIS,
        .data = NULL,
        .data_size = 0,
        .confidence = 0.85f,
        .timestamp = 0
    };
    
    rers_error_t err = rers_integration_submit_output(layer, &output);
    assert(err == RERS_SUCCESS);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Coordinate profiles for error analysis */
static void test_coordinate_error_analysis(void) {
    printf("  [TEST] Coordinate profiles for error analysis... ");
    
    setup();
    
    /* Submit outputs from different profiles */
    rers_profile_type_t profiles[] = {
        RERS_PROFILE_MSM,
        RERS_PROFILE_STP,
        RERS_PROFILE_BPME,
        RERS_PROFILE_TDT
    };
    
    for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++) {
        rers_profile_output_t output = {
            .profile = profiles[i],
            .task = RERS_TASK_ERROR_ANALYSIS,
            .data = NULL,
            .data_size = 0,
            .confidence = 0.8f + (i * 0.05f),
            .timestamp = 0
        };
        
        rers_integration_submit_output(layer, &output);
    }
    
    /* Coordinate task */
    rers_coordination_result_t result;
    rers_error_t err = rers_integration_coordinate(layer, 
                                                   RERS_TASK_ERROR_ANALYSIS,
                                                   &result);
    assert(err == RERS_SUCCESS);
    assert(result.task == RERS_TASK_ERROR_ANALYSIS);
    assert(result.overall_confidence > 0.0f);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get active profiles for task */
static void test_get_active_profiles(void) {
    printf("  [TEST] Get active profiles for task... ");
    
    setup();
    
    rers_profile_type_t profiles[10];
    size_t count;
    rers_error_t err = rers_integration_get_active_profiles(layer,
                                                            RERS_TASK_PATTERN_MATCHING,
                                                            profiles, 10, &count);
    assert(err == RERS_SUCCESS);
    assert(count > 0);
    assert(count <= 4);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Enable/disable profile */
static void test_enable_disable_profile(void) {
    printf("  [TEST] Enable/disable profile... ");
    
    setup();
    
    /* Disable STP profile */
    rers_error_t err = rers_integration_set_profile_enabled(layer,
                                                            RERS_PROFILE_STP,
                                                            false);
    assert(err == RERS_SUCCESS);
    
    /* Re-enable STP profile */
    err = rers_integration_set_profile_enabled(layer,
                                               RERS_PROFILE_STP,
                                               true);
    assert(err == RERS_SUCCESS);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Multiple task coordination */
static void test_multiple_task_coordination(void) {
    printf("  [TEST] Multiple task coordination... ");
    
    setup();
    
    rers_task_type_t tasks[] = {
        RERS_TASK_ERROR_ANALYSIS,
        RERS_TASK_PATTERN_MATCHING,
        RERS_TASK_TEST_GENERATION,
        RERS_TASK_STATE_TRACKING
    };
    
    /* Submit outputs for different tasks */
    for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        rers_profile_output_t output = {
            .profile = RERS_PROFILE_BPME,
            .task = tasks[i],
            .data = NULL,
            .data_size = 0,
            .confidence = 0.75f,
            .timestamp = 0
        };
        
        rers_integration_submit_output(layer, &output);
    }
    
    /* Coordinate each task */
    for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        rers_coordination_result_t result;
        rers_error_t err = rers_integration_coordinate(layer, tasks[i], &result);
        assert(err == RERS_SUCCESS);
        assert(result.task == tasks[i]);
    }
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get profile name */
static void test_get_profile_name(void) {
    printf("  [TEST] Get profile name... ");
    
    const char *name = rers_integration_get_profile_name(RERS_PROFILE_MSM);
    assert(name != NULL);
    assert(strstr(name, "MSM") != NULL);
    
    name = rers_integration_get_profile_name(RERS_PROFILE_BPME);
    assert(name != NULL);
    assert(strstr(name, "BPME") != NULL);
    
    printf("PASS\n");
}

/* Test: Get task name */
static void test_get_task_name(void) {
    printf("  [TEST] Get task name... ");
    
    const char *name = rers_integration_get_task_name(RERS_TASK_ERROR_ANALYSIS);
    assert(name != NULL);
    assert(strlen(name) > 0);
    
    name = rers_integration_get_task_name(RERS_TASK_TEST_GENERATION);
    assert(name != NULL);
    assert(strlen(name) > 0);
    
    printf("PASS\n");
}

/* Test: Output queue clearing after coordination (Bug fix verification) */
static void test_output_queue_clearing(void) {
    printf("  [TEST] Output queue clearing after coordination... ");
    
    setup();
    
    /* Test submitting more than 16 outputs across multiple coordination cycles */
    const int TOTAL_CYCLES = 5;
    const int OUTPUTS_PER_CYCLE = 10;
    
    for (int cycle = 0; cycle < TOTAL_CYCLES; cycle++) {
        /* Submit multiple outputs for the same task */
        for (int i = 0; i < OUTPUTS_PER_CYCLE; i++) {
            rers_profile_output_t output = {
                .profile = RERS_PROFILE_MSM,
                .task = RERS_TASK_ERROR_ANALYSIS,
                .data = NULL,
                .data_size = 0,
                .confidence = 0.85f,
                .timestamp = 0
            };
            
            rers_error_t err = rers_integration_submit_output(layer, &output);
            assert(err == RERS_SUCCESS);
        }
        
        /* Coordinate the task - this should clear the output queue */
        rers_coordination_result_t result;
        rers_error_t err = rers_integration_coordinate(layer, 
                                                       RERS_TASK_ERROR_ANALYSIS,
                                                       &result);
        assert(err == RERS_SUCCESS);
        assert(result.task == RERS_TASK_ERROR_ANALYSIS);
        assert(result.overall_confidence > 0.0f);
    }
    
    /* Verify we can still submit outputs after multiple cycles */
    rers_profile_output_t final_output = {
        .profile = RERS_PROFILE_STP,
        .task = RERS_TASK_ERROR_ANALYSIS,
        .data = NULL,
        .data_size = 0,
        .confidence = 0.90f,
        .timestamp = 0
    };
    
    rers_error_t err = rers_integration_submit_output(layer, &final_output);
    assert(err == RERS_SUCCESS);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Output queue overflow protection */
static void test_output_queue_overflow(void) {
    printf("  [TEST] Output queue overflow protection... ");
    
    setup();
    
    /* Submit exactly 16 outputs (MAX_OUTPUTS_PER_TASK) */
    for (int i = 0; i < 16; i++) {
        rers_profile_output_t output = {
            .profile = RERS_PROFILE_MSM,
            .task = RERS_TASK_ERROR_ANALYSIS,
            .data = NULL,
            .data_size = 0,
            .confidence = 0.85f,
            .timestamp = 0
        };
        
        rers_error_t err = rers_integration_submit_output(layer, &output);
        assert(err == RERS_SUCCESS);
    }
    
    /* Try to submit one more - should fail with COMPONENT_FAILED */
    rers_profile_output_t overflow_output = {
        .profile = RERS_PROFILE_STP,
        .task = RERS_TASK_ERROR_ANALYSIS,
        .data = NULL,
        .data_size = 0,
        .confidence = 0.90f,
        .timestamp = 0
    };
    
    rers_error_t err = rers_integration_submit_output(layer, &overflow_output);
    assert(err == RERS_ERROR_COMPONENT_FAILED);
    
    /* Coordinate to clear the queue */
    rers_coordination_result_t result;
    err = rers_integration_coordinate(layer, RERS_TASK_ERROR_ANALYSIS, &result);
    assert(err == RERS_SUCCESS);
    
    /* Now we should be able to submit again */
    err = rers_integration_submit_output(layer, &overflow_output);
    assert(err == RERS_SUCCESS);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Memory leak prevention with data copies */
static void test_memory_leak_prevention(void) {
    printf("  [TEST] Memory leak prevention with data copies... ");
    
    setup();
    
    const int CYCLES = 3;
    const int OUTPUTS_PER_CYCLE = 8;
    const int DATA_SIZE = 1024;
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        /* Submit outputs with data that needs to be copied */
        for (int i = 0; i < OUTPUTS_PER_CYCLE; i++) {
            char test_data[DATA_SIZE];
            memset(test_data, 'A' + (i % 26), DATA_SIZE);
            
            rers_profile_output_t output = {
                .profile = RERS_PROFILE_BPME,
                .task = RERS_TASK_PATTERN_MATCHING,
                .data = test_data,
                .data_size = DATA_SIZE,
                .confidence = 0.75f,
                .timestamp = 0
            };
            
            rers_error_t err = rers_integration_submit_output(layer, &output);
            assert(err == RERS_SUCCESS);
        }
        
        /* Coordinate - this should free all data copies */
        rers_coordination_result_t result;
        rers_error_t err = rers_integration_coordinate(layer,
                                                       RERS_TASK_PATTERN_MATCHING,
                                                       &result);
        assert(err == RERS_SUCCESS);
    }
    
    teardown();
    
    printf("PASS\n");
}

/* Main test runner */
int main(void) {
    printf("\n=== RERS Integration Layer Tests ===\n\n");
    
    test_init_shutdown();
    test_submit_output();
    test_coordinate_error_analysis();
    test_get_active_profiles();
    test_enable_disable_profile();
    test_multiple_task_coordination();
    test_get_profile_name();
    test_get_task_name();
    
    /* Bug fix verification tests */
    test_output_queue_clearing();
    test_output_queue_overflow();
    test_memory_leak_prevention();
    
    printf("\n=== All Integration Layer Tests Passed ===\n\n");
    return 0;
}
