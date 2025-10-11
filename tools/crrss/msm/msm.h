
/**
 * @file msm.h
 * @brief Memory-Safety Maniac Profile - Comprehensive Memory Safety Analysis
 * 
 * The MSM (Memory-Safety Maniac Profile) provides comprehensive memory safety
 * analysis for the BDI kernel, including:
 * - Pointer safety analysis and lifecycle tracking
 * - Real-time malloc/free tracking with detailed metadata
 * - NULL-check enforcement and validation
 * - Buffer overflow detection and prevention
 * - Use-after-free detection
 * - Double-free detection
 * - Memory leak detection
 * - Integration with CRRSS validation system
 * 
 * Phase 1B Stage 3 Implementation
 */

#ifndef MSM_H
#define MSM_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== MSM Configuration ====================

/**
 * @brief MSM tracking modes
 */
typedef enum {
    MSM_TRACKING_DISABLED = 0,
    MSM_TRACKING_BASIC = 1,      // Basic tracking without stack traces
    MSM_TRACKING_DETAILED = 2,   // Detailed tracking with stack traces
    MSM_TRACKING_PARANOID = 3    // Maximum safety checks (slower)
} msm_tracking_mode_t;

/**
 * @brief MSM configuration structure
 */
typedef struct {
    // Tracking options
    msm_tracking_mode_t tracking_mode;
    bool enable_pointer_tracking;
    bool enable_allocation_tracking;
    bool enable_null_check_enforcement;
    bool enable_buffer_overflow_detection;
    bool enable_use_after_free_detection;
    bool enable_double_free_detection;
    bool enable_leak_detection;
    
    // Capacity limits
    uint32_t max_tracked_pointers;
    uint32_t max_tracked_allocations;
    uint32_t max_stack_depth;
    
    // Analysis options
    bool generate_reports;
    bool enforce_null_checks;
    bool track_allocation_sites;
    const char* report_output_dir;
    
    // Integration
    bool integrate_with_bpme;
    bool integrate_with_sciv;
    bool integrate_with_memory_layer;
} msm_config_t;

// ==================== Pointer Lifecycle States ====================

/**
 * @brief Pointer lifecycle states
 */
typedef enum {
    POINTER_STATE_UNINITIALIZED = 0,
    POINTER_STATE_ALLOCATED = 1,
    POINTER_STATE_VALID = 2,
    POINTER_STATE_FREED = 3,
    POINTER_STATE_INVALID = 4,
    POINTER_STATE_DANGLING = 5
} pointer_state_t;

/**
 * @brief Pointer access type
 */
typedef enum {
    POINTER_ACCESS_READ = 0,
    POINTER_ACCESS_WRITE = 1,
    POINTER_ACCESS_EXECUTE = 2,
    POINTER_ACCESS_FREE = 3
} pointer_access_t;

// ==================== Memory Safety Issue Types ====================

/**
 * @brief Memory safety issue types
 */
typedef enum {
    MSM_ISSUE_MEMORY_LEAK = 0,
    MSM_ISSUE_USE_AFTER_FREE = 1,
    MSM_ISSUE_DOUBLE_FREE = 2,
    MSM_ISSUE_NULL_DEREF = 3,
    MSM_ISSUE_BUFFER_OVERFLOW = 4,
    MSM_ISSUE_BUFFER_UNDERFLOW = 5,
    MSM_ISSUE_UNINITIALIZED_POINTER = 6,
    MSM_ISSUE_DANGLING_POINTER = 7,
    MSM_ISSUE_INVALID_FREE = 8,
    MSM_ISSUE_MISSING_NULL_CHECK = 9,
    MSM_ISSUE_UNSAFE_POINTER_ARITHMETIC = 10,
    MSM_ISSUE_COUNT
} msm_issue_type_t;

// ==================== Data Structures ====================

/**
 * @brief Stack trace information
 */
typedef struct {
    const char* function_name;
    const char* file_path;
    uint32_t line_number;
    void* instruction_pointer;
} stack_frame_t;

/**
 * @brief Stack trace
 */
typedef struct {
    stack_frame_t* frames;
    uint32_t frame_count;
    uint32_t max_frames;
} stack_trace_t;

/**
 * @brief Allocation metadata
 */
