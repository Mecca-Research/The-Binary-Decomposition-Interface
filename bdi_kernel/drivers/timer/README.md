
# Timer Subsystem

## Overview
The timer subsystem provides high-resolution timing capabilities for the BDI kernel through three main components:

1. **HPET (High Precision Event Timer)** - Hardware timer with nanosecond precision
2. **RTC (Real-Time Clock)** - CMOS clock for wall time and periodic interrupts
3. **Clocksource** - Unified abstraction for time sources and clock events

## Components

### HPET Driver
- Supports up to 32 timers per HPET block
- Periodic and one-shot timer modes
- Multiple interrupt routing options (Legacy, Standard, FSB)
- Nanosecond-precision timing
- Frequency: Typically 14.318 MHz (70ns resolution)

### RTC Driver
- CMOS RTC interface (ports 0x70/0x71)
- Time reading/writing with BCD/binary mode support
- Alarm functionality
- Periodic interrupts (2Hz to 8192Hz)
- Wall clock time management

### Clocksource Abstraction
- Unified clock source registration and selection
- Monotonic clock support
- Wall clock time management
- Time conversion utilities
- Clock event device support

## Usage

### Initialization
```c
#include <bdi/drivers/hpet.h>
#include <bdi/drivers/rtc.h>
#include <bdi/drivers/clocksource.h>

// Initialize all timer subsystems
clocksource_init();  // Initializes HPET, RTC, and clocksource
```

### Using HPET Timers
```c
// Setup a periodic timer (1ms period)
void timer_callback(void *data) {
    printf("Timer fired!\n");
}

hpet_timer_setup(0, 1000000, true, timer_callback, NULL);
hpet_timer_enable(0);
```

### Reading Time
```c
// Get monotonic time
uint64_t ns = monotonic_clock_ns();
uint64_t us = monotonic_clock_us();
uint64_t ms = monotonic_clock_ms();

// Get wall clock time
time_t now = wall_clock_time();

// Read RTC
rtc_time_t time;
rtc_read_time(&time);
printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
       time.year, time.month, time.day,
       time.hour, time.minute, time.second);
```

### Setting Up Periodic Interrupts
```c
// RTC periodic interrupt at 1024Hz
void periodic_handler(void *data) {
    // Called 1024 times per second
}

rtc_set_periodic_rate(RTC_RATE_1024HZ);
rtc_enable_periodic_interrupt(periodic_handler, NULL);
```

## API Reference

### HPET Functions
- `int hpet_init(void)` - Initialize HPET
- `int hpet_enable(void)` - Enable HPET counter
- `int hpet_disable(void)` - Disable HPET counter
- `uint64_t hpet_read_counter(void)` - Read main counter
- `int hpet_timer_setup(...)` - Configure a timer
- `int hpet_timer_enable(uint32_t timer_id)` - Enable timer
- `int hpet_timer_disable(uint32_t timer_id)` - Disable timer

### RTC Functions
- `int rtc_init(void)` - Initialize RTC
- `int rtc_read_time(rtc_time_t *time)` - Read current time
- `int rtc_write_time(const rtc_time_t *time)` - Set time
- `time_t rtc_get_timestamp(void)` - Get Unix timestamp
- `int rtc_set_periodic_rate(uint8_t rate)` - Set periodic rate
- `int rtc_enable_periodic_interrupt(...)` - Enable periodic interrupt

### Clocksource Functions
- `int clocksource_init(void)` - Initialize subsystem
- `int clocksource_register(clocksource_t *cs)` - Register clock source
- `uint64_t clocksource_read(void)` - Read current clock source
- `uint64_t monotonic_clock_ns(void)` - Get monotonic time in ns
- `time_t wall_clock_time(void)` - Get wall clock time

## Building
```bash
cd drivers/timer
make clean
make all
```

## Testing
```bash
# Run timer tests
../../test_runner --subsystem=timer
```

## Performance
- HPET read latency: ~100ns
- Timer setup overhead: ~1μ
- Interrupt latency: <10μs
- Clock source read: ~50ns

## Notes
- HPET base address is currently hardcoded to 0xFED00000 (should be read from ACPI)
- RTC assumes standard CMOS interface at ports 0x70/0x71
- Timer callbacks execute in interrupt context
- All time values use nanosecond precision internally
