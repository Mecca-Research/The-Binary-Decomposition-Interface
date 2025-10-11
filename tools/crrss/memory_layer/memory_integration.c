/**
 * @file memory_integration.c
 * @brief Memory Integration Layer Implementation
 */

#include "memory_integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ==================== Internal Structures ====================

struct memory_integration_context {
    memory_integration_config_t config;
    allocation_record_t* allocation_records;
    uint32_t num_allocations;
    uint32_t max_allocations;
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
    bool initialized;
};

// ==================== Helper Functions ====================

static uint64_t get_timestamp(void) {
    return (uint64_t)time(NULL);
}

static allocation_record_t* find_allocation(
    memory_integration_context_t* ctx,
    void* address
) {
    for (uint32_t i = 0; i < ctx->num_allocations; i++) {
        if (ctx->allocation_records[i].address == address) {
            return &ctx->allocation_records[i];
        }
    }
    return NULL;
}

static bool contains_malloc_pattern(const char* line) {
    return (strstr(line, "malloc") || strstr(line, "calloc") || 
            strstr(line, "realloc") || strstr(line, "new"));
}

static bool contains_free_pattern(const char* line) {
    return (strstr(line, "free") || strstr(line, "delete"));
}

static bool has_paired_free(FILE* fp, const char* var_name, long start_pos) {
    char line[1024];
    long current_pos = ftell(fp);
    
    // Search forward for free
    while (fgets(line, sizeof(line), fp)) {
        if (contains_free_pattern(line) && strstr(line, var_name)) {
            fseek(fp, current_pos, SEEK_SET);
            return true;
        }
    }
    
    fseek(fp, current_pos, SEEK_SET);
    return false;
}

// ==================== Public API Implementation ====================

