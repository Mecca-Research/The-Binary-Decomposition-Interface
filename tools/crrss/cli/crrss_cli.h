
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
    CMD_HELP = 5,
    CMD_VERSION = 6,
    CMD_UNKNOWN = 7
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
