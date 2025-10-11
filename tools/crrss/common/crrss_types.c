
/**
 * @file crrss_types.c
 * @brief Implementation of common CRRSS utility functions
 */

#include "crrss_types.h"

// String mappings for enums
const char* BUG_PRIORITY_STRINGS[] = {
    "P0-CRITICAL",
    "P1-HIGH",
    "P2-MEDIUM",
    "P3-LOW",
    "UNKNOWN"
};

const char* BUG_CATEGORY_STRINGS[] = {
    "MEMORY",
    "CONCURRENCY",
    "LOGIC",
    "PERFORMANCE",
    "SECURITY",
    "API",
    "BUILD",
    "UNKNOWN"
};

const char* RISK_LEVEL_STRINGS[] = {
    "CRITICAL",
    "HIGH",
    "MEDIUM",
    "LOW",
    "NONE"
};

const char* VALIDATION_RESULT_STRINGS[] = {
    "PASS",
    "FAIL",
    "WARNING",
    "SKIPPED"
};

const char* crrss_status_to_string(crrss_status_t status) {
    switch (status) {
        case CRRSS_SUCCESS: return "SUCCESS";
        case CRRSS_ERROR_INVALID_PARAM: return "ERROR_INVALID_PARAM";
        case CRRSS_ERROR_NOT_INITIALIZED: return "ERROR_NOT_INITIALIZED";
        case CRRSS_ERROR_MEMORY_ALLOCATION: return "ERROR_MEMORY_ALLOCATION";
        case CRRSS_ERROR_FILE_ACCESS: return "ERROR_FILE_ACCESS";
        case CRRSS_ERROR_PARSE_FAILURE: return "ERROR_PARSE_FAILURE";
        case CRRSS_ERROR_DATABASE_ACCESS: return "ERROR_DATABASE_ACCESS";
        case CRRSS_ERROR_NOT_FOUND: return "ERROR_NOT_FOUND";
        case CRRSS_ERROR_VALIDATION_FAILED: return "ERROR_VALIDATION_FAILED";
        default: return "UNKNOWN_STATUS";
    }
}

const char* bug_priority_to_string(bug_priority_t priority) {
    if (priority >= 0 && priority < BUG_PRIORITY_UNKNOWN + 1) {
        return BUG_PRIORITY_STRINGS[priority];
    }
    return "INVALID";
}

const char* bug_category_to_string(bug_category_t category) {
    if (category >= 0 && category < BUG_CATEGORY_UNKNOWN + 1) {
        return BUG_CATEGORY_STRINGS[category];
    }
    return "INVALID";
}

const char* risk_level_to_string(risk_level_t level) {
    if (level >= 0 && level < RISK_LEVEL_NONE + 1) {
        return RISK_LEVEL_STRINGS[level];
    }
    return "INVALID";
}

const char* validation_result_to_string(validation_result_t result) {
    if (result >= 0 && result < VALIDATION_SKIPPED + 1) {
        return VALIDATION_RESULT_STRINGS[result];
    }
    return "INVALID";
}
