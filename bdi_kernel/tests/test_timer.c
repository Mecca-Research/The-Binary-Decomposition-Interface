
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "../include/bdi/drivers/hpet.h"
#include "../include/bdi/drivers/rtc.h"
#include "../include/bdi/drivers/clocksource.h"

static bool timer_fired = false;
static uint32_t timer_count = 0;

void test_timer_callback(void *data) {
    timer_fired = true;
    timer_count++;
}

void test_hpet_init(void) {
    printf("Testing HPET initialization...\n");
    
    int ret = hpet_init();
    assert(ret == 0 && "HPET initialization failed");
    
    uint64_t freq = hpet_get_frequency();
    assert(freq > 0 && "HPET frequency is zero");
    
    printf("  ✓ HPET initialized: %llu Hz\n", freq);
}

void test_hpet_counter(void) {
    printf("Testing HPET counter...\n");
    
    hpet_enable();
    
    uint64_t start = hpet_read_counter();
    usleep(1000);  // 1ms
    uint64_t end = hpet_read_counter();
    
    assert(end > start && "HPET counter not incrementing");
    
    uint64_t elapsed_ns = hpet_ticks_to_ns(end - start);
    assert(elapsed_ns >= 900000 && elapsed_ns <= 1100000 && 
           "HPET timing inaccurate");
    
    printf("  ✓ HPET counter working: %llu ns elapsed\n", elapsed_ns);
}

void test_hpet_timer(void) {
    printf("Testing HPET timer...\n");
    
    timer_fired = false;
    timer_count = 0;
    
    // Setup 10ms periodic timer
    int ret = hpet_timer_setup(0, 10000000, true, test_timer_callback, NULL);
    assert(ret == 0 && "HPET timer setup failed");
    
    ret = hpet_timer_enable(0);
    assert(ret == 0 && "HPET timer enable failed");
    
    // Wait for timer to fire
    usleep(50000);  // 50ms
    
    assert(timer_fired && "HPET timer did not fire");
    assert(timer_count >= 4 && timer_count <= 6 && 
           "HPET timer count incorrect");
    
    hpet_timer_disable(0);
    
    printf("  ✓ HPET timer working: %u callbacks\n", timer_count);
}

void test_rtc_init(void) {
    printf("Testing RTC initialization...\n");
    
    int ret = rtc_init();
    assert(ret == 0 && "RTC initialization failed");
    
    printf("  ✓ RTC initialized\n");
}

void test_rtc_time(void) {
    printf("Testing RTC time...\n");
    
    rtc_time_t time;
    int ret = rtc_read_time(&time);
    assert(ret == 0 && "RTC read time failed");
    
    assert(time.year >= 2024 && time.year <= 2100 && "RTC year invalid");
    assert(time.month >= 1 && time.month <= 12 && "RTC month invalid");
    assert(time.day >= 1 && time.day <= 31 && "RTC day invalid");
    assert(time.hour <= 23 && "RTC hour invalid");
    assert(time.minute <= 59 && "RTC minute invalid");
    assert(time.second <= 59 && "RTC second invalid");
    
    printf("  ✓ RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
           time.year, time.month, time.day,
           time.hour, time.minute, time.second);
}

void test_clocksource_init(void) {
    printf("Testing clocksource initialization...\n");
    
    int ret = clocksource_init();
    assert(ret == 0 && "Clocksource initialization failed");
    
    clocksource_t *cs = clocksource_get_best();
    assert(cs != NULL && "No clocksource available");
    
    printf("  ✓ Clocksource: %s (%llu Hz)\n", cs->name, cs->frequency);
}

void test_monotonic_clock(void) {
    printf("Testing monotonic clock...\n");
    
    uint64_t start = monotonic_clock_ns();
    usleep(1000);  // 1ms
    uint64_t end = monotonic_clock_ns();
    
    assert(end > start && "Monotonic clock not incrementing");
    
    uint64_t elapsed = end - start;
    assert(elapsed >= 900000 && elapsed <= 1100000 && 
           "Monotonic clock timing inaccurate");
    
    printf("  ✓ Monotonic clock: %llu ns elapsed\n", elapsed);
}

void test_wall_clock(void) {
    printf("Testing wall clock...\n");
    
    time_t now = wall_clock_time();
    assert(now > 0 && "Wall clock time invalid");
    
    // Should be after 2024-01-01
    assert(now > 1704067200 && "Wall clock time too old");
    
    printf("  ✓ Wall clock: %ld\n", now);
}

int main(void) {
    printf("=== Timer Subsystem Tests ===\n\n");
    
    test_hpet_init();
    test_hpet_counter();
    test_hpet_timer();
    
    test_rtc_init();
    test_rtc_time();
    
    test_clocksource_init();
    test_monotonic_clock();
    test_wall_clock();
    
    printf("\n=== All Timer Tests Passed ===\n");
    
    return 0;
}
