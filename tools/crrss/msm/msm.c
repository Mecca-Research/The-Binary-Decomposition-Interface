
/**
 * @file msm.c
 * @brief Memory-Safety Maniac Profile - Implementation
 * 
 * Phase 1B Stage 3 Implementation
 */

#include "msm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <execinfo.h>
#include <unistd.h>

// ==================== Internal Constants ====================

#define MSM_DEFAULT_MAX_POINTERS 10000
#define MSM_DEFAULT_MAX_ALLOCATIONS 10000
#define MSM_DEFAULT_MAX_STACK_DEPTH 32
#define MSM_HASH_TABLE_SIZE 1024

// ==================== Hash Table for Fast Lookups ====================

typedef struct hash_node {
    void* key;
    void* value;
    struct hash_node* next;
} hash_node_t;

typedef struct {
    hash_node_t* buckets[MSM_HASH_TABLE_SIZE];
    pthread_mutex_t locks[MSM_HASH_TABLE_SIZE];
    uint32_t size;
} hash_table_t;

// ==================== MSM Context Structure ====================

struct msm_context {
    // Configuration
    msm_config_t config;
    bool initialized;
    
    // Tracking data structures
    hash_table_t* allocation_table;
    hash_table_t* pointer_table;
    
    // Statistics
    msm_statistics_t stats;
    
    // Issue tracking
    msm_issue_t* issues;
    uint32_t issue_count;
    uint32_t max_issues;
    
    // Integration contexts
    void* bpme_ctx;
    void* sciv_ctx;
    void* memory_ctx;
    
    // Thread safety
    pthread_mutex_t context_lock;
    pthread_mutex_t issue_lock;
    pthread_mutex_t stats_lock;
};

// ==================== String Constants ====================

static const char* MSM_ISSUE_TYPE_STRINGS[] = {
    "Memory Leak",
    "Use-After-Free",
    "Double-Free",
    "NULL Dereference",
    "Buffer Overflow",
    "Buffer Underflow",
    "Uninitialized Pointer",
    "Dangling Pointer",
    "Invalid Free",
    "Missing NULL Check",
    "Unsafe Pointer Arithmetic"
};

static const char* POINTER_STATE_STRINGS[] = {
    "Uninitialized",
    "Allocated",
    "Valid",
    "Freed",
    "Invalid",
    "Dangling"
};

static const char* TRACKING_MODE_STRINGS[] = {
    "Disabled",
    "Basic",
    "Detailed",
    "Paranoid"
};

// ==================== Hash Table Implementation ====================

static uint32_t hash_pointer(void* ptr) {
    uint64_t val = (uint64_t)ptr;
    return (uint32_t)((val ^ (val >> 32)) % MSM_HASH_TABLE_SIZE);
}

static hash_table_t* hash_table_create(void) {
    hash_table_t* table = calloc(1, sizeof(hash_table_t));
    if (!table) return NULL;
    
    for (int i = 0; i < MSM_HASH_TABLE_SIZE; i++) {
        pthread_mutex_init(&table->locks[i], NULL);
    }
    
    return table;
}

static void hash_table_destroy(hash_table_t* table) {
    if (!table) return;
    
    for (int i = 0; i < MSM_HASH_TABLE_SIZE; i++) {
        hash_node_t* node = table->buckets[i];
        while (node) {
            hash_node_t* next = node->next;
            free(node);
            node = next;
        }
        pthread_mutex_destroy(&table->locks[i]);
    }
    
    free(table);
}

