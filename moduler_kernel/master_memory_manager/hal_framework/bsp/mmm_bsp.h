
/**
 * @file mmm_bsp.h
 * @brief Master Memory Manager Board Support Package Interface
 * 
 * Provides comprehensive Board Support Package (BSP) interface including:
 * - Hardware-independent application interface
 * - Hardware-specific peripheral interface
 * - GPIO and analog interface abstractions
 * - Device/board split architecture
 * - Static inline optimization for hot paths
 * 
 * Based on technical foundation from Hardware Abstraction Layer.pdf
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_BSP_H
#define MMM_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// BSP CONSTANTS AND DEFINITIONS
// =============================================================================

#define MMM_BSP_MAX_GPIO_PINS       64      ///< Maximum GPIO pins supported
#define MMM_BSP_MAX_ANALOG_CHANNELS 16      ///< Maximum analog channels
#define MMM_BSP_MAX_TIMERS          8       ///< Maximum timers supported
#define MMM_BSP_MAX_UART_PORTS      4       ///< Maximum UART ports
#define MMM_BSP_MAX_SPI_PORTS       4       ///< Maximum SPI ports
#define MMM_BSP_MAX_I2C_PORTS       4       ///< Maximum I2C ports

/**
 * @brief GPIO pin states
 */
typedef enum {
    MMM_BSP_GPIO_LOW = 0,       ///< GPIO pin low state
    MMM_BSP_GPIO_HIGH = 1       ///< GPIO pin high state
} mmm_bsp_gpio_state_t;

/**
 * @brief GPIO pin directions
 */
typedef enum {
    MMM_BSP_GPIO_INPUT = 0,     ///< GPIO pin as input
    MMM_BSP_GPIO_OUTPUT = 1     ///< GPIO pin as output
} mmm_bsp_gpio_direction_t;

/**
 * @brief GPIO pin pull-up/pull-down configuration
 */
typedef enum {
    MMM_BSP_GPIO_PULL_NONE = 0, ///< No pull resistor
    MMM_BSP_GPIO_PULL_UP,       ///< Pull-up resistor enabled
    MMM_BSP_GPIO_PULL_DOWN      ///< Pull-down resistor enabled
} mmm_bsp_gpio_pull_t;

/**
 * @brief Analog reference voltage sources
 */
typedef enum {
    MMM_BSP_ANALOG_REF_VDD = 0, ///< VDD as reference
    MMM_BSP_ANALOG_REF_VREF,    ///< External VREF as reference
    MMM_BSP_ANALOG_REF_INTERNAL ///< Internal reference
} mmm_bsp_analog_ref_t;

/**
 * @brief Timer modes
 */
typedef enum {
    MMM_BSP_TIMER_MODE_ONESHOT = 0, ///< One-shot timer mode
    MMM_BSP_TIMER_MODE_PERIODIC,    ///< Periodic timer mode
    MMM_BSP_TIMER_MODE_PWM,         ///< PWM generation mode
    MMM_BSP_TIMER_MODE_CAPTURE      ///< Input capture mode
} mmm_bsp_timer_mode_t;

/**
 * @brief Communication interface types
 */
typedef enum {
    MMM_BSP_COMM_UART = 0,      ///< UART communication
    MMM_BSP_COMM_SPI,           ///< SPI communication
    MMM_BSP_COMM_I2C,           ///< I2C communication
    MMM_BSP_COMM_CAN            ///< CAN communication
} mmm_bsp_comm_type_t;

/**
 * @brief GPIO pin configuration
 */
typedef struct {
    uint8_t pin_number;                 ///< Physical pin number
    mmm_bsp_gpio_direction_t direction; ///< Pin direction
    mmm_bsp_gpio_pull_t pull;          ///< Pull resistor configuration
    bool open_drain;                    ///< Open-drain configuration
    bool high_speed;                    ///< High-speed mode
    uint32_t alternate_function;        ///< Alternate function selection
} mmm_bsp_gpio_config_t;

/**
 * @brief Analog channel configuration
 */
typedef struct {
    uint8_t channel;                    ///< Analog channel number
    mmm_bsp_analog_ref_t reference;     ///< Reference voltage source
    uint8_t resolution_bits;            ///< ADC resolution in bits
    uint32_t sample_time;               ///< Sample time in cycles
    bool differential;                  ///< Differential input mode
} mmm_bsp_analog_config_t;

