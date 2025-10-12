/**
 * @file profile_rotation.h
 * @brief Personality Profile Rotation - Phase 3 Component
 * 
 * Automatic profile selection and cross-pollination of coding styles:
 * - Conservative: Safety-first, minimal changes
 * - Aggressive: Performance-focused, major refactoring
 * - Balanced: Mix of safety and performance
 * - Experimental: Try novel approaches
 */

#ifndef CRRSS_PROFILE_ROTATION_H
#define CRRSS_PROFILE_ROTATION_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Profile type
typedef enum {
    PROFILE_CONSERVATIVE = 0,
    PROFILE_AGGRESSIVE = 1,
    PROFILE_BALANCED = 2,
    PROFILE_EXPERIMENTAL = 3,
    PROFILE_ADAPTIVE = 4  // Automatically adapts
} profile_type_t;

// Task type
typedef enum {
    TASK_BUG_FIX = 0,
    TASK_REFACTORING = 1,
    TASK_OPTIMIZATION = 2,
    TASK_NEW_FEATURE = 3,
    TASK_MAINTENANCE = 4
} task_type_t;

// Profile configuration
typedef struct {
    profile_type_t type;
    double safety_weight;       // 0.0 - 1.0
    double performance_weight;  // 0.0 - 1.0
    double code_quality_weight; // 0.0 - 1.0
    double innovation_weight;   // 0.0 - 1.0
    bool enable_learning;       // Learn from successful patterns
    const char* description;
} profile_config_t;

// Style pattern
typedef struct {
    char pattern_name[128];
    char code_snippet[1024];
    uint32_t success_count;
    double effectiveness_score;
    profile_type_t origin_profile;
} style_pattern_t;

// Profile context
typedef struct profile_context profile_context_t;

/**
 * @brief Initialize profile rotation system
 */
profile_context_t* profile_initialize(void);

/**
 * @brief Shutdown profile rotation system
 */
void profile_shutdown(profile_context_t* ctx);

/**
 * @brief Select appropriate profile for task
 */
profile_type_t profile_select_for_task(
    profile_context_t* ctx,
    task_type_t task_type,
    uint32_t code_complexity,
    bool is_critical_module
);

/**
 * @brief Get profile configuration
 */
crrss_status_t profile_get_config(
    profile_context_t* ctx,
    profile_type_t profile,
    profile_config_t* config
);

/**
 * @brief Learn from successful pattern
 */
crrss_status_t profile_learn_pattern(
    profile_context_t* ctx,
    const style_pattern_t* pattern
);

/**
 * @brief Get recommended patterns for profile
 */
crrss_status_t profile_get_recommendations(
    profile_context_t* ctx,
    profile_type_t profile,
    style_pattern_t* patterns,
    uint32_t max_patterns,
    uint32_t* count
);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_PROFILE_ROTATION_H