static crrss_status_t hash_table_insert(hash_table_t* table, void* key, void* value) {
    if (!table || !key) return CRRSS_ERROR_INVALID_PARAM;
    
    uint32_t index = hash_pointer(key);
    pthread_mutex_lock(&table->locks[index]);
    
    // Check if key already exists
    hash_node_t* node = table->buckets[index];
    while (node) {
        if (node->key == key) {
            node->value = value;  // Update existing
            pthread_mutex_unlock(&table->locks[index]);
            return CRRSS_SUCCESS;
        }
        node = node->next;
    }
    
    // Insert new node
    hash_node_t* new_node = malloc(sizeof(hash_node_t));
    if (!new_node) {
        pthread_mutex_unlock(&table->locks[index]);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    new_node->key = key;
    new_node->value = value;
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
    table->size++;
    
    pthread_mutex_unlock(&table->locks[index]);
    return CRRSS_SUCCESS;
}

static void* hash_table_lookup(hash_table_t* table, void* key) {
    if (!table || !key) return NULL;
    
    uint32_t index = hash_pointer(key);
    pthread_mutex_lock(&table->locks[index]);
    
    hash_node_t* node = table->buckets[index];
    while (node) {
        if (node->key == key) {
            void* value = node->value;
            pthread_mutex_unlock(&table->locks[index]);
            return value;
        }
        node = node->next;
    }
    
    pthread_mutex_unlock(&table->locks[index]);
    return NULL;
}

static crrss_status_t hash_table_remove(hash_table_t* table, void* key) {
    if (!table || !key) return CRRSS_ERROR_INVALID_PARAM;
    
    uint32_t index = hash_pointer(key);
    pthread_mutex_lock(&table->locks[index]);
    
    hash_node_t* node = table->buckets[index];
    hash_node_t* prev = NULL;
    
    while (node) {
        if (node->key == key) {
            if (prev) {
                prev->next = node->next;
            } else {
                table->buckets[index] = node->next;
            }
            free(node);
            table->size--;
            pthread_mutex_unlock(&table->locks[index]);
            return CRRSS_SUCCESS;
        }
        prev = node;
        node = node->next;
    }
    
    pthread_mutex_unlock(&table->locks[index]);
    return CRRSS_ERROR_NOT_FOUND;
}

// ==================== Stack Trace Implementation ====================

static stack_trace_t* capture_stack_trace(uint32_t max_frames) {
    if (max_frames == 0) return NULL;
    
    stack_trace_t* trace = calloc(1, sizeof(stack_trace_t));
    if (!trace) return NULL;
    
    trace->frames = calloc(max_frames, sizeof(stack_frame_t));
    if (!trace->frames) {
        free(trace);
        return NULL;
    }
    
    trace->max_frames = max_frames;
    
    // Capture backtrace
    void** buffer = malloc(max_frames * sizeof(void*));
    if (!buffer) {
        free(trace->frames);
        free(trace);
        return NULL;
    }
    
    int frame_count = backtrace(buffer, max_frames);
    trace->frame_count = (uint32_t)frame_count;
    
    // Get symbols (function names)
    char** symbols = backtrace_symbols(buffer, frame_count);
    
    for (int i = 0; i < frame_count && i < max_frames; i++) {
        trace->frames[i].instruction_pointer = buffer[i];
        if (symbols && symbols[i]) {
            // Parse symbol information
            trace->frames[i].function_name = strdup(symbols[i]);
        }
        // Note: File path and line number would require additional debug info
    }
    
    free(symbols);
    free(buffer);
    
    return trace;
}

static void free_stack_trace(stack_trace_t* trace) {
    if (!trace) return;
    
    if (trace->frames) {
        for (uint32_t i = 0; i < trace->frame_count; i++) {
            if (trace->frames[i].function_name) {
                free((void*)trace->frames[i].function_name);
            }
        }
        free(trace->frames);
    }
    
    free(trace);
}

// ==================== Allocation Metadata Management ====================

static allocation_metadata_t* create_allocation_metadata(
    void* address,
    size_t size,
    const char* file,
    uint32_t line,
    const char* function,
    uint32_t max_stack_depth
) {
    allocation_metadata_t* meta = calloc(1, sizeof(allocation_metadata_t));
    if (!meta) return NULL;
    
    meta->address = address;
    meta->size = size;
    meta->is_freed = false;
    meta->counted_as_leak = false;  // Initialize leak tracking flag
    
    meta->allocation_site_file = file ? strdup(file) : NULL;
    meta->allocation_site_line = line;
    meta->allocation_function = function ? strdup(function) : NULL;
    clock_gettime(CLOCK_MONOTONIC, &meta->allocation_time);
    
    static uint64_t allocation_id_counter = 0;
    meta->allocation_id = __sync_fetch_and_add(&allocation_id_counter, 1);
    
    // Capture stack trace if enabled
    if (max_stack_depth > 0) {
        meta->allocation_trace = capture_stack_trace(max_stack_depth);
    }
    
    return meta;
}

static void free_allocation_metadata(allocation_metadata_t* meta) {
    if (!meta) return;
    
    if (meta->allocation_site_file) free((void*)meta->allocation_site_file);
    if (meta->allocation_function) free((void*)meta->allocation_function);
    if (meta->deallocation_site_file) free((void*)meta->deallocation_site_file);
    if (meta->deallocation_function) free((void*)meta->deallocation_function);
    
    if (meta->allocation_trace) free_stack_trace(meta->allocation_trace);
    if (meta->deallocation_trace) free_stack_trace(meta->deallocation_trace);
    
    free(meta);
}

// ==================== Issue Recording ====================

static crrss_status_t record_issue(
    msm_context_t* ctx,
    msm_issue_type_t issue_type,
    bug_priority_t priority,
    const char* file_path,
    uint32_t line_number,
    const char* function_name,
    const char* description,
    void* related_address
) {
    if (!ctx) return CRRSS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->issue_lock);
    
    // Reallocate if needed
    if (ctx->issue_count >= ctx->max_issues) {
        uint32_t new_max = ctx->max_issues * 2;
        msm_issue_t* new_issues = realloc(ctx->issues, new_max * sizeof(msm_issue_t));
        if (!new_issues) {
            pthread_mutex_unlock(&ctx->issue_lock);
            return CRRSS_ERROR_MEMORY_ALLOCATION;
        }
        ctx->issues = new_issues;
        ctx->max_issues = new_max;
    }
    
    msm_issue_t* issue = &ctx->issues[ctx->issue_count];
    memset(issue, 0, sizeof(msm_issue_t));
    
    issue->issue_type = issue_type;
    issue->priority = priority;
    issue->file_path = file_path ? strdup(file_path) : NULL;
    issue->line_number = line_number;
    issue->function_name = function_name ? strdup(function_name) : NULL;
    issue->description = description ? strdup(description) : NULL;
    issue->related_address = related_address;
    clock_gettime(CLOCK_MONOTONIC, &issue->detection_time);
    
    // Set risk level based on issue type
    switch (issue_type) {
        case MSM_ISSUE_USE_AFTER_FREE:
        case MSM_ISSUE_DOUBLE_FREE:
        case MSM_ISSUE_BUFFER_OVERFLOW:
            issue->risk_level = RISK_LEVEL_CRITICAL;
            break;
        case MSM_ISSUE_MEMORY_LEAK:
        case MSM_ISSUE_NULL_DEREF:
            issue->risk_level = RISK_LEVEL_HIGH;
            break;
        default:
            issue->risk_level = RISK_LEVEL_MEDIUM;
            break;
    }
    
    // Capture stack trace if in detailed mode
    if (ctx->config.tracking_mode >= MSM_TRACKING_DETAILED) {
        issue->issue_trace = capture_stack_trace(ctx->config.max_stack_depth);
    }
    
    ctx->issue_count++;
    
    pthread_mutex_unlock(&ctx->issue_lock);
    
    // Update statistics
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.total_issues_detected++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== Initialization & Shutdown ====================

msm_context_t* msm_initialize(const msm_config_t* config) {
    if (!config) return NULL;
    
    msm_context_t* ctx = calloc(1, sizeof(msm_context_t));
    if (!ctx) return NULL;
    
    // Copy configuration
    ctx->config = *config;
    
    // Set defaults if not configured
    if (ctx->config.max_tracked_pointers == 0) {
        ctx->config.max_tracked_pointers = MSM_DEFAULT_MAX_POINTERS;
    }
    if (ctx->config.max_tracked_allocations == 0) {
        ctx->config.max_tracked_allocations = MSM_DEFAULT_MAX_ALLOCATIONS;
    }
    if (ctx->config.max_stack_depth == 0) {
        ctx->config.max_stack_depth = MSM_DEFAULT_MAX_STACK_DEPTH;
    }
    
    // Initialize hash tables
    ctx->allocation_table = hash_table_create();
    ctx->pointer_table = hash_table_create();
    
    if (!ctx->allocation_table || !ctx->pointer_table) {
        if (ctx->allocation_table) hash_table_destroy(ctx->allocation_table);
        if (ctx->pointer_table) hash_table_destroy(ctx->pointer_table);
        free(ctx);
        return NULL;
    }
    
    // Initialize issues array
    ctx->max_issues = 1000;
    ctx->issues = calloc(ctx->max_issues, sizeof(msm_issue_t));
    if (!ctx->issues) {
        hash_table_destroy(ctx->allocation_table);
        hash_table_destroy(ctx->pointer_table);
        free(ctx);
        return NULL;
    }
    
    // Initialize mutexes
    pthread_mutex_init(&ctx->context_lock, NULL);
    pthread_mutex_init(&ctx->issue_lock, NULL);
    pthread_mutex_init(&ctx->stats_lock, NULL);
    
    // Initialize statistics
    memset(&ctx->stats, 0, sizeof(msm_statistics_t));
    clock_gettime(CLOCK_MONOTONIC, &ctx->stats.analysis_start_time);
    
    ctx->initialized = true;
    
    return ctx;
}

void msm_shutdown(msm_context_t* ctx) {
    if (!ctx) return;
    
    pthread_mutex_lock(&ctx->context_lock);
    
    if (!ctx->initialized) {
        pthread_mutex_unlock(&ctx->context_lock);
        return;
    }
    
    // Free all allocation metadata
    if (ctx->allocation_table) {
        for (int i = 0; i < MSM_HASH_TABLE_SIZE; i++) {
            hash_node_t* node = ctx->allocation_table->buckets[i];
            while (node) {
                if (node->value) {
                    free_allocation_metadata((allocation_metadata_t*)node->value);
                }
                node = node->next;
            }
        }
        hash_table_destroy(ctx->allocation_table);
    }
    
    // Free all pointer tracking info
    if (ctx->pointer_table) {
        for (int i = 0; i < MSM_HASH_TABLE_SIZE; i++) {
            hash_node_t* node = ctx->pointer_table->buckets[i];
            while (node) {
                if (node->value) {
                    free(node->value);
                }
                node = node->next;
            }
        }
        hash_table_destroy(ctx->pointer_table);
    }
    
    // Free issues
    if (ctx->issues) {
        for (uint32_t i = 0; i < ctx->issue_count; i++) {
            if (ctx->issues[i].file_path) free((void*)ctx->issues[i].file_path);
            if (ctx->issues[i].function_name) free((void*)ctx->issues[i].function_name);
            if (ctx->issues[i].description) free((void*)ctx->issues[i].description);
            if (ctx->issues[i].recommendation) free((void*)ctx->issues[i].recommendation);
            if (ctx->issues[i].issue_trace) free_stack_trace(ctx->issues[i].issue_trace);
        }
        free(ctx->issues);
    }
    
    ctx->initialized = false;
    
    pthread_mutex_unlock(&ctx->context_lock);
    
    // Destroy mutexes
    pthread_mutex_destroy(&ctx->context_lock);
    pthread_mutex_destroy(&ctx->issue_lock);
    pthread_mutex_destroy(&ctx->stats_lock);
    
    free(ctx);
}

crrss_status_t msm_reset(msm_context_t* ctx) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&ctx->context_lock);
    
    // Clear statistics
    pthread_mutex_lock(&ctx->stats_lock);
    memset(&ctx->stats, 0, sizeof(msm_statistics_t));
    clock_gettime(CLOCK_MONOTONIC, &ctx->stats.analysis_start_time);
    pthread_mutex_unlock(&ctx->stats_lock);
    
    // Reset leak counting flags for all tracked allocations
    if (ctx->allocation_table) {
        for (int i = 0; i < MSM_HASH_TABLE_SIZE; i++) {
            pthread_mutex_lock(&ctx->allocation_table->locks[i]);
            
            hash_node_t* node = ctx->allocation_table->buckets[i];
            while (node) {
                allocation_metadata_t* meta = (allocation_metadata_t*)node->value;
                if (meta) {
                    meta->counted_as_leak = false;
                }
                node = node->next;
            }
            
            pthread_mutex_unlock(&ctx->allocation_table->locks[i]);
        }
    }
    
    // Clear issues
    pthread_mutex_lock(&ctx->issue_lock);
    for (uint32_t i = 0; i < ctx->issue_count; i++) {
        if (ctx->issues[i].file_path) free((void*)ctx->issues[i].file_path);
        if (ctx->issues[i].function_name) free((void*)ctx->issues[i].function_name);
        if (ctx->issues[i].description) free((void*)ctx->issues[i].description);
        if (ctx->issues[i].recommendation) free((void*)ctx->issues[i].recommendation);
        if (ctx->issues[i].issue_trace) free_stack_trace(ctx->issues[i].issue_trace);
    }
    ctx->issue_count = 0;
    pthread_mutex_unlock(&ctx->issue_lock);
    
    pthread_mutex_unlock(&ctx->context_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== Allocation Tracking ====================

crrss_status_t msm_track_allocation(
    msm_context_t* ctx,
    void* address,
    size_t size,
    const char* file,
    uint32_t line,
    const char* function
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!address) return CRRSS_ERROR_INVALID_PARAM;
    
    if (!ctx->config.enable_allocation_tracking) {
        return CRRSS_SUCCESS;  // Tracking disabled
    }
    
    // Create allocation metadata
    uint32_t max_depth = (ctx->config.tracking_mode >= MSM_TRACKING_DETAILED) ? 
                         ctx->config.max_stack_depth : 0;
    
    allocation_metadata_t* meta = create_allocation_metadata(
        address, size, file, line, function, max_depth
    );
    
    if (!meta) return CRRSS_ERROR_MEMORY_ALLOCATION;
    
    // Insert into tracking table
    crrss_status_t status = hash_table_insert(ctx->allocation_table, address, meta);
    if (status != CRRSS_SUCCESS) {
        free_allocation_metadata(meta);
        return status;
    }
    
    // Update statistics
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.total_allocations_tracked++;
    ctx->stats.current_allocations++;
    ctx->stats.total_memory_tracked += size;
    if (ctx->stats.total_memory_tracked > ctx->stats.peak_memory_tracked) {
        ctx->stats.peak_memory_tracked = ctx->stats.total_memory_tracked;
    }
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_track_deallocation(
    msm_context_t* ctx,
    void* address,
    const char* file,
    uint32_t line,
    const char* function
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!address) return CRRSS_ERROR_INVALID_PARAM;
    
    if (!ctx->config.enable_allocation_tracking) {
        return CRRSS_SUCCESS;  // Tracking disabled
    }
    
    // Look up allocation metadata
    allocation_metadata_t* meta = hash_table_lookup(ctx->allocation_table, address);
    
    if (!meta) {
        // Freeing untracked memory - could be invalid free
        if (ctx->config.enable_double_free_detection) {
            char desc[256];
            snprintf(desc, sizeof(desc), 
                    "Attempting to free untracked memory at %p", address);
            record_issue(ctx, MSM_ISSUE_INVALID_FREE, BUG_PRIORITY_P1_HIGH,
                        file, line, function, desc, address);
        }
        return CRRSS_ERROR_NOT_FOUND;
    }
    
    // Check for double-free
    if (meta->is_freed && ctx->config.enable_double_free_detection) {
        char desc[256];
        snprintf(desc, sizeof(desc), 
                "Double-free detected at %p (previously freed at %s:%u)",
                address, meta->deallocation_site_file, meta->deallocation_site_line);
        
        record_issue(ctx, MSM_ISSUE_DOUBLE_FREE, BUG_PRIORITY_P0_CRITICAL,
                    file, line, function, desc, address);
        
        pthread_mutex_lock(&ctx->stats_lock);
        ctx->stats.double_free_detected++;
        pthread_mutex_unlock(&ctx->stats_lock);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    // Mark as freed
    meta->is_freed = true;
    meta->deallocation_site_file = file ? strdup(file) : NULL;
    meta->deallocation_site_line = line;
    meta->deallocation_function = function ? strdup(function) : NULL;
    clock_gettime(CLOCK_MONOTONIC, &meta->deallocation_time);
    
    // Capture deallocation stack trace if in detailed mode
    if (ctx->config.tracking_mode >= MSM_TRACKING_DETAILED) {
        meta->deallocation_trace = capture_stack_trace(ctx->config.max_stack_depth);
    }
    
    // Update statistics
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.total_deallocations_tracked++;
    ctx->stats.current_allocations--;
    ctx->stats.total_memory_tracked -= meta->size;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_get_allocation_metadata(
    msm_context_t* ctx,
    void* address,
    allocation_metadata_t* metadata
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!address || !metadata) return CRRSS_ERROR_INVALID_PARAM;
    
    allocation_metadata_t* meta = hash_table_lookup(ctx->allocation_table, address);
    if (!meta) return CRRSS_ERROR_NOT_FOUND;
    
    *metadata = *meta;  // Copy metadata
    
    return CRRSS_SUCCESS;
}

// ==================== Pointer Safety Analysis ====================

crrss_status_t msm_track_pointer(
    msm_context_t* ctx,
    void* pointer_addr,
    void* points_to,
    const char* file,
    uint32_t line,
    const char* function
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!pointer_addr) return CRRSS_ERROR_INVALID_PARAM;
    
    if (!ctx->config.enable_pointer_tracking) {
        return CRRSS_SUCCESS;  // Tracking disabled
    }
    
    pointer_tracking_info_t* info = calloc(1, sizeof(pointer_tracking_info_t));
    if (!info) return CRRSS_ERROR_MEMORY_ALLOCATION;
    
    info->pointer_address = pointer_addr;
    info->points_to = points_to;
    info->source_file = file ? strdup(file) : NULL;
    info->source_line = line;
    info->function_name = function ? strdup(function) : NULL;
    clock_gettime(CLOCK_MONOTONIC, &info->creation_time);
    info->last_access_time = info->creation_time;
    
    // Determine pointer state
    if (!points_to) {
        info->state = POINTER_STATE_UNINITIALIZED;
    } else {
        // Check if it points to tracked allocation
        allocation_metadata_t* alloc = hash_table_lookup(ctx->allocation_table, points_to);
        if (alloc) {
            info->allocation = alloc;
            if (alloc->is_freed) {
                info->state = POINTER_STATE_DANGLING;
            } else {
                info->state = POINTER_STATE_VALID;
            }
        } else {
            info->state = POINTER_STATE_VALID;  // Assume valid if not tracked
        }
    }
    
    // Insert into pointer table
    crrss_status_t status = hash_table_insert(ctx->pointer_table, pointer_addr, info);
    if (status != CRRSS_SUCCESS) {
        if (info->source_file) free((void*)info->source_file);
        if (info->function_name) free((void*)info->function_name);
        free(info);
        return status;
    }
    
    // Update statistics
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.total_pointers_tracked++;
    ctx->stats.current_pointers_tracked++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_track_pointer_access(
    msm_context_t* ctx,
    void* pointer_addr,
    pointer_access_t access_type,
    const char* file,
    uint32_t line
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!pointer_addr) return CRRSS_ERROR_INVALID_PARAM;
    
    if (!ctx->config.enable_pointer_tracking) {
        return CRRSS_SUCCESS;  // Tracking disabled
    }
    
    pointer_tracking_info_t* info = hash_table_lookup(ctx->pointer_table, pointer_addr);
    if (!info) {
        // Pointer not tracked - could be issue
        return CRRSS_ERROR_NOT_FOUND;
    }
    
    // Update access tracking
    info->access_count++;
    clock_gettime(CLOCK_MONOTONIC, &info->last_access_time);
    
    // Check for use-after-free
    if (info->allocation && info->allocation->is_freed && 
        ctx->config.enable_use_after_free_detection) {
        
        char desc[256];
        snprintf(desc, sizeof(desc),
                "Use-after-free detected: accessing freed memory at %p "
                "(freed at %s:%u)",
                info->points_to,
                info->allocation->deallocation_site_file,
                info->allocation->deallocation_site_line);
        
        record_issue(ctx, MSM_ISSUE_USE_AFTER_FREE, BUG_PRIORITY_P0_CRITICAL,
                    file, line, info->function_name, desc, info->points_to);
        
        pthread_mutex_lock(&ctx->stats_lock);
        ctx->stats.use_after_free_detected++;
        pthread_mutex_unlock(&ctx->stats_lock);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    // Check for NULL dereference
    if (!info->points_to && access_type != POINTER_ACCESS_FREE) {
        char desc[256];
        snprintf(desc, sizeof(desc),
                "NULL pointer dereference detected at %s:%u", file, line);
        
        record_issue(ctx, MSM_ISSUE_NULL_DEREF, BUG_PRIORITY_P0_CRITICAL,
                    file, line, info->function_name, desc, pointer_addr);
        
        pthread_mutex_lock(&ctx->stats_lock);
        ctx->stats.null_deref_detected++;
        pthread_mutex_unlock(&ctx->stats_lock);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_validate_pointer(
    msm_context_t* ctx,
    void* pointer,
    bool* is_valid
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!is_valid) return CRRSS_ERROR_INVALID_PARAM;
    
    *is_valid = false;
    
    if (!pointer) {
        return CRRSS_SUCCESS;  // NULL pointer is not valid
    }
    
    // Check if pointer points to tracked allocation
    allocation_metadata_t* alloc = hash_table_lookup(ctx->allocation_table, pointer);
    if (alloc) {
        *is_valid = !alloc->is_freed;
        return CRRSS_SUCCESS;
    }
    
    // Check if it's a tracked pointer
    pointer_tracking_info_t* info = hash_table_lookup(ctx->pointer_table, pointer);
    if (info) {
        *is_valid = (info->state == POINTER_STATE_VALID || 
                     info->state == POINTER_STATE_ALLOCATED);
        return CRRSS_SUCCESS;
    }
    
    // Not tracked - assume valid
    *is_valid = true;
    return CRRSS_SUCCESS;
}

// ==================== Pattern Detection (Static Analysis) ====================

// These functions perform static code analysis on source files

static bool contains_pattern(const char* line, const char* pattern) {
    return strstr(line, pattern) != NULL;
}

static bool is_pointer_dereference(const char* line) {
    // Simple pattern matching for pointer dereference
    return contains_pattern(line, "->") || contains_pattern(line, "*");
}

static bool has_null_check_before(const char** lines, int current_line, int start_line) {
    // Look backwards for NULL check
    for (int i = current_line - 1; i >= start_line && i >= current_line - 10; i--) {
        if (contains_pattern(lines[i], "if") && 
            (contains_pattern(lines[i], "!= NULL") || 
             contains_pattern(lines[i], "== NULL") ||
             contains_pattern(lines[i], "!"))) {
            return true;
        }
    }
    return false;
}

crrss_status_t msm_detect_use_after_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char line[1024];
    uint32_t line_num = 0;
    bool found_free = false;
    char freed_var[128] = {0};
    
    while (fgets(line, sizeof(line), fp) && *num_issues < max_issues) {
        line_num++;
        
        // Look for free() calls
        if (contains_pattern(line, "free(")) {
            found_free = true;
            // Extract variable name (simple heuristic)
            const char* start = strstr(line, "free(");
            if (start) {
                start += 5;  // Skip "free("
                const char* end = strchr(start, ')');
                if (end && end - start < sizeof(freed_var)) {
                    strncpy(freed_var, start, end - start);
                    freed_var[end - start] = '\0';
                }
            }
        }
        
        // Look for usage of freed variable
        if (found_free && freed_var[0] && contains_pattern(line, freed_var)) {
            // Check if it's not just another free or NULL assignment
            if (!contains_pattern(line, "free(") && 
                !contains_pattern(line, "= NULL") &&
                !contains_pattern(line, "= 0")) {
                
                msm_issue_t* issue = &issues[*num_issues];
                memset(issue, 0, sizeof(msm_issue_t));
                
                issue->issue_type = MSM_ISSUE_USE_AFTER_FREE;
                issue->priority = BUG_PRIORITY_P0_CRITICAL;
                issue->risk_level = RISK_LEVEL_CRITICAL;
                issue->file_path = strdup(file_path);
                issue->line_number = line_num;
                
                char desc[256];
                snprintf(desc, sizeof(desc),
                        "Potential use-after-free: variable '%s' used after free()",
                        freed_var);
                issue->description = strdup(desc);
                issue->recommendation = strdup("Ensure pointer is not used after free(), set to NULL after freeing");
                
                (*num_issues)++;
                
                // Record in context
                record_issue(ctx, MSM_ISSUE_USE_AFTER_FREE, BUG_PRIORITY_P0_CRITICAL,
                           file_path, line_num, NULL, desc, NULL);
            }
        }
    }
    
    fclose(fp);
    
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.files_analyzed++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_detect_double_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char line[1024];
    uint32_t line_num = 0;
    char freed_vars[10][128];  // Track up to 10 freed variables
    int freed_count = 0;
    
    while (fgets(line, sizeof(line), fp) && *num_issues < max_issues) {
        line_num++;
        
        // Look for free() calls
        if (contains_pattern(line, "free(")) {
            // Extract variable name
            const char* start = strstr(line, "free(");
            if (start) {
                start += 5;
                const char* end = strchr(start, ')');
                if (end && end - start < 128) {
                    char var[128];
                    strncpy(var, start, end - start);
                    var[end - start] = '\0';
                    
                    // Check if already freed
                    for (int i = 0; i < freed_count; i++) {
                        if (strcmp(freed_vars[i], var) == 0) {
                            // Double-free detected!
                            msm_issue_t* issue = &issues[*num_issues];
                            memset(issue, 0, sizeof(msm_issue_t));
                            
                            issue->issue_type = MSM_ISSUE_DOUBLE_FREE;
                            issue->priority = BUG_PRIORITY_P0_CRITICAL;
                            issue->risk_level = RISK_LEVEL_CRITICAL;
                            issue->file_path = strdup(file_path);
                            issue->line_number = line_num;
                            
                            char desc[256];
                            snprintf(desc, sizeof(desc),
                                    "Potential double-free: variable '%s' freed multiple times",
                                    var);
                            issue->description = strdup(desc);
                            issue->recommendation = strdup("Set pointer to NULL after free(), check before freeing");
                            
                            (*num_issues)++;
                            
                            record_issue(ctx, MSM_ISSUE_DOUBLE_FREE, BUG_PRIORITY_P0_CRITICAL,
                                       file_path, line_num, NULL, desc, NULL);
                            
                            break;
                        }
                    }
                    
                    // Add to freed list
                    if (freed_count < 10) {
                        strncpy(freed_vars[freed_count], var, sizeof(freed_vars[0]));
                        freed_count++;
                    }
                }
            }
        }
        
        // Reset freed tracking on NULL assignment
        if (contains_pattern(line, "= NULL") || contains_pattern(line, "= 0")) {
            // Could parse and remove from freed list, but keeping simple
        }
    }
    
    fclose(fp);
    
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.files_analyzed++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== NULL-Check Enforcement ====================

crrss_status_t msm_analyze_null_checks(
    msm_context_t* ctx,
    const char* file_path,
    null_check_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !results || !num_results) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_results = 0;
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char** lines = malloc(10000 * sizeof(char*));  // Support up to 10000 lines
    if (!lines) {
        fclose(fp);
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    
    int line_count = 0;
    char buffer[1024];
    
    // Read all lines
    while (fgets(buffer, sizeof(buffer), fp) && line_count < 10000) {
        lines[line_count] = strdup(buffer);
        line_count++;
    }
    
    fclose(fp);
    
    // Analyze for NULL checks
    for (int i = 0; i < line_count && *num_results < max_results; i++) {
        // Look for pointer dereference
        if (is_pointer_dereference(lines[i])) {
            // Check if there's a NULL check before this line
            bool has_check = has_null_check_before((const char**)lines, i, 0);
            
            if (!has_check) {
                null_check_result_t* result = &results[*num_results];
                memset(result, 0, sizeof(null_check_result_t));
                
                result->file_path = strdup(file_path);
                result->line_number = i + 1;
                result->null_check_present = false;
                result->null_check_required = true;
                result->suggestion = strdup("Add NULL check before pointer dereference");
                
                (*num_results)++;
                
                // Record issue
                char desc[256];
                snprintf(desc, sizeof(desc),
                        "Missing NULL check before pointer dereference at line %d", i + 1);
                record_issue(ctx, MSM_ISSUE_MISSING_NULL_CHECK, BUG_PRIORITY_P2_MEDIUM,
                           file_path, i + 1, NULL, desc, NULL);
                
                pthread_mutex_lock(&ctx->stats_lock);
                ctx->stats.missing_null_checks++;
                pthread_mutex_unlock(&ctx->stats_lock);
            }
        }
    }
    
    // Free lines
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
    
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.files_analyzed++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_validate_null_check(
    msm_context_t* ctx,
    void* pointer,
    const char* file,
    uint32_t line,
    bool* has_null_check
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!has_null_check) return CRRSS_ERROR_INVALID_PARAM;
    
    // Runtime validation - we can't know from runtime if there's a check in code
    // This would need to integrate with static analysis
    *has_null_check = false;  // Conservative assumption
    
    if (!pointer && ctx->config.enforce_null_checks) {
        char desc[256];
        snprintf(desc, sizeof(desc),
                "NULL pointer detected at %s:%u without check", file, line);
        record_issue(ctx, MSM_ISSUE_NULL_DEREF, BUG_PRIORITY_P0_CRITICAL,
                    file, line, NULL, desc, pointer);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Buffer Overflow Detection ====================

crrss_status_t msm_check_buffer_access(
    msm_context_t* ctx,
    void* buffer,
    size_t buffer_size,
    size_t access_offset,
    size_t access_size,
    const char* file,
    uint32_t line
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!buffer) return CRRSS_ERROR_INVALID_PARAM;
    
    // Check for overflow
    if (access_offset + access_size > buffer_size) {
        char desc[256];
        snprintf(desc, sizeof(desc),
                "Buffer overflow detected at %s:%u: accessing %zu bytes at offset %zu "
                "in buffer of size %zu",
                file, line, access_size, access_offset, buffer_size);
        
        record_issue(ctx, MSM_ISSUE_BUFFER_OVERFLOW, BUG_PRIORITY_P0_CRITICAL,
                    file, line, NULL, desc, buffer);
        
        pthread_mutex_lock(&ctx->stats_lock);
        ctx->stats.buffer_overflow_detected++;
        pthread_mutex_unlock(&ctx->stats_lock);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    // Check for underflow
    if (access_offset > buffer_size) {
        char desc[256];
        snprintf(desc, sizeof(desc),
                "Buffer underflow detected at %s:%u: offset %zu exceeds buffer size %zu",
                file, line, access_offset, buffer_size);
        
        record_issue(ctx, MSM_ISSUE_BUFFER_UNDERFLOW, BUG_PRIORITY_P0_CRITICAL,
                    file, line, NULL, desc, buffer);
        
        return CRRSS_ERROR_VALIDATION_FAILED;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_detect_buffer_overflow(
    msm_context_t* ctx,
    const char* file_path,
    buffer_analysis_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !results || !num_results) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_results = 0;
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char line[1024];
    uint32_t line_num = 0;
    
    // Look for potentially unsafe functions
    const char* unsafe_funcs[] = {
        "strcpy", "strcat", "sprintf", "gets", "scanf"
    };
    int num_unsafe = sizeof(unsafe_funcs) / sizeof(unsafe_funcs[0]);
    
    while (fgets(line, sizeof(line), fp) && *num_results < max_results) {
        line_num++;
        
        for (int i = 0; i < num_unsafe; i++) {
            if (contains_pattern(line, unsafe_funcs[i])) {
                buffer_analysis_result_t* result = &results[*num_results];
                memset(result, 0, sizeof(buffer_analysis_result_t));
                
                result->file_path = strdup(file_path);
                result->line_number = line_num;
                result->overflow_detected = true;
                
                char recommendation[256];
                snprintf(recommendation, sizeof(recommendation),
                        "Replace unsafe function '%s' with safe alternative (e.g., strncpy, strncat, snprintf)",
                        unsafe_funcs[i]);
                result->recommendation = strdup(recommendation);
                
                (*num_results)++;
                
                // Record issue
                char desc[256];
                snprintf(desc, sizeof(desc),
                        "Unsafe function '%s' may cause buffer overflow", unsafe_funcs[i]);
                record_issue(ctx, MSM_ISSUE_BUFFER_OVERFLOW, BUG_PRIORITY_P1_HIGH,
                           file_path, line_num, NULL, desc, NULL);
                
                break;
            }
        }
    }
    
    fclose(fp);
    
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.files_analyzed++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== Memory Leak Detection ====================

crrss_status_t msm_detect_leaks(
    msm_context_t* ctx,
    allocation_metadata_t* leaks,
    uint32_t max_leaks,
    uint32_t* num_leaks
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!leaks || !num_leaks) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_leaks = 0;
    
    if (!ctx->allocation_table) return CRRSS_SUCCESS;
    
    // Iterate through all tracked allocations
    for (int i = 0; i < MSM_HASH_TABLE_SIZE && *num_leaks < max_leaks; i++) {
        pthread_mutex_lock(&ctx->allocation_table->locks[i]);
        
        hash_node_t* node = ctx->allocation_table->buckets[i];
        while (node && *num_leaks < max_leaks) {
            allocation_metadata_t* meta = (allocation_metadata_t*)node->value;
            
            // If not freed, it's a potential leak
            if (meta && !meta->is_freed) {
                leaks[*num_leaks] = *meta;  // Copy metadata
                (*num_leaks)++;
                
                // Only increment the counter if this leak hasn't been counted yet
                if (!meta->counted_as_leak) {
                    pthread_mutex_lock(&ctx->stats_lock);
                    ctx->stats.memory_leaks_detected++;
                    pthread_mutex_unlock(&ctx->stats_lock);
                    
                    // Mark as counted to prevent double-counting in future calls
                    meta->counted_as_leak = true;
                }
            }
            
            node = node->next;
        }
        
        pthread_mutex_unlock(&ctx->allocation_table->locks[i]);
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_analyze_memory_leaks(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    FILE* fp = fopen(file_path, "r");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    char line[1024];
    uint32_t line_num = 0;
    bool found_alloc = false;
    bool found_free = false;
    char alloc_var[128] = {0};
    
    while (fgets(line, sizeof(line), fp) && *num_issues < max_issues) {
        line_num++;
        
        // Look for malloc/calloc/realloc
        if (contains_pattern(line, "malloc(") || 
            contains_pattern(line, "calloc(") ||
            contains_pattern(line, "realloc(")) {
            found_alloc = true;
            found_free = false;
            
            // Extract variable name (simplified)
            const char* eq = strchr(line, '=');
            if (eq) {
                const char* start = line;
                while (start < eq && (*start == ' ' || *start == '\t')) start++;
                const char* end = eq - 1;
                while (end > start && (*end == ' ' || *end == '\t')) end--;
                
                if (end - start < sizeof(alloc_var)) {
                    strncpy(alloc_var, start, end - start + 1);
                    alloc_var[end - start + 1] = '\0';
                }
            }
        }
        
        // Look for corresponding free
        if (found_alloc && alloc_var[0] && contains_pattern(line, "free(") &&
            contains_pattern(line, alloc_var)) {
            found_free = true;
        }
        
        // Check for function return or end of scope without free
        if (found_alloc && !found_free && 
            (contains_pattern(line, "return") || contains_pattern(line, "}"))) {
            
            msm_issue_t* issue = &issues[*num_issues];
            memset(issue, 0, sizeof(msm_issue_t));
            
            issue->issue_type = MSM_ISSUE_MEMORY_LEAK;
            issue->priority = BUG_PRIORITY_P1_HIGH;
            issue->risk_level = RISK_LEVEL_HIGH;
            issue->file_path = strdup(file_path);
            issue->line_number = line_num;
            
            char desc[256];
            snprintf(desc, sizeof(desc),
                    "Potential memory leak: allocated memory '%s' not freed before return/scope end",
                    alloc_var);
            issue->description = strdup(desc);
            issue->recommendation = strdup("Ensure all allocated memory is freed before return or end of scope");
            
            (*num_issues)++;
            
            record_issue(ctx, MSM_ISSUE_MEMORY_LEAK, BUG_PRIORITY_P1_HIGH,
                       file_path, line_num, NULL, desc, NULL);
            
            found_alloc = false;
        }
    }
    
    fclose(fp);
    
    pthread_mutex_lock(&ctx->stats_lock);
    ctx->stats.files_analyzed++;
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== Comprehensive Analysis ====================

crrss_status_t msm_analyze_file(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    uint32_t issues_found = 0;
    
    // Run all detectors
    uint32_t partial_issues = 0;
    
    // 1. Use-after-free detection
    if (ctx->config.enable_use_after_free_detection) {
        msm_detect_use_after_free(ctx, file_path, issues, max_issues, &partial_issues);
        issues_found += partial_issues;
    }
    
    // 2. Double-free detection
    if (ctx->config.enable_double_free_detection && issues_found < max_issues) {
        msm_detect_double_free(ctx, file_path, issues + issues_found, 
                              max_issues - issues_found, &partial_issues);
        issues_found += partial_issues;
    }
    
    // 3. Memory leak analysis
    if (ctx->config.enable_leak_detection && issues_found < max_issues) {
        msm_analyze_memory_leaks(ctx, file_path, issues + issues_found,
                                max_issues - issues_found, &partial_issues);
        issues_found += partial_issues;
    }
    
    // 4. NULL-check analysis (results stored separately, convert to issues)
    if (ctx->config.enable_null_check_enforcement && issues_found < max_issues) {
        null_check_result_t* null_results = malloc(100 * sizeof(null_check_result_t));
        if (null_results) {
            uint32_t null_count = 0;
            msm_analyze_null_checks(ctx, file_path, null_results, 100, &null_count);
            
            for (uint32_t i = 0; i < null_count && issues_found < max_issues; i++) {
                msm_issue_t* issue = &issues[issues_found];
                memset(issue, 0, sizeof(msm_issue_t));
                
                issue->issue_type = MSM_ISSUE_MISSING_NULL_CHECK;
                issue->priority = BUG_PRIORITY_P2_MEDIUM;
                issue->risk_level = RISK_LEVEL_MEDIUM;
                issue->file_path = strdup(null_results[i].file_path);
                issue->line_number = null_results[i].line_number;
                issue->description = strdup("Missing NULL check before pointer dereference");
                issue->recommendation = strdup(null_results[i].suggestion);
                
                issues_found++;
            }
            
            // Free null results
            for (uint32_t i = 0; i < null_count; i++) {
                if (null_results[i].file_path) free((void*)null_results[i].file_path);
                if (null_results[i].suggestion) free((void*)null_results[i].suggestion);
            }
            free(null_results);
        }
    }
    
    // 5. Buffer overflow detection
    if (ctx->config.enable_buffer_overflow_detection && issues_found < max_issues) {
        buffer_analysis_result_t* buffer_results = malloc(100 * sizeof(buffer_analysis_result_t));
        if (buffer_results) {
            uint32_t buffer_count = 0;
            msm_detect_buffer_overflow(ctx, file_path, buffer_results, 100, &buffer_count);
            
            for (uint32_t i = 0; i < buffer_count && issues_found < max_issues; i++) {
                msm_issue_t* issue = &issues[issues_found];
                memset(issue, 0, sizeof(msm_issue_t));
                
                issue->issue_type = MSM_ISSUE_BUFFER_OVERFLOW;
                issue->priority = BUG_PRIORITY_P1_HIGH;
                issue->risk_level = RISK_LEVEL_HIGH;
                issue->file_path = strdup(buffer_results[i].file_path);
                issue->line_number = buffer_results[i].line_number;
                issue->description = strdup("Potential buffer overflow from unsafe function");
                issue->recommendation = strdup(buffer_results[i].recommendation);
                
                issues_found++;
            }
            
            // Free buffer results
            for (uint32_t i = 0; i < buffer_count; i++) {
                if (buffer_results[i].file_path) free((void*)buffer_results[i].file_path);
                if (buffer_results[i].recommendation) free((void*)buffer_results[i].recommendation);
            }
            free(buffer_results);
        }
    }
    
    *num_issues = issues_found;
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_analyze_directory(
    msm_context_t* ctx,
    const char* dir_path,
    msm_report_t* report
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!dir_path || !report) return CRRSS_ERROR_INVALID_PARAM;
    
    // This would require directory traversal - simplified implementation
    // In full implementation, would recursively analyze all .c and .h files
    
    memset(report, 0, sizeof(msm_report_t));
    
    // Get current statistics
    msm_get_statistics(ctx, &report->statistics);
    
    // Copy issues
    pthread_mutex_lock(&ctx->issue_lock);
    report->issue_count = ctx->issue_count;
    report->max_issues = ctx->issue_count;
    if (ctx->issue_count > 0) {
        report->issues = malloc(ctx->issue_count * sizeof(msm_issue_t));
        if (report->issues) {
            memcpy(report->issues, ctx->issues, ctx->issue_count * sizeof(msm_issue_t));
        }
    }
    pthread_mutex_unlock(&ctx->issue_lock);
    
    // Calculate safety score
    msm_calculate_safety_score(ctx, &report->safety_score);
    
    // Determine overall risk
    if (report->statistics.use_after_free_detected > 0 || 
        report->statistics.double_free_detected > 0 ||
        report->statistics.buffer_overflow_detected > 0) {
        report->overall_risk = RISK_LEVEL_CRITICAL;
    } else if (report->statistics.memory_leaks_detected > 0 ||
               report->statistics.null_deref_detected > 0) {
        report->overall_risk = RISK_LEVEL_HIGH;
    } else if (report->statistics.missing_null_checks > 0) {
        report->overall_risk = RISK_LEVEL_MEDIUM;
    } else {
        report->overall_risk = RISK_LEVEL_LOW;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_analyze_snippet(
    msm_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!code_snippet || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    // Write snippet to temporary file and analyze
    char temp_file[] = "/tmp/msm_snippet_XXXXXX";
    int fd = mkstemp(temp_file);
    if (fd == -1) return CRRSS_ERROR_FILE_ACCESS;
    
    write(fd, code_snippet, snippet_length);
    close(fd);
    
    crrss_status_t status = msm_analyze_file(ctx, temp_file, issues, max_issues, num_issues);
    
    unlink(temp_file);
    
    return status;
}

// ==================== Statistics & Reporting ====================

crrss_status_t msm_get_statistics(
    msm_context_t* ctx,
    msm_statistics_t* stats
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!stats) return CRRSS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->stats_lock);
    *stats = ctx->stats;
    clock_gettime(CLOCK_MONOTONIC, &stats->analysis_end_time);
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_generate_report(
    msm_context_t* ctx,
    msm_report_t* report
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!report) return CRRSS_ERROR_INVALID_PARAM;
    
    memset(report, 0, sizeof(msm_report_t));
    
    // Get statistics
    msm_get_statistics(ctx, &report->statistics);
    
    // Copy all issues
    pthread_mutex_lock(&ctx->issue_lock);
    report->issue_count = ctx->issue_count;
    report->max_issues = ctx->issue_count;
    if (ctx->issue_count > 0) {
        report->issues = malloc(ctx->issue_count * sizeof(msm_issue_t));
        if (report->issues) {
            memcpy(report->issues, ctx->issues, ctx->issue_count * sizeof(msm_issue_t));
        }
    }
    pthread_mutex_unlock(&ctx->issue_lock);
    
    // Get memory leaks
    report->leak_count = 0;
    report->leak_records = malloc(1000 * sizeof(allocation_metadata_t));
    if (report->leak_records) {
        msm_detect_leaks(ctx, report->leak_records, 1000, &report->leak_count);
    }
    
    // Calculate safety score
    msm_calculate_safety_score(ctx, &report->safety_score);
    
    // Determine overall risk
    if (report->statistics.use_after_free_detected > 0 ||
        report->statistics.double_free_detected > 0 ||
        report->statistics.buffer_overflow_detected > 5) {
        report->overall_risk = RISK_LEVEL_CRITICAL;
    } else if (report->statistics.memory_leaks_detected > 10 ||
               report->statistics.buffer_overflow_detected > 0) {
        report->overall_risk = RISK_LEVEL_HIGH;
    } else if (report->statistics.missing_null_checks > 10) {
        report->overall_risk = RISK_LEVEL_MEDIUM;
    } else {
        report->overall_risk = RISK_LEVEL_LOW;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_export_report(
    msm_context_t* ctx,
    const msm_report_t* report,
    const char* output_path,
    const char* format
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!report || !output_path || !format) return CRRSS_ERROR_INVALID_PARAM;
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) return CRRSS_ERROR_FILE_ACCESS;
    
    if (strcmp(format, "text") == 0) {
        // Text format
        fprintf(fp, "=== MSM Memory Safety Analysis Report ===\n\n");
        
        fprintf(fp, "Statistics:\n");
        fprintf(fp, "  Total Allocations Tracked: %lu\n", report->statistics.total_allocations_tracked);
        fprintf(fp, "  Total Deallocations Tracked: %lu\n", report->statistics.total_deallocations_tracked);
        fprintf(fp, "  Current Allocations: %lu\n", report->statistics.current_allocations);
        fprintf(fp, "  Files Analyzed: %u\n", report->statistics.files_analyzed);
        fprintf(fp, "  Total Issues: %u\n", report->statistics.total_issues_detected);
        fprintf(fp, "\n");
        
        fprintf(fp, "Issue Breakdown:\n");
        fprintf(fp, "  Memory Leaks: %u\n", report->statistics.memory_leaks_detected);
        fprintf(fp, "  Use-After-Free: %u\n", report->statistics.use_after_free_detected);
        fprintf(fp, "  Double-Free: %u\n", report->statistics.double_free_detected);
        fprintf(fp, "  NULL Dereferences: %u\n", report->statistics.null_deref_detected);
        fprintf(fp, "  Buffer Overflows: %u\n", report->statistics.buffer_overflow_detected);
        fprintf(fp, "  Missing NULL Checks: %u\n", report->statistics.missing_null_checks);
        fprintf(fp, "\n");
        
        fprintf(fp, "Safety Score: %.2f/1.0\n", report->safety_score);
        fprintf(fp, "Overall Risk: %s\n", risk_level_to_string(report->overall_risk));
        fprintf(fp, "\n");
        
        fprintf(fp, "=== Detailed Issues ===\n");
        for (uint32_t i = 0; i < report->issue_count; i++) {
            msm_issue_t* issue = &report->issues[i];
            fprintf(fp, "\n[%u] %s\n", i + 1, msm_issue_type_to_string(issue->issue_type));
            fprintf(fp, "    Priority: %s\n", bug_priority_to_string(issue->priority));
            fprintf(fp, "    Risk: %s\n", risk_level_to_string(issue->risk_level));
            if (issue->file_path) {
                fprintf(fp, "    Location: %s:%u\n", issue->file_path, issue->line_number);
            }
            if (issue->description) {
                fprintf(fp, "    Description: %s\n", issue->description);
            }
            if (issue->recommendation) {
                fprintf(fp, "    Recommendation: %s\n", issue->recommendation);
            }
        }
        
    } else if (strcmp(format, "json") == 0) {
        // JSON format
        fprintf(fp, "{\n");
        fprintf(fp, "  \"statistics\": {\n");
        fprintf(fp, "    \"total_allocations\": %lu,\n", report->statistics.total_allocations_tracked);
        fprintf(fp, "    \"total_deallocations\": %lu,\n", report->statistics.total_deallocations_tracked);
        fprintf(fp, "    \"current_allocations\": %lu,\n", report->statistics.current_allocations);
        fprintf(fp, "    \"total_issues\": %u,\n", report->statistics.total_issues_detected);
        fprintf(fp, "    \"memory_leaks\": %u,\n", report->statistics.memory_leaks_detected);
        fprintf(fp, "    \"use_after_free\": %u,\n", report->statistics.use_after_free_detected);
        fprintf(fp, "    \"double_free\": %u,\n", report->statistics.double_free_detected);
        fprintf(fp, "    \"null_dereferences\": %u,\n", report->statistics.null_deref_detected);
        fprintf(fp, "    \"buffer_overflows\": %u,\n", report->statistics.buffer_overflow_detected);
        fprintf(fp, "    \"missing_null_checks\": %u\n", report->statistics.missing_null_checks);
        fprintf(fp, "  },\n");
        fprintf(fp, "  \"safety_score\": %.2f,\n", report->safety_score);
        fprintf(fp, "  \"overall_risk\": \"%s\",\n", risk_level_to_string(report->overall_risk));
        fprintf(fp, "  \"issue_count\": %u\n", report->issue_count);
        fprintf(fp, "}\n");
    }
    
    fclose(fp);
    
    return CRRSS_SUCCESS;
}

// ==================== Integration Functions ====================

crrss_status_t msm_integrate_bpme(msm_context_t* ctx, void* bpme_ctx) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    ctx->bpme_ctx = bpme_ctx;
    return CRRSS_SUCCESS;
}

crrss_status_t msm_integrate_sciv(msm_context_t* ctx, void* sciv_ctx) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    ctx->sciv_ctx = sciv_ctx;
    return CRRSS_SUCCESS;
}

crrss_status_t msm_integrate_memory_layer(msm_context_t* ctx, void* memory_ctx) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    ctx->memory_ctx = memory_ctx;
    return CRRSS_SUCCESS;
}

// ==================== Query Functions ====================

crrss_status_t msm_query_issues_by_type(
    msm_context_t* ctx,
    msm_issue_type_t issue_type,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    pthread_mutex_lock(&ctx->issue_lock);
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].issue_type == issue_type) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    pthread_mutex_unlock(&ctx->issue_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_query_issues_by_priority(
    msm_context_t* ctx,
    bug_priority_t priority,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    pthread_mutex_lock(&ctx->issue_lock);
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].priority == priority) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    pthread_mutex_unlock(&ctx->issue_lock);
    
    return CRRSS_SUCCESS;
}

crrss_status_t msm_query_issues_by_file(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!file_path || !issues || !num_issues) return CRRSS_ERROR_INVALID_PARAM;
    
    *num_issues = 0;
    
    pthread_mutex_lock(&ctx->issue_lock);
    
    for (uint32_t i = 0; i < ctx->issue_count && *num_issues < max_issues; i++) {
        if (ctx->issues[i].file_path && 
            strcmp(ctx->issues[i].file_path, file_path) == 0) {
            issues[*num_issues] = ctx->issues[i];
            (*num_issues)++;
        }
    }
    
    pthread_mutex_unlock(&ctx->issue_lock);
    
    return CRRSS_SUCCESS;
}

// ==================== Utility Functions ====================

const char* msm_issue_type_to_string(msm_issue_type_t issue_type) {
    if (issue_type >= MSM_ISSUE_COUNT) return "Unknown";
    return MSM_ISSUE_TYPE_STRINGS[issue_type];
}

const char* msm_pointer_state_to_string(pointer_state_t state) {
    if (state > POINTER_STATE_DANGLING) return "Unknown";
    return POINTER_STATE_STRINGS[state];
}

const char* msm_tracking_mode_to_string(msm_tracking_mode_t mode) {
    if (mode > MSM_TRACKING_PARANOID) return "Unknown";
    return TRACKING_MODE_STRINGS[mode];
}

crrss_status_t msm_calculate_safety_score(
    msm_context_t* ctx,
    double* score
) {
    if (!ctx || !ctx->initialized) return CRRSS_ERROR_NOT_INITIALIZED;
    if (!score) return CRRSS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->stats_lock);
    
    // Calculate score based on issues found
    double base_score = 1.0;
    
    // Critical issues have the highest impact
    base_score -= ctx->stats.use_after_free_detected * 0.10;
    base_score -= ctx->stats.double_free_detected * 0.10;
    base_score -= ctx->stats.buffer_overflow_detected * 0.08;
    base_score -= ctx->stats.null_deref_detected * 0.05;
    base_score -= ctx->stats.memory_leaks_detected * 0.03;
    base_score -= ctx->stats.missing_null_checks * 0.01;
    
    // Ensure score is between 0 and 1
    if (base_score < 0.0) base_score = 0.0;
    if (base_score > 1.0) base_score = 1.0;
    
    *score = base_score;
    
    pthread_mutex_unlock(&ctx->stats_lock);
    
    return CRRSS_SUCCESS;
}
