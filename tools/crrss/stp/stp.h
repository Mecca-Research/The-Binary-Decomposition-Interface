/**
 * @file stp.h
 * @brief Strict Typist Profile - Type Safety and Struct Integrity Analysis
 * 
 * The STP (Strict Typist Profile) provides comprehensive type safety analysis
 * for the BDI kernel, including:
 * - Type validation and mismatch detection
 * - Struct alignment and padding analysis
 * - Type casting safety checking
 * - Signed/unsigned conversion detection
 * - Const-correctness validation
 * - Integer overflow detection in casts
 * - Portability issue detection
 * 
 * Phase 2 Stage 1 Implementation
 */

#ifndef STP_H
#define STP_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== STP Configuration ====================

/**
 * @brief Type strictness levels
 */
typedef enum {
    STP_STRICTNESS_PERMISSIVE = 0,  // Allow most conversions
    STP_STRICTNESS_MODERATE = 1,    // Warn on potentially unsafe operations
    STP_STRICTNESS_STRICT = 2,      // Strict type checking
    STP_STRICTNESS_PARANOID = 3     // Maximum type safety enforcement
} stp_strictness_level_t;

/**
 * @brief STP configuration structure
 */
typedef struct {
    // Strictness level
    stp_strictness_level_t strictness_level;
    
    // Type validation options
    bool check_type_mismatches;
    bool check_implicit_conversions;
    bool check_signed_unsigned_mix;
    bool check_pointer_type_compat;
    bool check_type_punning;
    
    // Struct analysis options
    bool check_struct_padding;
    bool check_struct_alignment;
    bool check_unaligned_access;
    bool check_struct_packing;
    bool check_member_ordering;
    bool check_portability;
    
    // Type casting options
    bool check_unsafe_casts;
    bool check_narrowing_conversions;
    bool check_pointer_casts;
    bool check_const_correctness;
    bool check_integer_overflow_casts;
    
    // Analysis options
    bool generate_reports;
    bool suggest_fixes;
    bool check_c23_compliance;
    const char* report_output_dir;
    
    // Integration
    bool integrate_with_bpme;
    bool integrate_with_sciv;
    bool integrate_with_msm;
} stp_config_t;

// ==================== Type Safety Issue Types ====================

/**
 * @brief Type safety issue types
 */
typedef enum {
    STP_ISSUE_TYPE_MISMATCH = 0,
    STP_ISSUE_IMPLICIT_CONVERSION = 1,
    STP_ISSUE_SIGNED_UNSIGNED_MIX = 2,
    STP_ISSUE_POINTER_TYPE_INCOMPAT = 3,
    STP_ISSUE_TYPE_PUNNING = 4,
    STP_ISSUE_UNSAFE_CAST = 5,
    STP_ISSUE_NARROWING_CONVERSION = 6,
    STP_ISSUE_POINTER_CAST_UNSAFE = 7,
    STP_ISSUE_CONST_VIOLATION = 8,
    STP_ISSUE_INTEGER_OVERFLOW_CAST = 9,
    STP_ISSUE_STRUCT_PADDING = 10,
    STP_ISSUE_STRUCT_ALIGNMENT = 11,
    STP_ISSUE_UNALIGNED_ACCESS = 12,
    STP_ISSUE_STRUCT_PACKING = 13,
    STP_ISSUE_MEMBER_ORDERING = 14,
    STP_ISSUE_PORTABILITY = 15,
    STP_ISSUE_COUNT
} stp_issue_type_t;

/**
 * @brief Type information
 */
typedef struct {
    const char* type_name;
    size_t type_size;
    size_t type_alignment;
    bool is_signed;
    bool is_pointer;
    bool is_const;
    bool is_volatile;
    bool is_struct;
    bool is_union;
    bool is_array;
    const char* base_type;  // For pointers and arrays
} type_info_t;

/**
 * @brief Type conversion information
 */
typedef struct {
    type_info_t source_type;
    type_info_t target_type;
    bool is_explicit;  // Explicit cast vs implicit conversion
    bool is_safe;
    bool may_lose_data;
    bool may_change_sign;
    bool may_overflow;
    const char* recommendation;
} type_conversion_t;

/**
 * @brief Struct member information
 */
typedef struct {
    const char* member_name;
    type_info_t member_type;
    size_t offset;
    size_t size;
    size_t alignment;
    size_t padding_after;  // Padding bytes after this member
} struct_member_info_t;

/**
 * @brief Struct layout analysis
 */
typedef struct {
    const char* struct_name;
    size_t total_size;
    size_t useful_size;  // Size without padding
    size_t padding_bytes;
    size_t alignment;
    uint32_t member_count;
    struct_member_info_t* members;
    double padding_percentage;
    bool has_alignment_issues;
    bool has_portability_issues;
    const char* optimization_suggestion;
} struct_layout_t;

/**
 * @brief Type safety issue
 */
