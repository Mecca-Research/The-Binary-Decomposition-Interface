/**
 * @file master_memory_manager.h
 * @brief Master Memory Manager Core Header
 * CRITICAL FIX: Core MMM header definitions
 */

#ifndef MASTER_MEMORY_MANAGER_H
#define MASTER_MEMORY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return codes
typedef enum {
    MMM_SUCCESS = 0,
    MMM_ERROR_INVALID_PARAM = -1,
    MMM_ERROR_NOT_INITIALIZED = -2,
    MMM_ERROR_MEMORY_ALLOCATION = -3,
    MMM_ERROR_SYSTEM_FAILURE = -4
} mmm_status_t;

// Configuration structure
typedef struct {
    bool enable_x86_core;
    bool enable_hal_framework;
    bool enable_debug_mode;
    bool enable_performance_opt;
    size_t memory_pool_size;
    size_t tlb_cache_size;
    size_t page_size;
} mmm_config_t;

// Master control structure
typedef struct {
    uint64_t system_id;
    uint64_t total_memory;
    uint32_t active_components;
    uint32_t total_components;
    bool initialized;
} mmm_master_control_t;

// System status structure
typedef struct {
    uint32_t active_components;
    uint32_t total_components;
    double performance_score;
    uint64_t total_memory_allocated;
} mmm_system_status_t;

// Function declarations
mmm_status_t mmm_initialize(const mmm_config_t* config);
mmm_status_t mmm_shutdown(void);
bool mmm_is_initialized(void);
const char* mmm_status_to_string(mmm_status_t status);
void mmm_get_version_info(char* buffer, size_t buffer_size);
mmm_status_t mmm_health_check(void);

#ifdef __cplusplus
}
#endif

#endif // MASTER_MEMORY_MANAGER_H