memory_integration_context_t* memory_integration_initialize(
    const memory_integration_config_t* config
) {
    if (!config) {
        return NULL;
    }
    
    memory_integration_context_t* ctx = (memory_integration_context_t*)
        calloc(1, sizeof(memory_integration_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copy configuration
    ctx->config = *config;
    if (config->memory_subsystem_path) {
        ctx->config.memory_subsystem_path = strdup(config->memory_subsystem_path);
    }
    
    // Initialize allocation tracking
    if (config->track_allocations) {
        ctx->max_allocations = config->max_tracked_allocations > 0 ? 
                              config->max_tracked_allocations : 10000;
        ctx->allocation_records = (allocation_record_t*)calloc(
            ctx->max_allocations, sizeof(allocation_record_t)
        );
        if (!ctx->allocation_records) {
            free(ctx);
            return NULL;
        }
    }
    
    ctx->num_allocations = 0;
    ctx->total_allocated = 0;
    ctx->total_freed = 0;
    ctx->current_usage = 0;
    ctx->initialized = true;
    
    return ctx;
}

void memory_integration_shutdown(memory_integration_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->config.memory_subsystem_path) {
        free((void*)ctx->config.memory_subsystem_path);
    }
    
    if (ctx->allocation_records) {
        for (uint32_t i = 0; i < ctx->num_allocations; i++) {
            if (ctx->allocation_records[i].allocation_site) {
                free((void*)ctx->allocation_records[i].allocation_site);
            }
        }
        free(ctx->allocation_records);
    }
    
    free(ctx);
}

crrss_status_t memory_integration_analyze(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    memory_analysis_t* analysis
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!analysis) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Calculate analysis metrics
    analysis->total_allocations = ctx->total_allocated / 1024;  // Convert to KB
    analysis->total_deallocations = ctx->total_freed / 1024;
    
    // Detect potential leaks
    analysis->potential_leaks = 0;
    analysis->use_after_free_risks = 0;
    analysis->double_free_risks = 0;
    
    for (uint32_t i = 0; i < ctx->num_allocations; i++) {
        if (!ctx->allocation_records[i].is_freed) {
            analysis->potential_leaks++;
        }
    }
    
    // Calculate memory efficiency
    if (ctx->total_allocated > 0) {
        analysis->memory_efficiency = (double)ctx->total_freed / ctx->total_allocated;
    } else {
        analysis->memory_efficiency = 1.0;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_get_pool_info(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    memory_pool_info_t* pool_info
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!pool_info) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Provide simulated pool information
    pool_info->total_size = 1024 * 1024 * 1024;  // 1GB simulated
    pool_info->used_size = ctx->current_usage;
    pool_info->free_size = pool_info->total_size - pool_info->used_size;
    pool_info->allocation_count = (uint32_t)(ctx->total_allocated / 1024);
    pool_info->deallocation_count = (uint32_t)(ctx->total_freed / 1024);
    
    // Calculate fragmentation (simplified)
    if (pool_info->total_size > 0) {
        pool_info->fragmentation = (double)ctx->num_allocations / 1000.0;
        if (pool_info->fragmentation > 1.0) pool_info->fragmentation = 1.0;
    } else {
        pool_info->fragmentation = 0.0;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_detect_leaks(
    memory_integration_context_t* ctx,
    leak_detection_report_t* report
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!report) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Count potential leaks
    uint32_t leak_count = 0;
    uint64_t leaked_bytes = 0;
    
    for (uint32_t i = 0; i < ctx->num_allocations; i++) {
        if (!ctx->allocation_records[i].is_freed) {
            leak_count++;
            leaked_bytes += ctx->allocation_records[i].size;
        }
    }
    
    report->potential_leaks = leak_count;
    report->total_leaked_bytes = leaked_bytes;
    
    // Allocate and fill leak records if requested
    if (report->max_records > 0) {
        report->leak_records = (allocation_record_t*)calloc(
            report->max_records, sizeof(allocation_record_t)
        );
        
        uint32_t record_idx = 0;
        for (uint32_t i = 0; i < ctx->num_allocations && record_idx < report->max_records; i++) {
            if (!ctx->allocation_records[i].is_freed) {
                report->leak_records[record_idx++] = ctx->allocation_records[i];
            }
        }
    } else {
        report->leak_records = NULL;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_track_allocation(
    memory_integration_context_t* ctx,
    void* address,
    size_t size,
    const char* allocation_site
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!address || !allocation_site) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.track_allocations) {
        return CRRSS_SUCCESS;  // Tracking disabled
    }
    
    if (ctx->num_allocations >= ctx->max_allocations) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Record allocation
    ctx->allocation_records[ctx->num_allocations].address = address;
    ctx->allocation_records[ctx->num_allocations].size = size;
    ctx->allocation_records[ctx->num_allocations].allocation_site = strdup(allocation_site);
    ctx->allocation_records[ctx->num_allocations].timestamp = get_timestamp();
    ctx->allocation_records[ctx->num_allocations].is_freed = false;
    
    ctx->num_allocations++;
    ctx->total_allocated += size;
    ctx->current_usage += size;
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_track_deallocation(
    memory_integration_context_t* ctx,
    void* address
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!address) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.track_allocations) {
        return CRRSS_SUCCESS;
    }
    
    // Find allocation record
    allocation_record_t* record = find_allocation(ctx, address);
    if (!record) {
        return CRRSS_ERROR_NOT_FOUND;
    }
    
    if (record->is_freed && ctx->config.enable_double_free_detection) {
        // Double-free detected
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    record->is_freed = true;
    ctx->total_freed += record->size;
    ctx->current_usage -= record->size;
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_validate_patterns(
    memory_integration_context_t* ctx,
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
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    char line[1024];
    uint32_t line_num = 0;
    uint32_t issue_count = 0;
    
    while (fgets(line, sizeof(line), fp) && issue_count < max_issues) {
        line_num++;
        
        // Check for malloc without corresponding free
        if (contains_malloc_pattern(line)) {
            // Extract variable name (simplified)
            char var_name[256] = {0};
            sscanf(line, "%*[^=]=%*[^a-z]%255[a-z_]", var_name);
            
            if (var_name[0]) {
                long pos = ftell(fp);
                if (!has_paired_free(fp, var_name, pos)) {
                    issues[issue_count].file_path = strdup(file_path);
                    issues[issue_count].line_number = line_num;
                    issues[issue_count].result = VALIDATION_WARNING;
                    issues[issue_count].rule_name = "Memory Leak Detection";
                    issues[issue_count].message = strdup("Allocation without corresponding free");
                    issues[issue_count].suggestion = strdup("Ensure memory is freed");
                    issue_count++;
                }
            }
        }
    }
    
    fclose(fp);
    *num_issues = issue_count;
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_check_use_after_free(
    memory_integration_context_t* ctx,
    const char* file_path,
    uint32_t* uaf_count
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !uaf_count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    uint32_t count = 0;
    char line[1024];
    
    // Simple heuristic: look for pointer usage after free
    // This is a simplified check
    while (fgets(line, sizeof(line), fp)) {
        if (contains_free_pattern(line)) {
            // Check if pointer is used after this line
            count++;  // Simplified detection
        }
    }
    
    fclose(fp);
    *uaf_count = count > 0 ? 1 : 0;  // Binary: detected or not
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_check_double_free(
    memory_integration_context_t* ctx,
    const char* file_path,
    uint32_t* df_count
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !df_count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    uint32_t count = 0;
    char line[1024];
    char freed_vars[100][256];
    uint32_t freed_count = 0;
    
    // Track freed variables
    while (fgets(line, sizeof(line), fp)) {
        if (contains_free_pattern(line)) {
            char var_name[256];
            if (sscanf(line, "%*[^(](%255[^)])", var_name) == 1) {
                // Check if already freed
                for (uint32_t i = 0; i < freed_count; i++) {
                    if (strstr(freed_vars[i], var_name)) {
                        count++;
                        break;
                    }
                }
                
                // Add to freed list
                if (freed_count < 100) {
                    strncpy(freed_vars[freed_count++], var_name, 255);
                }
            }
        }
    }
    
    fclose(fp);
    *df_count = count;
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_get_statistics(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    uint64_t* total_allocs,
    uint64_t* total_frees,
    uint64_t* current_usage
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (total_allocs) *total_allocs = ctx->total_allocated;
    if (total_frees) *total_frees = ctx->total_freed;
    if (current_usage) *current_usage = ctx->current_usage;
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_calculate_efficiency(
    memory_integration_context_t* ctx,
    double* efficiency
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!efficiency) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (ctx->total_allocated > 0) {
        *efficiency = (double)ctx->total_freed / ctx->total_allocated;
        if (*efficiency > 1.0) *efficiency = 1.0;
    } else {
        *efficiency = 1.0;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t memory_integration_generate_report(
    memory_integration_context_t* ctx,
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
    
    fprintf(fp, "=== Memory Integration Report ===\n\n");
    fprintf(fp, "Total Allocated: %lu bytes\n", ctx->total_allocated);
    fprintf(fp, "Total Freed: %lu bytes\n", ctx->total_freed);
    fprintf(fp, "Current Usage: %lu bytes\n", ctx->current_usage);
    fprintf(fp, "Tracked Allocations: %u\n", ctx->num_allocations);
    
    // Count leaks
    uint32_t leak_count = 0;
    for (uint32_t i = 0; i < ctx->num_allocations; i++) {
        if (!ctx->allocation_records[i].is_freed) {
            leak_count++;
        }
    }
    fprintf(fp, "Potential Memory Leaks: %u\n", leak_count);
    
    // Calculate efficiency
    double efficiency = 0.0;
    memory_integration_calculate_efficiency(ctx, &efficiency);
    fprintf(fp, "Memory Efficiency: %.2f%%\n", efficiency * 100);
    
    fclose(fp);
    return CRRSS_SUCCESS;
}
