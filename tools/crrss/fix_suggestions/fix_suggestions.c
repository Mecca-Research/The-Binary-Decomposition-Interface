#include "fix_suggestions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fix_context {
    fix_config_t config;
    bool initialized;
};

fix_context_t* fix_initialize(const fix_config_t* config) {
    fix_context_t* ctx = (fix_context_t*)calloc(1, sizeof(fix_context_t));
    if (!ctx) return NULL;
    ctx->config = *config;
    ctx->initialized = true;
    return ctx;
}

void fix_shutdown(fix_context_t* ctx) {
    if (ctx) free(ctx);
}

crrss_status_t fix_suggest_for_file(
    fix_context_t* ctx __attribute__((unused)),
    const char* file_path,
    fix_suggestion_t* suggestions,
    uint32_t max_suggestions,
    uint32_t* count
) {
    if (!ctx || !file_path || !suggestions || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Simple example: detect strcpy and suggest strncpy
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char line[1024];
    uint32_t line_num = 0;
    
    while (fgets(line, sizeof(line), fp) && *count < max_suggestions) {
        line_num++;
        if (strstr(line, "strcpy(")) {
            fix_suggestion_t* fix = &suggestions[*count];
            fix->type = FIX_TYPE_BUFFER_OVERFLOW;
            strncpy(fix->file_path, file_path, FIX_MAX_PATH_LEN - 1);
            fix->line_start = line_num;
            fix->line_end = line_num;
            snprintf(fix->description, FIX_MAX_DESCRIPTION_LEN,
                    "Replace unsafe strcpy with strncpy");
            strncpy(fix->original_code, line, FIX_MAX_CODE_LEN - 1);
            snprintf(fix->suggested_code, FIX_MAX_CODE_LEN,
                    "    strncpy(dest, src, sizeof(dest) - 1);\n");
            fix->confidence = 0.9;
            fix->priority = BUG_PRIORITY_P1_HIGH;
            fix->rationale = "strcpy does not check buffer bounds";
            (*count)++;
        }
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

crrss_status_t fix_apply_suggestion(
    fix_context_t* ctx __attribute__((unused)),
    const fix_suggestion_t* suggestion __attribute__((unused)),
    bool create_backup __attribute__((unused))
) {
    // Implementation would read file, apply fix, write back
    return CRRSS_SUCCESS;
}

crrss_status_t fix_generate_diff(
    fix_context_t* ctx __attribute__((unused)),
    const fix_suggestion_t* suggestion __attribute__((unused)),
    char* diff_output,
    uint32_t max_len
) {
    if (!ctx || !suggestion || !diff_output) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    snprintf(diff_output, max_len,
            "--- %s:%u\n+++ %s:%u\n-%s+%s",
            suggestion->file_path, suggestion->line_start,
            suggestion->file_path, suggestion->line_start,
            suggestion->original_code,
            suggestion->suggested_code);
    
    return CRRSS_SUCCESS;
}
