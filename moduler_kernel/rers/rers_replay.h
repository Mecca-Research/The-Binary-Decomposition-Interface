
/**
 * @file rers_replay.h
 * @brief RERS Error Replay Engine - Handles multiple error types
 * 
 * Supports: segfaults, assertions, memory leaks, logic errors
 */

#ifndef RERS_REPLAY_H
#define RERS_REPLAY_H

#include "rers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error types supported by replay engine
 */
typedef enum {
    RERS_ERROR_TYPE_SEGFAULT = 0,     /**< Segmentation fault */
    RERS_ERROR_TYPE_ASSERTION,        /**< Assertion failure */
    RERS_ERROR_TYPE_MEMORY_LEAK,      /**< Memory leak */
    RERS_ERROR_TYPE_LOGIC_ERROR,      /**< Logic error */
    RERS_ERROR_TYPE_BUFFER_OVERFLOW,  /**< Buffer overflow */
    RERS_ERROR_TYPE_NULL_DEREF,       /**< NULL dereference */
    RERS_ERROR_TYPE_USE_AFTER_FREE,   /**< Use after free */
    RERS_ERROR_TYPE_DOUBLE_FREE,      /**< Double free */
    RERS_ERROR_TYPE_CUSTOM,           /**< Custom error type */
    RERS_ERROR_TYPE_COUNT
} rers_error_type_t;

/**
 * @brief Error context information
 */
typedef struct {
    rers_error_type_t type;           /**< Error type */
    const char *file;                 /**< Source file */
    int line;                         /**< Line number */
    const char *function;             /**< Function name */
    const char *message;              /**< Error message */
    void *context_data;               /**< Additional context */
    size_t context_size;              /**< Context data size */
    uint64_t timestamp;               /**< Error timestamp */
} rers_error_context_t;

/**
 * @brief Replay configuration
 */
typedef struct {
    size_t max_depth;                 /**< Maximum replay depth */
    bool enable_segfault;             /**< Handle segfaults */
    bool enable_assertion;            /**< Handle assertions */
    bool enable_memory_leak;          /**< Handle memory leaks */
    bool enable_logic_error;          /**< Handle logic errors */
} rers_replay_config_t;

/**
 * @brief Replay engine handle (opaque)
 */
typedef struct rers_replay_engine rers_replay_engine_t;

/**
 * @brief Initialize replay engine
 * 
 * @param config Configuration structure
 * @param engine Output parameter for engine handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_replay_init(const rers_replay_config_t *config,
                               rers_replay_engine_t **engine);

/**
 * @brief Shutdown replay engine
 * 
 * @param engine Engine handle
 */
void rers_replay_shutdown(rers_replay_engine_t *engine);

/**
 * @brief Record an error for replay
 * 
 * @param engine Engine handle
 * @param context Error context
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_replay_record(rers_replay_engine_t *engine,
                                const rers_error_context_t *context);

/**
 * @brief Replay a recorded error
 * 
 * @param engine Engine handle
 * @param error_id Error identifier to replay
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_replay_execute(rers_replay_engine_t *engine,
                                 uint64_t error_id);

/**
 * @brief Get number of recorded errors
 * 
 * @param engine Engine handle
 * @return Number of recorded errors
 */
size_t rers_replay_get_count(rers_replay_engine_t *engine);

/**
 * @brief Get error type name string
 * 
 * @param type Error type
 * @return Type name string
 */
const char *rers_replay_get_type_name(rers_error_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* RERS_REPLAY_H */
