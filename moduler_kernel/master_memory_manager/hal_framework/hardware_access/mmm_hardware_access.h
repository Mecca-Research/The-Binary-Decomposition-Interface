
/**
 * @file mmm_hardware_access.h
 * @brief Master Memory Manager Hardware Access Functions Interface
 * 
 * Provides comprehensive hardware access function layer including:
 * - Facade pattern wrapper functions with HAL_ prefix
 * - ISR abstraction macros for interrupt service routines
 * - Complex hardware operations abstraction
 * - Motor control and PWM operations
 * - System-level hardware access
 * 
 * Based on technical foundation from Hardware Abstraction Layer.pdf
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_HARDWARE_ACCESS_H
#define MMM_HARDWARE_ACCESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// HARDWARE ACCESS CONSTANTS
// =============================================================================

#define MMM_HAL_MAX_MOTORS          4       ///< Maximum number of motors
#define MMM_HAL_MAX_PWM_CHANNELS    8       ///< Maximum PWM channels
#define MMM_HAL_MAX_ADC_CHANNELS    16      ///< Maximum ADC channels
#define MMM_HAL_MAX_DMA_CHANNELS    8       ///< Maximum DMA channels

/**
 * @brief Motor identifiers
 */
typedef enum {
    MMM_HAL_MOTOR_1 = 0,        ///< Motor 1
    MMM_HAL_MOTOR_2,            ///< Motor 2
    MMM_HAL_MOTOR_3,            ///< Motor 3
    MMM_HAL_MOTOR_4,            ///< Motor 4
    MMM_HAL_MOTOR_COUNT         ///< Total motor count
} mmm_hal_motor_id_t;

/**
 * @brief PWM channel identifiers
 */
typedef enum {
    MMM_HAL_PWM_CHANNEL_1 = 0,  ///< PWM Channel 1
    MMM_HAL_PWM_CHANNEL_2,      ///< PWM Channel 2
    MMM_HAL_PWM_CHANNEL_3,      ///< PWM Channel 3
    MMM_HAL_PWM_CHANNEL_4,      ///< PWM Channel 4
    MMM_HAL_PWM_CHANNEL_5,      ///< PWM Channel 5
    MMM_HAL_PWM_CHANNEL_6,      ///< PWM Channel 6
    MMM_HAL_PWM_CHANNEL_7,      ///< PWM Channel 7
    MMM_HAL_PWM_CHANNEL_8,      ///< PWM Channel 8
    MMM_HAL_PWM_CHANNEL_COUNT   ///< Total PWM channel count
} mmm_hal_pwm_channel_t;

/**
 * @brief Hardware access context
 */
typedef struct {
    bool initialized;           ///< Initialization status
    uint32_t error_count;       ///< Error counter
    uint32_t operation_count;   ///< Operation counter
    void *motor_contexts[MMM_HAL_MAX_MOTORS]; ///< Motor contexts
    void *pwm_contexts[MMM_HAL_MAX_PWM_CHANNELS]; ///< PWM contexts
} mmm_hal_context_t;

// =============================================================================
// ISR ABSTRACTION MACROS
// =============================================================================

/**
 * @brief Math error trap function macro
 */
#define MMM_HAL_MATHERROR_TRAP_FUNCTION    _MathError

/**
 * @brief DMAC error trap function macro
 */
#define MMM_HAL_DMAC_TRAP_FUNCTION         _DMACError

/**
 * @brief ADC1 interrupt service routine macro
 */
#define MMM_HAL_ADC1_ISR                   _AD1Interrupt

/**
 * @brief DMA0 interrupt service routine macro
 */
#define MMM_HAL_DMA0_ISR                   _DMA0Interrupt

/**
 * @brief Timer1 interrupt service routine macro
 */
#define MMM_HAL_TMR1_ISR                   _T1Interrupt

/**
 * @brief UART1 receive interrupt service routine macro
 */
#define MMM_HAL_UART1_RX_ISR               _U1RXInterrupt

/**
 * @brief UART1 transmit interrupt service routine macro
 */
#define MMM_HAL_UART1_TX_ISR               _U1TXInterrupt

/**
 * @brief PWM interrupt service routine macro
 */
#define MMM_HAL_PWM_ISR                    _PWMInterrupt

/**
 * @brief QEI interrupt service routine macro
 */
#define MMM_HAL_QEI_ISR                    _QEIInterrupt

// =============================================================================
// MOTOR CONTROL FUNCTIONS
// =============================================================================

/**
 * @brief Initialize hardware access functions
 * @return Status code (0 = success, negative = error)
 */
int mmm_hardware_access_initialize(void);

/**
 * @brief Shutdown hardware access functions
 * @return Status code (0 = success, negative = error)
 */
