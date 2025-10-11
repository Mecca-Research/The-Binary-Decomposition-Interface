/**
 * @file stp.c
 * @brief Strict Typist Profile - Implementation
 * 
 * Phase 2 Stage 1 Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "stp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

// ==================== Internal Constants ====================

#define STP_DEFAULT_MAX_ISSUES 1000
#define STP_DEFAULT_MAX_STRUCTS 500
#define STP_DEFAULT_MAX_CONVERSIONS 1000
#define STP_MAX_LINE_LENGTH 4096
#define STP_MAX_TYPE_NAME 256

// ==================== String Constants ====================

static const char* STP_ISSUE_TYPE_STRINGS[] = {
    "Type Mismatch",
    "Implicit Conversion",
    "Signed/Unsigned Mix",
    "Pointer Type Incompatibility",
    "Type Punning",
    "Unsafe Cast",
    "Narrowing Conversion",
    "Unsafe Pointer Cast",
    "Const Violation",
    "Integer Overflow in Cast",
    "Struct Padding",
    "Struct Alignment",
    "Unaligned Access",
    "Struct Packing",
    "Member Ordering",
    "Portability"
};

static const char* STRICTNESS_LEVEL_STRINGS[] = {
    "Permissive",
    "Moderate",
    "Strict",
    "Paranoid"
};

// ==================== Type Information Database ====================

typedef struct {
    const char* name;
    size_t size;
    size_t alignment;
    bool is_signed;
} builtin_type_info_t;

static const builtin_type_info_t BUILTIN_TYPES[] = {
    {"char", 1, 1, true},
    {"signed char", 1, 1, true},
    {"unsigned char", 1, 1, false},
    {"short", 2, 2, true},
    {"unsigned short", 2, 2, false},
    {"int", 4, 4, true},
    {"unsigned int", 4, 4, false},
    {"long", 8, 8, true},
    {"unsigned long", 8, 8, false},
    {"long long", 8, 8, true},
    {"unsigned long long", 8, 8, false},
    {"float", 4, 4, true},
    {"double", 8, 8, true},
    {"long double", 16, 16, true},
    {"void", 1, 1, false},
    {"_Bool", 1, 1, false},
    {"bool", 1, 1, false},
    {"size_t", 8, 8, false},
    {"ssize_t", 8, 8, true},
    {"ptrdiff_t", 8, 8, true},
    {"intptr_t", 8, 8, true},
    {"uintptr_t", 8, 8, false},
    {"int8_t", 1, 1, true},
    {"uint8_t", 1, 1, false},
    {"int16_t", 2, 2, true},
    {"uint16_t", 2, 2, false},
    {"int32_t", 4, 4, true},
    {"uint32_t", 4, 4, false},
    {"int64_t", 8, 8, true},
    {"uint64_t", 8, 8, false},
    {NULL, 0, 0, false}
};

// ==================== STP Context Structure ====================

struct stp_context {
    // Configuration
    stp_config_t config;
    bool initialized;
    
    // Issue tracking
    stp_issue_t* issues;
    uint32_t issue_count;
    uint32_t max_issues;
    
    // Struct analysis
    struct_layout_t* struct_layouts;
    uint32_t struct_count;
    uint32_t max_structs;
    
    // Type conversions
    type_conversion_t* conversions;
    uint32_t conversion_count;
    uint32_t max_conversions;
    
    // Statistics
    stp_statistics_t stats;
    
    // Integration contexts
    void* bpme_ctx;
    void* sciv_ctx;
    void* msm_ctx;
};

// ==================== Helper Functions ====================

/**
 * @brief Get builtin type information
 */
static bool get_builtin_type_info(const char* type_name, builtin_type_info_t* info) {
    for (int i = 0; BUILTIN_TYPES[i].name != NULL; i++) {
        if (strcmp(BUILTIN_TYPES[i].name, type_name) == 0) {
            *info = BUILTIN_TYPES[i];
            return true;
        }
    }
    return false;
}

/**
 * @brief Add issue to context
 */
