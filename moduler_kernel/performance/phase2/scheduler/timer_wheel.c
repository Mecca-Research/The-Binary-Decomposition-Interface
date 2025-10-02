
/**
 * @file timer_wheel.c
 * @brief Hierarchical timer wheel implementation
 */

#include "timer_wheel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Get level and slot for timer
 */
static void get_timer_position(timer_wheel_t* wheel, uint64_t expires_at,
                               int* level, uint32_t* slot) {
    uint64_t delta = expires_at - wheel->current_time_ms;
    
    if (delta < TIMER_WHEEL_LEVEL1_TICK_MS) {
        // Level 0: 0-255ms
        *level = 0;
        *slot = (wheel->levels[0].current_slot + delta / TIMER_WHEEL_LEVEL0_TICK_MS) % TIMER_WHEEL_SLOTS;
    } else if (delta < TIMER_WHEEL_LEVEL2_TICK_MS) {
        // Level 1: 256ms-64s
        *level = 1;
        *slot = (wheel->levels[1].current_slot + delta / TIMER_WHEEL_LEVEL1_TICK_MS) % TIMER_WHEEL_SLOTS;
    } else if (delta < TIMER_WHEEL_LEVEL3_TICK_MS) {
        // Level 2: 64s-4.5h
        *level = 2;
        *slot = (wheel->levels[2].current_slot + delta / TIMER_WHEEL_LEVEL2_TICK_MS) % TIMER_WHEEL_SLOTS;
    } else {
        // Level 3: 4.5h+
        *level = 3;
        *slot = (wheel->levels[3].current_slot + delta / TIMER_WHEEL_LEVEL3_TICK_MS) % TIMER_WHEEL_SLOTS;
    }
}

/**
 * @brief Insert timer into wheel
 */
static void insert_timer(timer_wheel_t* wheel, timer_entry_t* timer) {
    int level;
    uint32_t slot;
    
    get_timer_position(wheel, timer->expires_at, &level, &slot);
    
    // Insert at head of list
    timer->next = wheel->levels[level].slots[slot];
    timer->prev = NULL;
    
    if (timer->next) {
        timer->next->prev = timer;
    }
    
    wheel->levels[level].slots[slot] = timer;
}

/**
 * @brief Remove timer from wheel
 */
static void remove_timer(timer_wheel_t* wheel, timer_entry_t* timer, int level, uint32_t slot) {
    if (timer->prev) {
        timer->prev->next = timer->next;
    } else {
        wheel->levels[level].slots[slot] = timer->next;
    }
    
    if (timer->next) {
        timer->next->prev = timer->prev;
    }
}

/**
 * @brief Cascade timers from higher level
 */
static void cascade_level(timer_wheel_t* wheel, int level) {
    if (level >= TIMER_WHEEL_LEVELS) {
        return;
    }
    
    uint32_t slot = wheel->levels[level].current_slot;
    timer_entry_t* timer = wheel->levels[level].slots[slot];
    wheel->levels[level].slots[slot] = NULL;
    
    // Re-insert all timers from this slot
    while (timer) {
        timer_entry_t* next = timer->next;
        timer->next = NULL;
        timer->prev = NULL;
        insert_timer(wheel, timer);
        timer = next;
    }
    
    wheel->stats.total_cascades++;
}

timer_wheel_t* timer_wheel_create(void) {
    timer_wheel_t* wheel = calloc(1, sizeof(timer_wheel_t));
    if (!wheel) {
        return NULL;
    }
    
    // Initialize levels
    wheel->levels[0].tick_ms = TIMER_WHEEL_LEVEL0_TICK_MS;
    wheel->levels[1].tick_ms = TIMER_WHEEL_LEVEL1_TICK_MS;
    wheel->levels[2].tick_ms = TIMER_WHEEL_LEVEL2_TICK_MS;
    wheel->levels[3].tick_ms = TIMER_WHEEL_LEVEL3_TICK_MS;
    
    wheel->current_time_ms = 0;
    wheel->next_timer_id = 1;
    wheel->initialized = true;
    
    return wheel;
}

void timer_wheel_destroy(timer_wheel_t* wheel) {
    if (!wheel) {
        return;
    }
    
    // Free all timers
    for (int level = 0; level < TIMER_WHEEL_LEVELS; level++) {
        for (uint32_t slot = 0; slot < TIMER_WHEEL_SLOTS; slot++) {
            timer_entry_t* timer = wheel->levels[level].slots[slot];
            while (timer) {
                timer_entry_t* next = timer->next;
                free(timer);
                timer = next;
            }
        }
    }
    
    free(wheel);
}

timer_id_t timer_wheel_add(timer_wheel_t* wheel, uint64_t delay_ms,
                            timer_callback_t callback, void* arg) {
    if (!wheel || !callback) {
        return TIMER_ID_INVALID;
    }
    
    timer_entry_t* timer = malloc(sizeof(timer_entry_t));
    if (!timer) {
        return TIMER_ID_INVALID;
    }
    
    timer->id = wheel->next_timer_id++;
    timer->expires_at = wheel->current_time_ms + delay_ms;
    timer->callback = callback;
    timer->arg = arg;
    timer->next = NULL;
    timer->prev = NULL;
    
    insert_timer(wheel, timer);
    
    wheel->stats.total_timers_added++;
    wheel->stats.current_timers++;
    
    if (wheel->stats.current_timers > wheel->stats.peak_timers) {
        wheel->stats.peak_timers = wheel->stats.current_timers;
    }
    
    return timer->id;
}

