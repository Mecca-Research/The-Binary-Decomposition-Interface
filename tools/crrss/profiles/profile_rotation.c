#include "profile_rotation.h"
#include <stdlib.h>
#include <string.h>

struct profile_context {
    profile_config_t profiles[5];
    style_pattern_t* learned_patterns;
    uint32_t pattern_count;
    bool initialized;
};

profile_context_t* profile_initialize(void) {
    profile_context_t* ctx = (profile_context_t*)calloc(1, sizeof(profile_context_t));
    if (!ctx) return NULL;
    
    // Initialize profiles
    ctx->profiles[PROFILE_CONSERVATIVE] = (profile_config_t){
        .type = PROFILE_CONSERVATIVE,
        .safety_weight = 1.0,
        .performance_weight = 0.3,
        .code_quality_weight = 0.8,
        .innovation_weight = 0.1,
        .enable_learning = true,
        .description = "Safety-first, minimal changes"
    };
    
    ctx->profiles[PROFILE_AGGRESSIVE] = (profile_config_t){
        .type = PROFILE_AGGRESSIVE,
        .safety_weight = 0.5,
        .performance_weight = 1.0,
        .code_quality_weight = 0.6,
        .innovation_weight = 0.7,
        .enable_learning = true,
        .description = "Performance-focused, major refactoring"
    };
    
    ctx->profiles[PROFILE_BALANCED] = (profile_config_t){
        .type = PROFILE_BALANCED,
        .safety_weight = 0.7,
        .performance_weight = 0.7,
        .code_quality_weight = 0.8,
        .innovation_weight = 0.5,
        .enable_learning = true,
        .description = "Balanced approach"
    };
    
    ctx->profiles[PROFILE_EXPERIMENTAL] = (profile_config_t){
        .type = PROFILE_EXPERIMENTAL,
        .safety_weight = 0.6,
        .performance_weight = 0.8,
        .code_quality_weight = 0.7,
        .innovation_weight = 1.0,
        .enable_learning = true,
        .description = "Novel approaches"
    };
    
    ctx->initialized = true;
    return ctx;
}

void profile_shutdown(profile_context_t* ctx) {
    if (ctx) {
        if (ctx->learned_patterns) free(ctx->learned_patterns);
        free(ctx);
    }
}

profile_type_t profile_select_for_task(
    profile_context_t* ctx,
    task_type_t task_type,
    uint32_t code_complexity,
    bool is_critical_module
) {
    if (!ctx) return PROFILE_BALANCED;
    
    // Decision logic
    if (is_critical_module || code_complexity > 20) {
        return PROFILE_CONSERVATIVE;
    }
    
    switch (task_type) {
        case TASK_BUG_FIX:
            return PROFILE_CONSERVATIVE;
        case TASK_OPTIMIZATION:
            return PROFILE_AGGRESSIVE;
        case TASK_NEW_FEATURE:
            return PROFILE_EXPERIMENTAL;
        case TASK_REFACTORING:
            return PROFILE_BALANCED;
        default:
            return PROFILE_BALANCED;
    }
}

crrss_status_t profile_get_config(
    profile_context_t* ctx,
    profile_type_t profile,
    profile_config_t* config
) {
    if (!ctx || !config || profile >= 5) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *config = ctx->profiles[profile];
    return CRRSS_SUCCESS;
}