typedef struct {
    stp_issue_type_t issue_type;
    bug_priority_t priority;
    risk_level_t risk_level;
    
    // Location
    const char* file_path;
    uint32_t line_number;
    uint32_t column_number;
    const char* function_name;
    
    // Description
    const char* description;
    const char* recommendation;
    const char* code_snippet;
    
    // Type information
    type_conversion_t* conversion_info;  // For type conversion issues
    struct_layout_t* struct_info;        // For struct issues
    
    // Severity
    bool is_error;      // True if this should be treated as error
    bool is_warning;    // True if this is a warning
    bool is_info;       // True if this is informational
} stp_issue_t;

/**
 * @brief Type safety statistics
 */
typedef struct {
    // Type validation statistics
    uint32_t type_mismatches_found;
    uint32_t implicit_conversions_found;
    uint32_t signed_unsigned_mix_found;
    uint32_t pointer_type_issues_found;
    uint32_t type_punning_found;
    
    // Struct analysis statistics
    uint32_t structs_analyzed;
    uint32_t struct_padding_issues;
    uint32_t struct_alignment_issues;
    uint32_t unaligned_access_found;
    uint32_t struct_packing_issues;
    uint32_t member_ordering_issues;
    uint32_t portability_issues;
    
    // Type casting statistics
    uint32_t unsafe_casts_found;
    uint32_t narrowing_conversions_found;
    uint32_t pointer_casts_found;
    uint32_t const_violations_found;
    uint32_t integer_overflow_casts_found;
    
    // Overall statistics
    uint32_t total_issues_found;
    uint32_t files_analyzed;
    uint32_t functions_analyzed;
    uint32_t lines_analyzed;
    
    // Severity breakdown
    uint32_t critical_issues;
    uint32_t high_priority_issues;
    uint32_t medium_priority_issues;
    uint32_t low_priority_issues;
} stp_statistics_t;

/**
 * @brief STP comprehensive report
 */
typedef struct {
    stp_statistics_t statistics;
    stp_issue_t* issues;
    uint32_t issue_count;
    uint32_t max_issues;
    
    // Struct analysis results
    struct_layout_t* struct_layouts;
    uint32_t struct_count;
    
    // Type conversion analysis
    type_conversion_t* conversions;
    uint32_t conversion_count;
    
    // Overall assessment
    double type_safety_score;  // 0.0 to 1.0
    risk_level_t overall_risk;
    const char* summary;
} stp_report_t;

// ==================== STP Context ====================

typedef struct stp_context stp_context_t;

// ==================== Initialization & Shutdown ====================

/**
 * @brief Initialize the STP system
 * @param config Configuration parameters
 * @return Initialized STP context or NULL on failure
 */
stp_context_t* stp_initialize(const stp_config_t* config);

/**
 * @brief Shutdown the STP system and free resources
 * @param ctx STP context
 */
void stp_shutdown(stp_context_t* ctx);

/**
 * @brief Reset STP analysis state
 * @param ctx STP context
 * @return Status code
 */
crrss_status_t stp_reset(stp_context_t* ctx);

/**
 * @brief Configure STP system
 * @param ctx STP context
 * @param config New configuration
 * @return Status code
 */
crrss_status_t stp_configure(stp_context_t* ctx, const stp_config_t* config);

/**
 * @brief Set strictness level
 * @param ctx STP context
 * @param level Strictness level
 * @return Status code
 */
crrss_status_t stp_set_strictness_level(stp_context_t* ctx, stp_strictness_level_t level);

// ==================== Type Validation Engine ====================

/**
 * @brief Validate type compatibility between two types
 * @param ctx STP context
 * @param source_type Source type
 * @param target_type Target type
 * @param is_safe Output: whether conversion is safe
 * @return Status code
 */
crrss_status_t stp_validate_type_compatibility(
    stp_context_t* ctx,
    const type_info_t* source_type,
    const type_info_t* target_type,
    bool* is_safe
);

/**
 * @brief Detect type mismatches in file
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_type_mismatches(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect implicit type conversions
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_implicit_conversions(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect signed/unsigned mismatches
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_signed_unsigned_mix(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect pointer type incompatibilities
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_pointer_type_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect type punning
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_type_punning(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Struct Alignment Analyzer ====================

/**
 * @brief Analyze struct layout
 * @param ctx STP context
 * @param struct_name Name of struct to analyze
 * @param file_path File containing struct definition
 * @param layout Output struct layout information
 * @return Status code
 */
crrss_status_t stp_analyze_struct_layout(
    stp_context_t* ctx,
    const char* struct_name,
    const char* file_path,
    struct_layout_t* layout
);

/**
 * @brief Detect struct padding issues
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_struct_padding_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect struct alignment issues
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_alignment_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect unaligned memory access
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_unaligned_access(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect struct packing issues
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_packing_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect portability issues in structs
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_portability_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Type Casting Safety Checker ====================

/**
 * @brief Analyze type cast safety
 * @param ctx STP context
 * @param source_type Source type
 * @param target_type Target type
 * @param conversion Output conversion analysis
 * @return Status code
 */
