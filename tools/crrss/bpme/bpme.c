/**
 * @file bpme.c
 * @brief Bug Prior Mapping Engine Implementation
 * 
 * Implements bug pattern detection and prediction based on historical
 * bug data from comprehensive analysis (PRs #1-165).
 */

#include "bpme.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

// ==================== Internal Structures ====================

/**
 * @brief Pattern rule for detection
 */
typedef struct {
    code_pattern_t pattern_type;
    const char* regex_pattern;
    const char* keywords[10];
    bug_priority_t default_priority;
    double base_risk_score;
} pattern_rule_t;

/**
 * @brief Bug pattern database entry
 */
typedef struct {
    code_pattern_t pattern;
    bug_category_t category;
    bug_priority_t priority;
    uint32_t historical_count;
    double risk_multiplier;
} pattern_db_entry_t;

/**
 * @brief BPME Context implementation
 */
struct bpme_context {
    bpme_config_t config;
    pattern_db_entry_t* pattern_database;
    uint32_t pattern_db_size;
    bug_prediction_t* prediction_cache;
    uint32_t cache_size;
    uint32_t total_scans;
    uint32_t total_predictions;
    double accuracy;
    bool initialized;
};

// ==================== Pattern Detection Rules ====================

static const pattern_rule_t PATTERN_RULES[] = {
    // Memory patterns
    {
        PATTERN_MEMORY_LEAK,
        NULL,
        {"malloc", "calloc", "realloc", "new", "alloc", NULL},
        BUG_PRIORITY_P1_HIGH,
        0.7
    },
    {
        PATTERN_USE_AFTER_FREE,
        NULL,
        {"free", "delete", "destroy", NULL},
        BUG_PRIORITY_P0_CRITICAL,
        0.9
    },
    {
        PATTERN_DOUBLE_FREE,
        NULL,
        {"free", "delete", NULL},
        BUG_PRIORITY_P0_CRITICAL,
        0.95
    },
    {
        PATTERN_NULL_DEREF,
        NULL,
        {"->", "*", ".", NULL},
        BUG_PRIORITY_P1_HIGH,
        0.75
    },
    {
        PATTERN_BUFFER_OVERFLOW,
        NULL,
        {"strcpy", "strcat", "sprintf", "gets", NULL},
        BUG_PRIORITY_P0_CRITICAL,
        0.85
    },
    // Concurrency patterns
    {
        PATTERN_RACE_CONDITION,
        NULL,
        {"pthread", "mutex", "lock", "atomic", NULL},
        BUG_PRIORITY_P1_HIGH,
        0.8
    },
    {
        PATTERN_DEADLOCK,
        NULL,
        {"lock", "mutex", "wait", NULL},
        BUG_PRIORITY_P0_CRITICAL,
        0.75
    },
    // Logic patterns
    {
        PATTERN_UNINITIALIZED_VAR,
        NULL,
        {"int", "char", "void", "struct", NULL},
        BUG_PRIORITY_P2_MEDIUM,
        0.6
    },
    {
        PATTERN_UNCHECKED_RETURN,
        NULL,
        {"return", "malloc", "calloc", "fopen", NULL},
        BUG_PRIORITY_P1_HIGH,
        0.7
    },
    {
        PATTERN_MISSING_ERROR_CHECK,
        NULL,
        {"if", "error", "status", "result", NULL},
        BUG_PRIORITY_P2_MEDIUM,
        0.65
    }
};

#define NUM_PATTERN_RULES (sizeof(PATTERN_RULES) / sizeof(pattern_rule_t))

// ==================== Helper Functions ====================

static bool file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static bool is_c_source_file(const char* filename) {
    size_t len = strlen(filename);
    return (len > 2 && (
        strcmp(filename + len - 2, ".c") == 0 ||
        strcmp(filename + len - 2, ".h") == 0 ||
        (len > 4 && strcmp(filename + len - 4, ".cpp") == 0) ||
        (len > 4 && strcmp(filename + len - 4, ".hpp") == 0)
    ));
}

__attribute__((unused))
static bool contains_keyword(const char* line, const char* keyword) {
    return (strstr(line, keyword) != NULL);
}

