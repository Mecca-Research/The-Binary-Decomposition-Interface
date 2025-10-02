
/**
 * @file test_timer_wheel.c
 * @brief Test hierarchical timer wheel
 */

#include "timer_wheel.h"
#include <stdio.h>
#include <assert.h>

static int g_callback_count = 0;

static void test_callback(void* arg) {
    int* value = (int*)arg;
    printf("Timer expired with value: %d\n", *value);
    g_callback_count++;
}

int main(void) {
    printf("Testing hierarchical timer wheel...\n\n");
    
    // Create timer wheel
    timer_wheel_t* wheel = timer_wheel_create();
    assert(wheel != NULL);
    
    // Add timers
    int val1 = 1, val2 = 2, val3 = 3;
    
    timer_id_t id1 = timer_wheel_add(wheel, 10, test_callback, &val1);
    assert(id1 != TIMER_ID_INVALID);
    printf("Added timer 1 (10ms)\n");
    
    timer_id_t id2 = timer_wheel_add(wheel, 50, test_callback, &val2);
    assert(id2 != TIMER_ID_INVALID);
    printf("Added timer 2 (50ms)\n");
    
    timer_id_t id3 = timer_wheel_add(wheel, 100, test_callback, &val3);
    assert(id3 != TIMER_ID_INVALID);
    printf("Added timer 3 (100ms)\n");
    
    // Advance time and process timers
    printf("\nAdvancing time...\n");
    
    for (int i = 0; i < 110; i++) {
        uint32_t expired = timer_wheel_tick(wheel);
        if (expired > 0) {
            printf("Tick %d: %u timers expired\n", i, expired);
        }
    }
    
    assert(g_callback_count == 3);
    printf("\nAll timers expired correctly\n");
    
    // Test cancellation
    g_callback_count = 0;
    
    timer_id_t id4 = timer_wheel_add(wheel, 20, test_callback, &val1);
    assert(id4 != TIMER_ID_INVALID);
    
    assert(timer_wheel_cancel(wheel, id4) == 0);
    printf("Cancelled timer 4\n");
    
    timer_wheel_advance_to(wheel, wheel->current_time_ms + 30);
    assert(g_callback_count == 0);
    printf("Cancelled timer did not fire\n");
    
    // Print statistics
    timer_wheel_print_stats(wheel);
    
    // Cleanup
    timer_wheel_destroy(wheel);
    
    printf("\nAll timer wheel tests passed!\n");
    return 0;
}