crrss_status_t stp_analyze_cast_safety(
    stp_context_t* ctx,
    const type_info_t* source_type,
    const type_info_t* target_type,
    type_conversion_t* conversion
);

/**
 * @brief Detect unsafe type casts
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_unsafe_casts(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect narrowing conversions
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_narrowing_conversions(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect unsafe pointer casts
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_pointer_cast_issues(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect const-correctness violations
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_const_violations(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Detect integer overflow in casts
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_detect_overflow_casts(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Comprehensive Analysis ====================

/**
 * @brief Analyze a single file for all type safety issues
 * @param ctx STP context
 * @param file_path File to analyze
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_analyze_file(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Analyze entire directory for type safety issues
 * @param ctx STP context
 * @param dir_path Directory to analyze
 * @param report Output comprehensive report
 * @return Status code
 */
crrss_status_t stp_analyze_directory(
    stp_context_t* ctx,
    const char* dir_path,
    stp_report_t* report
);

/**
 * @brief Analyze code snippet for type safety issues
 * @param ctx STP context
 * @param code_snippet Code to analyze
 * @param snippet_length Length of code
 * @param issues Output array for issues
 * @param max_issues Maximum issues to return
 * @param num_issues Number of issues found
 * @return Status code
 */
crrss_status_t stp_analyze_snippet(
    stp_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Statistics & Reporting ====================

/**
 * @brief Get STP statistics
 * @param ctx STP context
 * @param stats Output statistics
 * @return Status code
 */
crrss_status_t stp_get_statistics(
    stp_context_t* ctx,
    stp_statistics_t* stats
);

/**
 * @brief Generate comprehensive STP report
 * @param ctx STP context
 * @param report Output report
 * @return Status code
 */
crrss_status_t stp_generate_report(
    stp_context_t* ctx,
    stp_report_t* report
);

/**
 * @brief Export report to file
 * @param ctx STP context
 * @param report Report to export
 * @param output_path Output file path
 * @param format Format ("text", "json", "html")
 * @return Status code
 */
crrss_status_t stp_export_report(
    stp_context_t* ctx,
    const stp_report_t* report,
    const char* output_path,
    const char* format
);

/**
 * @brief Calculate type safety score
 * @param ctx STP context
 * @param score Output safety score (0.0-1.0)
 * @return Status code
 */
crrss_status_t stp_calculate_safety_score(
    stp_context_t* ctx,
    double* score
);

// ==================== CRRSS Integration ====================

/**
 * @brief Integrate STP with BPME for pattern detection
 * @param ctx STP context
 * @param bpme_ctx BPME context
 * @return Status code
 */
crrss_status_t stp_integrate_bpme(
    stp_context_t* ctx,
    void* bpme_ctx
);

/**
 * @brief Integrate STP with SCIV for validation
 * @param ctx STP context
 * @param sciv_ctx SCIV context
 * @return Status code
 */
crrss_status_t stp_integrate_sciv(
    stp_context_t* ctx,
    void* sciv_ctx
);

/**
 * @brief Integrate STP with MSM for memory safety
 * @param ctx STP context
 * @param msm_ctx MSM context
 * @return Status code
 */
crrss_status_t stp_integrate_msm(
    stp_context_t* ctx,
    void* msm_ctx
);

// ==================== Query Functions ====================

/**
 * @brief Query issues by type
 * @param ctx STP context
 * @param issue_type Issue type to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t stp_query_issues_by_type(
    stp_context_t* ctx,
    stp_issue_type_t issue_type,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Query issues by priority
 * @param ctx STP context
 * @param priority Priority level to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t stp_query_issues_by_priority(
    stp_context_t* ctx,
    bug_priority_t priority,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

/**
 * @brief Query issues by file
 * @param ctx STP context
 * @param file_path File path to query
 * @param issues Output array
 * @param max_issues Maximum results
 * @param num_issues Number of results
 * @return Status code
 */
crrss_status_t stp_query_issues_by_file(
    stp_context_t* ctx,
    const char* file_path,
    stp_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// ==================== Utility Functions ====================

/**
 * @brief Get string representation of issue type
 * @param issue_type Issue type
 * @return String representation
 */
const char* stp_issue_type_to_string(stp_issue_type_t issue_type);

/**
 * @brief Get string representation of strictness level
 * @param level Strictness level
 * @return String representation
 */
const char* stp_strictness_level_to_string(stp_strictness_level_t level);

/**
 * @brief Get type information from type name
 * @param ctx STP context
 * @param type_name Name of type
 * @param type_info Output type information
 * @return Status code
 */
crrss_status_t stp_get_type_info(
    stp_context_t* ctx,
    const char* type_name,
    type_info_t* type_info
);

/**
 * @brief Suggest struct layout optimization
 * @param ctx STP context
 * @param layout Current struct layout
 * @param suggestion Output optimization suggestion
 * @return Status code
 */
crrss_status_t stp_suggest_struct_optimization(
    stp_context_t* ctx,
    const struct_layout_t* layout,
    const char** suggestion
);

#ifdef __cplusplus
}
#endif

#endif // STP_H
