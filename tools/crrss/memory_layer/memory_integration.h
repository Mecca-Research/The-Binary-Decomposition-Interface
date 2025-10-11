
/**
 * @file memory_integration.h
 * @brief Memory Integration Layer - Integrates with BDI memory subsystems
 * 
 * Provides tooling access to BDI's memory management systems:
 * - HAM (Hardware Abstraction Memory)
 * - PMM (Physical Memory Manager)
 * - VMM (Virtual Memory Manager)
 */

#ifndef MEMORY_INTEGRATION_H
#define MEMORY_INTEGRATION_H

#include "../common/crrss_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Memory Subsystem Types ====================
typedef enum {
    MEMORY_SUBSYSTEM_HAM = 0,
    MEMORY_SUBSYSTEM_PMM = 1,
    MEMORY_SUBSYSTEM_VMM = 2,
    MEMORY_SUBSYSTEM_UNKNOWN = 3
} memory_subsystem_t;

// ==================== Memory Pool Information ====================
typedef struct {
    uint64_t total_size;
    uint64_t used_size;
    uint64_t free_size;
    uint32_t allocation_count;
    uint32_t deallocation_count;
    double fragmentation;  // 0.0 to 1.0
} memory_pool_info_t;

// ==================== Memory Allocation Record ====================
typedef struct {
    void* address;
    size_t size;
    const char* allocation_site;  // File:Line
    uint64_t timestamp;
    bool is_freed;
} allocation_record_t;

// ==================== Memory Leak Detection ====================
typedef struct {
    uint32_t potential_leaks;
    allocation_record_t* leak_records;
    uint32_t max_records;
    uint64_t total_leaked_bytes;
} leak_detection_report_t;

// ==================== Configuration ====================
typedef struct {
    bool enable_leak_detection;
    bool enable_use_after_free_detection;
    bool enable_double_free_detection;
    bool track_allocations;
    uint32_t max_tracked_allocations;
    const char* memory_subsystem_path;  // Path to BDI memory subsystems
} memory_integration_config_t;

// ==================== Context ====================
typedef struct memory_integration_context memory_integration_context_t;

// ==================== Initialization ====================
/**
 * @brief Initialize Memory Integration Layer
 * @param config Configuration parameters
 * @return Initialized context or NULL on failure
 */
memory_integration_context_t* memory_integration_initialize(
    const memory_integration_config_t* config
);

/**
 * @brief Shutdown Memory Integration Layer
 * @param ctx Integration context
 */
void memory_integration_shutdown(memory_integration_context_t* ctx);

// ==================== Memory Analysis ====================
/**
 * @brief Analyze memory usage patterns
 * @param ctx Integration context
 * @param subsystem Memory subsystem to analyze
 * @param analysis Output analysis results
 * @return Status code
 */
crrss_status_t memory_integration_analyze(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    memory_analysis_t* analysis
);

/**
 * @brief Get memory pool information
 * @param ctx Integration context
 * @param subsystem Memory subsystem
 * @param pool_info Output pool information
 * @return Status code
 */
crrss_status_t memory_integration_get_pool_info(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    memory_pool_info_t* pool_info
);

// ==================== Leak Detection ====================
/**
 * @brief Detect memory leaks
 * @param ctx Integration context
 * @param report Output leak detection report
 * @return Status code
 */
crrss_status_t memory_integration_detect_leaks(
    memory_integration_context_t* ctx,
    leak_detection_report_t* report
);

/**
 * @brief Track allocation
 * @param ctx Integration context
 * @param address Allocated address
 * @param size Allocation size
 * @param allocation_site Source location
 * @return Status code
 */
crrss_status_t memory_integration_track_allocation(
    memory_integration_context_t* ctx,
    void* address,
    size_t size,
    const char* allocation_site
);

/**
 * @brief Track deallocation
 * @param ctx Integration context
 * @param address Freed address
 * @return Status code
 */
crrss_status_t memory_integration_track_deallocation(
    memory_integration_context_t* ctx,
    void* address
);

// ==================== Pattern Validation ====================
/**
 * @brief Validate memory allocation patterns in file
 * @param ctx Integration context
 * @param file_path File to validate
 * @param issues Output validation issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t memory_integration_validate_patterns(
    memory_integration_context_t* ctx,
    const char* file_path,
    validation_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Check for use-after-free patterns
 * @param ctx Integration context
 * @param file_path File to check
 * @param uaf_count Output count of potential UAF
 * @return Status code
 */
crrss_status_t memory_integration_check_use_after_free(
    memory_integration_context_t* ctx,
    const char* file_path,
    uint32_t* uaf_count
);

/**
 * @brief Check for double-free patterns
 * @param ctx Integration context
 * @param file_path File to check
 * @param df_count Output count of potential double-free
 * @return Status code
 */
crrss_status_t memory_integration_check_double_free(
    memory_integration_context_t* ctx,
    const char* file_path,
    uint32_t* df_count
);

// ==================== Statistics ====================
/**
 * @brief Get memory subsystem statistics
 * @param ctx Integration context
 * @param subsystem Memory subsystem
 * @param total_allocs Total allocations
 * @param total_frees Total deallocations
 * @param current_usage Current memory usage
 * @return Status code
 */
crrss_status_t memory_integration_get_statistics(
    memory_integration_context_t* ctx,
    memory_subsystem_t subsystem,
    uint64_t* total_allocs,
    uint64_t* total_frees,
    uint64_t* current_usage
);

/**
 * @brief Calculate memory efficiency
 * @param ctx Integration context
 * @param efficiency Output efficiency score (0.0-1.0)
 * @return Status code
 */
crrss_status_t memory_integration_calculate_efficiency(
    memory_integration_context_t* ctx,
    double* efficiency
);

// ==================== Reporting ====================
/**
 * @brief Generate memory analysis report
 * @param ctx Integration context
 * @param output_path Path for report output
 * @param format Report format
 * @return Status code
 */
crrss_status_t memory_integration_generate_report(
    memory_integration_context_t* ctx,
    const char* output_path,
    const char* format
);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_INTEGRATION_H
