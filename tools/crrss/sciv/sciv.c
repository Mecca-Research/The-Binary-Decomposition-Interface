/**
 * @file sciv.c
 * @brief Self-Check Internal Validator Implementation
 */

#include "sciv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

// ==================== Internal Structures ====================

typedef struct {
    validation_rule_t rule;
    const char* rule_name;
    const char* description;
    bool enabled;
    uint32_t violations;
} rule_entry_t;

struct sciv_context {
    sciv_config_t config;
    rule_entry_t rules[RULE_COUNT];
    uint32_t total_validations;
    uint32_t total_issues;
    double avg_compliance;
    validation_issue_t* issue_cache;
    uint32_t issue_cache_size;
    uint32_t cached_issues;
    bool initialized;
};

// ==================== Rule Definitions ====================

static const char* RULE_NAMES[] = {
    "Memory Safety",
    "Error Handling",
    "NULL Checks",
    "Coding Style",
    "Function Complexity",
    "Comment Quality",
    "Naming Convention",
    "API Usage",
    "Concurrency Safety",
    "Resource Cleanup"
};

static const char* RULE_DESCRIPTIONS[] = {
    "Validates memory allocation, deallocation, and access patterns",
    "Ensures proper error checking and handling",
    "Verifies NULL pointer checks before dereference",
    "Checks compliance with coding style guidelines",
    "Measures and validates function complexity",
    "Validates code documentation quality",
    "Checks naming conventions for variables and functions",
    "Validates correct API usage patterns",
    "Checks for thread-safety and synchronization",
    "Ensures proper resource acquisition and cleanup"
};

// ==================== Helper Functions ====================

static bool is_c_source_file(const char* filename) {
    size_t len = strlen(filename);
    return (len > 2 && (
        strcmp(filename + len - 2, ".c") == 0 ||
        strcmp(filename + len - 2, ".h") == 0 ||
        (len > 4 && strcmp(filename + len - 4, ".cpp") == 0) ||
        (len > 4 && strcmp(filename + len - 4, ".hpp") == 0)
    ));
}

static uint32_t count_lines(const char* file_path) {
    FILE* fp = fopen(file_path, "r");
    if (!fp) return 0;
    
    uint32_t lines = 0;
    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') lines++;
    }
    
    fclose(fp);
    return lines;
}

static uint32_t calculate_cyclomatic_complexity(const char* code) {
    uint32_t complexity = 1;  // Base complexity
    
    // Count decision points
    const char* keywords[] = {"if", "for", "while", "case", "&&", "||", "?", NULL};
    
    for (int i = 0; keywords[i]; i++) {
        const char* pos = code;
        while ((pos = strstr(pos, keywords[i])) != NULL) {
            complexity++;
            pos += strlen(keywords[i]);
        }
    }
    
    return complexity;
}

__attribute__((unused))
static bool has_null_check(const char* line, const char* var_name) {
    char pattern1[256], pattern2[256];
    snprintf(pattern1, sizeof(pattern1), "%s == NULL", var_name);
    snprintf(pattern2, sizeof(pattern2), "%s != NULL", var_name);
    
    return (strstr(line, pattern1) != NULL || strstr(line, pattern2) != NULL);
}

static bool has_error_check(const char* line) {
    return (strstr(line, "if") && (
        strstr(line, "== NULL") || strstr(line, "!= NULL") ||
        strstr(line, "< 0") || strstr(line, "== -1") ||
        strstr(line, "error") || strstr(line, "status")
    ));
}

__attribute__((unused))
static bool is_proper_function_name(const char* name) {
    if (!name || !*name) return false;
    
    // Check if starts with lowercase or underscore (kernel style)
    if (!islower(name[0]) && name[0] != '_') return false;
    
    // Check for snake_case
    for (const char* p = name; *p; p++) {
        if (!islower(*p) && !isdigit(*p) && *p != '_') {
            return false;
        }
    }
    
    return true;
}

static validation_result_t check_memory_safety_line(const char* line, uint32_t line_num) {
    (void)line_num;  // Parameter reserved for future use
    // Check for malloc without NULL check
    if (strstr(line, "malloc") || strstr(line, "calloc") || strstr(line, "realloc")) {
        // This is a simplified check - proper implementation would track across lines
        return VALIDATION_WARNING;
    }
    
    // Check for potential buffer overflow
    if (strstr(line, "strcpy") || strstr(line, "strcat") || strstr(line, "sprintf")) {
        return VALIDATION_FAIL;
    }
    
    return VALIDATION_PASS;
}