typedef struct {
    void* address;
    size_t size;
    bool is_freed;
    
    // Allocation info
    const char* allocation_site_file;
    uint32_t allocation_site_line;
    const char* allocation_function;
    struct timespec allocation_time;
    uint64_t allocation_id;
    
    // Deallocation info (if freed)
    const char* deallocation_site_file;
    uint32_t deallocation_site_line;
    const char* deallocation_function;
    struct timespec deallocation_time;
    
    // Stack traces
    stack_trace_t* allocation_trace;
    stack_trace_t* deallocation_trace;
    
    // Access tracking
    uint64_t read_count;
    uint64_t write_count;
    struct timespec last_access_time;
} allocation_metadata_t;

/**
 * @brief Pointer tracking information
 */
typedef struct {
    void* pointer_address;
    void* points_to;
    pointer_state_t state;
    
    // Lifecycle tracking
    struct timespec creation_time;
    struct timespec last_access_time;
    uint64_t access_count;
    
    // Associated allocation
    allocation_metadata_t* allocation;
    
    // Source location
    const char* source_file;
    uint32_t source_line;
    const char* function_name;
} pointer_tracking_info_t;

/**
 * @brief Memory safety issue
 */
typedef struct {
    msm_issue_type_t issue_type;
    bug_priority_t priority;
    risk_level_t risk_level;
    
    // Location
    const char* file_path;
    uint32_t line_number;
    const char* function_name;
    
    // Description
    const char* description;
    const char* recommendation;
    
    // Related data
    void* related_address;
    allocation_metadata_t* related_allocation;
    pointer_tracking_info_t* related_pointer;
    
    // Timing
    struct timespec detection_time;
    
    // Stack trace at issue detection
    stack_trace_t* issue_trace;
} msm_issue_t;

/**
 * @brief NULL-check enforcement result
 */
typedef struct {
    const char* file_path;
    uint32_t line_number;
    const char* function_name;
    const char* pointer_variable;
    bool null_check_present;
    bool null_check_required;
    const char* suggestion;
} null_check_result_t;

/**
 * @brief Buffer overflow analysis result
 */
typedef struct {
    const char* file_path;
    uint32_t line_number;
    const char* buffer_name;
    size_t buffer_size;
    size_t access_size;
    bool overflow_detected;
    bool underflow_detected;
    const char* recommendation;
} buffer_analysis_result_t;

/**
 * @brief MSM statistics
 */
typedef struct {
    // Tracking statistics
    uint64_t total_allocations_tracked;
    uint64_t total_deallocations_tracked;
    uint64_t current_allocations;
    uint64_t total_pointers_tracked;
    uint64_t current_pointers_tracked;
    
    // Issue detection statistics
    uint32_t memory_leaks_detected;
    uint32_t use_after_free_detected;
    uint32_t double_free_detected;
    uint32_t null_deref_detected;
    uint32_t buffer_overflow_detected;
    uint32_t missing_null_checks;
    uint32_t total_issues_detected;
    
    // Performance metrics
    uint64_t total_memory_tracked;
    uint64_t peak_memory_tracked;
    double tracking_overhead_percent;
    
    // Analysis statistics
    uint32_t files_analyzed;
    uint32_t functions_analyzed;
    struct timespec analysis_start_time;
    struct timespec analysis_end_time;
} msm_statistics_t;

/**
 * @brief MSM comprehensive report
 */
typedef struct {
    msm_statistics_t statistics;
    msm_issue_t* issues;
    uint32_t issue_count;
    uint32_t max_issues;
    
    // Memory analysis
    allocation_metadata_t* leak_records;
    uint32_t leak_count;
    
    // NULL-check analysis
    null_check_result_t* null_check_results;
    uint32_t null_check_result_count;
    
    // Buffer analysis
    buffer_analysis_result_t* buffer_analysis_results;
    uint32_t buffer_analysis_count;
    
    // Overall assessment
    double safety_score;  // 0.0 to 1.0
    risk_level_t overall_risk;
} msm_report_t;

// ==================== MSM Context ====================

typedef struct msm_context msm_context_t;

// ==================== Initialization & Shutdown ====================

/**
 * @brief Initialize the MSM system
 * @param config Configuration parameters
 * @return Initialized MSM context or NULL on failure
 */
msm_context_t* msm_initialize(const msm_config_t* config);

/**
 * @brief Shutdown the MSM system and free resources
 * @param ctx MSM context
 */
