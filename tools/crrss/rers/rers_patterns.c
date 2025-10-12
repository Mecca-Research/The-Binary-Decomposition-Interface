
/**
 * @file rers_patterns.c
 * @brief RERS Bug Pattern Database Implementation
 */

#include "rers_patterns.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_PATTERNS 1024
#define MAX_SIGNATURE_LEN 256
#define MAX_DESCRIPTION_LEN 512
#define MAX_FIX_SUGGESTION_LEN 512

/**
 * @brief Pattern entry
 */
typedef struct {
    rers_pattern_t pattern;
    char signature_copy[MAX_SIGNATURE_LEN];
    char description_copy[MAX_DESCRIPTION_LEN];
    char fix_suggestion_copy[MAX_FIX_SUGGESTION_LEN];
    bool valid;
} rers_pattern_entry_t;

/**
 * @brief Pattern database structure
 */
struct rers_pattern_db {
    rers_pattern_config_t config;
    rers_pattern_entry_t patterns[MAX_PATTERNS];
    size_t pattern_count;
    uint64_t next_pattern_id;
};

/* Match confidence names */
static const char *confidence_names[] = {
    "Exact",
    "High",
    "Medium",
    "Low",
    "None"
};

/**
 * @brief Calculate similarity between two strings (simple algorithm)
 */
static float calculate_similarity(const char *str1, const char *str2) {
    if (!str1 || !str2) {
        return 0.0f;
    }

    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    
    if (len1 == 0 && len2 == 0) {
        return 1.0f;
    }
    
    if (len1 == 0 || len2 == 0) {
        return 0.0f;
    }

    /* Simple substring matching */
    size_t matches = 0;
    size_t max_len = len1 > len2 ? len1 : len2;
    
    for (size_t i = 0; i < len1 && i < len2; i++) {
        if (str1[i] == str2[i]) {
            matches++;
        }
    }
    
    return (float)matches / (float)max_len;
}

/* Initialize pattern database */
rers_error_t rers_pattern_init(const rers_pattern_config_t *config,
                               rers_pattern_db_t **db) {
    if (!config || !db) {
        return RERS_ERROR_INVALID_PARAM;
    }

    rers_pattern_db_t *database = calloc(1, sizeof(rers_pattern_db_t));
    if (!database) {
        return RERS_ERROR_NO_MEMORY;
    }

    database->config = *config;
    database->pattern_count = 0;
    database->next_pattern_id = 1;

    *db = database;
    return RERS_SUCCESS;
}

/* Shutdown pattern database */
void rers_pattern_shutdown(rers_pattern_db_t *db) {
    if (!db) {
        return;
    }
    free(db);
}

/* Add a pattern to the database */
rers_error_t rers_pattern_add(rers_pattern_db_t *db,
                              const rers_pattern_t *pattern,
                              uint64_t *pattern_id) {
    if (!db || !pattern) {
        return RERS_ERROR_INVALID_PARAM;
    }

    /* Check if we have space */
    if (db->pattern_count >= MAX_PATTERNS) {
        return RERS_ERROR_COMPONENT_FAILED;
    }

    /* Create pattern entry */
    rers_pattern_entry_t *entry = &db->patterns[db->pattern_count];
    entry->pattern = *pattern;
    entry->pattern.pattern_id = db->next_pattern_id++;
    entry->pattern.match_count = 0;
    entry->pattern.created_at = (uint64_t)time(NULL);
    entry->valid = true;

    /* Copy signature */
    if (pattern->signature) {
        strncpy(entry->signature_copy, pattern->signature,
                sizeof(entry->signature_copy) - 1);
        entry->signature_copy[sizeof(entry->signature_copy) - 1] = '\0';
        entry->pattern.signature = entry->signature_copy;
    }

    /* Copy description */
    if (pattern->description) {
        strncpy(entry->description_copy, pattern->description,
                sizeof(entry->description_copy) - 1);
        entry->description_copy[sizeof(entry->description_copy) - 1] = '\0';
        entry->pattern.description = entry->description_copy;
    }

    /* Copy fix suggestion */
    if (pattern->fix_suggestion) {
        strncpy(entry->fix_suggestion_copy, pattern->fix_suggestion,
                sizeof(entry->fix_suggestion_copy) - 1);
        entry->fix_suggestion_copy[sizeof(entry->fix_suggestion_copy) - 1] = '\0';
        entry->pattern.fix_suggestion = entry->fix_suggestion_copy;
    }

    if (pattern_id) {
        *pattern_id = entry->pattern.pattern_id;
    }

    db->pattern_count++;
    return RERS_SUCCESS;
}

