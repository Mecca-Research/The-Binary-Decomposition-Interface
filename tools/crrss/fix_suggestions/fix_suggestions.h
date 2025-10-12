/**
 * @file fix_suggestions.h
 * @brief Automated Fix Suggestions - Phase 3 Component
 * 
 * Generates automated code fix suggestions for common issues:
 * - Buffer overflow fixes
 * - Memory leak fixes
 * - Null pointer dereference fixes
 * - Style/formatting fixes
 * - Performance optimizations
 */

#ifndef CRRSS_FIX_SUGGESTIONS_H
#define CRRSS_FIX_SUGGESTIONS_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FIX_MAX_DESCRIPTION_LEN 512
#define FIX_MAX_CODE_LEN 2048
#define FIX_MAX_PATH_LEN 512

// Fix suggestion type
typedef enum {
    FIX_TYPE_BUFFER_OVERFLOW = 0,
    FIX_TYPE_MEMORY_LEAK = 1,
    FIX_TYPE_NULL_DEREF = 2,
    FIX_TYPE_USE_AFTER_FREE = 3,
    FIX_TYPE_DOUBLE_FREE = 4,
    FIX_TYPE_STYLE = 5,
    FIX_TYPE_PERFORMANCE = 6,
    FIX_TYPE_SECURITY = 7
} fix_type_t;

// Fix suggestion
typedef struct {
    fix_type_t type;
    char file_path[FIX_MAX_PATH_LEN];
    uint32_t line_start;
    uint32_t line_end;
    char description[FIX_MAX_DESCRIPTION_LEN];
    char original_code[FIX_MAX_CODE_LEN];
    char suggested_code[FIX_MAX_CODE_LEN];
    double confidence;  // 0.0 - 1.0
    bug_priority_t priority;
    const char* rationale;
} fix_suggestion_t;

// Configuration
typedef struct {
    bool enable_buffer_overflow_fixes;
    bool enable_memory_leak_fixes;
    bool enable_null_deref_fixes;
    bool enable_style_fixes;
    bool enable_performance_fixes;
    double min_confidence;  // Minimum confidence to suggest fix
    const char* template_directory;
} fix_config_t;

// Fix context
typedef struct fix_context fix_context_t;

/**
 * @brief Initialize fix suggestion system
 */
fix_context_t* fix_initialize(const fix_config_t* config);

/**
 * @brief Shutdown fix suggestion system
 */
void fix_shutdown(fix_context_t* ctx);

/**
 * @brief Generate fix suggestions for a file
 */
crrss_status_t fix_suggest_for_file(
    fix_context_t* ctx,
    const char* file_path,
    fix_suggestion_t* suggestions,
    uint32_t max_suggestions,
    uint32_t* count
);

/**
 * @brief Apply a fix suggestion
 */
crrss_status_t fix_apply_suggestion(
    fix_context_t* ctx,
    const fix_suggestion_t* suggestion,
    bool create_backup
);

/**
 * @brief Generate diff for a fix suggestion
 */
crrss_status_t fix_generate_diff(
    fix_context_t* ctx,
    const fix_suggestion_t* suggestion,
    char* diff_output,
    uint32_t max_len
);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_FIX_SUGGESTIONS_H
