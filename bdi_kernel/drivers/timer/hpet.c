
#include <stdlib.h>       // For calloc/free
#include "../../include/bdi/drivers/hpet.h"
#include <string.h>
#include <stdio.h>

/* HPET MMIO base address (typically from ACPI) */
#define HPET_DEFAULT_BASE   0xFED00000

static hpet_device_t hpet_device;
static bool hpet_initialized = false;

/* Helper functions for MMIO access */
static inline uint64_t hpet_read64(volatile void *addr) {
    return *(volatile uint64_t *)addr;
}

static inline void hpet_write64(volatile void *addr, uint64_t value) {
    *(volatile uint64_t *)addr = value;
}

static inline uint32_t hpet_read32(volatile void *addr) {
    return *(volatile uint32_t *)addr;
}

static inline void hpet_write32(volatile void *addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

/* Read HPET register */
static uint64_t hpet_read_reg(uint32_t offset) {
    return hpet_read64((volatile uint8_t *)hpet_device.base_address + offset);
}

/* Write HPET register */
static void hpet_write_reg(uint32_t offset, uint64_t value) {
    hpet_write64((volatile uint8_t *)hpet_device.base_address + offset, value);
}

/* Initialize HPET device */
int hpet_init(void) {
    uint64_t caps;
    uint32_t period;
    
    if (hpet_initialized) {
        return 0;
    }
    
    /* Map HPET MMIO region */
    hpet_device.base_address = (volatile void *)HPET_DEFAULT_BASE;
    
    /* Read capabilities */
    caps = hpet_read_reg(HPET_GENERAL_CAPS_ID);
    
    /* Extract capability information */
    hpet_device.revision = caps & HPET_CAP_REV_ID_MASK;
    hpet_device.num_timers = ((caps & HPET_CAP_NUM_TIM_MASK) >> HPET_CAP_NUM_TIM_SHIFT) + 1;
    hpet_device.is_64bit = (caps & HPET_CAP_COUNT_SIZE) != 0;
    hpet_device.legacy_capable = (caps & HPET_CAP_LEG_RT_CAP) != 0;
    hpet_device.vendor_id = (caps >> HPET_CAP_VENDOR_ID_SHIFT) & 0xFFFF;
    
    /* Get counter period in femtoseconds */
    period = (caps >> HPET_CAP_COUNTER_CLK_SHIFT);
    hpet_device.period_fs = period;
    
    /* Calculate frequency (Hz) = 10^15 / period_fs */
    hpet_device.frequency = 1000000000000000ULL / period;
    
    /* Allocate timer structures */
    hpet_device.timers = calloc(hpet_device.num_timers, sizeof(hpet_timer_t));
    if (!hpet_device.timers) {
        return -1;
    }
    
    /* Initialize timer structures */
    for (uint32_t i = 0; i < hpet_device.num_timers; i++) {
        hpet_device.timers[i].id = i;
        hpet_device.timers[i].enabled = false;
        hpet_device.timers[i].periodic = false;
        
        /* Read timer capabilities */
        uint32_t timer_offset = HPET_TIMER_CONFIG_BASE + (i * HPET_TIMER_STRIDE);
        hpet_device.timers[i].config = hpet_read_reg(timer_offset);
    }
    
    /* Disable HPET initially */
    hpet_write_reg(HPET_GENERAL_CONFIG, 0);
    
    /* Clear main counter */
    hpet_write_reg(HPET_MAIN_COUNTER, 0);
    
    hpet_initialized = true;
    
    printf("HPET: Initialized - %u timers, %llu Hz, %s\n",
           hpet_device.num_timers,
           hpet_device.frequency,
           hpet_device.is_64bit ? "64-bit" : "32-bit");
    
    return 0;
}

/* Enable HPET */
int hpet_enable(void) {
    if (!hpet_initialized) {
        return -1;
    }
    
    uint64_t config = hpet_read_reg(HPET_GENERAL_CONFIG);
    config |= HPET_CFG_ENABLE;
    hpet_write_reg(HPET_GENERAL_CONFIG, config);
    
    return 0;
}

/* Disable HPET */
int hpet_disable(void) {
    if (!hpet_initialized) {
        return -1;
    }
    
    uint64_t config = hpet_read_reg(HPET_GENERAL_CONFIG);
    config &= ~HPET_CFG_ENABLE;
    hpet_write_reg(HPET_GENERAL_CONFIG, config);
    
    return 0;
}

/* Read main counter */
uint64_t hpet_read_counter(void) {
    if (!hpet_initialized) {
        return 0;
    }
    
    return hpet_read_reg(HPET_MAIN_COUNTER);
}

/* Write main counter */
void hpet_write_counter(uint64_t value) {
    if (!hpet_initialized) {
        return;
    }
    
    /* Disable HPET before writing counter */
    hpet_disable();
    hpet_write_reg(HPET_MAIN_COUNTER, value);
    hpet_enable();
}

/* Setup timer */
int hpet_timer_setup(uint32_t timer_id, uint64_t period_ns, bool periodic,
                     void (*callback)(void *), void *data) {
    if (!hpet_initialized || timer_id >= hpet_device.num_timers) {
        return -1;
    }
    
    hpet_timer_t *timer = &hpet_device.timers[timer_id];
    uint32_t config_offset = HPET_TIMER_CONFIG_BASE + (timer_id * HPET_TIMER_STRIDE);
    uint32_t comp_offset = HPET_TIMER_COMPARATOR_BASE + (timer_id * HPET_TIMER_STRIDE);
    
    /* Disable timer */
    uint64_t config = hpet_read_reg(config_offset);
    config &= ~HPET_TN_INT_ENB;
    hpet_write_reg(config_offset, config);
    
    /* Configure timer mode */
    if (periodic) {
        /* Check if timer supports periodic mode */
        if (!(config & HPET_TN_PER_INT_CAP)) {
            return -1;
        }
        
        config |= HPET_TN_TYPE_PERIODIC;
        config |= HPET_TN_VAL_SET;
        timer->periodic = true;
    } else {
        config &= ~HPET_TN_TYPE_PERIODIC;
        timer->periodic = false;
    }
    
    /* Set interrupt type to level-triggered */
    config |= HPET_TN_INT_TYPE_LEVEL;
    
    /* Write configuration */
    hpet_write_reg(config_offset, config);
    
    /* Calculate comparator value */
    uint64_t ticks = hpet_ns_to_ticks(period_ns);
    uint64_t current = hpet_read_counter();
    uint64_t comparator = current + ticks;
    
    /* Write comparator value */
    hpet_write_reg(comp_offset, comparator);
    
    /* For periodic mode, write period again */
    if (periodic) {
        hpet_write_reg(comp_offset, ticks);
    }
    
    /* Store callback */
    timer->callback = callback;
    timer->callback_data = data;
    timer->config = config;
    timer->comparator = comparator;
    
    return 0;
}

/* Enable timer */
int hpet_timer_enable(uint32_t timer_id) {
    if (!hpet_initialized || timer_id >= hpet_device.num_timers) {
        return -1;
    }
    
    hpet_timer_t *timer = &hpet_device.timers[timer_id];
    uint32_t config_offset = HPET_TIMER_CONFIG_BASE + (timer_id * HPET_TIMER_STRIDE);
    
    uint64_t config = hpet_read_reg(config_offset);
    config |= HPET_TN_INT_ENB;
    hpet_write_reg(config_offset, config);
    
    timer->enabled = true;
    
    return 0;
}

/* Disable timer */
int hpet_timer_disable(uint32_t timer_id) {
    if (!hpet_initialized || timer_id >= hpet_device.num_timers) {
        return -1;
    }
    
    hpet_timer_t *timer = &hpet_device.timers[timer_id];
    uint32_t config_offset = HPET_TIMER_CONFIG_BASE + (timer_id * HPET_TIMER_STRIDE);
    
    uint64_t config = hpet_read_reg(config_offset);
    config &= ~HPET_TN_INT_ENB;
    hpet_write_reg(config_offset, config);
    
    timer->enabled = false;
    
    return 0;
}

/* Set timer IRQ */
int hpet_timer_set_irq(uint32_t timer_id, uint32_t irq) {
    if (!hpet_initialized || timer_id >= hpet_device.num_timers) {
        return -1;
    }
    
    hpet_timer_t *timer = &hpet_device.timers[timer_id];
    uint32_t config_offset = HPET_TIMER_CONFIG_BASE + (timer_id * HPET_TIMER_STRIDE);
    
    uint64_t config = hpet_read_reg(config_offset);
    
    /* Check if IRQ is supported */
    uint64_t route_cap = config >> HPET_TN_INT_ROUTE_CAP_SHIFT;
    if (!(route_cap & (1ULL << irq))) {
        return -1;
    }
    
    /* Set IRQ routing */
    config &= ~HPET_TN_INT_ROUTE_MASK;
    config |= (irq << HPET_TN_INT_ROUTE_SHIFT) & HPET_TN_INT_ROUTE_MASK;
    
    hpet_write_reg(config_offset, config);
    
    timer->irq = irq;
    
    return 0;
}

/* Get HPET frequency */
uint64_t hpet_get_frequency(void) {
    return hpet_device.frequency;
}

/* Convert nanoseconds to HPET ticks */
uint64_t hpet_ns_to_ticks(uint64_t ns) {
    /* ticks = (ns * frequency) / 1000000000 */
    return (ns * hpet_device.frequency) / 1000000000ULL;
}

/* Convert HPET ticks to nanoseconds */
uint64_t hpet_ticks_to_ns(uint64_t ticks) {
    /* ns = (ticks * 1000000000) / frequency */
    return (ticks * 1000000000ULL) / hpet_device.frequency;
}

/* HPET interrupt handler */
void hpet_interrupt_handler(uint32_t timer_id) {
    if (!hpet_initialized || timer_id >= hpet_device.num_timers) {
        return;
    }
    
    hpet_timer_t *timer = &hpet_device.timers[timer_id];
    
    /* Clear interrupt status */
    uint32_t status_offset = HPET_GENERAL_INT_STATUS;
    uint64_t status = hpet_read_reg(status_offset);
    status |= (1ULL << timer_id);
    hpet_write_reg(status_offset, status);
    
    /* Call callback if registered */
    if (timer->callback) {
        timer->callback(timer->callback_data);
    }
}