int mmm_hardware_access_shutdown(void);

/**
 * @brief Disable PWM upper transistors override for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmUpperTransistorsOverrideDisable(mmm_hal_motor_id_t motor_id);

/**
 * @brief Set PWM upper transistors override to low for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmUpperTransistorsOverrideLow(mmm_hal_motor_id_t motor_id);

/**
 * @brief Set identical duty cycles for all PWM channels of motor
 * @param motor_id Motor identifier
 * @param duty_cycle Duty cycle value (0-100%)
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmSetDutyCyclesIdentical(mmm_hal_motor_id_t motor_id, uint16_t duty_cycle);

/**
 * @brief Set individual duty cycles for motor PWM channels
 * @param motor_id Motor identifier
 * @param duty_cycles Array of duty cycle values
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmSetDutyCycles(mmm_hal_motor_id_t motor_id, const uint16_t *duty_cycles);

/**
 * @brief Enable PWM output for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmOutputEnable(mmm_hal_motor_id_t motor_id);

/**
 * @brief Disable PWM output for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmOutputDisable(mmm_hal_motor_id_t motor_id);

/**
 * @brief Start PWM generation for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmStart(mmm_hal_motor_id_t motor_id);

/**
 * @brief Stop PWM generation for motor
 * @param motor_id Motor identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PwmStop(mmm_hal_motor_id_t motor_id);

// =============================================================================
// ADC HARDWARE ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Start ADC conversion sequence
 * @param channel_mask Bitmask of channels to convert
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_AdcConversionStart(uint16_t channel_mask);

/**
 * @brief Stop ADC conversion sequence
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_AdcConversionStop(void);

/**
 * @brief Read ADC conversion results
 * @param results Array to store conversion results
 * @param channel_count Number of channels to read
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_AdcResultsRead(uint16_t *results, uint8_t channel_count);

/**
 * @brief Check if ADC conversion sequence is complete
 * @return True if conversion complete
 */
bool MMM_HAL_AdcIsConversionComplete(void);

/**
 * @brief Configure ADC sampling parameters
 * @param sample_time Sample time in ADC clock cycles
 * @param resolution ADC resolution in bits
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_AdcConfigureSampling(uint16_t sample_time, uint8_t resolution);

// =============================================================================
// DMA HARDWARE ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Configure DMA channel for memory-to-memory transfer
 * @param channel DMA channel number
 * @param src_addr Source address
 * @param dest_addr Destination address
 * @param transfer_size Transfer size in bytes
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_DmaConfigureMemToMem(uint8_t channel, void *src_addr, void *dest_addr, size_t transfer_size);

/**
 * @brief Configure DMA channel for peripheral-to-memory transfer
 * @param channel DMA channel number
 * @param periph_addr Peripheral address
 * @param mem_addr Memory address
 * @param transfer_size Transfer size in bytes
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_DmaConfigurePeriphToMem(uint8_t channel, void *periph_addr, void *mem_addr, size_t transfer_size);

/**
 * @brief Start DMA transfer
 * @param channel DMA channel number
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_DmaTransferStart(uint8_t channel);

/**
 * @brief Stop DMA transfer
 * @param channel DMA channel number
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_DmaTransferStop(uint8_t channel);

/**
 * @brief Check if DMA transfer is complete
 * @param channel DMA channel number
 * @return True if transfer complete
 */
bool MMM_HAL_DmaIsTransferComplete(uint8_t channel);

// =============================================================================
// TIMER HARDWARE ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Configure timer for PWM generation
 * @param timer_id Timer identifier
 * @param frequency PWM frequency in Hz
 * @param resolution PWM resolution in bits
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_TimerConfigurePwm(uint8_t timer_id, uint32_t frequency, uint8_t resolution);

/**
 * @brief Configure timer for input capture
 * @param timer_id Timer identifier
 * @param capture_edge Capture edge (rising/falling/both)
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_TimerConfigureCapture(uint8_t timer_id, uint8_t capture_edge);

/**
 * @brief Start timer operation
 * @param timer_id Timer identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_TimerStart(uint8_t timer_id);

/**
 * @brief Stop timer operation
 * @param timer_id Timer identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_TimerStop(uint8_t timer_id);

/**
 * @brief Get timer counter value
 * @param timer_id Timer identifier
 * @param counter_value Pointer to store counter value
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_TimerGetCounter(uint8_t timer_id, uint32_t *counter_value);

// =============================================================================
// COMMUNICATION HARDWARE ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Configure UART communication parameters
 * @param uart_id UART identifier
 * @param baudrate Baud rate
 * @param data_bits Number of data bits
 * @param parity Parity setting
 * @param stop_bits Number of stop bits
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_UartConfigure(uint8_t uart_id, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);

/**
 * @brief Send data via UART
 * @param uart_id UART identifier
 * @param data Data buffer to send
 * @param length Data length
 * @return Number of bytes sent or negative error code
 */