/**
 * @brief Timer configuration
 */
typedef struct {
    uint8_t timer_id;                   ///< Timer identifier
    mmm_bsp_timer_mode_t mode;          ///< Timer mode
    uint32_t frequency;                 ///< Timer frequency in Hz
    uint32_t period;                    ///< Timer period
    uint8_t prescaler;                  ///< Timer prescaler
    bool interrupt_enabled;             ///< Interrupt enable
} mmm_bsp_timer_config_t;

/**
 * @brief Communication interface configuration
 */
typedef struct {
    mmm_bsp_comm_type_t type;           ///< Communication type
    uint8_t port_id;                    ///< Port identifier
    uint32_t baudrate;                  ///< Baud rate (UART)
    uint8_t data_bits;                  ///< Data bits (UART)
    uint8_t stop_bits;                  ///< Stop bits (UART)
    bool parity_enabled;                ///< Parity enable (UART)
    uint32_t clock_frequency;           ///< Clock frequency (SPI/I2C)
    bool master_mode;                   ///< Master mode (SPI/I2C)
} mmm_bsp_comm_config_t;

/**
 * @brief BSP board information
 */
typedef struct {
    const char *board_name;             ///< Board name string
    const char *board_version;          ///< Board version string
    const char *mcu_name;               ///< MCU name string
    uint32_t system_clock_hz;           ///< System clock frequency
    uint32_t gpio_pin_count;            ///< Number of GPIO pins
    uint32_t analog_channel_count;      ///< Number of analog channels
    uint32_t timer_count;               ///< Number of timers
    uint32_t uart_port_count;           ///< Number of UART ports
    uint32_t spi_port_count;            ///< Number of SPI ports
    uint32_t i2c_port_count;            ///< Number of I2C ports
} mmm_bsp_board_info_t;

/**
 * @brief BSP context structure
 */
typedef struct {
    mmm_bsp_board_info_t board_info;    ///< Board information
    mmm_bsp_gpio_config_t *gpio_configs; ///< GPIO configurations
    mmm_bsp_analog_config_t *analog_configs; ///< Analog configurations
    mmm_bsp_timer_config_t *timer_configs; ///< Timer configurations
    mmm_bsp_comm_config_t *comm_configs; ///< Communication configurations
    bool initialized;                   ///< Initialization status
    uint32_t error_count;               ///< Error counter
} mmm_bsp_context_t;

// =============================================================================
// APPLICATION INTERFACE (Hardware-Independent)
// =============================================================================

// Generic GPIO macros (mapped to hardware-specific implementations)
#define MMM_BSP_LED_STATUS              MMM_BSP_GPIO_PIN_A7
#define MMM_BSP_LED_ERROR               MMM_BSP_GPIO_PIN_A8
#define MMM_BSP_BUTTON_USER             MMM_BSP_GPIO_PIN_A9
#define MMM_BSP_BUTTON_RESET            MMM_BSP_GPIO_PIN_A10

// Generic analog channels
#define MMM_BSP_ANALOG_VOLTAGE_MONITOR  MMM_BSP_ANALOG_CHANNEL_0
#define MMM_BSP_ANALOG_TEMPERATURE      MMM_BSP_ANALOG_CHANNEL_1
#define MMM_BSP_ANALOG_USER_INPUT       MMM_BSP_ANALOG_CHANNEL_2

// Generic timer assignments
#define MMM_BSP_TIMER_SYSTEM            MMM_BSP_TIMER_0
#define MMM_BSP_TIMER_PWM               MMM_BSP_TIMER_1
#define MMM_BSP_TIMER_USER              MMM_BSP_TIMER_2

// =============================================================================
// PERIPHERAL INTERFACE (Hardware-Specific)
// =============================================================================

// Hardware-specific GPIO pin definitions (example for generic board)
#define MMM_BSP_GPIO_PIN_A0             0
#define MMM_BSP_GPIO_PIN_A1             1
#define MMM_BSP_GPIO_PIN_A2             2
#define MMM_BSP_GPIO_PIN_A3             3
#define MMM_BSP_GPIO_PIN_A4             4
#define MMM_BSP_GPIO_PIN_A5             5
#define MMM_BSP_GPIO_PIN_A6             6
#define MMM_BSP_GPIO_PIN_A7             7
#define MMM_BSP_GPIO_PIN_A8             8
#define MMM_BSP_GPIO_PIN_A9             9
#define MMM_BSP_GPIO_PIN_A10            10