void msm_shutdown(msm_context_t* ctx);

/**
 * @brief Reset MSM tracking state
 * @param ctx MSM context
 * @return Status code
 */
crrss_status_t msm_reset(msm_context_t* ctx);

// ==================== Allocation Tracking ====================

/**
 * @brief Track memory allocation
 * @param ctx MSM context
 * @param address Allocated address
 * @param size Allocation size
 * @param file Source file
 * @param line Source line
 * @param function Function name
 * @return Status code
 */
crrss_status_t msm_track_allocation(
    msm_context_t* ctx,
    void* address,
    size_t size,
    const char* file,
    uint32_t line,
    const char* function
);

/**
 * @brief Track memory deallocation
 * @param ctx MSM context
 * @param address Freed address
 * @param file Source file
 * @param line Source line
 * @param function Function name
 * @return Status code
 */
crrss_status_t msm_track_deallocation(
    msm_context_t* ctx,
    void* address,
    const char* file,
    uint32_t line,
    const char* function
);

/**
 * @brief Get allocation metadata
 * @param ctx MSM context
 * @param address Address to query
 * @param metadata Output metadata
 * @return Status code
 */
crrss_status_t msm_get_allocation_metadata(
    msm_context_t* ctx,
    void* address,
    allocation_metadata_t* metadata
);

// ==================== Pointer Safety Analysis ====================

/**
 * @brief Track pointer creation/assignment
 * @param ctx MSM context
 * @param pointer_addr Pointer variable address
 * @param points_to Address it points to
 * @param file Source file
 * @param line Source line
 * @param function Function name
 * @return Status code
 */
crrss_status_t msm_track_pointer(
    msm_context_t* ctx,
    void* pointer_addr,
    void* points_to,
    const char* file,
    uint32_t line,
    const char* function
);

/**
 * @brief Track pointer access
 * @param ctx MSM context
 * @param pointer_addr Pointer being accessed
 * @param access_type Type of access
 * @param file Source file
 * @param line Source line
 * @return Status code
 */
crrss_status_t msm_track_pointer_access(
    msm_context_t* ctx,
    void* pointer_addr,
    pointer_access_t access_type,
    const char* file,
    uint32_t line
);

/**
 * @brief Check if pointer is valid
 * @param ctx MSM context
 * @param pointer Address to check
 * @param is_valid Output validity status
 * @return Status code
 */
crrss_status_t msm_validate_pointer(
    msm_context_t* ctx,
    void* pointer,
    bool* is_valid
);

/**
 * @brief Detect use-after-free patterns in file
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t msm_detect_use_after_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect double-free patterns in file
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t msm_detect_double_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== NULL-Check Enforcement ====================

/**
 * @brief Analyze file for NULL-check compliance
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param results Output array for results
 * @param max_results Maximum results to return
 * @param num_results Number of results found
 * @return Status code
 */
crrss_status_t msm_analyze_null_checks(
    msm_context_t* ctx,
    const char* file_path,
    null_check_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
);

/**
 * @brief Validate NULL check before pointer usage
 * @param ctx MSM context
 * @param pointer Pointer to validate
 * @param file Source file
 * @param line Source line
 * @param has_null_check Whether NULL check is present
 * @return Status code
 */
crrss_status_t msm_validate_null_check(
    msm_context_t* ctx,
    void* pointer,
    const char* file,
    uint32_t line,
    bool* has_null_check
);

// ==================== Buffer Overflow Detection ====================

/**
 * @brief Analyze buffer access for overflow
 * @param ctx MSM context
 * @param buffer Buffer address
 * @param buffer_size Buffer size
 * @param access_offset Access offset
 * @param access_size Access size
 * @param file Source file
 * @param line Source line
 * @return Status code
 */
crrss_status_t msm_check_buffer_access(
    msm_context_t* ctx,
    void* buffer,
    size_t buffer_size,
    size_t access_offset,
    size_t access_size,
    const char* file,
    uint32_t line
);

/**
 * @brief Detect buffer overflow patterns in file
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param results Output array for results
 * @param max_results Maximum results to return
 * @param num_results Number of results found
 * @return Status code
 */
crrss_status_t msm_detect_buffer_overflow(
    msm_context_t* ctx,
    const char* file_path,
    buffer_analysis_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
);

// ==================== Memory Leak Detection ====================

