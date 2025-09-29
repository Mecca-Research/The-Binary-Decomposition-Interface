
/**
 * @file mmm_peripheral_drivers.h
 * @brief Master Memory Manager Peripheral Drivers Interface
 * 
 * Provides comprehensive peripheral driver interface including:
 * - Modular peripheral driver architecture
 * - Consistent naming conventions with peripheral prefixes
 * - System function support (oscillator, GPIO, interrupts, watchdog)
 * - ADC, PWM, DMA, QEI, Timer, UART module support
 * 
 * Based on technical foundation from Hardware Abstraction Layer.pdf
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_PERIPHERAL_DRIVERS_H
#define MMM_PERIPHERAL_DRIVERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// PERIPHERAL DRIVER CONSTANTS
// =============================================================================

#define MMM_PERIPH_MAX_INSTANCES    8       ///< Maximum peripheral instances
#define MMM_PERIPH_NAME_MAX_LEN     32      ///< Maximum peripheral name length
#define MMM_PERIPH_VERSION_LEN      16      ///< Peripheral version string length

/**
 * @brief Peripheral types
 */
typedef enum {
    MMM_PERIPH_TYPE_OSCILLATOR = 0, ///< Oscillator peripheral
    MMM_PERIPH_TYPE_GPIO,           ///< GPIO peripheral
    MMM_PERIPH_TYPE_INTERRUPT,      ///< Interrupt controller
    MMM_PERIPH_TYPE_WATCHDOG,       ///< Watchdog timer
    MMM_PERIPH_TYPE_ADC,            ///< Analog-to-Digital Converter
    MMM_PERIPH_TYPE_PWM,            ///< Pulse Width Modulation
    MMM_PERIPH_TYPE_DMA,            ///< Direct Memory Access
    MMM_PERIPH_TYPE_QEI,            ///< Quadrature Encoder Interface
    MMM_PERIPH_TYPE_TIMER,          ///< Timer peripheral
    MMM_PERIPH_TYPE_UART,           ///< Universal Asynchronous Receiver-Transmitter
    MMM_PERIPH_TYPE_COUNT           ///< Total peripheral types
} mmm_periph_type_t;

/**
 * @brief Peripheral states
 */
typedef enum {
    MMM_PERIPH_STATE_UNINITIALIZED = 0, ///< Peripheral not initialized
    MMM_PERIPH_STATE_INITIALIZED,       ///< Peripheral initialized
    MMM_PERIPH_STATE_RUNNING,           ///< Peripheral running
    MMM_PERIPH_STATE_STOPPED,           ///< Peripheral stopped
    MMM_PERIPH_STATE_ERROR              ///< Peripheral in error state
} mmm_periph_state_t;

/**
 * @brief DMA channel identifiers
 */
typedef enum {
    MMM_DMA_CHANNEL_0 = 0,
    MMM_DMA_CHANNEL_1,
    MMM_DMA_CHANNEL_2,
    MMM_DMA_CHANNEL_3,
    MMM_DMA_CHANNEL_4,
    MMM_DMA_CHANNEL_5,
    MMM_DMA_CHANNEL_6,
    MMM_DMA_CHANNEL_7,
    MMM_DMA_CHANNEL_COUNT
} mmm_dma_channel_t;

/**
 * @brief Peripheral descriptor
 */
typedef struct {
    mmm_periph_type_t type;             ///< Peripheral type
    uint8_t instance;                   ///< Peripheral instance number
    char name[MMM_PERIPH_NAME_MAX_LEN]; ///< Peripheral name
    char version[MMM_PERIPH_VERSION_LEN]; ///< Peripheral version
    mmm_periph_state_t state;           ///< Current state
    void *base_address;                 ///< Base address of peripheral registers
    uint32_t clock_frequency;           ///< Peripheral clock frequency
    bool interrupt_enabled;             ///< Interrupt enable status
    uint8_t interrupt_priority;         ///< Interrupt priority
    void *driver_data;                  ///< Driver-specific data
} mmm_periph_descriptor_t;

/**
 * @brief Peripheral driver interface
 */