static bool detect_memory_leak_pattern(const char* code, size_t length) {
    (void)length;  // Parameter reserved for future use
    bool has_alloc = false;
    bool has_free = false;
    
    // Simple heuristic: look for malloc/calloc without corresponding free
    const char* alloc_keywords[] = {"malloc", "calloc", "realloc", NULL};
    const char* free_keywords[] = {"free", NULL};
    
    for (int i = 0; alloc_keywords[i]; i++) {
        if (strstr(code, alloc_keywords[i])) {
            has_alloc = true;
            break;
        }
    }
    
    for (int i = 0; free_keywords[i]; i++) {
        if (strstr(code, free_keywords[i])) {
            has_free = true;
            break;
        }
    }
    
    return (has_alloc && !has_free);
}

static bool detect_null_deref_pattern(const char* line) {
    // Look for pointer dereference without NULL check
    if (strstr(line, "->") || strstr(line, "*")) {
        // Check if there's a NULL check nearby (simplified)
        if (strstr(line, "!= NULL") || strstr(line, "== NULL") ||
            strstr(line, "if (") || strstr(line, "assert(")) {
            return false;
        }
        return true;
    }
    return false;
}

static bool detect_buffer_overflow_pattern(const char* line) {
    // Detect unsafe string functions
    const char* unsafe_funcs[] = {
        "strcpy", "strcat", "sprintf", "gets", "scanf", NULL
    };
    
    for (int i = 0; unsafe_funcs[i]; i++) {
        if (strstr(line, unsafe_funcs[i])) {
            return true;
        }
    }
    return false;
}

static bool detect_unchecked_return_pattern(const char* line) {
    // Look for function calls that should be checked
    const char* check_funcs[] = {
        "malloc", "calloc", "realloc", "fopen", "open", NULL
    };
    
    for (int i = 0; check_funcs[i]; i++) {
        if (strstr(line, check_funcs[i])) {
            // Check if result is checked
            if (strstr(line, "if") || strstr(line, "==") || 
                strstr(line, "!=") || strstr(line, "assert")) {
                return false;
            }
            return true;
        }
    }
    return false;
}

__attribute__((unused))
static double calculate_risk_score(code_pattern_t pattern, uint32_t occurrences) {
    double base_score = 0.5;
    
    // Adjust based on pattern type
    for (size_t i = 0; i < NUM_PATTERN_RULES; i++) {
        if (PATTERN_RULES[i].pattern_type == pattern) {
            base_score = PATTERN_RULES[i].base_risk_score;
            break;
        }
    }
    
    // Increase score with occurrences
    double multiplier = 1.0 + (occurrences * 0.1);
    if (multiplier > 2.0) multiplier = 2.0;
    
    double score = base_score * multiplier;
    if (score > 1.0) score = 1.0;
    
    return score;
}

static bug_category_t pattern_to_category(code_pattern_t pattern) {
    if (pattern <= PATTERN_BUFFER_OVERFLOW) {
        return BUG_CATEGORY_MEMORY;
    } else if (pattern == PATTERN_RACE_CONDITION || pattern == PATTERN_DEADLOCK) {
        return BUG_CATEGORY_CONCURRENCY;
    } else {
        return BUG_CATEGORY_LOGIC;
    }
}

static risk_level_t score_to_risk_level(double score) {
    if (score >= 0.9) return RISK_LEVEL_CRITICAL;
    if (score >= 0.7) return RISK_LEVEL_HIGH;
    if (score >= 0.4) return RISK_LEVEL_MEDIUM;
    if (score > 0.0) return RISK_LEVEL_LOW;
    return RISK_LEVEL_NONE;
}

// ==================== Cache Management ====================

/**
 * @brief Add predictions to the internal cache
 * @param ctx BPME context
 * @param predictions Array of predictions to cache
 * @param num_predictions Number of predictions to cache
 * @return Number of predictions successfully cached
 */
