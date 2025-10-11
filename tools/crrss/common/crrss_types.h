
/**
 * @file crrss_types.h
 * @brief Common types and definitions for CRRSS system
 * @description CRRSS - Code Review, Reliability, and Static Safety System
 */

#ifndef CRRSS_TYPES_H
#define CRRSS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Return Codes ====================
typedef enum {
    CRRSS_SUCCESS = 0,
    CRRSS_ERROR_INVALID_PARAM = -1,
    CRRSS_ERROR_NOT_INITIALIZED = -2,
    CRRSS_ERROR_MEMORY_ALLOCATION = -3,
    CRRSS_ERROR_FILE_ACCESS = -4,
    CRRSS_ERROR_PARSE_FAILURE = -5,
    CRRSS_ERROR_DATABASE_ACCESS = -6,
    CRRSS_ERROR_NOT_FOUND = -7,
    CRRSS_ERROR_VALIDATION_FAILED = -8
} crrss_status_t;

// ==================== Bug Priority Levels ====================
typedef enum {
    BUG_PRIORITY_P0_CRITICAL = 0,    // System crash, data corruption
    BUG_PRIORITY_P1_HIGH = 1,        // Major functionality broken
    BUG_PRIORITY_P2_MEDIUM = 2,      // Minor functionality issues
    BUG_PRIORITY_P3_LOW = 3,         // Cosmetic, optimization opportunities
    BUG_PRIORITY_UNKNOWN = 4
} bug_priority_t;

// ==================== Bug Categories ====================
typedef enum {
    BUG_CATEGORY_MEMORY = 0,         // Memory leaks, use-after-free, etc.
    BUG_CATEGORY_CONCURRENCY = 1,    // Race conditions, deadlocks
    BUG_CATEGORY_LOGIC = 2,          // Logic errors, wrong algorithms
    BUG_CATEGORY_PERFORMANCE = 3,    // Performance issues
    BUG_CATEGORY_SECURITY = 4,       // Security vulnerabilities
    BUG_CATEGORY_API = 5,            // API misuse
    BUG_CATEGORY_BUILD = 6,          // Build system issues
    BUG_CATEGORY_UNKNOWN = 7
} bug_category_t;

// ==================== Validation Result Types ====================
typedef enum {
    VALIDATION_PASS = 0,
    VALIDATION_FAIL = 1,
    VALIDATION_WARNING = 2,
    VALIDATION_SKIPPED = 3
} validation_result_t;

// ==================== Code Pattern Types ====================
typedef enum {
    PATTERN_MEMORY_LEAK = 0,
    PATTERN_USE_AFTER_FREE = 1,
    PATTERN_DOUBLE_FREE = 2,
    PATTERN_NULL_DEREF = 3,
    PATTERN_BUFFER_OVERFLOW = 4,
    PATTERN_RACE_CONDITION = 5,
    PATTERN_DEADLOCK = 6,
    PATTERN_UNINITIALIZED_VAR = 7,
    PATTERN_UNCHECKED_RETURN = 8,
    PATTERN_MISSING_ERROR_CHECK = 9,
    PATTERN_UNSAFE_CAST = 10,
    PATTERN_COUNT
} code_pattern_t;

// ==================== Risk Assessment ====================
typedef enum {
    RISK_LEVEL_CRITICAL = 0,  // Immediate attention required
    RISK_LEVEL_HIGH = 1,      // Should be addressed soon
    RISK_LEVEL_MEDIUM = 2,    // Moderate risk
    RISK_LEVEL_LOW = 3,       // Low risk
    RISK_LEVEL_NONE = 4       // No risk detected
} risk_level_t;

// ==================== Component Types ====================
typedef enum {
    COMPONENT_MEMORY_MANAGER = 0,
    COMPONENT_SCHEDULER = 1,
    COMPONENT_FILESYSTEM = 2,
    COMPONENT_DEVICE_DRIVER = 3,
    COMPONENT_NETWORK = 4,
    COMPONENT_AI_SUBSYSTEM = 5,
    COMPONENT_UNKNOWN = 6
} component_type_t;

// ==================== Structures ====================

/**
 * @brief Bug pattern information
 */
typedef struct {
    code_pattern_t pattern_type;
    const char* pattern_name;
    const char* description;
    bug_priority_t typical_priority;
    uint32_t occurrence_count;
    double risk_score;  // 0.0 to 1.0
} bug_pattern_info_t;

/**
 * @brief Bug prediction result
 */
typedef struct {
    const char* file_path;
    uint32_t line_number;
    bug_category_t category;
    bug_priority_t priority;
    risk_level_t risk_level;
    double confidence;  // 0.0 to 1.0
    const char* description;
    const char* recommendation;
    code_pattern_t pattern_detected;
} bug_prediction_t;

/**
 * @brief Validation issue
 */
typedef struct {
    const char* file_path;
    uint32_t line_number;
    validation_result_t result;
    const char* rule_name;
    const char* message;
    const char* suggestion;
} validation_issue_t;

/**
 * @brief Memory analysis result
 */
typedef struct {
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t potential_leaks;
    uint64_t use_after_free_risks;
    uint64_t double_free_risks;
    double memory_efficiency;  // 0.0 to 1.0
} memory_analysis_t;

/**
 * @brief Code statistics
 */
typedef struct {
    uint32_t total_files;
    uint32_t total_lines;
    uint32_t total_functions;
    uint32_t bug_patterns_found;
    uint32_t validation_passes;
    uint32_t validation_failures;
    uint32_t validation_warnings;
    double code_quality_score;  // 0.0 to 1.0
} code_statistics_t;

/**
 * @brief System health metrics
 */
typedef struct {
    bool initialized;
    uint32_t components_active;
    uint32_t total_scans_performed;
    uint32_t bugs_predicted;
    uint32_t validations_performed;
    double overall_system_health;  // 0.0 to 1.0
} system_health_t;

// ==================== String Constants ====================
extern const char* BUG_PRIORITY_STRINGS[];
extern const char* BUG_CATEGORY_STRINGS[];
extern const char* RISK_LEVEL_STRINGS[];
extern const char* VALIDATION_RESULT_STRINGS[];

// ==================== Utility Functions ====================
const char* crrss_status_to_string(crrss_status_t status);
const char* bug_priority_to_string(bug_priority_t priority);
const char* bug_category_to_string(bug_category_t category);
const char* risk_level_to_string(risk_level_t level);
const char* validation_result_to_string(validation_result_t result);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_TYPES_H
