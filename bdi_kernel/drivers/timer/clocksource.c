
#include "../../include/bdi/drivers/clocksource.h"
#include "../../include/bdi/drivers/hpet.h"
#include "../../include/bdi/drivers/rtc.h"
#include <string.h>
#include <stdio.h>

static clocksource_t *clocksource_list = NULL;
static clocksource_t *current_clocksource = NULL;
static clock_event_device_t *current_clockevent = NULL;
static uint64_t monotonic_offset = 0;
static time_t wall_clock_offset = 0;

/* HPET clocksource */
static uint64_t hpet_clocksource_read(void) {
    return hpet_read_counter();
}

static clocksource_t hpet_clocksource = {
    .name = "hpet",
    .type = CLOCKSOURCE_HPET,
    .read = hpet_clocksource_read,
    .rating = 300,
    .is_continuous = true,
    .is_monotonic = true,
};

/* Calculate multiplier and shift for time conversion */
void clocks_calc_mult_shift(uint32_t *mult, uint32_t *shift,
                            uint64_t from, uint64_t to, uint64_t maxsec) {
    uint64_t tmp;
    uint32_t sft, sftacc = 32;
    
    tmp = ((uint64_t)maxsec * from) >> 32;
    while (tmp) {
        tmp >>= 1;
        sftacc--;
    }
    
    for (sft = 32; sft > 0; sft--) {
        tmp = (uint64_t)to << sft;
        tmp += from / 2;
        tmp /= from;
        if ((tmp >> sftacc) == 0)
            break;
    }
    
    *mult = tmp;
    *shift = sft;
}

/* Convert cycles to nanoseconds */
uint64_t cycles_to_ns(uint64_t cycles, uint32_t mult, uint32_t shift) {
    return (cycles * mult) >> shift;
}

/* Convert nanoseconds to cycles */
uint64_t ns_to_cycles(uint64_t ns, uint32_t mult, uint32_t shift) {
    return (ns << shift) / mult;
}

/* Initialize clocksource subsystem */
int clocksource_init(void) {
    /* Initialize HPET */
    if (hpet_init() == 0) {
        hpet_clocksource.frequency = hpet_get_frequency();
        
        /* Calculate mult/shift for ns conversion */
        clocks_calc_mult_shift(&hpet_clocksource.mult, &hpet_clocksource.shift,
                              hpet_clocksource.frequency, 1000000000ULL, 600);
        
        clocksource_register(&hpet_clocksource);
        hpet_enable();
    }
    
    /* Initialize RTC */
    rtc_init();
    
    /* Set initial wall clock time from RTC */
    wall_clock_offset = rtc_get_timestamp();
    
    printf("Clocksource: Initialized with %s\n",
           current_clocksource ? current_clocksource->name : "none");
    
    return 0;
}

/* Register a clocksource */
int clocksource_register(clocksource_t *cs) {
    if (!cs) {
        return -1;
    }
    
    /* Add to list */
    cs->next = clocksource_list;
    clocksource_list = cs;
    
    /* Select if better than current */
    if (!current_clocksource || cs->rating > current_clocksource->rating) {
        current_clocksource = cs;
        printf("Clocksource: Selected %s (rating %u, freq %llu Hz)\n",
               cs->name, cs->rating, cs->frequency);
    }
    
    return 0;
}

/* Unregister a clocksource */
int clocksource_unregister(clocksource_t *cs) {
    if (!cs) {
        return -1;
    }
    
    /* Remove from list */
    clocksource_t **prev = &clocksource_list;
    while (*prev) {
        if (*prev == cs) {
            *prev = cs->next;
            break;
        }
        prev = &(*prev)->next;
    }
    
    /* Select new clocksource if this was current */
    if (current_clocksource == cs) {
        current_clocksource = NULL;
        clocksource_t *best = clocksource_list;
        for (clocksource_t *c = clocksource_list; c; c = c->next) {
            if (!best || c->rating > best->rating) {
                best = c;
            }
        }
        current_clocksource = best;
    }
    
    return 0;
}

/* Get best clocksource */
clocksource_t *clocksource_get_best(void) {
    return current_clocksource;
}

/* Read current clocksource value */
uint64_t clocksource_read(void) {
    if (!current_clocksource) {
        return 0;
    }
    
    return current_clocksource->read();
}

/* Read clocksource in nanoseconds */
uint64_t clocksource_read_ns(void) {
    if (!current_clocksource) {
        return 0;
    }
    
    uint64_t cycles = current_clocksource->read();
    return cycles_to_ns(cycles, current_clocksource->mult, current_clocksource->shift);
}

/* Register clock event device */
int clockevent_register(clock_event_device_t *dev) {
    if (!dev) {
        return -1;
    }
    
    /* Select if better than current */
    if (!current_clockevent || dev->rating > current_clockevent->rating) {
        current_clockevent = dev;
        printf("Clock event: Selected %s (rating %u)\n", dev->name, dev->rating);
    }
    
    return 0;
}

/* Set next event */
int clockevent_set_next_event(uint64_t delta_ns) {
    if (!current_clockevent || !current_clockevent->set_next_event) {
        return -1;
    }
    
    return current_clockevent->set_next_event(delta_ns, current_clockevent);
}

/* Set periodic mode */
int clockevent_set_periodic(uint64_t period_ns) {
    if (!current_clockevent || !current_clockevent->set_periodic) {
        return -1;
    }
    
    return current_clockevent->set_periodic(current_clockevent);
}

/* Set oneshot mode */
int clockevent_set_oneshot(void) {
    if (!current_clockevent || !current_clockevent->set_oneshot) {
        return -1;
    }
    
    return current_clockevent->set_oneshot(current_clockevent);
}

/* Get monotonic clock in nanoseconds */
uint64_t monotonic_clock_ns(void) {
    return clocksource_read_ns() + monotonic_offset;
}

/* Get monotonic clock in microseconds */
uint64_t monotonic_clock_us(void) {
    return monotonic_clock_ns() / 1000;
}

/* Get monotonic clock in milliseconds */
uint64_t monotonic_clock_ms(void) {
    return monotonic_clock_ns() / 1000000;
}

/* Get wall clock time */
time_t wall_clock_time(void) {
    uint64_t ns = monotonic_clock_ns();
    return wall_clock_offset + (ns / 1000000000ULL);
}

/* Set wall clock time */
int wall_clock_set_time(time_t timestamp) {
    wall_clock_offset = timestamp;
    
    /* Update RTC */
    return rtc_set_timestamp(timestamp);
}
