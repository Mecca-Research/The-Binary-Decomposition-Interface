
/**
 * @file phase1_init.h
 * @brief Phase 1 initialization and integration
 */

#ifndef PHASE1_INIT_H
#define PHASE1_INIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Phase 1 configuration
typedef struct {
    uint32_t num_cores;
    size_t ring_capacity;
    size_t arena_size;
    size_t fiber_stack_size;
    bool enable_performance_counters;
} phase1_config_t;

// Phase 1 status
typedef enum {
    PHASE1_SUCCESS = 0,
    PHASE1_ERROR_INVALID_CONFIG = -1,
    PHASE1_ERROR_INITIALIZATION = -2,
    PHASE1_ERROR_NOT_INITIALIZED = -3
} phase1_status_t;

/**
 * @brief Initialize Phase 1 performance foundation
 * 
 * @param config Configuration
 * @return Status code
 */
phase1_status_t phase1_init(const phase1_config_t* config);

/**
 * @brief Shutdown Phase 1
 * 
 * @return Status code
 */
phase1_status_t phase1_shutdown(void);

/**
 * @brief Check if Phase 1 is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool phase1_is_initialized(void);

/**
 * @brief Get default configuration
 * 
 * @return Default configuration
 */
phase1_config_t phase1_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_INIT_H