int timer_wheel_cancel(timer_wheel_t* wheel, timer_id_t id) {
    if (!wheel || id == TIMER_ID_INVALID) {
        return -1;
    }
    
    // Search all levels and slots
    for (int level = 0; level < TIMER_WHEEL_LEVELS; level++) {
        for (uint32_t slot = 0; slot < TIMER_WHEEL_SLOTS; slot++) {
            timer_entry_t* timer = wheel->levels[level].slots[slot];
            
            while (timer) {
                if (timer->id == id) {
                    remove_timer(wheel, timer, level, slot);
                    free(timer);
                    
                    wheel->stats.total_timers_cancelled++;
                    wheel->stats.current_timers--;
                    
                    return 0;
                }
                timer = timer->next;
            }
        }
    }
    
    return -1;
}

uint32_t timer_wheel_tick(timer_wheel_t* wheel) {
    if (!wheel) {
        return 0;
    }
    
    uint32_t expired_count = 0;
    
    // Process level 0 (finest granularity)
    uint32_t slot = wheel->levels[0].current_slot;
    timer_entry_t* timer = wheel->levels[0].slots[slot];
    wheel->levels[0].slots[slot] = NULL;
    
    while (timer) {
        timer_entry_t* next = timer->next;
        
        // Execute callback
        if (timer->callback) {
            timer->callback(timer->arg);
        }
        
        free(timer);
        expired_count++;
        
        wheel->stats.total_timers_expired++;
        wheel->stats.current_timers--;
        
        timer = next;
    }
    
    // Advance level 0
    wheel->levels[0].current_slot = (wheel->levels[0].current_slot + 1) % TIMER_WHEEL_SLOTS;
    wheel->current_time_ms += TIMER_WHEEL_LEVEL0_TICK_MS;
    
    // Check if we need to cascade
    if (wheel->levels[0].current_slot == 0) {
        // Cascade level 1
        cascade_level(wheel, 1);
        wheel->levels[1].current_slot = (wheel->levels[1].current_slot + 1) % TIMER_WHEEL_SLOTS;
        
        if (wheel->levels[1].current_slot == 0) {
            // Cascade level 2
            cascade_level(wheel, 2);
            wheel->levels[2].current_slot = (wheel->levels[2].current_slot + 1) % TIMER_WHEEL_SLOTS;
            
            if (wheel->levels[2].current_slot == 0) {
                // Cascade level 3
                cascade_level(wheel, 3);
                wheel->levels[3].current_slot = (wheel->levels[3].current_slot + 1) % TIMER_WHEEL_SLOTS;
            }
        }
    }
    
    wheel->stats.total_ticks++;
    
    return expired_count;
}

uint32_t timer_wheel_advance_to(timer_wheel_t* wheel, uint64_t time_ms) {
    if (!wheel || time_ms <= wheel->current_time_ms) {
        return 0;
    }
    
    uint32_t total_expired = 0;
    
    while (wheel->current_time_ms < time_ms) {
        total_expired += timer_wheel_tick(wheel);
    }
    
    return total_expired;
}

uint64_t timer_wheel_next_expiration(timer_wheel_t* wheel) {
    if (!wheel) {
        return UINT64_MAX;
    }
    
    // Find earliest timer in level 0
    for (uint32_t i = 0; i < TIMER_WHEEL_SLOTS; i++) {
        uint32_t slot = (wheel->levels[0].current_slot + i) % TIMER_WHEEL_SLOTS;
        if (wheel->levels[0].slots[slot]) {
            return wheel->current_time_ms + i * TIMER_WHEEL_LEVEL0_TICK_MS;
        }
    }
    
    // No timers in level 0, check higher levels
    // (simplified - would need to cascade to get exact time)
    return UINT64_MAX;
}

int timer_wheel_get_stats(timer_wheel_t* wheel, timer_wheel_stats_t* stats) {
    if (!wheel || !stats) {
        return -1;
    }
    
    *stats = wheel->stats;
    return 0;
}

void timer_wheel_reset_stats(timer_wheel_t* wheel) {
    if (!wheel) {
        return;
    }
    
    memset(&wheel->stats, 0, sizeof(wheel->stats));
}

void timer_wheel_print_stats(timer_wheel_t* wheel) {
    if (!wheel) {
        return;
    }
    
    printf("Timer Wheel Statistics:\n");
    printf("  Total Timers Added: %lu\n", wheel->stats.total_timers_added);
    printf("  Total Timers Cancelled: %lu\n", wheel->stats.total_timers_cancelled);
    printf("  Total Timers Expired: %lu\n", wheel->stats.total_timers_expired);
    printf("  Current Timers: %u\n", wheel->stats.current_timers);
    printf("  Peak Timers: %u\n", wheel->stats.peak_timers);
    printf("  Total Ticks: %lu\n", wheel->stats.total_ticks);
    printf("  Total Cascades: %lu\n", wheel->stats.total_cascades);
    printf("  Current Time: %lu ms\n", wheel->current_time_ms);
}
