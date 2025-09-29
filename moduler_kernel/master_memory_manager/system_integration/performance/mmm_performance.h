
/**
 * @file mmm_performance.h
 * @brief Master Memory Manager Performance Optimization Interface
 * 
 * Provides comprehensive performance optimization including:
 * - Performance monitoring and profiling
 * - Optimization strategies
 * - Resource utilization tracking
 * - Bottleneck identification
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef MMM_PERFORMANCE_H
#define MMM_PERFORMANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Performance context
 */
typedef struct {
    bool initialized;
    uint64_t operation_count;
    uint64_t total_cycles;
} mmm_performance_context_t;

/**
 * @brief Initialize performance optimization system
 * @return Status code (0 = success, negative = error)
 */
int mmm_performance_initialize(void);

/**
 * @brief Shutdown performance optimization system
 * @return Status code (0 = success, negative = error)
 */
int mmm_performance_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // MMM_PERFORMANCE_H