static uint32_t cache_predictions(
    bpme_context_t* ctx,
    const bug_prediction_t* predictions,
    uint32_t num_predictions
) {
    if (!ctx || !predictions || num_predictions == 0) {
        return 0;
    }
    
    uint32_t cached = 0;
    uint32_t current_cache_count = 0;
    
    // Find current number of cached predictions
    // (ctx->total_predictions tracks all predictions, including those not cached)
    for (uint32_t i = 0; i < ctx->cache_size; i++) {
        if (ctx->prediction_cache[i].file_path != NULL) {
            current_cache_count++;
        } else {
            break;  // Assume cache is contiguous
        }
    }
    
    // Cache as many predictions as possible
    for (uint32_t i = 0; i < num_predictions && current_cache_count < ctx->cache_size; i++) {
        // Deep copy the prediction
        ctx->prediction_cache[current_cache_count].file_path = 
            predictions[i].file_path ? strdup(predictions[i].file_path) : NULL;
        ctx->prediction_cache[current_cache_count].line_number = predictions[i].line_number;
        ctx->prediction_cache[current_cache_count].category = predictions[i].category;
        ctx->prediction_cache[current_cache_count].priority = predictions[i].priority;
        ctx->prediction_cache[current_cache_count].risk_level = predictions[i].risk_level;
        ctx->prediction_cache[current_cache_count].confidence = predictions[i].confidence;
        ctx->prediction_cache[current_cache_count].description = predictions[i].description;
        ctx->prediction_cache[current_cache_count].recommendation = predictions[i].recommendation;
        ctx->prediction_cache[current_cache_count].pattern_detected = predictions[i].pattern_detected;
        
        current_cache_count++;
        cached++;
    }
    
    return cached;
}

// ==================== Pattern Detection ====================

static crrss_status_t detect_patterns_in_file(
    bpme_context_t* ctx,
    const char* file_path,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    (void)ctx;  // Parameter reserved for future use
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[1024];
    uint32_t line_num = 0;
    uint32_t pred_count = 0;
    
    // Read file line by line
    while (fgets(line, sizeof(line), fp) && pred_count < max_predictions) {
        line_num++;
        
        // Detect NULL dereference pattern
        if (detect_null_deref_pattern(line)) {
            predictions[pred_count].file_path = strdup(file_path);
            predictions[pred_count].line_number = line_num;
            predictions[pred_count].category = BUG_CATEGORY_MEMORY;
            predictions[pred_count].priority = BUG_PRIORITY_P1_HIGH;
            predictions[pred_count].risk_level = RISK_LEVEL_HIGH;
            predictions[pred_count].confidence = 0.7;
            predictions[pred_count].description = "Potential NULL pointer dereference";
            predictions[pred_count].recommendation = "Add NULL check before dereferencing";
            predictions[pred_count].pattern_detected = PATTERN_NULL_DEREF;
            pred_count++;
        }
        
        // Detect buffer overflow pattern
        if (detect_buffer_overflow_pattern(line) && pred_count < max_predictions) {
            predictions[pred_count].file_path = strdup(file_path);
            predictions[pred_count].line_number = line_num;
            predictions[pred_count].category = BUG_CATEGORY_SECURITY;
            predictions[pred_count].priority = BUG_PRIORITY_P0_CRITICAL;
            predictions[pred_count].risk_level = RISK_LEVEL_CRITICAL;
            predictions[pred_count].confidence = 0.85;
            predictions[pred_count].description = "Unsafe string function usage";
            predictions[pred_count].recommendation = "Use safe alternatives (strncpy, snprintf)";
            predictions[pred_count].pattern_detected = PATTERN_BUFFER_OVERFLOW;
            pred_count++;
        }
        
        // Detect unchecked return pattern
        if (detect_unchecked_return_pattern(line) && pred_count < max_predictions) {
            predictions[pred_count].file_path = strdup(file_path);
            predictions[pred_count].line_number = line_num;
            predictions[pred_count].category = BUG_CATEGORY_LOGIC;
            predictions[pred_count].priority = BUG_PRIORITY_P1_HIGH;
            predictions[pred_count].risk_level = RISK_LEVEL_HIGH;
            predictions[pred_count].confidence = 0.75;
            predictions[pred_count].description = "Unchecked return value";
            predictions[pred_count].recommendation = "Check return value for errors";
            predictions[pred_count].pattern_detected = PATTERN_UNCHECKED_RETURN;
            pred_count++;
        }
    }
    
    fclose(fp);
    *num_predictions = pred_count;
    
    return CRRSS_SUCCESS;
}

// ==================== Public API Implementation ====================

