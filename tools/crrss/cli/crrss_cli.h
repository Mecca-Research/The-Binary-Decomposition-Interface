
/**
 * @file crrss_cli.h
 * @brief CRRSS Command-Line Interface
 */

#ifndef CRRSS_CLI_H
#define CRRSS_CLI_H

#include "../common/crrss_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Command types
typedef enum {
    CMD_QUERY = 0,
    CMD_STATS = 1,
    CMD_ANALYZE = 2,
    CMD_VALIDATE = 3,
    CMD_REPORT = 4,
    CMD_MSM = 5,           // Memory Safety Maniac command
    CMD_STP = 6,           // Strict Typist Profile command
    CMD_TDT = 7,           // Test-Driven Timmy command
    CMD_RERS = 8,          // Runtime Error Replay System command
    CMD_CONSULT = 9,       // Pre-generation consultation
    CMD_CONFIGURE = 10,    // Configuration management
    CMD_PROFILE = 11,      // Profile selection
    CMD_LOOKUP = 12,       // Bug pattern lookup
    CMD_INTERACTIVE = 13,  // Interactive mode
    CMD_HELP = 14,
    CMD_VERSION = 15,
    CMD_UNKNOWN = 16
} crrss_command_t;

// Query options
typedef struct {
    bug_priority_t priority;
    bug_category_t category;
    const char* file_path;
    const char* component;
    bool show_details;
    uint32_t max_results;
} query_options_t;

// Stats options
typedef struct {
    const char* directory;
    bool show_detailed;
    bool show_memory_stats;
    bool show_validation_stats;
    const char* output_format;  // "text", "json", "csv"
} stats_options_t;

// MSM options
typedef struct {
    const char* file_path;
    const char* directory;
    bool enable_tracking;
    bool detect_use_after_free;
    bool detect_double_free;
    bool detect_leaks;
    bool detect_null_deref;
    bool detect_buffer_overflow;
    bool generate_report;
    const char* report_output;
    const char* report_format;  // "text", "json", "html"
    uint32_t max_issues;
} msm_options_t;

// STP options
typedef struct {
    const char* file_path;
    const char* directory;
    uint32_t strictness_level;  // 0=permissive, 1=moderate, 2=strict, 3=paranoid
    bool check_type_safety;
    bool check_struct_alignment;
    bool check_type_casts;
    bool generate_report;
    const char* report_output;
    const char* report_format;
    uint32_t max_issues;
} stp_options_t;

// TDT options
typedef struct {
    const char* file_path;
    const char* directory;
    bool generate_tests;
    bool check_coverage;
    bool run_tests;
    const char* test_output_dir;
    const char* report_output;
    const char* report_format;
    uint32_t max_tests;
} tdt_options_t;

// RERS options
typedef struct {
    const char* error_log;
    const char* replay_target;
    bool enable_learning;
    bool enable_patterns;
    bool generate_report;
    const char* report_output;
    const char* report_format;
} rers_options_t;

// Consult options (pre-generation consultation)
typedef struct {
    const char* project_directory;
    const char* project_type;  // "kernel", "userspace", "library", etc.
    bool auto_detect;
    bool suggest_profiles;
    bool suggest_config;
    const char* output_config;
} consult_options_t;

// Configure options
typedef struct {
    const char* config_file;  // Path to .crrssrc file
    bool show_config;
    bool init_config;
    bool validate_config;
    const char* set_option;   // "key=value" format
} configure_options_t;

// Profile options
typedef struct {
    bool list_profiles;
    const char* select_profile;  // "msm", "stp", "tdt", "rers", "all"
    bool show_profile_info;
    const char* profile_name;
} profile_options_t;

// Lookup options (bug pattern lookup)
typedef struct {
    const char* pattern_name;
    const char* pattern_id;
    bug_category_t category;
    bug_priority_t priority;
    bool list_all;
    bool show_details;
} lookup_options_t;

