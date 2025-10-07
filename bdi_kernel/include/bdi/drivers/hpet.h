
#ifndef BDI_HPET_H
#define BDI_HPET_H

#include <stdint.h>
#include <stdbool.h>

/* HPET Register Offsets */
#define HPET_GENERAL_CAPS_ID        0x000
#define HPET_GENERAL_CONFIG         0x010
#define HPET_GENERAL_INT_STATUS     0x020
#define HPET_MAIN_COUNTER           0x0F0
#define HPET_TIMER_CONFIG_BASE      0x100
#define HPET_TIMER_COMPARATOR_BASE  0x108
#define HPET_TIMER_FSB_ROUTE_BASE   0x110

/* Timer register stride */
#define HPET_TIMER_STRIDE           0x20

/* General Capabilities and ID Register bits */
#define HPET_CAP_REV_ID_MASK        0xFF
#define HPET_CAP_NUM_TIM_MASK       0x1F00
#define HPET_CAP_NUM_TIM_SHIFT      8
#define HPET_CAP_COUNT_SIZE         (1ULL << 13)
#define HPET_CAP_LEG_RT_CAP         (1ULL << 15)
#define HPET_CAP_VENDOR_ID_SHIFT    16
#define HPET_CAP_COUNTER_CLK_SHIFT  32

/* General Configuration Register bits */
#define HPET_CFG_ENABLE             (1ULL << 0)
#define HPET_CFG_LEG_RT             (1ULL << 1)

/* Timer Configuration bits */
#define HPET_TN_INT_TYPE_LEVEL      (1ULL << 1)
#define HPET_TN_INT_ENB             (1ULL << 2)
#define HPET_TN_TYPE_PERIODIC       (1ULL << 3)
#define HPET_TN_PER_INT_CAP         (1ULL << 4)
#define HPET_TN_SIZE_CAP            (1ULL << 5)
#define HPET_TN_VAL_SET             (1ULL << 6)
#define HPET_TN_32MODE              (1ULL << 8)
#define HPET_TN_INT_ROUTE_SHIFT     9
#define HPET_TN_INT_ROUTE_MASK      0x3E00
#define HPET_TN_FSB_EN              (1ULL << 14)
#define HPET_TN_FSB_INT_DEL_CAP     (1ULL << 15)
#define HPET_TN_INT_ROUTE_CAP_SHIFT 32

/* HPET Timer structure */
typedef struct hpet_timer {
    uint32_t id;
    uint64_t config;
    uint64_t comparator;
    uint64_t fsb_route;
    bool periodic;
    bool enabled;
    uint32_t irq;
    void (*callback)(void *);
    void *callback_data;
} hpet_timer_t;

/* HPET Device structure */
typedef struct hpet_device {
    volatile void *base_address;
    uint64_t frequency;
    uint32_t period_fs;  /* Period in femtoseconds */
    uint32_t num_timers;
    uint8_t revision;
    uint16_t vendor_id;
    bool is_64bit;
    bool legacy_capable;
    hpet_timer_t *timers;
} hpet_device_t;

/* HPET API */
int hpet_init(void);
int hpet_enable(void);
int hpet_disable(void);
uint64_t hpet_read_counter(void);
void hpet_write_counter(uint64_t value);

/* Timer management */
int hpet_timer_setup(uint32_t timer_id, uint64_t period_ns, bool periodic,
                     void (*callback)(void *), void *data);
int hpet_timer_enable(uint32_t timer_id);
int hpet_timer_disable(uint32_t timer_id);
int hpet_timer_set_irq(uint32_t timer_id, uint32_t irq);

/* Clock source interface */
uint64_t hpet_get_frequency(void);
uint64_t hpet_ns_to_ticks(uint64_t ns);
uint64_t hpet_ticks_to_ns(uint64_t ticks);

/* Interrupt handler */
void hpet_interrupt_handler(uint32_t timer_id);

#endif /* BDI_HPET_H */
