
#include "../../include/bdi/drivers/rtc.h"
#include <string.h>
#include <stdio.h>

/* I/O port access functions (architecture-specific) */
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t value);

static rtc_device_t rtc_device;
static bool rtc_initialized = false;

/* Read RTC register */
static uint8_t rtc_read_reg(uint8_t reg) {
    outb(RTC_INDEX_PORT, reg);
    return inb(RTC_DATA_PORT);
}

/* Write RTC register */
static void rtc_write_reg(uint8_t reg, uint8_t value) {
    outb(RTC_INDEX_PORT, reg);
    outb(RTC_DATA_PORT, value);
}

/* Wait for RTC update to complete */
static void rtc_wait_update(void) {
    while (rtc_read_reg(RTC_STATUS_A) & RTC_UIP) {
        /* Wait */
    }
}

/* Convert BCD to binary */
uint8_t rtc_bcd_to_binary(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* Convert binary to BCD */
uint8_t rtc_binary_to_bcd(uint8_t binary) {
    return ((binary / 10) << 4) | (binary % 10);
}

/* Initialize RTC */
int rtc_init(void) {
    if (rtc_initialized) {
        return 0;
    }
    
    /* Read status register B to determine mode */
    uint8_t status_b = rtc_read_reg(RTC_STATUS_B);
    
    rtc_device.binary_mode = (status_b & RTC_DM) != 0;
    rtc_device.hour_24_mode = (status_b & RTC_24H) != 0;
    rtc_device.century_register = RTC_CENTURY;
    
    /* Disable all interrupts initially */
    status_b &= ~(RTC_PIE | RTC_AIE | RTC_UIE);
    rtc_write_reg(RTC_STATUS_B, status_b);
    
    /* Clear any pending interrupts */
    rtc_read_reg(RTC_STATUS_C);
    
    rtc_initialized = true;
    
    printf("RTC: Initialized - %s mode, %s hour format\n",
           rtc_device.binary_mode ? "binary" : "BCD",
           rtc_device.hour_24_mode ? "24-hour" : "12-hour");
    
    return 0;
}

/* Read current time from RTC */
int rtc_read_time(rtc_time_t *time) {
    if (!rtc_initialized || !time) {
        return -1;
    }
    
    /* Wait for update to complete */
    rtc_wait_update();
    
    /* Read time values */
    time->second = rtc_read_reg(RTC_SECONDS);
    time->minute = rtc_read_reg(RTC_MINUTES);
    time->hour = rtc_read_reg(RTC_HOURS);
    time->day = rtc_read_reg(RTC_DAY);
    time->month = rtc_read_reg(RTC_MONTH);
    time->year = rtc_read_reg(RTC_YEAR);
    time->weekday = rtc_read_reg(RTC_WEEKDAY);
    
    /* Read century if available */
    uint8_t century = rtc_read_reg(rtc_device.century_register);
    
    /* Convert from BCD if necessary */
    if (!rtc_device.binary_mode) {
        time->second = rtc_bcd_to_binary(time->second);
        time->minute = rtc_bcd_to_binary(time->minute);
        time->hour = rtc_bcd_to_binary(time->hour);
        time->day = rtc_bcd_to_binary(time->day);
        time->month = rtc_bcd_to_binary(time->month);
        time->year = rtc_bcd_to_binary(time->year);
        century = rtc_bcd_to_binary(century);
    }
    
    /* Calculate full year */
    time->year += century * 100;
    
    return 0;
}

/* Write time to RTC */
int rtc_write_time(const rtc_time_t *time) {
    if (!rtc_initialized || !time) {
        return -1;
    }
    
    uint8_t second = time->second;
    uint8_t minute = time->minute;
    uint8_t hour = time->hour;
    uint8_t day = time->day;
    uint8_t month = time->month;
    uint8_t year = time->year % 100;
    uint8_t century = time->year / 100;
    
    /* Convert to BCD if necessary */
    if (!rtc_device.binary_mode) {
        second = rtc_binary_to_bcd(second);
        minute = rtc_binary_to_bcd(minute);
        hour = rtc_binary_to_bcd(hour);
        day = rtc_binary_to_bcd(day);
        month = rtc_binary_to_bcd(month);
        year = rtc_binary_to_bcd(year);
        century = rtc_binary_to_bcd(century);
    }
    
    /* Disable updates */
    uint8_t status_b = rtc_read_reg(RTC_STATUS_B);
    rtc_write_reg(RTC_STATUS_B, status_b | RTC_SET);
    
    /* Write time values */
    rtc_write_reg(RTC_SECONDS, second);
    rtc_write_reg(RTC_MINUTES, minute);
    rtc_write_reg(RTC_HOURS, hour);
    rtc_write_reg(RTC_DAY, day);
    rtc_write_reg(RTC_MONTH, month);
    rtc_write_reg(RTC_YEAR, year);
    rtc_write_reg(rtc_device.century_register, century);
    
    /* Re-enable updates */
    rtc_write_reg(RTC_STATUS_B, status_b);
    
    return 0;
}

/* Get timestamp */
time_t rtc_get_timestamp(void) {
    rtc_time_t time;
    struct tm tm;
    
    if (rtc_read_time(&time) < 0) {
        return -1;
    }
    
    tm.tm_sec = time.second;
    tm.tm_min = time.minute;
    tm.tm_hour = time.hour;
    tm.tm_mday = time.day;
    tm.tm_mon = time.month - 1;
    tm.tm_year = time.year - 1900;
    
    return mktime(&tm);
}

/* Set timestamp */
int rtc_set_timestamp(time_t timestamp) {
    struct tm *tm = gmtime(&timestamp);
    rtc_time_t time;
    
    if (!tm) {
        return -1;
    }
    
    time.second = tm->tm_sec;
    time.minute = tm->tm_min;
    time.hour = tm->tm_hour;
    time.day = tm->tm_mday;
    time.month = tm->tm_mon + 1;
    time.year = tm->tm_year + 1900;
    time.weekday = tm->tm_wday;
    
    return rtc_write_time(&time);
}

/* Set periodic interrupt rate */
int rtc_set_periodic_rate(uint8_t rate) {
    if (!rtc_initialized || rate > 15) {
        return -1;
    }
    
    /* Disable interrupts */
    uint8_t status_b = rtc_read_reg(RTC_STATUS_B);
    rtc_write_reg(RTC_STATUS_B, status_b & ~RTC_PIE);
    
    /* Set rate */
    uint8_t status_a = rtc_read_reg(RTC_STATUS_A);
    status_a = (status_a & 0xF0) | (rate & 0x0F);
    rtc_write_reg(RTC_STATUS_A, status_a);
    
    rtc_device.periodic_rate = rate;
    
    return 0;
}

/* Enable periodic interrupt */
int rtc_enable_periodic_interrupt(void (*callback)(void *), void *data) {
    if (!rtc_initialized) {
        return -1;
    }
    
    rtc_device.periodic_callback = callback;
    rtc_device.periodic_callback_data = data;
    
    /* Enable periodic interrupt */
    uint8_t status_b = rtc_read_reg(RTC_STATUS_B);
    status_b |= RTC_PIE;
    rtc_write_reg(RTC_STATUS_B, status_b);
    
    return 0;
}

/* Disable periodic interrupt */
int rtc_disable_periodic_interrupt(void) {
    if (!rtc_initialized) {
        return -1;
    }
    
    /* Disable periodic interrupt */
    uint8_t status_b = rtc_read_reg(RTC_STATUS_B);
    status_b &= ~RTC_PIE;
    rtc_write_reg(RTC_STATUS_B, status_b);
    
    rtc_device.periodic_callback = NULL;
    rtc_device.periodic_callback_data = NULL;
    
    return 0;
}

/* RTC interrupt handler */
void rtc_interrupt_handler(void) {
    if (!rtc_initialized) {
        return;
    }
    
    /* Read status register C to clear interrupt */
    uint8_t status_c = rtc_read_reg(RTC_STATUS_C);
    
    /* Handle periodic interrupt */
    if ((status_c & RTC_PF) && rtc_device.periodic_callback) {
        rtc_device.periodic_callback(rtc_device.periodic_callback_data);
    }
    
    /* Handle alarm interrupt */
    if ((status_c & RTC_AF) && rtc_device.alarm.enabled && rtc_device.alarm.callback) {
        rtc_device.alarm.callback(rtc_device.alarm.callback_data);
    }
}
