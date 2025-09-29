
/**
 * @file mmm_memory_protection.h
 * @brief Master Memory Manager Memory Protection Interface
 * 
 * Provides comprehensive memory protection including:
 * - Memory access control
 * - Privilege level enforcement
 * - Memory region protection
 * - Access violation detection
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_MEMORY_PROTECTION_H
#define MMM_MEMORY_PROTECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory protection context
 */
typedef struct {
    bool initialized;
    uint32_t violation_count;
} mmm_memory_protection_context_t;

/**
 * @brief Initialize memory protection system
 * @return Status code (0 = success, negative = error)
 */
int mmm_memory_protection_initialize(void);

/**
 * @brief Shutdown memory protection system
 * @return Status code (0 = success, negative = error)
 */
int mmm_memory_protection_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_MEMORY_PROTECTION_H