static validation_result_t check_error_handling_line(const char* line) {
    // Check if function call exists without error check
    if ((strstr(line, "malloc") || strstr(line, "open") || 
         strstr(line, "fopen") || strstr(line, "read")) &&
        !has_error_check(line)) {
        return VALIDATION_WARNING;
    }
    
    return VALIDATION_PASS;
}

static validation_result_t check_null_checks_line(const char* line) {
    // Check for pointer dereference without NULL check
    if ((strstr(line, "->") || strstr(line, "*")) &&
        !strstr(line, "NULL")) {
        return VALIDATION_WARNING;
    }
    
    return VALIDATION_PASS;
}

// ==================== Validation Implementation ====================

static crrss_status_t validate_file_content(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[1024];
    uint32_t line_num = 0;
    uint32_t issue_count = 0;
    
    while (fgets(line, sizeof(line), fp) && issue_count < max_issues) {
        line_num++;
        
        // Memory safety check
        if (ctx->rules[RULE_MEMORY_SAFETY].enabled) {
            validation_result_t result = check_memory_safety_line(line, line_num);
            if (result != VALIDATION_PASS && issue_count < max_issues) {
                issues[issue_count].file_path = strdup(file_path);
                issues[issue_count].line_number = line_num;
                issues[issue_count].result = result;
                issues[issue_count].rule_name = "Memory Safety";
                issues[issue_count].message = strdup("Potential memory safety issue");
                issues[issue_count].suggestion = strdup("Use safe string functions (strncpy, snprintf)");
                issue_count++;
                ctx->rules[RULE_MEMORY_SAFETY].violations++;
            }
        }
        
        // Error handling check
        if (ctx->rules[RULE_ERROR_HANDLING].enabled) {
            validation_result_t result = check_error_handling_line(line);
            if (result != VALIDATION_PASS && issue_count < max_issues) {
                issues[issue_count].file_path = strdup(file_path);
                issues[issue_count].line_number = line_num;
                issues[issue_count].result = result;
                issues[issue_count].rule_name = "Error Handling";
                issues[issue_count].message = strdup("Missing error check");
                issues[issue_count].suggestion = strdup("Add NULL/error check");
                issue_count++;
                ctx->rules[RULE_ERROR_HANDLING].violations++;
            }
        }
        
        // NULL checks
        if (ctx->rules[RULE_NULL_CHECKS].enabled) {
            validation_result_t result = check_null_checks_line(line);
            if (result != VALIDATION_PASS && issue_count < max_issues) {
                issues[issue_count].file_path = strdup(file_path);
                issues[issue_count].line_number = line_num;
                issues[issue_count].result = result;
                issues[issue_count].rule_name = "NULL Checks";
                issues[issue_count].message = strdup("Potential NULL dereference");
                issues[issue_count].suggestion = strdup("Add NULL check before dereference");
                issue_count++;
                ctx->rules[RULE_NULL_CHECKS].violations++;
            }
        }
    }
    
    fclose(fp);
    *num_issues = issue_count;
    
    return CRRSS_SUCCESS;
}

// ==================== Public API Implementation ====================

