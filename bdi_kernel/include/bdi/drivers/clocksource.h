
#ifndef BDI_CLOCKSOURCE_H
#define BDI_CLOCKSOURCE_H

#include <stdint.h>
#include <stdbool.h>

/* Clock source types */
typedef enum {
    CLOCKSOURCE_HPET,
    CLOCKSOURCE_TSC,
    CLOCKSOURCE_PIT,
    CLOCKSOURCE_RTC,
    CLOCKSOURCE_ACPI_PM,
    CLOCKSOURCE_MAX
} clocksource_type_t;

/* Clock source structure */
typedef struct clocksource {
    const char *name;
    clocksource_type_t type;
    uint64_t (*read)(void);
    uint64_t frequency;
    uint32_t rating;  /* Higher is better */
    uint32_t mult;    /* Multiplier for ns conversion */
    uint32_t shift;   /* Shift for ns conversion */
    bool is_continuous;
    bool is_monotonic;
    struct clocksource *next;
} clocksource_t;

/* Clock event device structure */
typedef struct clock_event_device {
    const char *name;
    uint32_t features;
    uint64_t min_delta_ns;
    uint64_t max_delta_ns;
    uint32_t rating;
    int (*set_next_event)(uint64_t delta_ns, struct clock_event_device *dev);
    int (*set_periodic)(struct clock_event_device *dev);
    int (*set_oneshot)(struct clock_event_device *dev);
    void (*event_handler)(struct clock_event_device *dev);
    void *private_data;
} clock_event_device_t;

/* Clock event features */
#define CLOCK_EVT_FEAT_PERIODIC     (1 << 0)
#define CLOCK_EVT_FEAT_ONESHOT      (1 << 1)
#define CLOCK_EVT_FEAT_KTIME        (1 << 2)

/* Clocksource API */
int clocksource_init(void);
int clocksource_register(clocksource_t *cs);
int clocksource_unregister(clocksource_t *cs);
clocksource_t *clocksource_get_best(void);
uint64_t clocksource_read(void);
uint64_t clocksource_read_ns(void);

/* Clock event API */
int clockevent_register(clock_event_device_t *dev);
int clockevent_set_next_event(uint64_t delta_ns);
int clockevent_set_periodic(uint64_t period_ns);
int clockevent_set_oneshot(void);

/* Time conversion utilities */
uint64_t cycles_to_ns(uint64_t cycles, uint32_t mult, uint32_t shift);
uint64_t ns_to_cycles(uint64_t ns, uint32_t mult, uint32_t shift);
void clocks_calc_mult_shift(uint32_t *mult, uint32_t *shift,
                            uint64_t from, uint64_t to, uint64_t maxsec);

/* Monotonic clock */
uint64_t monotonic_clock_ns(void);
uint64_t monotonic_clock_us(void);
uint64_t monotonic_clock_ms(void);

/* Wall clock time */
time_t wall_clock_time(void);
int wall_clock_set_time(time_t timestamp);

#endif /* BDI_CLOCKSOURCE_H */
