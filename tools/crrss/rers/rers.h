
/**
 * @file rers.h
 * @brief Enhanced Runtime Error Replay System (RERS) - Main Interface
 * 
 * RERS provides comprehensive error replay, active learning, pattern matching,
 * and integration with CRRSS personality profiles (MSM, STP, BPME, TDT).
 * 
 * @version 1.0
 * @date 2025-10-12
 */

#ifndef RERS_H
#define RERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version Information */
#define RERS_VERSION_MAJOR 1
#define RERS_VERSION_MINOR 0
#define RERS_VERSION_PATCH 0

/* Configuration Constants */
#define RERS_MAX_ERROR_TYPES 16
#define RERS_MAX_PATTERNS 1024
#define RERS_MAX_PROFILES 4

/**
 * @brief RERS component identifiers
 */
typedef enum {
    RERS_COMPONENT_REPLAY = 0,     /**< Error Replay Engine */
    RERS_COMPONENT_LEARNING,       /**< Active Learning System */
    RERS_COMPONENT_PATTERNS,       /**< Bug Pattern Database */
    RERS_COMPONENT_INTEGRATION,    /**< Integration Layer */
    RERS_COMPONENT_MAX
} rers_component_t;

/**
 * @brief RERS error codes
 */
typedef enum {
    RERS_SUCCESS = 0,              /**< Operation successful */
    RERS_ERROR_INVALID_PARAM,      /**< Invalid parameter */
    RERS_ERROR_NO_MEMORY,          /**< Memory allocation failed */
    RERS_ERROR_NOT_INITIALIZED,    /**< RERS not initialized */
    RERS_ERROR_ALREADY_INITIALIZED,/**< RERS already initialized */
    RERS_ERROR_COMPONENT_FAILED,   /**< Component operation failed */
    RERS_ERROR_PATTERN_NOT_FOUND,  /**< Pattern not found */
    RERS_ERROR_REPLAY_FAILED,      /**< Replay operation failed */
    RERS_ERROR_MAX
} rers_error_t;

/**
 * @brief RERS configuration structure
 */
typedef struct {
    bool enable_replay;            /**< Enable error replay */
    bool enable_learning;          /**< Enable active learning */
    bool enable_patterns;          /**< Enable pattern matching */
    bool enable_integration;       /**< Enable profile integration */
    size_t max_patterns;           /**< Maximum patterns to store */
    size_t max_replay_depth;       /**< Maximum replay depth */
    uint32_t learning_threshold;   /**< Learning activation threshold */
} rers_config_t;

/**
 * @brief RERS statistics structure
 */
typedef struct {
    uint64_t errors_detected;      /**< Total errors detected */
    uint64_t errors_replayed;      /**< Errors successfully replayed */
    uint64_t patterns_learned;     /**< Patterns learned */
    uint64_t patterns_matched;     /**< Successful pattern matches */
    uint64_t integrations_executed;/**< Profile integrations executed */
} rers_stats_t;

/**
 * @brief RERS system handle (opaque)
 */
typedef struct rers_system rers_system_t;

/**
 * @brief Initialize RERS system
 * 
 * @param config Configuration structure (NULL for defaults)
 * @param system Output parameter for system handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_init(const rers_config_t *config, rers_system_t **system);

/**
 * @brief Shutdown RERS system
 * 
 * @param system System handle to shutdown
 */
void rers_shutdown(rers_system_t *system);

/**
 * @brief Get RERS statistics
 * 
 * @param system System handle
 * @param stats Output parameter for statistics
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_get_stats(rers_system_t *system, rers_stats_t *stats);

/**
 * @brief Reset RERS statistics
 * 
 * @param system System handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_reset_stats(rers_system_t *system);

/**
 * @brief Get RERS version string
 * 
 * @return Version string (e.g., "1.0.0")
 */
const char *rers_get_version(void);

/**
 * @brief Get error description string
 * 
 * @param error Error code
 * @return Human-readable error description
 */
const char *rers_get_error_string(rers_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* RERS_H */
