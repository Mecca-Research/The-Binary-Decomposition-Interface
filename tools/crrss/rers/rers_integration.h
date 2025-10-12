
/**
 * @file rers_integration.h
 * @brief RERS Integration Layer - Coordinates CRRSS Personality Profiles
 * 
 * Integrates with:
 * - MSM (Multi-State Machine)
 * - STP (Self-Testing Protocol)
 * - BPME (Bug Pattern Matching Engine)
 * - TDT (Test-Driven Thinking)
 */

#ifndef RERS_INTEGRATION_H
#define RERS_INTEGRATION_H

#include "rers.h"
#include "rers_replay.h"
#include "rers_learning.h"
#include "rers_patterns.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRRSS personality profile types
 */
typedef enum {
    RERS_PROFILE_MSM = 0,             /**< Multi-State Machine */
    RERS_PROFILE_STP,                 /**< Self-Testing Protocol */
    RERS_PROFILE_BPME,                /**< Bug Pattern Matching Engine */
    RERS_PROFILE_TDT,                 /**< Test-Driven Thinking */
    RERS_PROFILE_COUNT
} rers_profile_type_t;

/**
 * @brief Task types for profile coordination
 */
typedef enum {
    RERS_TASK_ERROR_ANALYSIS = 0,     /**< Analyze error */
    RERS_TASK_PATTERN_MATCHING,       /**< Match patterns */
    RERS_TASK_TEST_GENERATION,        /**< Generate tests */
    RERS_TASK_STATE_TRACKING,         /**< Track states */
    RERS_TASK_BUG_CLASSIFICATION,     /**< Classify bugs */
    RERS_TASK_COUNT
} rers_task_type_t;

/**
 * @brief Profile output data
 */
typedef struct {
    rers_profile_type_t profile;      /**< Profile type */
    rers_task_type_t task;            /**< Task type */
    void *data;                       /**< Output data */
    size_t data_size;                 /**< Data size */
    float confidence;                 /**< Confidence score (0.0-1.0) */
    uint64_t timestamp;               /**< Timestamp */
} rers_profile_output_t;

/**
 * @brief Coordination result
 */
typedef struct {
    rers_task_type_t task;            /**< Task type */
    rers_profile_type_t primary_profile; /**< Primary profile used */
    uint32_t profiles_used;           /**< Bitmask of profiles used */
    float overall_confidence;         /**< Overall confidence */
    const char *recommendation;       /**< Recommendation */
    void *result_data;                /**< Result data */
    size_t result_size;               /**< Result size */
} rers_coordination_result_t;

/**
 * @brief Integration configuration
 */
typedef struct {
    size_t max_profiles;              /**< Maximum profiles */
    bool enable_coordination;         /**< Enable coordination */
} rers_integration_config_t;

/**
 * @brief Integration layer handle (opaque)
 */
typedef struct rers_integration_layer rers_integration_layer_t;

/**
 * @brief Initialize integration layer
 * 
 * @param config Configuration structure
 * @param layer Output parameter for layer handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_integration_init(const rers_integration_config_t *config,
                                   rers_integration_layer_t **layer);

/**
 * @brief Shutdown integration layer
 * 
 * @param layer Layer handle
 */
void rers_integration_shutdown(rers_integration_layer_t *layer);

/**
 * @brief Submit profile output for coordination
 * 
 * @param layer Layer handle
 * @param output Profile output data
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_integration_submit_output(rers_integration_layer_t *layer,
                                            const rers_profile_output_t *output);

/**
 * @brief Coordinate profiles for a task
 * 
 * @param layer Layer handle
 * @param task Task type
 * @param result Output parameter for coordination result
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_integration_coordinate(rers_integration_layer_t *layer,
                                         rers_task_type_t task,
                                         rers_coordination_result_t *result);

/**
 * @brief Get active profiles for a task
 * 
 * @param layer Layer handle
 * @param task Task type
 * @param profiles Output array for profile types
 * @param max_profiles Maximum profiles to return
 * @param count Output parameter for actual count
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_integration_get_active_profiles(rers_integration_layer_t *layer,
                                                  rers_task_type_t task,
                                                  rers_profile_type_t *profiles,
                                                  size_t max_profiles,
                                                  size_t *count);

/**
 * @brief Enable/disable a profile
 * 
 * @param layer Layer handle
 * @param profile Profile type
 * @param enabled Enable flag
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_integration_set_profile_enabled(rers_integration_layer_t *layer,
                                                  rers_profile_type_t profile,
                                                  bool enabled);

/**
 * @brief Get profile type name string
 * 
 * @param profile Profile type
 * @return Profile name string
 */
const char *rers_integration_get_profile_name(rers_profile_type_t profile);

/**
 * @brief Get task type name string
 * 
 * @param task Task type
 * @return Task name string
 */
const char *rers_integration_get_task_name(rers_task_type_t task);

#ifdef __cplusplus
}
#endif

#endif /* RERS_INTEGRATION_H */
