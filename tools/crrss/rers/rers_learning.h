
/**
 * @file rers_learning.h
 * @brief RERS Active Learning System - Hierarchical and Priority-Based Learning
 */

#ifndef RERS_LEARNING_H
#define RERS_LEARNING_H

#include "rers.h"
#include "rers_replay.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bug priority levels
 */
typedef enum {
    RERS_PRIORITY_CRITICAL = 0,       /**< Critical (security, crashes) */
    RERS_PRIORITY_HIGH,               /**< High (data loss, corruption) */
    RERS_PRIORITY_MEDIUM,             /**< Medium (functional issues) */
    RERS_PRIORITY_LOW,                /**< Low (minor issues) */
    RERS_PRIORITY_COUNT
} rers_priority_t;

/**
 * @brief Learning hierarchy levels
 */
typedef enum {
    RERS_HIERARCHY_ERROR_TYPE = 0,    /**< Error type level */
    RERS_HIERARCHY_COMPONENT,         /**< Component level */
    RERS_HIERARCHY_SUBSYSTEM,         /**< Subsystem level */
    RERS_HIERARCHY_SYSTEM,            /**< System level */
    RERS_HIERARCHY_GLOBAL,            /**< Global level */
    RERS_HIERARCHY_COUNT
} rers_hierarchy_level_t;

/**
 * @brief Bug information for learning
 */
typedef struct {
    uint64_t bug_id;                  /**< Unique bug identifier */
    rers_error_type_t error_type;     /**< Associated error type */
    rers_priority_t priority;         /**< Bug priority */
    rers_hierarchy_level_t level;     /**< Hierarchy level */
    const char *component;            /**< Component name */
    const char *description;          /**< Bug description */
    uint32_t occurrence_count;        /**< Number of occurrences */
    uint64_t first_seen;              /**< First occurrence timestamp */
    uint64_t last_seen;               /**< Last occurrence timestamp */
} rers_bug_info_t;

/**
 * @brief Learning configuration
 */
typedef struct {
    size_t max_hierarchy_depth;       /**< Maximum hierarchy depth */
    uint32_t priority_levels;         /**< Number of priority levels */
    uint32_t learning_threshold;      /**< Min occurrences for learning */
} rers_learning_config_t;

/**
 * @brief Learning system handle (opaque)
 */
typedef struct rers_learning_system rers_learning_system_t;

/**
 * @brief Initialize learning system
 * 
 * @param config Configuration structure
 * @param system Output parameter for system handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_learning_init(const rers_learning_config_t *config,
                                rers_learning_system_t **system);

/**
 * @brief Shutdown learning system
 * 
 * @param system System handle
 */
void rers_learning_shutdown(rers_learning_system_t *system);

/**
 * @brief Learn from a new bug
 * 
 * @param system System handle
 * @param bug_info Bug information
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_learning_learn(rers_learning_system_t *system,
                                 const rers_bug_info_t *bug_info);

/**
 * @brief Get bugs by priority
 * 
 * @param system System handle
 * @param priority Priority level
 * @param bugs Output array for bug IDs
 * @param max_bugs Maximum number of bugs to return
 * @param count Output parameter for actual count
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_learning_get_by_priority(rers_learning_system_t *system,
                                           rers_priority_t priority,
                                           uint64_t *bugs,
                                           size_t max_bugs,
                                           size_t *count);

/**
 * @brief Get bugs by hierarchy level
 * 
 * @param system System handle
 * @param level Hierarchy level
 * @param bugs Output array for bug IDs
 * @param max_bugs Maximum number of bugs to return
 * @param count Output parameter for actual count
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_learning_get_by_level(rers_learning_system_t *system,
                                        rers_hierarchy_level_t level,
                                        uint64_t *bugs,
                                        size_t max_bugs,
                                        size_t *count);

/**
 * @brief Get total number of learned bugs
 * 
 * @param system System handle
 * @return Number of learned bugs
 */
size_t rers_learning_get_count(rers_learning_system_t *system);

/**
 * @brief Get priority name string
 * 
 * @param priority Priority level
 * @return Priority name string
 */
const char *rers_learning_get_priority_name(rers_priority_t priority);

/**
 * @brief Get hierarchy level name string
 * 
 * @param level Hierarchy level
 * @return Level name string
 */
const char *rers_learning_get_level_name(rers_hierarchy_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* RERS_LEARNING_H */