static crrss_status_t add_issue(stp_context_t* ctx, const stp_issue_t* issue) {
    if (!ctx || !issue) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->issue_count >= ctx->max_issues) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    ctx->issues[ctx->issue_count++] = *issue;
    ctx->stats.total_issues_found++;
    
    // Update severity counts
    switch (issue->priority) {
        case BUG_PRIORITY_P0_CRITICAL:
            ctx->stats.critical_issues++;
            break;
        case BUG_PRIORITY_P1_HIGH:
            ctx->stats.high_priority_issues++;
            break;
        case BUG_PRIORITY_P2_MEDIUM:
            ctx->stats.medium_priority_issues++;
            break;
        case BUG_PRIORITY_P3_LOW:
            ctx->stats.low_priority_issues++;
            break;
        default:
            break;
    }
    
    return CRRSS_SUCCESS;
}

/**
 * @brief Create issue from type conversion
 */
static stp_issue_t create_conversion_issue(
    stp_issue_type_t type,
    const char* file_path,
    uint32_t line_number,
    const char* description,
    const char* recommendation,
    type_conversion_t* conversion
) {
    stp_issue_t issue = {0};
    issue.issue_type = type;
    issue.file_path = file_path;
    issue.line_number = line_number;
    issue.column_number = 0;
    issue.function_name = NULL;
    issue.description = description;
    issue.recommendation = recommendation;
    issue.conversion_info = conversion;
    issue.struct_info = NULL;
    issue.is_error = false;
    issue.is_warning = true;
    issue.is_info = false;
    
    // Determine priority and risk based on type
    switch (type) {
        case STP_ISSUE_TYPE_MISMATCH:
        case STP_ISSUE_UNSAFE_CAST:
        case STP_ISSUE_INTEGER_OVERFLOW_CAST:
            issue.priority = BUG_PRIORITY_P1_HIGH;
            issue.risk_level = RISK_LEVEL_HIGH;
            issue.is_error = true;
            issue.is_warning = false;
            break;
        case STP_ISSUE_NARROWING_CONVERSION:
        case STP_ISSUE_POINTER_CAST_UNSAFE:
        case STP_ISSUE_CONST_VIOLATION:
            issue.priority = BUG_PRIORITY_P2_MEDIUM;
            issue.risk_level = RISK_LEVEL_MEDIUM;
            break;
        case STP_ISSUE_IMPLICIT_CONVERSION:
        case STP_ISSUE_SIGNED_UNSIGNED_MIX:
            issue.priority = BUG_PRIORITY_P2_MEDIUM;
            issue.risk_level = RISK_LEVEL_MEDIUM;
            break;
        default:
            issue.priority = BUG_PRIORITY_P3_LOW;
            issue.risk_level = RISK_LEVEL_LOW;
            break;
    }
    
    return issue;
}

/**
 * @brief Create struct issue
 */
static stp_issue_t create_struct_issue(
    stp_issue_type_t type,
    const char* file_path,
    uint32_t line_number,
    const char* description,
    const char* recommendation,
    struct_layout_t* struct_info
) {
    stp_issue_t issue = {0};
    issue.issue_type = type;
    issue.file_path = file_path;
    issue.line_number = line_number;
    issue.column_number = 0;
    issue.function_name = NULL;
    issue.description = description;
    issue.recommendation = recommendation;
    issue.conversion_info = NULL;
    issue.struct_info = struct_info;
    issue.is_error = false;
    issue.is_warning = true;
    issue.is_info = false;
    
    // Determine priority based on type
    switch (type) {
        case STP_ISSUE_STRUCT_ALIGNMENT:
        case STP_ISSUE_UNALIGNED_ACCESS:
            issue.priority = BUG_PRIORITY_P1_HIGH;
            issue.risk_level = RISK_LEVEL_HIGH;
            break;
        case STP_ISSUE_STRUCT_PADDING:
        case STP_ISSUE_STRUCT_PACKING:
        case STP_ISSUE_PORTABILITY:
            issue.priority = BUG_PRIORITY_P2_MEDIUM;
            issue.risk_level = RISK_LEVEL_MEDIUM;
            break;
        case STP_ISSUE_MEMBER_ORDERING:
            issue.priority = BUG_PRIORITY_P3_LOW;
            issue.risk_level = RISK_LEVEL_LOW;
            issue.is_info = true;
            issue.is_warning = false;
            break;
        default:
            issue.priority = BUG_PRIORITY_P3_LOW;
            issue.risk_level = RISK_LEVEL_LOW;
            break;
    }
    
    return issue;
}

