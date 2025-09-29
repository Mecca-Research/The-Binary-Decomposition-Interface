
/**
 * @file mmm_interrupt_mgmt.h
 * @brief Master Memory Manager Interrupt Management Interface
 * 
 * Provides comprehensive interrupt management including:
 * - Interrupt controller abstraction
 * - ISR registration and handling
 * - Interrupt priority management
 * - Nested interrupt support
 * - Performance monitoring
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_INTERRUPT_MGMT_H
#define MMM_INTERRUPT_MGMT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// INTERRUPT MANAGEMENT CONSTANTS
// =============================================================================

#define MMM_INT_MAX_INTERRUPTS      256     ///< Maximum number of interrupts
#define MMM_INT_MAX_PRIORITY        15      ///< Maximum interrupt priority
#define MMM_INT_MAX_NESTING_LEVEL   8       ///< Maximum interrupt nesting level

/**
 * @brief Interrupt handler function pointer type
 */
typedef void (*mmm_interrupt_handler_t)(void *context);

/**
 * @brief Interrupt descriptor
 */
typedef struct {
    uint8_t interrupt_id;               ///< Interrupt identifier
    uint8_t priority;                   ///< Interrupt priority (0-15)
    bool enabled;                       ///< Interrupt enabled status
    mmm_interrupt_handler_t handler;    ///< Interrupt handler function
    void *context;                      ///< Handler context data
    uint64_t call_count;                ///< Number of times called
    uint64_t total_cycles;              ///< Total execution cycles
    uint32_t max_cycles;                ///< Maximum execution cycles
    const char *name;                   ///< Interrupt name
} mmm_interrupt_descriptor_t;

/**
 * @brief Interrupt management context
 */
typedef struct {
    mmm_interrupt_descriptor_t interrupts[MMM_INT_MAX_INTERRUPTS];
    uint8_t current_priority;           ///< Current interrupt priority
    uint8_t nesting_level;              ///< Current nesting level
    bool global_enabled;                ///< Global interrupt enable status
    uint64_t total_interrupts;          ///< Total interrupt count
    uint32_t max_nesting_reached;       ///< Maximum nesting level reached
    bool initialized;                   ///< Initialization status
} mmm_interrupt_context_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize interrupt management system
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_mgmt_initialize(void);

/**
 * @brief Shutdown interrupt management system
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_mgmt_shutdown(void);

/**
 * @brief Register interrupt handler
 * @param interrupt_id Interrupt identifier
 * @param priority Interrupt priority
 * @param handler Handler function
 * @param context Handler context
 * @param name Interrupt name
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_register_handler(uint8_t interrupt_id, uint8_t priority, 
                                   mmm_interrupt_handler_t handler, void *context, const char *name);

/**
 * @brief Unregister interrupt handler
 * @param interrupt_id Interrupt identifier
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_unregister_handler(uint8_t interrupt_id);

/**
 * @brief Enable interrupt
 * @param interrupt_id Interrupt identifier
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_enable(uint8_t interrupt_id);

/**
 * @brief Disable interrupt
 * @param interrupt_id Interrupt identifier
 * @return Status code (0 = success, negative = error)
 */
int mmm_interrupt_disable(uint8_t interrupt_id);

/**
 * @brief Get interrupt management context
 * @return Pointer to interrupt context
 */
mmm_interrupt_context_t *mmm_interrupt_get_context(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_INTERRUPT_MGMT_H