typedef struct {
    int (*initialize)(mmm_periph_descriptor_t *periph);
    int (*shutdown)(mmm_periph_descriptor_t *periph);
    int (*start)(mmm_periph_descriptor_t *periph);
    int (*stop)(mmm_periph_descriptor_t *periph);
    int (*configure)(mmm_periph_descriptor_t *periph, const void *config);
    int (*read)(mmm_periph_descriptor_t *periph, void *data, size_t size);
    int (*write)(mmm_periph_descriptor_t *periph, const void *data, size_t size);
    int (*ioctl)(mmm_periph_descriptor_t *periph, uint32_t cmd, void *arg);
} mmm_periph_driver_interface_t;

/**
 * @brief Peripheral driver context
 */
typedef struct {
    mmm_periph_descriptor_t peripherals[MMM_PERIPH_TYPE_COUNT][MMM_PERIPH_MAX_INSTANCES];
    mmm_periph_driver_interface_t drivers[MMM_PERIPH_TYPE_COUNT];
    bool initialized;
    uint32_t error_count;
} mmm_periph_context_t;

// =============================================================================
// OSCILLATOR PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize oscillator peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_OSCILLATOR_Initialize(void);

/**
 * @brief Configure oscillator frequency
 * @param frequency Desired frequency in Hz
 * @return Status code (0 = success, negative = error)
 */
int MMM_OSCILLATOR_SetFrequency(uint32_t frequency);

/**
 * @brief Get current oscillator frequency
 * @return Current frequency in Hz
 */
uint32_t MMM_OSCILLATOR_GetFrequency(void);

// =============================================================================
// GPIO PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize GPIO peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_GPIO_Initialize(void);

/**
 * @brief Configure GPIO pin
 * @param pin Pin number
 * @param direction Pin direction (input/output)
 * @param pull Pull-up/pull-down configuration
 * @return Status code (0 = success, negative = error)
 */
int MMM_GPIO_ConfigurePin(uint8_t pin, uint8_t direction, uint8_t pull);

/**
 * @brief Set GPIO pin state
 * @param pin Pin number
 * @param state Pin state (high/low)
 * @return Status code (0 = success, negative = error)
 */
int MMM_GPIO_SetPin(uint8_t pin, bool state);

/**
 * @brief Get GPIO pin state
 * @param pin Pin number
 * @return Pin state (1 = high, 0 = low, negative = error)
 */
int MMM_GPIO_GetPin(uint8_t pin);

// =============================================================================
// INTERRUPT PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize interrupt controller
 * @return Status code (0 = success, negative = error)
 */
int MMM_INTERRUPT_Initialize(void);

/**
 * @brief Enable interrupt
 * @param interrupt_id Interrupt identifier
 * @param priority Interrupt priority
 * @return Status code (0 = success, negative = error)
 */
int MMM_INTERRUPT_Enable(uint8_t interrupt_id, uint8_t priority);

/**
 * @brief Disable interrupt
 * @param interrupt_id Interrupt identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_INTERRUPT_Disable(uint8_t interrupt_id);

// =============================================================================
// WATCHDOG PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize watchdog timer
 * @return Status code (0 = success, negative = error)
 */
int MMM_WATCHDOG_Initialize(void);

/**
 * @brief Start watchdog timer
 * @param timeout_ms Timeout in milliseconds
 * @return Status code (0 = success, negative = error)
 */
int MMM_WATCHDOG_Start(uint32_t timeout_ms);

/**
 * @brief Feed/kick watchdog timer
 * @return Status code (0 = success, negative = error)
 */
int MMM_WATCHDOG_Feed(void);

/**
 * @brief Stop watchdog timer
 * @return Status code (0 = success, negative = error)
 */
int MMM_WATCHDOG_Stop(void);

// =============================================================================
// ADC PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize ADC1 peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_ADC1_Initialize(void);

/**
 * @brief Start ADC conversion
 * @param channel ADC channel number
 * @return Status code (0 = success, negative = error)
 */
int MMM_ADC1_StartConversion(uint8_t channel);

/**
 * @brief Read ADC conversion result
 * @param result Pointer to store conversion result
 * @return Status code (0 = success, negative = error)
 */
int MMM_ADC1_ReadResult(uint16_t *result);

/**
 * @brief Check if ADC conversion is complete
 * @return True if conversion complete
 */
bool MMM_ADC1_IsConversionComplete(void);

// =============================================================================
// PWM PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize PWM2 peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_PWM2_Initialize(void);

/**
 * @brief Set PWM duty cycle
 * @param duty_cycle Duty cycle value (0-100%)
 * @return Status code (0 = success, negative = error)
 */