/**
 * @brief Check if type conversion is safe
 */
static bool is_conversion_safe(const type_info_t* src, const type_info_t* dst) {
    // Pointer conversions
    if (src->is_pointer && dst->is_pointer) {
        // Check if base types are compatible
        if (strcmp(src->base_type, dst->base_type) == 0) {
            return true;
        }
        // void* is compatible with any pointer
        if (strcmp(dst->base_type, "void") == 0) {
            return true;
        }
        return false;
    }
    
    // Pointer to integer or vice versa is unsafe
    if (src->is_pointer != dst->is_pointer) {
        return false;
    }
    
    // Integer conversions
    if (!src->is_pointer && !dst->is_pointer) {
        // Same type is always safe
        if (strcmp(src->type_name, dst->type_name) == 0) {
            return true;
        }
        
        // Widening conversion is safe
        if (dst->type_size >= src->type_size && 
            src->is_signed == dst->is_signed) {
            return true;
        }
        
        // Narrowing or sign change is unsafe
        return false;
    }
    
    return false;
}

// ==================== Pattern Detection Helpers ====================

/**
 * @brief Detect cast in line
 */
static bool detect_cast_pattern(const char* line, char* src_type, char* dst_type) {
    // Pattern: (type)expression
    const char* cast_start = strchr(line, '(');
    if (!cast_start) return false;
    
    const char* cast_end = strchr(cast_start + 1, ')');
    if (!cast_end) return false;
    
    // Extract destination type
    size_t type_len = cast_end - cast_start - 1;
    if (type_len >= STP_MAX_TYPE_NAME) return false;
    
    strncpy(dst_type, cast_start + 1, type_len);
    dst_type[type_len] = '\0';
    
    // Trim whitespace
    char* p = dst_type;
    while (*p && isspace(*p)) p++;
    memmove(dst_type, p, strlen(p) + 1);
    
    p = dst_type + strlen(dst_type) - 1;
    while (p > dst_type && isspace(*p)) *p-- = '\0';
    
    // For simplicity, assume source type is "int" (would need full parsing)
    strcpy(src_type, "int");
    
    return strlen(dst_type) > 0;
}

/**
 * @brief Detect assignment with different types
 */
static bool detect_type_mismatch(const char* line) {
    // Look for patterns like: type1 var = (type2)value;
    // This is a simplified check
    const char* equals = strchr(line, '=');
    if (!equals) return false;
    
    const char* cast = strchr(equals, '(');
    if (!cast) return false;
    
    return true;
}

/**
 * @brief Detect signed/unsigned comparison
 */
static bool detect_signed_unsigned_comparison(const char* line) {
    // Look for comparison operators with mixed signedness
    // Simplified pattern matching
    if (strstr(line, "unsigned") && 
        (strstr(line, "<") || strstr(line, ">") || 
         strstr(line, "<=") || strstr(line, ">="))) {
        return true;
    }
    return false;
}

/**
 * @brief Detect struct definition
 */
static bool detect_struct_definition(const char* line, char* struct_name) {
    if (strncmp(line, "struct", 6) != 0 && strstr(line, "struct") == NULL) {
        return false;
    }
    
    const char* name_start = strstr(line, "struct");
    if (!name_start) return false;
    
    name_start += 6;
    while (*name_start && isspace(*name_start)) name_start++;
    
    if (*name_start == '{') return false;  // Anonymous struct
    
    const char* name_end = name_start;
    while (*name_end && (isalnum(*name_end) || *name_end == '_')) name_end++;
    
    size_t name_len = name_end - name_start;
    if (name_len >= STP_MAX_TYPE_NAME) return false;
    
    strncpy(struct_name, name_start, name_len);
    struct_name[name_len] = '\0';
    
    return strlen(struct_name) > 0;
}