bpme_context_t* bpme_initialize(const bpme_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    bpme_context_t* ctx = (bpme_context_t*)calloc(1, sizeof(bpme_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copy configuration
    ctx->config = *config;
    if (config->knowledge_base_path) {
        ctx->config.knowledge_base_path = strdup(config->knowledge_base_path);
    }
    
    // Initialize pattern database with default patterns
    ctx->pattern_db_size = NUM_PATTERN_RULES;
    ctx->pattern_database = (pattern_db_entry_t*)calloc(
        ctx->pattern_db_size, sizeof(pattern_db_entry_t)
    );
    
    // Populate default patterns
    for (size_t i = 0; i < NUM_PATTERN_RULES && i < ctx->pattern_db_size; i++) {
        ctx->pattern_database[i].pattern = PATTERN_RULES[i].pattern_type;
        ctx->pattern_database[i].category = pattern_to_category(PATTERN_RULES[i].pattern_type);
        ctx->pattern_database[i].priority = PATTERN_RULES[i].default_priority;
        ctx->pattern_database[i].historical_count = 0;
        ctx->pattern_database[i].risk_multiplier = PATTERN_RULES[i].base_risk_score;
    }
    
    // Allocate prediction cache
    ctx->cache_size = config->max_predictions > 0 ? config->max_predictions : 1000;
    ctx->prediction_cache = (bug_prediction_t*)calloc(
        ctx->cache_size, sizeof(bug_prediction_t)
    );
    
    ctx->total_scans = 0;
    ctx->total_predictions = 0;
    ctx->accuracy = 0.0;
    ctx->initialized = true;
    
    return ctx;
}

void bpme_shutdown(bpme_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->config.knowledge_base_path) {
        free((void*)ctx->config.knowledge_base_path);
    }
    
    if (ctx->pattern_database) {
        free(ctx->pattern_database);
    }
    
    if (ctx->prediction_cache) {
        // Free cached predictions
        for (uint32_t i = 0; i < ctx->total_predictions && i < ctx->cache_size; i++) {
            if (ctx->prediction_cache[i].file_path) {
                free((void*)ctx->prediction_cache[i].file_path);
            }
        }
        free(ctx->prediction_cache);
    }
    
    free(ctx);
}

crrss_status_t bpme_analyze_file(
    bpme_context_t* ctx,
    const char* file_path,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !predictions || !num_predictions) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!file_exists(file_path)) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Detect patterns in the file
    crrss_status_t status = detect_patterns_in_file(
        ctx, file_path, predictions, max_predictions, num_predictions
    );
    
    if (status == CRRSS_SUCCESS) {
        ctx->total_scans++;
        ctx->total_predictions += *num_predictions;
        
        // Cache the predictions for later queries
        cache_predictions(ctx, predictions, *num_predictions);
    }
    
    return status;
}

