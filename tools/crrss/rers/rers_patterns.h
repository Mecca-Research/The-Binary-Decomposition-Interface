
/**
 * @file rers_patterns.h
 * @brief RERS Bug Pattern Database - In-Memory Pattern Storage and Matching
 */

#ifndef RERS_PATTERNS_H
#define RERS_PATTERNS_H

#include "rers.h"
#include "rers_replay.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pattern match confidence levels
 */
typedef enum {
    RERS_MATCH_EXACT = 0,             /**< Exact match */
    RERS_MATCH_HIGH,                  /**< High confidence */
    RERS_MATCH_MEDIUM,                /**< Medium confidence */
    RERS_MATCH_LOW,                   /**< Low confidence */
    RERS_MATCH_NONE,                  /**< No match */
    RERS_MATCH_COUNT
} rers_match_confidence_t;

/**
 * @brief Bug pattern structure
 */
typedef struct {
    uint64_t pattern_id;              /**< Unique pattern identifier */
    rers_error_type_t error_type;     /**< Error type */
    const char *signature;            /**< Pattern signature */
    const char *description;          /**< Pattern description */
    const char *fix_suggestion;       /**< Suggested fix */
    uint32_t match_count;             /**< Number of matches */
    uint64_t created_at;              /**< Creation timestamp */
} rers_pattern_t;

/**
 * @brief Pattern match result
 */
typedef struct {
    uint64_t pattern_id;              /**< Matched pattern ID */
    rers_match_confidence_t confidence; /**< Match confidence */
    float similarity_score;           /**< Similarity score (0.0-1.0) */
    const char *pattern_description;  /**< Pattern description */
    const char *fix_suggestion;       /**< Suggested fix */
} rers_match_result_t;

/**
 * @brief Pattern configuration
 */
typedef struct {
    size_t max_patterns;              /**< Maximum patterns to store */
    bool enable_fuzzy_match;          /**< Enable fuzzy matching */
} rers_pattern_config_t;

/**
 * @brief Pattern database handle (opaque)
 */
typedef struct rers_pattern_db rers_pattern_db_t;

/**
 * @brief Initialize pattern database
 * 
 * @param config Configuration structure
 * @param db Output parameter for database handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_pattern_init(const rers_pattern_config_t *config,
                               rers_pattern_db_t **db);

/**
 * @brief Shutdown pattern database
 * 
 * @param db Database handle
 */
void rers_pattern_shutdown(rers_pattern_db_t *db);

/**
 * @brief Add a pattern to the database
 * 
 * @param db Database handle
 * @param pattern Pattern to add
 * @param pattern_id Output parameter for assigned pattern ID
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_pattern_add(rers_pattern_db_t *db,
                              const rers_pattern_t *pattern,
                              uint64_t *pattern_id);

/**
 * @brief Match an error against patterns
 * 
 * @param db Database handle
 * @param error_context Error context to match
 * @param result Output parameter for match result
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_pattern_match(rers_pattern_db_t *db,
                                const rers_error_context_t *error_context,
                                rers_match_result_t *result);

/**
 * @brief Get pattern by ID
 * 
 * @param db Database handle
 * @param pattern_id Pattern identifier
 * @param pattern Output parameter for pattern
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_pattern_get(rers_pattern_db_t *db,
                              uint64_t pattern_id,
                              rers_pattern_t *pattern);

/**
 * @brief Get number of patterns in database
 * 
 * @param db Database handle
 * @return Number of patterns
 */
size_t rers_pattern_get_count(rers_pattern_db_t *db);

/**
 * @brief Clear all patterns from database
 * 
 * @param db Database handle
 * @return RERS_SUCCESS on success, error code otherwise
 */
rers_error_t rers_pattern_clear(rers_pattern_db_t *db);

/**
 * @brief Get match confidence name string
 * 
 * @param confidence Confidence level
 * @return Confidence name string
 */
const char *rers_pattern_get_confidence_name(rers_match_confidence_t confidence);

#ifdef __cplusplus
}
#endif

#endif /* RERS_PATTERNS_H */
