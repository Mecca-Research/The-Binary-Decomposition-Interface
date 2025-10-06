
/**
 * @file fault_injection.h
 * @brief Fault Injection Framework for Red-Team Testing
 * @details Provides configurable fault injection for testing error handling
 *          and recovery mechanisms in the memory subsystem.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H

#include "../../../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Fault Types
// ============================================================================

typedef enum {
    FAULT_NONE = 0,
    FAULT_ALLOC_FAIL,           // Allocation failure
    FAULT_NUMA_FAIL,            // NUMA allocation failure
    FAULT_PAGE_FAULT,           // Page fault
    FAULT_OOM,                  // Out of memory
    FAULT_CORRUPTION,           // Memory corruption
    FAULT_DOUBLE_FREE,          // Double free
    FAULT_INVALID_FREE,         // Invalid free
    FAULT_ALIGNMENT_ERROR,      // Alignment error
    FAULT_TIMEOUT,              // Operation timeout
    FAULT_DEADLOCK,             // Deadlock
    FAULT_RACE_CONDITION,       // Race condition
    FAULT_MAX
} fault_type_t;

// ============================================================================
// Fault Configuration
// ============================================================================

typedef struct {
    fault_type_t type;
    double probability;         // 0.0 to 1.0
    uint32_t trigger_count;     // Trigger after N operations
    uint32_t max_faults;        // Maximum faults to inject
    bool enabled;
    const char *description;
} fault_config_t;

// ============================================================================
// Fault Statistics
// ============================================================================

typedef struct {
    uint64_t total_checks;
    uint64_t faults_injected;
    uint64_t faults_by_type[FAULT_MAX];
    uint64_t operations_affected;
} fault_stats_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize fault injection framework
 * @param seed Random seed for reproducibility
 */
void fault_injection_init(uint32_t seed);

/**
 * @brief Cleanup fault injection framework
 */
void fault_injection_cleanup(void);

/**
 * @brief Enable fault injection
 * @param type Fault type to enable
 * @param probability Probability of fault (0.0 to 1.0)
 * @param description Optional description
 * @return true on success
 */
bool fault_injection_enable(fault_type_t type, double probability, 
                           const char *description);

/**
 * @brief Disable fault injection
 * @param type Fault type to disable
 */
void fault_injection_disable(fault_type_t type);

/**
 * @brief Disable all fault injection
 */
void fault_injection_disable_all(void);

/**
 * @brief Check if a fault should be injected
 * @param type Fault type to check
 * @return true if fault should be injected
 */
bool fault_injection_should_fail(fault_type_t type);

/**
 * @brief Set trigger count for fault
 * @param type Fault type
 * @param count Trigger after N operations
 */
void fault_injection_set_trigger(fault_type_t type, uint32_t count);

/**
 * @brief Set maximum faults for type
 * @param type Fault type
 * @param max Maximum faults to inject
 */
void fault_injection_set_max_faults(fault_type_t type, uint32_t max);

/**
 * @brief Get fault statistics
 * @return Fault statistics
 */
fault_stats_t fault_injection_get_stats(void);

/**
 * @brief Reset fault statistics
 */
void fault_injection_reset_stats(void);

/**
 * @brief Print fault statistics
 */
void fault_injection_print_stats(void);

/**
 * @brief Get fault type name
 * @param type Fault type
 * @return Fault type name string
 */
const char *fault_injection_get_type_name(fault_type_t type);

// ============================================================================
// Convenience Macros
// ============================================================================

#define INJECT_FAULT(type) fault_injection_should_fail(type)

#define INJECT_ALLOC_FAULT() INJECT_FAULT(FAULT_ALLOC_FAIL)
#define INJECT_NUMA_FAULT() INJECT_FAULT(FAULT_NUMA_FAIL)
#define INJECT_OOM_FAULT() INJECT_FAULT(FAULT_OOM)

#endif // FAULT_INJECTION_H
