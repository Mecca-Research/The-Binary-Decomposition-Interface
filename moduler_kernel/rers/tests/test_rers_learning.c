
/**
 * @file test_rers_learning.c
 * @brief Unit tests for RERS Learning System
 */

#include "../rers_learning.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test fixture */
static rers_learning_system_t *system = NULL;

/* Setup function */
static void setup(void) {
    rers_learning_config_t config = {
        .max_hierarchy_depth = 5,
        .priority_levels = 4,
        .learning_threshold = 5
    };
    
    rers_error_t err = rers_learning_init(&config, &system);
    assert(err == RERS_SUCCESS);
    assert(system != NULL);
}

/* Teardown function */
static void teardown(void) {
    if (system) {
        rers_learning_shutdown(system);
        system = NULL;
    }
}

/* Test: Initialize and shutdown */
static void test_init_shutdown(void) {
    printf("  [TEST] Initialize and shutdown... ");
    
    rers_learning_config_t config = {
        .max_hierarchy_depth = 3,
        .priority_levels = 4,
        .learning_threshold = 3
    };
    
    rers_learning_system_t *test_system = NULL;
    rers_error_t err = rers_learning_init(&config, &test_system);
    assert(err == RERS_SUCCESS);
    assert(test_system != NULL);
    
    rers_learning_shutdown(test_system);
    
    printf("PASS\n");
}

/* Test: Learn from critical bug */
static void test_learn_critical_bug(void) {
    printf("  [TEST] Learn from critical bug... ");
    
    setup();
    
    rers_bug_info_t bug = {
        .bug_id = 0,
        .error_type = RERS_ERROR_TYPE_SEGFAULT,
        .priority = RERS_PRIORITY_CRITICAL,
        .level = RERS_HIERARCHY_ERROR_TYPE,
        .component = "memory_manager",
        .description = "NULL pointer dereference in allocation",
        .occurrence_count = 0,
        .first_seen = 0,
        .last_seen = 0
    };
    
    rers_error_t err = rers_learning_learn(system, &bug);
    assert(err == RERS_SUCCESS);
    
    size_t count = rers_learning_get_count(system);
    assert(count == 1);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Learn bugs with different priorities */
static void test_learn_multiple_priorities(void) {
    printf("  [TEST] Learn bugs with different priorities... ");
    
    setup();
    
    rers_priority_t priorities[] = {
        RERS_PRIORITY_CRITICAL,
        RERS_PRIORITY_HIGH,
        RERS_PRIORITY_MEDIUM,
        RERS_PRIORITY_LOW
    };
    
    const char *components[] = {
        "component_critical",
        "component_high",
        "component_medium",
        "component_low"
    };
    
    for (size_t i = 0; i < sizeof(priorities) / sizeof(priorities[0]); i++) {
        rers_bug_info_t bug = {
            .bug_id = 0,
            .error_type = RERS_ERROR_TYPE_LOGIC_ERROR,
            .priority = priorities[i],
            .level = RERS_HIERARCHY_COMPONENT,
            .component = components[i],
            .description = "Test bug",
            .occurrence_count = 0
        };
        
        rers_error_t err = rers_learning_learn(system, &bug);
        assert(err == RERS_SUCCESS);
    }
    
    size_t count = rers_learning_get_count(system);
    assert(count == 4);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get bugs by priority */
static void test_get_by_priority(void) {
    printf("  [TEST] Get bugs by priority... ");
    
    setup();
    
    /* Add critical bugs with unique component names */
    for (int i = 0; i < 3; i++) {
        char component_name[64];
        snprintf(component_name, sizeof(component_name), "component_a_%d", i);
        
        rers_bug_info_t bug = {
            .bug_id = 0,
            .error_type = RERS_ERROR_TYPE_SEGFAULT,
            .priority = RERS_PRIORITY_CRITICAL,
            .level = RERS_HIERARCHY_ERROR_TYPE,
            .component = component_name,
            .description = "Critical bug",
            .occurrence_count = 0
        };
        
        rers_learning_learn(system, &bug);
    }
    
    /* Get critical bugs */
    uint64_t bugs[10];
    size_t count;
    rers_error_t err = rers_learning_get_by_priority(system, 
                                                     RERS_PRIORITY_CRITICAL,
                                                     bugs, 10, &count);
    assert(err == RERS_SUCCESS);
    assert(count == 3);
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Hierarchical learning */
static void test_hierarchical_learning(void) {
    printf("  [TEST] Hierarchical learning... ");
    
    setup();
    
    rers_hierarchy_level_t levels[] = {
        RERS_HIERARCHY_ERROR_TYPE,
        RERS_HIERARCHY_COMPONENT,
        RERS_HIERARCHY_SUBSYSTEM,
        RERS_HIERARCHY_SYSTEM,
        RERS_HIERARCHY_GLOBAL
    };
    
    const char *components[] = {
        "hierarchical_error_type",
        "hierarchical_component",
        "hierarchical_subsystem",
        "hierarchical_system",
        "hierarchical_global"
    };
    
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        rers_bug_info_t bug = {
            .bug_id = 0,
            .error_type = RERS_ERROR_TYPE_MEMORY_LEAK,
            .priority = RERS_PRIORITY_HIGH,
            .level = levels[i],
            .component = components[i],
            .description = "Hierarchical bug",
            .occurrence_count = 0
        };
        
        rers_error_t err = rers_learning_learn(system, &bug);
        assert(err == RERS_SUCCESS);
    }
    
    /* Verify bugs at each level */
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        uint64_t bugs[10];
        size_t count;
        rers_error_t err = rers_learning_get_by_level(system, levels[i],
                                                      bugs, 10, &count);
        assert(err == RERS_SUCCESS);
        assert(count == 1);
    }
    
    teardown();
    
    printf("PASS\n");
}

/* Test: Get priority name */
static void test_get_priority_name(void) {
    printf("  [TEST] Get priority name... ");
    
    const char *name = rers_learning_get_priority_name(RERS_PRIORITY_CRITICAL);
    assert(name != NULL);
    assert(strcmp(name, "Critical") == 0);
    
    name = rers_learning_get_priority_name(RERS_PRIORITY_HIGH);
    assert(name != NULL);
    assert(strcmp(name, "High") == 0);
    
    printf("PASS\n");
}

/* Main test runner */
int main(void) {
    printf("\n=== RERS Learning System Tests ===\n\n");
    
    test_init_shutdown();
    test_learn_critical_bug();
    test_learn_multiple_priorities();
    test_get_by_priority();
    test_hierarchical_learning();
    test_get_priority_name();
    
    printf("\n=== All Learning System Tests Passed ===\n\n");
    return 0;
}