sciv_context_t* sciv_initialize(const sciv_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    sciv_context_t* ctx = (sciv_context_t*)calloc(1, sizeof(sciv_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copy configuration
    ctx->config = *config;
    if (config->coding_standard) {
        ctx->config.coding_standard = strdup(config->coding_standard);
    }
    
    // Initialize rules
    for (int i = 0; i < RULE_COUNT; i++) {
        ctx->rules[i].rule = (validation_rule_t)i;
        ctx->rules[i].rule_name = RULE_NAMES[i];
        ctx->rules[i].description = RULE_DESCRIPTIONS[i];
        ctx->rules[i].enabled = true;  // Enable all by default
        ctx->rules[i].violations = 0;
    }
    
    // Allocate issue cache
    ctx->issue_cache_size = 10000;  // Large cache for issues
    ctx->issue_cache = (validation_issue_t*)calloc(
        ctx->issue_cache_size, sizeof(validation_issue_t)
    );
    
    ctx->total_validations = 0;
    ctx->total_issues = 0;
    ctx->avg_compliance = 1.0;
    ctx->cached_issues = 0;
    ctx->initialized = true;
    
    return ctx;
}

void sciv_shutdown(sciv_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->config.coding_standard) {
        free((void*)ctx->config.coding_standard);
    }
    
    if (ctx->issue_cache) {
        for (uint32_t i = 0; i < ctx->cached_issues; i++) {
            if (ctx->issue_cache[i].file_path) {
                free((void*)ctx->issue_cache[i].file_path);
            }
            if (ctx->issue_cache[i].message) {
                free((void*)ctx->issue_cache[i].message);
            }
            if (ctx->issue_cache[i].suggestion) {
                free((void*)ctx->issue_cache[i].suggestion);
            }
        }
        free(ctx->issue_cache);
    }
    
    free(ctx);
}

crrss_status_t sciv_validate_file(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    crrss_status_t status = validate_file_content(
        ctx, file_path, issues, max_issues, num_issues
    );
    
    if (status == CRRSS_SUCCESS) {
        ctx->total_validations++;
        ctx->total_issues += *num_issues;
        
        // Update compliance score
        uint32_t total_lines = count_lines(file_path);
        if (total_lines > 0) {
            double file_compliance = 1.0 - ((double)*num_issues / total_lines);
            ctx->avg_compliance = (ctx->avg_compliance * (ctx->total_validations - 1) + 
                                  file_compliance) / ctx->total_validations;
        }
    }
    
    return status;
}

crrss_status_t sciv_validate_directory(
    sciv_context_t* ctx,
    const char* dir_path,
    validation_report_t* report
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!dir_path || !report) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    report->total_files_validated = 0;
    report->total_issues_found = 0;
    report->errors = 0;
    report->warnings = 0;
    report->suggestions = 0;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode) && is_c_source_file(entry->d_name)) {
                validation_issue_t file_issues[1000];
                uint32_t num_issues = 0;
                
                crrss_status_t status = sciv_validate_file(
                    ctx, full_path, file_issues, 1000, &num_issues
                );
                
                if (status == CRRSS_SUCCESS) {
                    report->total_files_validated++;
                    report->total_issues_found += num_issues;
                    
                    // Count issue types
                    for (uint32_t i = 0; i < num_issues; i++) {
                        if (file_issues[i].result == VALIDATION_FAIL) {
                            report->errors++;
                        } else if (file_issues[i].result == VALIDATION_WARNING) {
                            report->warnings++;
                        }
                    }
                    
                    // Clean up
                    for (uint32_t i = 0; i < num_issues; i++) {
                        free((void*)file_issues[i].file_path);
                        free((void*)file_issues[i].message);
                        free((void*)file_issues[i].suggestion);
                    }
                }
            } else if (S_ISDIR(st.st_mode)) {
                // Recursive validation
                validation_report_t sub_report;
                sciv_validate_directory(ctx, full_path, &sub_report);
                
                report->total_files_validated += sub_report.total_files_validated;
                report->total_issues_found += sub_report.total_issues_found;
                report->errors += sub_report.errors;
                report->warnings += sub_report.warnings;
            }
        }
    }
    
    closedir(dir);
    
    // Calculate compliance score
    if (report->total_files_validated > 0) {
        report->compliance_score = ctx->avg_compliance;
    } else {
        report->compliance_score = 1.0;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t sciv_validate_snippet(
    sciv_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    (void)snippet_length;  // Parameter reserved for future use
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!code_snippet || !issues || !num_issues) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Simple validation for code snippet
    uint32_t issue_count = 0;
    
    if (strstr(code_snippet, "strcpy") && issue_count < max_issues) {
        issues[issue_count].file_path = strdup("<snippet>");
        issues[issue_count].line_number = 0;
        issues[issue_count].result = VALIDATION_FAIL;
        issues[issue_count].rule_name = "Memory Safety";
        issues[issue_count].message = strdup("Unsafe function used");
        issues[issue_count].suggestion = strdup("Use strncpy instead");
        issue_count++;
    }
    
    *num_issues = issue_count;
    return CRRSS_SUCCESS;
}

crrss_status_t sciv_check_rule(
    sciv_context_t* ctx,
    const char* file_path,
    validation_rule_t rule,
    validation_result_t* result
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !result || rule >= RULE_COUNT) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Validate specific rule
    validation_issue_t issues[100];
    uint32_t num_issues = 0;
    
    crrss_status_t status = sciv_validate_file(ctx, file_path, issues, 100, &num_issues);
    
    if (status == CRRSS_SUCCESS) {
        *result = (num_issues == 0) ? VALIDATION_PASS : VALIDATION_WARNING;
    }
    
    // Clean up
    for (uint32_t i = 0; i < num_issues; i++) {
        free((void*)issues[i].file_path);
        free((void*)issues[i].message);
        free((void*)issues[i].suggestion);
    }
    
    return status;
}