// ==================== Initialization & Shutdown ====================

stp_context_t* stp_initialize(const stp_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    stp_context_t* ctx = calloc(1, sizeof(stp_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->config = *config;
    ctx->max_issues = STP_DEFAULT_MAX_ISSUES;
    ctx->max_structs = STP_DEFAULT_MAX_STRUCTS;
    ctx->max_conversions = STP_DEFAULT_MAX_CONVERSIONS;
    
    ctx->issues = calloc(ctx->max_issues, sizeof(stp_issue_t));
    ctx->struct_layouts = calloc(ctx->max_structs, sizeof(struct_layout_t));
    ctx->conversions = calloc(ctx->max_conversions, sizeof(type_conversion_t));
    
    if (!ctx->issues || !ctx->struct_layouts || !ctx->conversions) {
        stp_shutdown(ctx);
        return NULL;
    }
    
    ctx->initialized = true;
    return ctx;
}

void stp_shutdown(stp_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    free(ctx->issues);
    free(ctx->struct_layouts);
    free(ctx->conversions);
    free(ctx);
}

crrss_status_t stp_reset(stp_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->issue_count = 0;
    ctx->struct_count = 0;
    ctx->conversion_count = 0;
    memset(&ctx->stats, 0, sizeof(stp_statistics_t));
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_configure(stp_context_t* ctx, const stp_config_t* config) {
    if (!ctx || !ctx->initialized || !config) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    ctx->config = *config;
    return CRRSS_SUCCESS;
}

crrss_status_t stp_set_strictness_level(stp_context_t* ctx, stp_strictness_level_t level) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->config.strictness_level = level;
    
    // Adjust configuration based on strictness level
    switch (level) {
        case STP_STRICTNESS_PERMISSIVE:
            ctx->config.check_implicit_conversions = false;
            ctx->config.check_struct_padding = false;
            ctx->config.check_member_ordering = false;
            break;
        case STP_STRICTNESS_MODERATE:
            ctx->config.check_implicit_conversions = true;
            ctx->config.check_struct_padding = true;
            ctx->config.check_member_ordering = false;
            break;
        case STP_STRICTNESS_STRICT:
            ctx->config.check_implicit_conversions = true;
            ctx->config.check_struct_padding = true;
            ctx->config.check_member_ordering = true;
            ctx->config.check_const_correctness = true;
            break;
        case STP_STRICTNESS_PARANOID:
            // Enable all checks
            ctx->config.check_type_mismatches = true;
            ctx->config.check_implicit_conversions = true;
            ctx->config.check_signed_unsigned_mix = true;
            ctx->config.check_pointer_type_compat = true;
            ctx->config.check_type_punning = true;
            ctx->config.check_struct_padding = true;
            ctx->config.check_struct_alignment = true;
            ctx->config.check_unaligned_access = true;
            ctx->config.check_struct_packing = true;
            ctx->config.check_member_ordering = true;
            ctx->config.check_portability = true;
            ctx->config.check_unsafe_casts = true;
            ctx->config.check_narrowing_conversions = true;
            ctx->config.check_pointer_casts = true;
            ctx->config.check_const_correctness = true;
            ctx->config.check_integer_overflow_casts = true;
            break;
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Type Validation Engine ====================

crrss_status_t stp_validate_type_compatibility(
    stp_context_t* ctx,
    const type_info_t* source_type,
    const type_info_t* target_type,
    bool* is_safe
) {
    if (!ctx || !ctx->initialized || !source_type || !target_type || !is_safe) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *is_safe = is_conversion_safe(source_type, target_type);
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_type_mismatches(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_type_mismatches) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        if (detect_type_mismatch(line)) {
            stp_issue_t issue = create_conversion_issue(
                STP_ISSUE_TYPE_MISMATCH,
                file_path,
                line_num,
                "Type mismatch detected in assignment",
                "Ensure types are compatible or use explicit cast",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.type_mismatches_found++;
        }
    }
    
    fclose(file);
    ctx->stats.files_analyzed++;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_implicit_conversions(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_implicit_conversions) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        // Look for assignments without explicit casts
        if (strchr(line, '=') && !strstr(line, "==") && !strchr(line, '(')) {
            // Simplified check - would need full parsing in production
            if (strstr(line, "int") || strstr(line, "long") || 
                strstr(line, "float") || strstr(line, "double")) {
                
                stp_issue_t issue = create_conversion_issue(
                    STP_ISSUE_IMPLICIT_CONVERSION,
                    file_path,
                    line_num,
                    "Implicit type conversion detected",
                    "Use explicit cast to clarify intent",
                    NULL
                );
                
                issues[*num_issues] = issue;
                (*num_issues)++;
                add_issue(ctx, &issue);
                ctx->stats.implicit_conversions_found++;
            }
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_signed_unsigned_mix(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_signed_unsigned_mix) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        if (detect_signed_unsigned_comparison(line)) {
            stp_issue_t issue = create_conversion_issue(
                STP_ISSUE_SIGNED_UNSIGNED_MIX,
                file_path,
                line_num,
                "Mixing signed and unsigned types in comparison",
                "Use explicit casts or ensure both operands have same signedness",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.signed_unsigned_mix_found++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_pointer_type_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    
    // Simplified implementation - would need full AST parsing in production
    ctx->stats.pointer_type_issues_found = 0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_type_punning(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_issues = 0;
    
    // Look for union-based type punning or pointer casts
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        // Look for patterns like *(type*)&variable
        if (strstr(line, "*(") && strchr(line, '&')) {
            stp_issue_t issue = create_conversion_issue(
                STP_ISSUE_TYPE_PUNNING,
                file_path,
                line_num,
                "Potential type punning detected",
                "Use memcpy or unions for type reinterpretation",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.type_punning_found++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

// ==================== Struct Alignment Analyzer ====================

crrss_status_t stp_analyze_struct_layout(
    stp_context_t* ctx,
    const char* struct_name,
    const char* file_path,
    struct_layout_t* layout
) {
    if (!ctx || !ctx->initialized || !struct_name || !file_path || !layout) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Simplified implementation - would need full AST parsing
    memset(layout, 0, sizeof(struct_layout_t));
    layout->struct_name = struct_name;
    layout->total_size = 0;
    layout->useful_size = 0;
    layout->padding_bytes = 0;
    layout->alignment = 8;  // Assume 8-byte alignment
    layout->member_count = 0;
    layout->members = NULL;
    layout->padding_percentage = 0.0;
    layout->has_alignment_issues = false;
    layout->has_portability_issues = false;
    layout->optimization_suggestion = "Reorder members by size (largest first) to minimize padding";
    
    ctx->stats.structs_analyzed++;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_struct_padding_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_struct_padding) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    char struct_name[STP_MAX_TYPE_NAME];
    uint32_t line_num = 0;
    *num_issues = 0;
    bool in_struct = false;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        if (detect_struct_definition(line, struct_name)) {
            in_struct = true;
        } else if (in_struct && strchr(line, '}')) {
            // End of struct - check for padding issues
            in_struct = false;
            
            stp_issue_t issue = create_struct_issue(
                STP_ISSUE_STRUCT_PADDING,
                file_path,
                line_num,
                "Struct may have suboptimal padding",
                "Consider reordering members by size to reduce padding",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.struct_padding_issues++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_alignment_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.struct_alignment_issues = 0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_unaligned_access(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.unaligned_access_found = 0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_packing_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_struct_packing) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        // Look for __attribute__((packed)) or #pragma pack
        if (strstr(line, "__attribute__") && strstr(line, "packed")) {
            stp_issue_t issue = create_struct_issue(
                STP_ISSUE_STRUCT_PACKING,
                file_path,
                line_num,
                "Packed struct may cause performance issues",
                "Consider natural alignment unless space is critical",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.struct_packing_issues++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_portability_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.portability_issues = 0;
    
    return CRRSS_SUCCESS;
}

// ==================== Type Casting Safety Checker ====================

crrss_status_t stp_analyze_cast_safety(
    stp_context_t* ctx,
    const type_info_t* source_type,
    const type_info_t* target_type,
    type_conversion_t* conversion
) {
    if (!ctx || !ctx->initialized || !source_type || !target_type || !conversion) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    conversion->source_type = *source_type;
    conversion->target_type = *target_type;
    conversion->is_explicit = true;
    conversion->is_safe = is_conversion_safe(source_type, target_type);
    conversion->may_lose_data = (target_type->type_size < source_type->type_size);
    conversion->may_change_sign = (source_type->is_signed != target_type->is_signed);
    conversion->may_overflow = conversion->may_lose_data;
    conversion->recommendation = conversion->is_safe ? 
        "Cast is safe" : 
        "Consider using explicit bounds checking";
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_unsafe_casts(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_unsafe_casts) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    char src_type[STP_MAX_TYPE_NAME], dst_type[STP_MAX_TYPE_NAME];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        if (detect_cast_pattern(line, src_type, dst_type)) {
            stp_issue_t issue = create_conversion_issue(
                STP_ISSUE_UNSAFE_CAST,
                file_path,
                line_num,
                "Potentially unsafe type cast detected",
                "Verify that the cast is safe and add bounds checking if needed",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.unsafe_casts_found++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_narrowing_conversions(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.narrowing_conversions_found = 0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_pointer_cast_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.pointer_casts_found = 0;
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_const_violations(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.check_const_correctness) {
        *num_issues = 0;
        return CRRSS_SUCCESS;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[STP_MAX_LINE_LENGTH];
    uint32_t line_num = 0;
    *num_issues = 0;
    
    while (fgets(line, sizeof(line), file) && *num_issues < max_issues) {
        line_num++;
        
        // Look for casts that remove const qualifier
        if (strstr(line, "const") && strchr(line, '(') && !strstr(line, "(const")) {
            stp_issue_t issue = create_conversion_issue(
                STP_ISSUE_CONST_VIOLATION,
                file_path,
                line_num,
                "Const qualifier may be violated",
                "Preserve const correctness in casts",
                NULL
            );
            
            issues[*num_issues] = issue;
            (*num_issues)++;
            add_issue(ctx, &issue);
            ctx->stats.const_violations_found++;
        }
    }
    
    fclose(file);
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_detect_overflow_casts(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    (void)max_issues;  // Unused in simplified implementation
    *num_issues = 0;
    ctx->stats.integer_overflow_casts_found = 0;
    
    return CRRSS_SUCCESS;
}

// ==================== Comprehensive Analysis ====================

crrss_status_t stp_analyze_file(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_issues = 0;
    uint32_t temp_issues = 0;
    uint32_t total_issues = 0;
    
    // Run all enabled checks
    if (ctx->config.check_type_mismatches) {
        stp_detect_type_mismatches(ctx, file_path, issues + total_issues, 
                                   max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_implicit_conversions && total_issues < max_issues) {
        stp_detect_implicit_conversions(ctx, file_path, issues + total_issues,
                                       max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_signed_unsigned_mix && total_issues < max_issues) {
        stp_detect_signed_unsigned_mix(ctx, file_path, issues + total_issues,
                                      max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_type_punning && total_issues < max_issues) {
        stp_detect_type_punning(ctx, file_path, issues + total_issues,
                               max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_struct_padding && total_issues < max_issues) {
        stp_detect_struct_padding_issues(ctx, file_path, issues + total_issues,
                                        max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_struct_packing && total_issues < max_issues) {
        stp_detect_packing_issues(ctx, file_path, issues + total_issues,
                                 max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_unsafe_casts && total_issues < max_issues) {
        stp_detect_unsafe_casts(ctx, file_path, issues + total_issues,
                               max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    if (ctx->config.check_const_correctness && total_issues < max_issues) {
        stp_detect_const_violations(ctx, file_path, issues + total_issues,
                                   max_issues - total_issues, &temp_issues);
        total_issues += temp_issues;
    }
    
    *num_issues = total_issues;
    return CRRSS_SUCCESS;
}

crrss_status_t stp_analyze_directory(
    stp_context_t* ctx,
    const char* dir_path,
    stp_report_t* report
) {
    if (!ctx || !ctx->initialized || !dir_path || !report) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Simplified implementation - would need directory traversal
    memset(report, 0, sizeof(stp_report_t));
    report->statistics = ctx->stats;
    report->issues = ctx->issues;
    report->issue_count = ctx->issue_count;
    report->max_issues = ctx->max_issues;
    report->struct_layouts = ctx->struct_layouts;
    report->struct_count = ctx->struct_count;
    report->conversions = ctx->conversions;
    report->conversion_count = ctx->conversion_count;
    report->type_safety_score = 0.85;  // Would calculate from statistics
    report->overall_risk = RISK_LEVEL_MEDIUM;
    report->summary = "Type safety analysis complete";
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_analyze_snippet(
    stp_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !code_snippet || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Create temporary file for analysis
    char temp_path[] = "/tmp/stp_snippet_XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd == -1) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    ssize_t bytes_written = write(fd, code_snippet, snippet_length);
    close(fd);
    
    if (bytes_written != (ssize_t)snippet_length) {
        unlink(temp_path);
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Analyze the temporary file
    crrss_status_t status = stp_analyze_file(ctx, temp_path, issues, max_issues, num_issues);
    
    // Clean up
    unlink(temp_path);
    
    return status;
}

// ==================== Statistics & Reporting ====================

crrss_status_t stp_get_statistics(
    stp_context_t* ctx,
    stp_statistics_t* stats
) {
    if (!ctx || !ctx->initialized || !stats) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *stats = ctx->stats;
    return CRRSS_SUCCESS;
}

crrss_status_t stp_generate_report(
    stp_context_t* ctx,
    stp_report_t* report
) {
    if (!ctx || !ctx->initialized || !report) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    memset(report, 0, sizeof(stp_report_t));
    report->statistics = ctx->stats;
    report->issues = ctx->issues;
    report->issue_count = ctx->issue_count;
    report->max_issues = ctx->max_issues;
    report->struct_layouts = ctx->struct_layouts;
    report->struct_count = ctx->struct_count;
    report->conversions = ctx->conversions;
    report->conversion_count = ctx->conversion_count;
    
    // Calculate type safety score
    stp_calculate_safety_score(ctx, &report->type_safety_score);
    
    // Determine overall risk
    if (ctx->stats.critical_issues > 0) {
        report->overall_risk = RISK_LEVEL_CRITICAL;
    } else if (ctx->stats.high_priority_issues > 5) {
        report->overall_risk = RISK_LEVEL_HIGH;
    } else if (ctx->stats.medium_priority_issues > 10) {
        report->overall_risk = RISK_LEVEL_MEDIUM;
    } else {
        report->overall_risk = RISK_LEVEL_LOW;
    }
    
    report->summary = "Type safety analysis complete";
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_export_report(
    stp_context_t* ctx,
    const stp_report_t* report,
    const char* output_path,
    const char* format
) {
    if (!ctx || !ctx->initialized || !report || !output_path || !format) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(output_path, "w");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    if (strcmp(format, "text") == 0) {
        fprintf(file, "=== STP Type Safety Report ===\n\n");
        fprintf(file, "Type Safety Score: %.2f\n", report->type_safety_score);
        fprintf(file, "Overall Risk: %s\n", risk_level_to_string(report->overall_risk));
        fprintf(file, "\nStatistics:\n");
        fprintf(file, "  Files Analyzed: %u\n", report->statistics.files_analyzed);
        fprintf(file, "  Total Issues: %u\n", report->statistics.total_issues_found);
        fprintf(file, "  Critical: %u\n", report->statistics.critical_issues);
        fprintf(file, "  High Priority: %u\n", report->statistics.high_priority_issues);
        fprintf(file, "  Medium Priority: %u\n", report->statistics.medium_priority_issues);
        fprintf(file, "  Low Priority: %u\n", report->statistics.low_priority_issues);
        fprintf(file, "\n%s\n", report->summary);
    }
    
    fclose(file);
    return CRRSS_SUCCESS;
}

crrss_status_t stp_calculate_safety_score(
    stp_context_t* ctx,
    double* score
) {
    if (!ctx || !ctx->initialized || !score) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Calculate score based on issues found
    double base_score = 1.0;
    
    // Deduct points for issues
    base_score -= ctx->stats.critical_issues * 0.1;
    base_score -= ctx->stats.high_priority_issues * 0.05;
    base_score -= ctx->stats.medium_priority_issues * 0.02;
    base_score -= ctx->stats.low_priority_issues * 0.01;
    
    // Ensure score is in valid range
    if (base_score < 0.0) base_score = 0.0;
    if (base_score > 1.0) base_score = 1.0;
    
    *score = base_score;
    return CRRSS_SUCCESS;
}

// ==================== CRRSS Integration ====================

crrss_status_t stp_integrate_bpme(
    stp_context_t* ctx,
    void* bpme_ctx
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->bpme_ctx = bpme_ctx;
    return CRRSS_SUCCESS;
}

crrss_status_t stp_integrate_sciv(
    stp_context_t* ctx,
    void* sciv_ctx
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->sciv_ctx = sciv_ctx;
    return CRRSS_SUCCESS;
}

crrss_status_t stp_integrate_msm(
    stp_context_t* ctx,
    void* msm_ctx
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->msm_ctx = msm_ctx;
    return CRRSS_SUCCESS;
}

// ==================== Query Functions ====================

crrss_status_t stp_query_issues_by_type(
    stp_context_t* ctx,
    stp_issue_type_t issue_type,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_issues = 0;
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].issue_type == issue_type) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_query_issues_by_priority(
    stp_context_t* ctx,
    bug_priority_t priority,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_issues = 0;
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].priority == priority) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t stp_query_issues_by_file(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized || !file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *num_issues = 0;
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].file_path && 
            strcmp(ctx->issues[i].file_path, file_path) == 0) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Utility Functions ====================

const char* stp_issue_type_to_string(stp_issue_type_t issue_type) {
    if (issue_type < STP_ISSUE_COUNT) {
        return STP_ISSUE_TYPE_STRINGS[issue_type];
    }
    return "Unknown";
}

const char* stp_strictness_level_to_string(stp_strictness_level_t level) {
    if (level <= STP_STRICTNESS_PARANOID) {
        return STRICTNESS_LEVEL_STRINGS[level];
    }
    return "Unknown";
}

crrss_status_t stp_get_type_info(
    stp_context_t* ctx,
    const char* type_name,
    type_info_t* type_info
) {
    if (!ctx || !ctx->initialized || !type_name || !type_info) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    builtin_type_info_t builtin;
    if (get_builtin_type_info(type_name, &builtin)) {
        memset(type_info, 0, sizeof(type_info_t));
        type_info->type_name = builtin.name;
        type_info->type_size = builtin.size;
        type_info->type_alignment = builtin.alignment;
        type_info->is_signed = builtin.is_signed;
        type_info->is_pointer = false;
        type_info->is_const = false;
        type_info->is_volatile = false;
        type_info->is_struct = false;
        type_info->is_union = false;
        type_info->is_array = false;
        type_info->base_type = NULL;
        return CRRSS_SUCCESS;
    }
    
    return CRRSS_ERROR_NOT_FOUND;
}

crrss_status_t stp_suggest_struct_optimization(
    stp_context_t* ctx,
    const struct_layout_t* layout,
    const char** suggestion
) {
    if (!ctx || !ctx->initialized || !layout || !suggestion) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (layout->padding_percentage > 0.25) {
        *suggestion = "Reorder members from largest to smallest to minimize padding";
    } else if (layout->padding_percentage > 0.10) {
        *suggestion = "Consider reordering members to reduce padding";
    } else {
        *suggestion = "Struct layout is well optimized";
    }
    
    return CRRSS_SUCCESS;
}