crrss_status_t bpme_analyze_directory(
    bpme_context_t* ctx,
    const char* dir_path,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!dir_path || !predictions || !num_predictions) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    uint32_t total_predictions = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL && total_predictions < max_predictions) {
        if (entry->d_name[0] == '.') continue;
        
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode) && is_c_source_file(entry->d_name)) {
                uint32_t file_predictions = 0;
                crrss_status_t status = bpme_analyze_file(
                    ctx, full_path,
                    predictions + total_predictions,
                    max_predictions - total_predictions,
                    &file_predictions
                );
                
                if (status == CRRSS_SUCCESS) {
                    total_predictions += file_predictions;
                }
            } else if (S_ISDIR(st.st_mode)) {
                // Recursive directory analysis
                uint32_t dir_predictions = 0;
                bpme_analyze_directory(
                    ctx, full_path,
                    predictions + total_predictions,
                    max_predictions - total_predictions,
                    &dir_predictions
                );
                total_predictions += dir_predictions;
            }
        }
    }
    
    closedir(dir);
    *num_predictions = total_predictions;
    
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_analyze_snippet(
    bpme_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!code_snippet || !predictions || !num_predictions) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Detect memory leak pattern
    if (detect_memory_leak_pattern(code_snippet, snippet_length) && max_predictions > 0) {
        predictions[0].file_path = strdup("<snippet>");
        predictions[0].line_number = 0;
        predictions[0].category = BUG_CATEGORY_MEMORY;
        predictions[0].priority = BUG_PRIORITY_P1_HIGH;
        predictions[0].risk_level = RISK_LEVEL_HIGH;
        predictions[0].confidence = 0.6;
        predictions[0].description = "Potential memory leak";
        predictions[0].recommendation = "Ensure all allocations are freed";
        predictions[0].pattern_detected = PATTERN_MEMORY_LEAK;
        *num_predictions = 1;
        
        ctx->total_predictions++;
        
        // Cache the prediction for later queries
        cache_predictions(ctx, predictions, *num_predictions);
        
        return CRRSS_SUCCESS;
    }
    
    *num_predictions = 0;
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_assess_change_risk(
    bpme_context_t* ctx,
    const char* file_path,
    const char* diff,
    risk_level_t* risk_level
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !risk_level) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Simple risk assessment based on file type and change size
    double risk_score = 0.0;
    
    // Higher risk for memory manager changes
    if (strstr(file_path, "memory") || strstr(file_path, "alloc")) {
        risk_score += 0.3;
    }
    
    // Higher risk for concurrency code
    if (strstr(file_path, "thread") || strstr(file_path, "lock")) {
        risk_score += 0.3;
    }
    
    // Assess diff size if provided
    if (diff) {
        size_t diff_size = strlen(diff);
        if (diff_size > 1000) risk_score += 0.2;
        else if (diff_size > 500) risk_score += 0.1;
    }
    
    *risk_level = score_to_risk_level(risk_score);
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_get_component_risk(
    bpme_context_t* ctx,
    component_type_t component,
    risk_level_t* risk_level
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!risk_level) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Risk assessment based on component type
    switch (component) {
        case COMPONENT_MEMORY_MANAGER:
            *risk_level = RISK_LEVEL_HIGH;
            break;
        case COMPONENT_SCHEDULER:
            *risk_level = RISK_LEVEL_HIGH;
            break;
        case COMPONENT_FILESYSTEM:
            *risk_level = RISK_LEVEL_MEDIUM;
            break;
        case COMPONENT_DEVICE_DRIVER:
            *risk_level = RISK_LEVEL_MEDIUM;
            break;
        case COMPONENT_AI_SUBSYSTEM:
            *risk_level = RISK_LEVEL_LOW;
            break;
        default:
            *risk_level = RISK_LEVEL_NONE;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_load_pattern_database(
    bpme_context_t* ctx,
    const char* database_path
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!database_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement actual database loading
    // For now, use default patterns
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_get_pattern_info(
    bpme_context_t* ctx,
    code_pattern_t pattern,
    bug_pattern_info_t* info
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!info || pattern >= PATTERN_COUNT) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Find pattern in database
    for (uint32_t i = 0; i < ctx->pattern_db_size; i++) {
        if (ctx->pattern_database[i].pattern == pattern) {
            info->pattern_type = pattern;
            info->typical_priority = ctx->pattern_database[i].priority;
            info->occurrence_count = ctx->pattern_database[i].historical_count;
            info->risk_score = ctx->pattern_database[i].risk_multiplier;
            
            // Set descriptive information
            static const char* pattern_names[] = {
                "Memory Leak", "Use After Free", "Double Free", "NULL Dereference",
                "Buffer Overflow", "Race Condition", "Deadlock", "Uninitialized Variable",
                "Unchecked Return", "Missing Error Check", "Unsafe Cast"
            };
            info->pattern_name = pattern_names[pattern];
            info->description = "Pattern detected in codebase";
            
            return CRRSS_SUCCESS;
        }
    }
    
    return CRRSS_ERROR_NOT_FOUND;
}

crrss_status_t bpme_get_statistics(
    bpme_context_t* ctx,
    uint32_t* total_scans,
    uint32_t* bugs_predicted,
    double* accuracy
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (total_scans) *total_scans = ctx->total_scans;
    if (bugs_predicted) *bugs_predicted = ctx->total_predictions;
    if (accuracy) *accuracy = ctx->accuracy;
    
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_query_by_priority(
    bpme_context_t* ctx,
    bug_priority_t priority,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!predictions || !num_predictions) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    uint32_t found = 0;
    for (uint32_t i = 0; i < ctx->total_predictions && i < ctx->cache_size && found < max_predictions; i++) {
        if (ctx->prediction_cache[i].priority == priority) {
            predictions[found++] = ctx->prediction_cache[i];
        }
    }
    
    *num_predictions = found;
    return CRRSS_SUCCESS;
}

crrss_status_t bpme_query_by_category(
    bpme_context_t* ctx,
    bug_category_t category,
    bug_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* num_predictions
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!predictions || !num_predictions) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    uint32_t found = 0;
    for (uint32_t i = 0; i < ctx->total_predictions && i < ctx->cache_size && found < max_predictions; i++) {
        if (ctx->prediction_cache[i].category == category) {
            predictions[found++] = ctx->prediction_cache[i];
        }
    }
    
    *num_predictions = found;
    return CRRSS_SUCCESS;
}