/**
 * @brief Detect memory leaks
 * @param ctx MSM context
 * @param leaks Output array for leak records
 * @param max_leaks Maximum leaks to return
 * @param num_leaks Number of leaks found
 * @return Status code
 */
crrss_status_t msm_detect_leaks(
    msm_context_t* ctx,
    allocation_metadata_t* leaks,
    uint32_t max_leaks,
    uint32_t* num_leaks
);

/**
 * @brief Analyze file for potential memory leaks
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t msm_analyze_memory_leaks(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Comprehensive Analysis ====================

/**
 * @brief Analyze a single file for all memory safety issues
 * @param ctx MSM context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t msm_analyze_file(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Analyze entire directory for memory safety issues
 * @param ctx MSM context
 * @param dir_path Directory to analyze
 * @param report Output comprehensive report
 * @return Status code
 */
crrss_status_t msm_analyze_directory(
    msm_context_t* ctx,
    const char* dir_path,
    msm_report_t* report
);

/**
 * @brief Analyze code snippet for memory safety issues
 * @param ctx MSM context
 * @param code_snippet Code to analyze
 * @param snippet_length Length of code
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t msm_analyze_snippet(
    msm_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Statistics & Reporting ====================

/**
 * @brief Get MSM statistics
 * @param ctx MSM context
 * @param stats Output statistics
 * @return Status code
 */
crrss_status_t msm_get_statistics(
    msm_context_t* ctx,
    msm_statistics_t* stats
);

/**
 * @brief Generate comprehensive MSM report
 * @param ctx MSM context
 * @param report Output report
 * @return Status code
 */
crrss_status_t msm_generate_report(
    msm_context_t* ctx,
    msm_report_t* report
);

/**
 * @brief Export report to file
 * @param ctx MSM context
 * @param report Report to export
 * @param output_path Output file path
 * @param format Format ("text", "json", "html")
 * @return Status code
 */
crrss_status_t msm_export_report(
    msm_context_t* ctx,
    const msm_report_t* report,
    const char* output_path,
    const char* format
);

// ==================== CRRSS Integration ====================

/**
 * @brief Integrate MSM with BPME for pattern detection
 * @param ctx MSM context
 * @param bpme_ctx BPME context
 * @return Status code
 */
crrss_status_t msm_integrate_bpme(
    msm_context_t* ctx,
    void* bpme_ctx
);

/**
 * @brief Integrate MSM with SCIV for validation
 * @param ctx MSM context
 * @param sciv_ctx SCIV context
 * @return Status code
 */
crrss_status_t msm_integrate_sciv(
    msm_context_t* ctx,
    void* sciv_ctx
);

/**
 * @brief Integrate MSM with Memory Integration Layer
 * @param ctx MSM context
 * @param memory_ctx Memory integration context
 * @return Status code
 */
crrss_status_t msm_integrate_memory_layer(
    msm_context_t* ctx,
    void* memory_ctx
);

// ==================== Query Functions ====================

/**
 * @brief Query issues by type
 * @param ctx MSM context
 * @param issue_type Issue type to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t msm_query_issues_by_type(
    msm_context_t* ctx,
    msm_issue_type_t issue_type,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Query issues by priority
 * @param ctx MSM context
 * @param priority Priority level to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t msm_query_issues_by_priority(
    msm_context_t* ctx,
    bug_priority_t priority,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Query issues by file
 * @param ctx MSM context
 * @param file_path File path to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t msm_query_issues_by_file(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Utility Functions ====================

/**
 * @brief Get string representation of issue type
 * @param issue_type Issue type
 * @return String representation
 */
const char* msm_issue_type_to_string(msm_issue_type_t issue_type);

/**
 * @brief Get string representation of pointer state
 * @param state Pointer state
 * @return String representation
 */
const char* msm_pointer_state_to_string(pointer_state_t state);

/**
 * @brief Get string representation of tracking mode
 * @param mode Tracking mode
 * @return String representation
 */
const char* msm_tracking_mode_to_string(msm_tracking_mode_t mode);

/**
 * @brief Calculate memory safety score
 * @param ctx MSM context
 * @param score Output safety score (0.0-1.0)
 * @return Status code
 */
crrss_status_t msm_calculate_safety_score(
    msm_context_t* ctx,
    double* score
);

#ifdef __cplusplus
}
#endif

#endif // MSM_H