int MMM_HAL_UartSendData(uint8_t uart_id, const uint8_t *data, size_t length);

/**
 * @brief Receive data via UART
 * @param uart_id UART identifier
 * @param data Data buffer to receive into
 * @param max_length Maximum data length
 * @return Number of bytes received or negative error code
 */
int MMM_HAL_UartReceiveData(uint8_t uart_id, uint8_t *data, size_t max_length);

/**
 * @brief Check UART transmission status
 * @param uart_id UART identifier
 * @return True if transmission complete
 */
bool MMM_HAL_UartIsTxComplete(uint8_t uart_id);

/**
 * @brief Check UART receive data availability
 * @param uart_id UART identifier
 * @return True if data available
 */
bool MMM_HAL_UartIsRxDataAvailable(uint8_t uart_id);

// =============================================================================
// SYSTEM HARDWARE ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Configure system clock
 * @param clock_source Clock source selection
 * @param frequency Desired frequency in Hz
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_SystemClockConfigure(uint8_t clock_source, uint32_t frequency);

/**
 * @brief Get system clock frequency
 * @return Current system clock frequency in Hz
 */
uint32_t MMM_HAL_SystemClockGetFrequency(void);

/**
 * @brief Enable peripheral clock
 * @param peripheral_id Peripheral identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PeripheralClockEnable(uint8_t peripheral_id);

/**
 * @brief Disable peripheral clock
 * @param peripheral_id Peripheral identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PeripheralClockDisable(uint8_t peripheral_id);

/**
 * @brief Reset peripheral
 * @param peripheral_id Peripheral identifier
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_PeripheralReset(uint8_t peripheral_id);

/**
 * @brief Configure interrupt priority
 * @param interrupt_id Interrupt identifier
 * @param priority Interrupt priority (0-7)
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_InterruptSetPriority(uint8_t interrupt_id, uint8_t priority);

/**
 * @brief Enable global interrupts
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_InterruptGlobalEnable(void);

/**
 * @brief Disable global interrupts
 * @return Status code (0 = success, negative = error)
 */
int MMM_HAL_InterruptGlobalDisable(void);

// =============================================================================
// HARDWARE ACCESS CONTEXT FUNCTIONS
// =============================================================================

/**
 * @brief Get hardware access context
 * @return Pointer to hardware access context
 */
mmm_hal_context_t *mmm_hardware_access_get_context(void);

/**
 * @brief Reset hardware access error counter
 */
void mmm_hardware_access_reset_error_counter(void);

/**
 * @brief Get hardware access error count
 * @return Current error count
 */
uint32_t mmm_hardware_access_get_error_count(void);

/**
 * @brief Get hardware access operation count
 * @return Current operation count
 */
uint32_t mmm_hardware_access_get_operation_count(void);

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Fast PWM duty cycle set (inline for performance)
 * @param motor_id Motor identifier
 * @param duty_cycle Duty cycle value
 */
static inline void MMM_HAL_PwmSetDutyCycleFast(mmm_hal_motor_id_t motor_id, uint16_t duty_cycle)
{
    // Direct hardware register access for maximum performance
    // In real implementation, would write directly to PWM registers
    MMM_HAL_PwmSetDutyCyclesIdentical(motor_id, duty_cycle);
}

/**
 * @brief Fast ADC result read (inline for performance)
 * @param channel ADC channel
 * @return ADC result value
 */
static inline uint16_t MMM_HAL_AdcReadResultFast(uint8_t channel)
{
    // Direct hardware register access for maximum performance
    // In real implementation, would read directly from ADC result register
    uint16_t result = 0;
    if (channel < MMM_HAL_MAX_ADC_CHANNELS) {
        // Placeholder - would access hardware register directly
        result = 2048; // Mid-scale value
    }
    return result;
}

/**
 * @brief Fast GPIO set (inline for performance)
 * @param pin GPIO pin number
 */
static inline void MMM_HAL_GpioSetFast(uint8_t pin)
{
    // Direct hardware register access for maximum performance
    // In real implementation, would use bit manipulation on GPIO registers
    (void)pin; // Placeholder
}

/**
 * @brief Fast GPIO clear (inline for performance)
 * @param pin GPIO pin number
 */
static inline void MMM_HAL_GpioClearFast(uint8_t pin)
{
    // Direct hardware register access for maximum performance
    (void)pin; // Placeholder
}

#ifdef __cplusplus
}
#endif

#endif // MMM_HARDWARE_ACCESS_H