// Hardware-specific analog channel definitions
#define MMM_BSP_ANALOG_CHANNEL_0        0
#define MMM_BSP_ANALOG_CHANNEL_1        1
#define MMM_BSP_ANALOG_CHANNEL_2        2
#define MMM_BSP_ANALOG_CHANNEL_3        3

// Hardware-specific timer definitions
#define MMM_BSP_TIMER_0                 0
#define MMM_BSP_TIMER_1                 1
#define MMM_BSP_TIMER_2                 2
#define MMM_BSP_TIMER_3                 3

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize BSP system
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_initialize(void);

/**
 * @brief Shutdown BSP system
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_shutdown(void);

/**
 * @brief Get board information
 * @return Pointer to board information structure
 */
const mmm_bsp_board_info_t *mmm_bsp_get_board_info(void);

/**
 * @brief Configure GPIO pin
 * @param config GPIO configuration structure
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_gpio_configure(const mmm_bsp_gpio_config_t *config);

/**
 * @brief Set GPIO pin state
 * @param pin_number GPIO pin number
 * @param state Pin state to set
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_gpio_set_state(uint8_t pin_number, mmm_bsp_gpio_state_t state);

/**
 * @brief Get GPIO pin state
 * @param pin_number GPIO pin number
 * @return Pin state or negative error code
 */
int mmm_bsp_gpio_get_state(uint8_t pin_number);

/**
 * @brief Toggle GPIO pin state
 * @param pin_number GPIO pin number
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_gpio_toggle(uint8_t pin_number);

/**
 * @brief Configure analog channel
 * @param config Analog configuration structure
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_analog_configure(const mmm_bsp_analog_config_t *config);

/**
 * @brief Read analog channel value
 * @param channel Analog channel number
 * @param value Pointer to store read value
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_analog_read(uint8_t channel, uint32_t *value);

/**
 * @brief Configure timer
 * @param config Timer configuration structure
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_timer_configure(const mmm_bsp_timer_config_t *config);

/**
 * @brief Start timer
 * @param timer_id Timer identifier
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_timer_start(uint8_t timer_id);

/**
 * @brief Stop timer
 * @param timer_id Timer identifier
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_timer_stop(uint8_t timer_id);

/**
 * @brief Get timer value
 * @param timer_id Timer identifier
 * @param value Pointer to store timer value
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_timer_get_value(uint8_t timer_id, uint32_t *value);

/**
 * @brief Configure communication interface
 * @param config Communication configuration structure
 * @return Status code (0 = success, negative = error)
 */
int mmm_bsp_comm_configure(const mmm_bsp_comm_config_t *config);

/**
 * @brief Send data via communication interface
 * @param type Communication type
 * @param port_id Port identifier
 * @param data Data buffer to send
 * @param length Data length
 * @return Number of bytes sent or negative error code
 */
int mmm_bsp_comm_send(mmm_bsp_comm_type_t type, uint8_t port_id, const uint8_t *data, size_t length);

/**
 * @brief Receive data via communication interface
 * @param type Communication type
 * @param port_id Port identifier
 * @param data Data buffer to receive into
 * @param length Maximum data length
 * @return Number of bytes received or negative error code
 */
int mmm_bsp_comm_receive(mmm_bsp_comm_type_t type, uint8_t port_id, uint8_t *data, size_t length);

/**
 * @brief Get BSP context
 * @return Pointer to BSP context structure
 */
mmm_bsp_context_t *mmm_bsp_get_context(void);

/**
 * @brief Reset BSP error counter
 */
void mmm_bsp_reset_error_counter(void);

/**
 * @brief Get BSP error count
 * @return Current error count
 */
uint32_t mmm_bsp_get_error_count(void);

// =============================================================================
// STATIC INLINE FUNCTIONS (Hot Path Optimizations)
// =============================================================================

/**
 * @brief Activate status LED (static inline for performance)
 */