int MMM_PWM2_DutyCycleSet(uint16_t duty_cycle);

/**
 * @brief Start PWM generation
 * @return Status code (0 = success, negative = error)
 */
int MMM_PWM2_Start(void);

/**
 * @brief Stop PWM generation
 * @return Status code (0 = success, negative = error)
 */
int MMM_PWM2_Stop(void);

// =============================================================================
// DMA PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize DMA peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_DMA_Initialize(void);

/**
 * @brief Enable software trigger for DMA channel
 * @param channel DMA channel
 * @return Status code (0 = success, negative = error)
 */
int MMM_DMA_SoftwareTriggerEnable(mmm_dma_channel_t channel);

/**
 * @brief Configure DMA transfer
 * @param channel DMA channel
 * @param src_addr Source address
 * @param dest_addr Destination address
 * @param size Transfer size in bytes
 * @return Status code (0 = success, negative = error)
 */
int MMM_DMA_ConfigureTransfer(mmm_dma_channel_t channel, void *src_addr, void *dest_addr, size_t size);

/**
 * @brief Start DMA transfer
 * @param channel DMA channel
 * @return Status code (0 = success, negative = error)
 */
int MMM_DMA_StartTransfer(mmm_dma_channel_t channel);

// =============================================================================
// QEI PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize QEI1 peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_QEI1_Initialize(void);

/**
 * @brief Read 16-bit position count
 * @return Current position count
 */
uint16_t MMM_QEI1_PositionCount16bitRead(void);

/**
 * @brief Reset position counter
 * @return Status code (0 = success, negative = error)
 */
int MMM_QEI1_PositionCountReset(void);

// =============================================================================
// TIMER PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize TMR1 peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_TMR1_Initialize(void);

/**
 * @brief Start timer
 * @return Status code (0 = success, negative = error)
 */
int MMM_TMR1_Start(void);

/**
 * @brief Stop timer
 * @return Status code (0 = success, negative = error)
 */
int MMM_TMR1_Stop(void);

/**
 * @brief Set timer period
 * @param period Timer period value
 * @return Status code (0 = success, negative = error)
 */
int MMM_TMR1_PeriodSet(uint32_t period);

/**
 * @brief Get timer counter value
 * @return Current timer counter value
 */
uint32_t MMM_TMR1_CounterGet(void);

// =============================================================================
// UART PERIPHERAL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize UART1 peripheral
 * @return Status code (0 = success, negative = error)
 */
int MMM_UART1_Initialize(void);

/**
 * @brief Send byte via UART
 * @param data Byte to send
 * @return Status code (0 = success, negative = error)
 */
int MMM_UART1_SendByte(uint8_t data);

/**
 * @brief Receive byte from UART
 * @param data Pointer to store received byte
 * @return Status code (0 = success, negative = error)
 */
int MMM_UART1_ReceiveByte(uint8_t *data);

/**
 * @brief Check if UART transmit is ready
 * @return True if ready to transmit
 */
bool MMM_UART1_IsTxReady(void);

/**
 * @brief Check if UART receive data is available
 * @return True if data available
 */
bool MMM_UART1_IsRxDataAvailable(void);

// =============================================================================
// PERIPHERAL DRIVER MANAGEMENT FUNCTIONS
// =============================================================================

/**
 * @brief Initialize peripheral drivers system
 * @return Status code (0 = success, negative = error)
 */
int mmm_peripheral_drivers_initialize(void);

/**
 * @brief Shutdown peripheral drivers system
 * @return Status code (0 = success, negative = error)
 */
int mmm_peripheral_drivers_shutdown(void);

/**
 * @brief Register peripheral driver
 * @param type Peripheral type
 * @param driver Driver interface
 * @return Status code (0 = success, negative = error)
 */
int mmm_peripheral_register_driver(mmm_periph_type_t type, const mmm_periph_driver_interface_t *driver);

/**
 * @brief Get peripheral descriptor
 * @param type Peripheral type
 * @param instance Peripheral instance
 * @return Pointer to peripheral descriptor or NULL
 */
mmm_periph_descriptor_t *mmm_peripheral_get_descriptor(mmm_periph_type_t type, uint8_t instance);

/**
 * @brief Get peripheral driver context
 * @return Pointer to peripheral context
 */
mmm_periph_context_t *mmm_peripheral_get_context(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_PERIPHERAL_DRIVERS_H