// Validate options (comprehensive validation)
typedef struct {
    const char* file_path;
    const char* directory;
    bool use_msm;
    bool use_stp;
    bool use_tdt;
    bool use_rers;
    bool use_all_profiles;
    bool generate_report;
    const char* report_output;
    const char* report_format;
    uint32_t max_issues;
} validate_options_t;

// CLI context
typedef struct crrss_cli_context crrss_cli_context_t;

/**
 * @brief Initialize CLI context
 * @return CLI context or NULL on failure
 */
crrss_cli_context_t* crrss_cli_initialize(void);

/**
 * @brief Shutdown CLI context
 * @param ctx CLI context
 */
void crrss_cli_shutdown(crrss_cli_context_t* ctx);

/**
 * @brief Parse command line arguments
 * @param ctx CLI context
 * @param argc Argument count
 * @param argv Argument values
 * @return Parsed command type
 */
crrss_command_t crrss_cli_parse_command(
    crrss_cli_context_t* ctx,
    int argc,
    char** argv
);

/**
 * @brief Execute query command
 * @param ctx CLI context
 * @param options Query options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_query(
    crrss_cli_context_t* ctx,
    const query_options_t* options
);

/**
 * @brief Execute stats command
 * @param ctx CLI context
 * @param options Stats options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_stats(
    crrss_cli_context_t* ctx,
    const stats_options_t* options
);

/**
 * @brief Execute MSM command
 * @param ctx CLI context
 * @param options MSM options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_msm(
    crrss_cli_context_t* ctx,
    const msm_options_t* options
);

/**
 * @brief Execute STP command
 * @param ctx CLI context
 * @param options STP options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_stp(
    crrss_cli_context_t* ctx,
    const stp_options_t* options
);

/**
 * @brief Execute TDT command
 * @param ctx CLI context
 * @param options TDT options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_tdt(
    crrss_cli_context_t* ctx,
    const tdt_options_t* options
);

/**
 * @brief Execute RERS command
 * @param ctx CLI context
 * @param options RERS options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_rers(
    crrss_cli_context_t* ctx,
    const rers_options_t* options
);

/**
 * @brief Execute consultation command
 * @param ctx CLI context
 * @param options Consult options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_consult(
    crrss_cli_context_t* ctx,
    const consult_options_t* options
);

/**
 * @brief Execute configure command
 * @param ctx CLI context
 * @param options Configure options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_configure(
    crrss_cli_context_t* ctx,
    const configure_options_t* options
);

/**
 * @brief Execute profile command
 * @param ctx CLI context
 * @param options Profile options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_profile(
    crrss_cli_context_t* ctx,
    const profile_options_t* options
);

/**
 * @brief Execute lookup command
 * @param ctx CLI context
 * @param options Lookup options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_lookup(
    crrss_cli_context_t* ctx,
    const lookup_options_t* options
);

/**
 * @brief Execute comprehensive validation command
 * @param ctx CLI context
 * @param options Validate options
 * @return Status code
 */
crrss_status_t crrss_cli_execute_validate(
    crrss_cli_context_t* ctx,
    const validate_options_t* options
);

/**
 * @brief Run interactive mode
 * @param ctx CLI context
 * @return Status code
 */
crrss_status_t crrss_cli_interactive_mode(crrss_cli_context_t* ctx);

/**
 * @brief Load configuration from file
 * @param ctx CLI context
 * @param config_path Path to configuration file
 * @return Status code
 */
crrss_status_t crrss_cli_load_config(
    crrss_cli_context_t* ctx,
    const char* config_path
);

/**
 * @brief Save configuration to file
 * @param ctx CLI context
 * @param config_path Path to configuration file
 * @return Status code
 */
crrss_status_t crrss_cli_save_config(
    crrss_cli_context_t* ctx,
    const char* config_path
);

/**
 * @brief Print help message
 */
void crrss_cli_print_help(void);

/**
 * @brief Print version information
 */
void crrss_cli_print_version(void);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_CLI_H