/* Match an error against patterns */
rers_error_t rers_pattern_match(rers_pattern_db_t *db,
                                const rers_error_context_t *error_context,
                                rers_match_result_t *result) {
    if (!db || !error_context || !result) {
        return RERS_ERROR_INVALID_PARAM;
    }

    /* Initialize result */
    memset(result, 0, sizeof(rers_match_result_t));
    result->confidence = RERS_MATCH_NONE;
    result->similarity_score = 0.0f;

    float best_score = 0.0f;
    rers_pattern_entry_t *best_match = NULL;

    /* Search for matching patterns */
    for (size_t i = 0; i < db->pattern_count; i++) {
        if (!db->patterns[i].valid) {
            continue;
        }

        rers_pattern_entry_t *entry = &db->patterns[i];
        
        /* Check error type match */
        if (entry->pattern.error_type != error_context->type) {
            continue;
        }

        /* Calculate signature similarity */
        float score = 0.0f;
        if (entry->pattern.signature && error_context->message) {
            score = calculate_similarity(entry->pattern.signature, 
                                        error_context->message);
        }

        /* Also consider function name matching */
        if (entry->pattern.signature && error_context->function) {
            float func_score = calculate_similarity(entry->pattern.signature,
                                                   error_context->function);
            score = (score + func_score) / 2.0f;
        }

        /* Update best match */
        if (score > best_score) {
            best_score = score;
            best_match = entry;
        }
    }

    /* Set result if match found */
    if (best_match && best_score > 0.0f) {
        result->pattern_id = best_match->pattern.pattern_id;
        result->similarity_score = best_score;
        result->pattern_description = best_match->pattern.description;
        result->fix_suggestion = best_match->pattern.fix_suggestion;

        /* Determine confidence level */
        if (best_score >= 0.95f) {
            result->confidence = RERS_MATCH_EXACT;
        } else if (best_score >= 0.75f) {
            result->confidence = RERS_MATCH_HIGH;
        } else if (best_score >= 0.50f) {
            result->confidence = RERS_MATCH_MEDIUM;
        } else if (best_score >= 0.25f) {
            result->confidence = RERS_MATCH_LOW;
        }

        /* Update pattern match count */
        best_match->pattern.match_count++;
        
        return RERS_SUCCESS;
    }

    return RERS_ERROR_PATTERN_NOT_FOUND;
}

/* Get pattern by ID */
rers_error_t rers_pattern_get(rers_pattern_db_t *db,
                              uint64_t pattern_id,
                              rers_pattern_t *pattern) {
    if (!db || !pattern) {
        return RERS_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; i < db->pattern_count; i++) {
        if (db->patterns[i].valid && 
            db->patterns[i].pattern.pattern_id == pattern_id) {
            *pattern = db->patterns[i].pattern;
            return RERS_SUCCESS;
        }
    }

    return RERS_ERROR_PATTERN_NOT_FOUND;
}

/* Get number of patterns in database */
size_t rers_pattern_get_count(rers_pattern_db_t *db) {
    if (!db) {
        return 0;
    }
    return db->pattern_count;
}

/* Clear all patterns from database */
rers_error_t rers_pattern_clear(rers_pattern_db_t *db) {
    if (!db) {
        return RERS_ERROR_INVALID_PARAM;
    }

    memset(db->patterns, 0, sizeof(db->patterns));
    db->pattern_count = 0;
    return RERS_SUCCESS;
}

/* Get match confidence name string */
const char *rers_pattern_get_confidence_name(rers_match_confidence_t confidence) {
    if (confidence >= 0 && confidence < RERS_MATCH_COUNT) {
        return confidence_names[confidence];
    }
    return "Unknown";
}
