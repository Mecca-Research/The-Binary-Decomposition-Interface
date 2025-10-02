
/**
 * @file timer_wheel.h
 * @brief Hierarchical timer wheel for O(1) timer operations
 * 
 * 4-level hierarchical timer wheel supporting wide time ranges.
 * Based on Varghese & Lauck (1987) paper.
 */

#ifndef PHASE2_TIMER_WHEEL_H
#define PHASE2_TIMER_WHEEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Timer wheel configuration
#define TIMER_WHEEL_LEVELS 4
#define TIMER_WHEEL_SLOTS 256
#define TIMER_WHEEL_LEVEL0_TICK_MS 1      // 1ms per tick
#define TIMER_WHEEL_LEVEL1_TICK_MS 256    // 256ms per tick
#define TIMER_WHEEL_LEVEL2_TICK_MS 65536  // ~64s per tick
#define TIMER_WHEEL_LEVEL3_TICK_MS 16777216  // ~4.5h per tick

// Timer ID type
typedef uint64_t timer_id_t;
#define TIMER_ID_INVALID 0

// Timer callback
typedef void (*timer_callback_t)(void* arg);

/**
 * @brief Timer entry
 */
typedef struct timer_entry {
    timer_id_t id;
    uint64_t expires_at;        // Absolute expiration time (ms)
    timer_callback_t callback;
    void* arg;
    struct timer_entry* next;
    struct timer_entry* prev;
} timer_entry_t;

/**
 * @brief Timer wheel level
 */
typedef struct {
    timer_entry_t* slots[TIMER_WHEEL_SLOTS];
    uint32_t current_slot;
    uint64_t tick_ms;           // Tick duration in ms
} timer_wheel_level_t;

/**
 * @brief Timer wheel statistics
 */
typedef struct {
    uint64_t total_timers_added;
    uint64_t total_timers_cancelled;
    uint64_t total_timers_expired;
    uint64_t total_ticks;
    uint64_t total_cascades;
    uint32_t current_timers;
    uint32_t peak_timers;
} timer_wheel_stats_t;

/**
 * @brief Timer wheel structure
 */
typedef struct {
    timer_wheel_level_t levels[TIMER_WHEEL_LEVELS];
    uint64_t current_time_ms;
    timer_id_t next_timer_id;
    timer_wheel_stats_t stats;
    bool initialized;
} timer_wheel_t;

/**
 * @brief Create timer wheel
 * 
 * @return Pointer to timer wheel, or NULL on failure
 */
timer_wheel_t* timer_wheel_create(void);

/**
 * @brief Destroy timer wheel
 * 
 * @param wheel Timer wheel
 */
void timer_wheel_destroy(timer_wheel_t* wheel);

/**
 * @brief Add timer
 * 
 * @param wheel Timer wheel
 * @param delay_ms Delay in milliseconds
 * @param callback Callback function
 * @param arg Callback argument
 * @return Timer ID, or TIMER_ID_INVALID on failure
 */
timer_id_t timer_wheel_add(timer_wheel_t* wheel, uint64_t delay_ms,
                            timer_callback_t callback, void* arg);

/**
 * @brief Cancel timer
 * 
 * @param wheel Timer wheel
 * @param id Timer ID
 * @return 0 on success, -1 if not found
 */
int timer_wheel_cancel(timer_wheel_t* wheel, timer_id_t id);

/**
 * @brief Advance timer wheel by one tick
 * 
 * Processes expired timers and cascades higher levels.
 * 
 * @param wheel Timer wheel
 * @return Number of timers expired
 */
uint32_t timer_wheel_tick(timer_wheel_t* wheel);

/**
 * @brief Advance timer wheel to specific time
 * 
 * @param wheel Timer wheel
 * @param time_ms Target time in milliseconds
 * @return Number of timers expired
 */
uint32_t timer_wheel_advance_to(timer_wheel_t* wheel, uint64_t time_ms);

/**
 * @brief Get next timer expiration time
 * 
 * @param wheel Timer wheel
 * @return Next expiration time (ms), or UINT64_MAX if no timers
 */
uint64_t timer_wheel_next_expiration(timer_wheel_t* wheel);

/**
 * @brief Get timer wheel statistics
 * 
 * @param wheel Timer wheel
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int timer_wheel_get_stats(timer_wheel_t* wheel, timer_wheel_stats_t* stats);

/**
 * @brief Reset timer wheel statistics
 * 
 * @param wheel Timer wheel
 */
void timer_wheel_reset_stats(timer_wheel_t* wheel);

/**
 * @brief Print timer wheel statistics
 * 
 * @param wheel Timer wheel
 */
void timer_wheel_print_stats(timer_wheel_t* wheel);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_TIMER_WHEEL_H