static inline void mmm_bsp_led_status_activate(void)
{
    // Direct hardware access for maximum performance
    // In real implementation, would access hardware registers directly
    mmm_bsp_gpio_set_state(MMM_BSP_LED_STATUS, MMM_BSP_GPIO_HIGH);
}

/**
 * @brief Deactivate status LED (static inline for performance)
 */
static inline void mmm_bsp_led_status_deactivate(void)
{
    // Direct hardware access for maximum performance
    mmm_bsp_gpio_set_state(MMM_BSP_LED_STATUS, MMM_BSP_GPIO_LOW);
}

/**
 * @brief Toggle status LED (static inline for performance)
 */
static inline void mmm_bsp_led_status_toggle(void)
{
    // Direct hardware access for maximum performance
    mmm_bsp_gpio_toggle(MMM_BSP_LED_STATUS);
}

/**
 * @brief Activate error LED (static inline for performance)
 */
static inline void mmm_bsp_led_error_activate(void)
{
    mmm_bsp_gpio_set_state(MMM_BSP_LED_ERROR, MMM_BSP_GPIO_HIGH);
}

/**
 * @brief Deactivate error LED (static inline for performance)
 */
static inline void mmm_bsp_led_error_deactivate(void)
{
    mmm_bsp_gpio_set_state(MMM_BSP_LED_ERROR, MMM_BSP_GPIO_LOW);
}

/**
 * @brief Check if user button is pressed (static inline for performance)
 * @return True if button is pressed
 */
static inline bool mmm_bsp_button_user_is_pressed(void)
{
    int state = mmm_bsp_gpio_get_state(MMM_BSP_BUTTON_USER);
    return (state == MMM_BSP_GPIO_LOW); // Assuming active-low button
}

/**
 * @brief Check if reset button is pressed (static inline for performance)
 * @return True if button is pressed
 */
static inline bool mmm_bsp_button_reset_is_pressed(void)
{
    int state = mmm_bsp_gpio_get_state(MMM_BSP_BUTTON_RESET);
    return (state == MMM_BSP_GPIO_LOW); // Assuming active-low button
}

/**
 * @brief Fast GPIO pin set (static inline for performance)
 * @param pin_number GPIO pin number
 */
static inline void mmm_bsp_gpio_set_fast(uint8_t pin_number)
{
    // Direct register access for maximum performance
    // In real implementation, would use bit manipulation on hardware registers
    mmm_bsp_gpio_set_state(pin_number, MMM_BSP_GPIO_HIGH);
}

/**
 * @brief Fast GPIO pin clear (static inline for performance)
 * @param pin_number GPIO pin number
 */
static inline void mmm_bsp_gpio_clear_fast(uint8_t pin_number)
{
    // Direct register access for maximum performance
    mmm_bsp_gpio_set_state(pin_number, MMM_BSP_GPIO_LOW);
}

/**
 * @brief Fast GPIO pin read (static inline for performance)
 * @param pin_number GPIO pin number
 * @return Pin state (0 or 1)
 */
static inline uint8_t mmm_bsp_gpio_read_fast(uint8_t pin_number)
{
    // Direct register access for maximum performance
    int state = mmm_bsp_gpio_get_state(pin_number);
    return (state > 0) ? 1 : 0;
}

/**
 * @brief Get system tick count (static inline for performance)
 * @return Current system tick count
 */
static inline uint32_t mmm_bsp_get_tick_count(void)
{
    // In real implementation, would read hardware timer register
    static uint32_t tick_counter = 0;
    return ++tick_counter;
}

/**
 * @brief Microsecond delay (static inline for performance)
 * @param microseconds Delay in microseconds
 */
static inline void mmm_bsp_delay_us(uint32_t microseconds)
{
    // In real implementation, would use hardware timer or cycle counting
    // This is a placeholder implementation
    volatile uint32_t cycles = microseconds * 100; // Approximate
    while (cycles--) {
        __asm__ volatile ("nop");
    }
}

/**
 * @brief Millisecond delay (static inline for performance)
 * @param milliseconds Delay in milliseconds
 */
static inline void mmm_bsp_delay_ms(uint32_t milliseconds)
{
    mmm_bsp_delay_us(milliseconds * 1000);
}

#ifdef __cplusplus
}
#endif

#endif // MMM_BSP_H
