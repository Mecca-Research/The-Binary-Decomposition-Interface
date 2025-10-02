
/**
 * @file test_fiber.c
 * @brief Unit tests for fiber system
 */

#include "../fibers/fiber.h"
#include "../fibers/fiber_scheduler.h"
#include <stdio.h>
#include <assert.h>

static int g_counter = 0;

// Simple fiber entry point
void simple_fiber(void* arg) {
    int* value = (int*)arg;
    g_counter += *value;
}

// Test fiber creation and destruction
void test_fiber_basic(void) {
    printf("Testing fiber basic operations...\n");
    
    int value = 42;
    fiber_t* fiber = fiber_create(simple_fiber, &value, 0, FIBER_PRIORITY_NORMAL);
    assert(fiber != NULL);
    assert(fiber->state == FIBER_STATE_READY);
    assert(fiber->priority == FIBER_PRIORITY_NORMAL);
    
    fiber_destroy(fiber);
    printf("Fiber basic operations: PASSED\n");
}

// Test fiber scheduler
void test_fiber_scheduler(void) {
    printf("Testing fiber scheduler...\n");
    
    fiber_scheduler_t* scheduler = fiber_scheduler_create(0);
    assert(scheduler != NULL);
    
    g_counter = 0;
    
    // Spawn fibers
    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        uint64_t fiber_id = fiber_scheduler_spawn(scheduler, simple_fiber, &values[i],
                                                   0, FIBER_PRIORITY_NORMAL);
        assert(fiber_id != 0);
    }
    
    // Run scheduler
    fiber_scheduler_run(scheduler);
    
    // Check that all fibers ran
    assert(g_counter == 15);  // 1+2+3+4+5
    
    fiber_scheduler_destroy(scheduler);
    printf("Fiber scheduler: PASSED\n");
}

int main(void) {
    printf("=== Fiber System Tests ===\n");
    
    test_fiber_basic();
    test_fiber_scheduler();
    
    printf("=== All fiber tests PASSED ===\n");
    return 0;
}