crrss_status_t sciv_configure_rule(
    sciv_context_t* ctx,
    validation_rule_t rule,
    bool enabled
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (rule >= RULE_COUNT) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    ctx->rules[rule].enabled = enabled;
    return CRRSS_SUCCESS;
}

crrss_status_t sciv_validate_memory_patterns(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    // Enable only memory safety rule
    bool old_states[RULE_COUNT];
    for (int i = 0; i < RULE_COUNT; i++) {
        old_states[i] = ctx->rules[i].enabled;
        ctx->rules[i].enabled = (i == RULE_MEMORY_SAFETY);
    }
    
    crrss_status_t status = sciv_validate_file(ctx, file_path, issues, max_issues, num_issues);
    
    // Restore rule states
    for (int i = 0; i < RULE_COUNT; i++) {
        ctx->rules[i].enabled = old_states[i];
    }
    
    return status;
}

crrss_status_t sciv_validate_error_handling(
    sciv_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    // Enable only error handling rule
    bool old_states[RULE_COUNT];
    for (int i = 0; i < RULE_COUNT; i++) {
        old_states[i] = ctx->rules[i].enabled;
        ctx->rules[i].enabled = (i == RULE_ERROR_HANDLING);
    }
    
    crrss_status_t status = sciv_validate_file(ctx, file_path, issues, max_issues, num_issues);
    
    // Restore rule states
    for (int i = 0; i < RULE_COUNT; i++) {
        ctx->rules[i].enabled = old_states[i];
    }
    
    return status;
}

crrss_status_t sciv_calculate_complexity(
    sciv_context_t* ctx,
    const char* file_path,
    uint32_t* complexity
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !complexity) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Read entire file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(fp);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    size_t bytes_read = fread(content, 1, file_size, fp);
    content[bytes_read] = '\0';
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        free(content);
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    *complexity = calculate_cyclomatic_complexity(content);
    
    free(content);
    return CRRSS_SUCCESS;
}

crrss_status_t sciv_get_quality_score(
    sciv_context_t* ctx,
    const char* file_path,
    double* score
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !score) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    validation_issue_t issues[1000];
    uint32_t num_issues = 0;
    
    crrss_status_t status = sciv_validate_file(ctx, file_path, issues, 1000, &num_issues);
    
    if (status == CRRSS_SUCCESS) {
        uint32_t total_lines = count_lines(file_path);
        if (total_lines > 0) {
            *score = 1.0 - ((double)num_issues / total_lines);
            if (*score < 0.0) *score = 0.0;
        } else {
            *score = 1.0;
        }
        
        // Clean up
        for (uint32_t i = 0; i < num_issues; i++) {
            free((void*)issues[i].file_path);
            free((void*)issues[i].message);
            free((void*)issues[i].suggestion);
        }
    }
    
    return status;
}

crrss_status_t sciv_get_statistics(
    sciv_context_t* ctx,
    uint32_t* total_validations,
    uint32_t* total_issues,
    double* avg_compliance
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (total_validations) *total_validations = ctx->total_validations;
    if (total_issues) *total_issues = ctx->total_issues;
    if (avg_compliance) *avg_compliance = ctx->avg_compliance;
    
    return CRRSS_SUCCESS;
}

crrss_status_t sciv_generate_report(
    sciv_context_t* ctx,
    const char* output_path,
    const char* format
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!output_path || !format) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Generate simple text report
    fprintf(fp, "=== SCIV Validation Report ===\n\n");
    fprintf(fp, "Total Validations: %u\n", ctx->total_validations);
    fprintf(fp, "Total Issues Found: %u\n", ctx->total_issues);
    fprintf(fp, "Average Compliance: %.2f%%\n", ctx->avg_compliance * 100);
    fprintf(fp, "\nRule Violations:\n");
    
    for (int i = 0; i < RULE_COUNT; i++) {
        fprintf(fp, "  %s: %u\n", ctx->rules[i].rule_name, ctx->rules[i].violations);
    }
    
    fclose(fp);
    return CRRSS_SUCCESS;
}
