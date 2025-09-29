/**
 * @file mmm_common.h
 * @brief Common definitions for Master Memory Manager Phase 4
 * CRITICAL FIX: Shared enums and constants to avoid conflicts
 */

#ifndef MMM_COMMON_H
#define MMM_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common return codes
#define MMM_SUCCESS 0
#define MMM_ERROR_INVALID_PARAM -1
#define MMM_ERROR_NOT_INITIALIZED -2
#define MMM_ERROR_MEMORY_ALLOCATION -3
#define MMM_ERROR_SYSTEM_FAILURE -4

// Common priority levels (shared across all components)
typedef enum {
    MMM_PRIORITY_LOW = 1,
    MMM_PRIORITY_NORMAL,
    MMM_PRIORITY_HIGH,
    MMM_PRIORITY_CRITICAL
} mmm_priority_t;

// Common optimization levels
typedef enum {
    MMM_OPT_LEVEL_NONE = 0,
    MMM_OPT_LEVEL_BASIC = 1,
    MMM_OPT_LEVEL_STANDARD = 2,
    MMM_OPT_LEVEL_AGGRESSIVE = 3,
    MMM_OPT_LEVEL_MAXIMUM = 4
} mmm_optimization_level_t;

// Common optimization types
typedef enum {
    MMM_OPT_MEMORY_LAYOUT = 1,
    MMM_OPT_CACHE_OPTIMIZATION = 2,
    MMM_OPT_TASK_SCHEDULING = 3,
    MMM_OPT_POWER_MANAGEMENT = 4
} mmm_optimization_type_t;

#ifdef __cplusplus
}
#endif

#endif // MMM_COMMON_H
