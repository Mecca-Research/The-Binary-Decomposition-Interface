
#ifndef BDI_RTC_H
#define BDI_RTC_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* RTC I/O Ports (CMOS/RTC) */
#define RTC_INDEX_PORT      0x70
#define RTC_DATA_PORT       0x71

/* RTC Register Indices */
#define RTC_SECONDS         0x00
#define RTC_MINUTES         0x02
#define RTC_HOURS           0x04
#define RTC_WEEKDAY         0x06
#define RTC_DAY             0x07
#define RTC_MONTH           0x08
#define RTC_YEAR            0x09
#define RTC_CENTURY         0x32

#define RTC_STATUS_A        0x0A
#define RTC_STATUS_B        0x0B
#define RTC_STATUS_C        0x0C
#define RTC_STATUS_D        0x0D

/* Status Register A bits */
#define RTC_UIP             0x80  /* Update In Progress */
#define RTC_RATE_MASK       0x0F

/* Status Register B bits */
#define RTC_SET             0x80  /* Disable updates */
#define RTC_PIE             0x40  /* Periodic Interrupt Enable */
#define RTC_AIE             0x20  /* Alarm Interrupt Enable */
#define RTC_UIE             0x10  /* Update-ended Interrupt Enable */
#define RTC_SQWE            0x08  /* Square Wave Enable */
#define RTC_DM              0x04  /* Data Mode (1=binary, 0=BCD) */
#define RTC_24H             0x02  /* 24-hour mode */
#define RTC_DSE             0x01  /* Daylight Savings Enable */

/* Status Register C bits (read-only) */
#define RTC_IRQF            0x80  /* Interrupt Request Flag */
#define RTC_PF              0x40  /* Periodic Interrupt Flag */
#define RTC_AF              0x20  /* Alarm Interrupt Flag */
#define RTC_UF              0x10  /* Update-ended Interrupt Flag */

/* Periodic interrupt rates (Hz) */
#define RTC_RATE_NONE       0x00
#define RTC_RATE_8192HZ     0x03
#define RTC_RATE_4096HZ     0x04
#define RTC_RATE_2048HZ     0x05
#define RTC_RATE_1024HZ     0x06
#define RTC_RATE_512HZ      0x07
#define RTC_RATE_256HZ      0x08
#define RTC_RATE_128HZ      0x09
#define RTC_RATE_64HZ       0x0A
#define RTC_RATE_32HZ       0x0B
#define RTC_RATE_16HZ       0x0C
#define RTC_RATE_8HZ        0x0D
#define RTC_RATE_4HZ        0x0E
#define RTC_RATE_2HZ        0x0F

/* RTC Time structure */
typedef struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday;
} rtc_time_t;

/* RTC Alarm structure */
typedef struct rtc_alarm {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    bool enabled;
    void (*callback)(void *);
    void *callback_data;
} rtc_alarm_t;

/* RTC Device structure */
typedef struct rtc_device {
    bool binary_mode;
    bool hour_24_mode;
    uint8_t century_register;
    rtc_alarm_t alarm;
    void (*periodic_callback)(void *);
    void *periodic_callback_data;
    uint32_t periodic_rate;
} rtc_device_t;

/* RTC API */
int rtc_init(void);
int rtc_read_time(rtc_time_t *time);
int rtc_write_time(const rtc_time_t *time);
time_t rtc_get_timestamp(void);
int rtc_set_timestamp(time_t timestamp);

/* Alarm functions */
int rtc_set_alarm(const rtc_alarm_t *alarm);
int rtc_get_alarm(rtc_alarm_t *alarm);
int rtc_enable_alarm(void);
int rtc_disable_alarm(void);

/* Periodic interrupt functions */
int rtc_set_periodic_rate(uint8_t rate);
int rtc_enable_periodic_interrupt(void (*callback)(void *), void *data);
int rtc_disable_periodic_interrupt(void);

/* Interrupt handler */
void rtc_interrupt_handler(void);

/* Utility functions */
uint8_t rtc_bcd_to_binary(uint8_t bcd);
uint8_t rtc_binary_to_bcd(uint8_t binary);

#endif /* BDI_RTC_H */
