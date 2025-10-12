
/**
 * @file crrss_main.c
 * @brief CRRSS Main Entry Point
 */

#include "crrss_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void parse_query_options(int argc, char** argv, query_options_t* options) {
    // Set defaults
    options->priority = BUG_PRIORITY_UNKNOWN;
    options->category = BUG_CATEGORY_UNKNOWN;
    options->file_path = NULL;
    options->component = NULL;
    options->show_details = false;
    options->max_results = 100;
    
    static struct option long_options[] = {
        {"priority", required_argument, 0, 'p'},
        {"category", required_argument, 0, 'c'},
        {"file", required_argument, 0, 'f'},
        {"component", required_argument, 0, 'C'},
        {"details", no_argument, 0, 'd'},
        {"max-results", required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    optind = 2;  // Skip "crrss query"
    
    while ((opt = getopt_long(argc, argv, "p:c:f:C:dn:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                if (strcmp(optarg, "P0") == 0) options->priority = BUG_PRIORITY_P0_CRITICAL;
                else if (strcmp(optarg, "P1") == 0) options->priority = BUG_PRIORITY_P1_HIGH;
                else if (strcmp(optarg, "P2") == 0) options->priority = BUG_PRIORITY_P2_MEDIUM;
                else if (strcmp(optarg, "P3") == 0) options->priority = BUG_PRIORITY_P3_LOW;
                break;
            case 'c':
                if (strcmp(optarg, "memory") == 0) options->category = BUG_CATEGORY_MEMORY;
                else if (strcmp(optarg, "concurrency") == 0) options->category = BUG_CATEGORY_CONCURRENCY;
                else if (strcmp(optarg, "logic") == 0) options->category = BUG_CATEGORY_LOGIC;
                else if (strcmp(optarg, "performance") == 0) options->category = BUG_CATEGORY_PERFORMANCE;
                else if (strcmp(optarg, "security") == 0) options->category = BUG_CATEGORY_SECURITY;
                break;
            case 'f':
                options->file_path = optarg;
                break;
            case 'C':
                options->component = optarg;
                break;
            case 'd':
                options->show_details = true;
                break;
            case 'n':
                options->max_results = atoi(optarg);
                break;
        }
    }
}

static void parse_stats_options(int argc, char** argv, stats_options_t* options) {
    // Set defaults
    options->directory = NULL;
    options->show_detailed = false;
    options->show_memory_stats = false;
    options->show_validation_stats = false;
    options->output_format = "text";
    
    static struct option long_options[] = {
        {"directory", required_argument, 0, 'd'},
        {"detailed", no_argument, 0, 'D'},
        {"memory", no_argument, 0, 'm'},
        {"validation", no_argument, 0, 'v'},
        {"format", required_argument, 0, 'F'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    optind = 2;  // Skip "crrss stats"
    
    while ((opt = getopt_long(argc, argv, "d:DmvF:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'd':
                options->directory = optarg;
                break;
            case 'D':
                options->show_detailed = true;
                break;
            case 'm':
                options->show_memory_stats = true;
                break;
            case 'v':
                options->show_validation_stats = true;
                break;
            case 'F':
                options->output_format = optarg;
                break;
        }
    }
}

static void parse_msm_options(int argc, char** argv, msm_options_t* options) {
    // Set defaults
    options->file_path = NULL;
    options->directory = NULL;
    options->enable_tracking = true;
    options->detect_use_after_free = true;
    options->detect_double_free = true;
    options->detect_leaks = true;
    options->detect_null_deref = true;
    options->detect_buffer_overflow = true;
    options->generate_report = false;
    options->report_output = NULL;
    options->report_format = "text";
    options->max_issues = 1000;
    
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"directory", required_argument, 0, 'd'},
        {"report", required_argument, 0, 'r'},
        {"format", required_argument, 0, 'F'},
        {"max-issues", required_argument, 0, 'n'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    optind = 2;  // Skip "crrss msm"
    
    while ((opt = getopt_long(argc, argv, "f:d:r:F:n:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                options->file_path = optarg;
                break;
            case 'd':
                options->directory = optarg;
                break;
            case 'r':
                options->report_output = optarg;
                options->generate_report = true;
                break;
            case 'F':
                options->report_format = optarg;
                break;
            case 'n':
                options->max_issues = atoi(optarg);
                break;
        }
    }
}

static void parse_stp_options(int argc, char** argv, stp_options_t* options) {
    // Set defaults
    options->file_path = NULL;
    options->directory = NULL;
    options->strictness_level = 2;  // Strict by default
    options->check_type_safety = true;
    options->check_struct_alignment = true;
    options->check_type_casts = true;
    options->generate_report = false;
    options->report_output = NULL;
    options->report_format = "text";
    options->max_issues = 1000;
    
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"directory", required_argument, 0, 'd'},
        {"strictness", required_argument, 0, 's'},
        {"report", required_argument, 0, 'r'},
        {"format", required_argument, 0, 'F'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "f:d:s:r:F:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f': options->file_path = optarg; break;
            case 'd': options->directory = optarg; break;
            case 's': options->strictness_level = atoi(optarg); break;
            case 'r': options->report_output = optarg; options->generate_report = true; break;
            case 'F': options->report_format = optarg; break;
        }
    }
}

static void parse_validate_options(int argc, char** argv, validate_options_t* options) {
    // Set defaults
    options->file_path = NULL;
    options->directory = NULL;
    options->use_msm = false;
    options->use_stp = false;
    options->use_tdt = false;
    options->use_rers = false;
    options->use_all_profiles = false;
    options->generate_report = false;
    options->report_output = NULL;
    options->report_format = "text";
    options->max_issues = 1000;
    
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"directory", required_argument, 0, 'd'},
        {"use-msm", no_argument, 0, 'M'},
        {"use-stp", no_argument, 0, 'S'},
        {"use-tdt", no_argument, 0, 'T'},
        {"use-rers", no_argument, 0, 'R'},
        {"use-all-profiles", no_argument, 0, 'A'},
        {"report", required_argument, 0, 'r'},
        {"format", required_argument, 0, 'F'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "f:d:MSTRAr:F:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f': options->file_path = optarg; break;
            case 'd': options->directory = optarg; break;
            case 'M': options->use_msm = true; break;
            case 'S': options->use_stp = true; break;
            case 'T': options->use_tdt = true; break;
            case 'R': options->use_rers = true; break;
            case 'A': options->use_all_profiles = true; break;
            case 'r': options->report_output = optarg; options->generate_report = true; break;
            case 'F': options->report_format = optarg; break;
        }
    }
}

static void parse_consult_options(int argc, char** argv, consult_options_t* options) {
    options->project_directory = NULL;
    options->project_type = NULL;
    options->auto_detect = true;
    options->suggest_profiles = true;
    options->suggest_config = true;
    options->output_config = NULL;
    
    static struct option long_options[] = {
        {"directory", required_argument, 0, 'd'},
        {"type", required_argument, 0, 't'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "d:t:o:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd': options->project_directory = optarg; break;
            case 't': options->project_type = optarg; break;
            case 'o': options->output_config = optarg; break;
        }
    }
}

static void parse_configure_options(int argc, char** argv, configure_options_t* options) {
    options->config_file = NULL;
    options->show_config = false;
    options->init_config = false;
    options->validate_config = false;
    options->set_option = NULL;
    
    static struct option long_options[] = {
        {"show", no_argument, 0, 's'},
        {"init", no_argument, 0, 'i'},
        {"validate", no_argument, 0, 'v'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "sivf:", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': options->show_config = true; break;
            case 'i': options->init_config = true; break;
            case 'v': options->validate_config = true; break;
            case 'f': options->config_file = optarg; break;
        }
    }
}

static void parse_profile_options(int argc, char** argv, profile_options_t* options) {
    options->list_profiles = false;
    options->select_profile = NULL;
    options->show_profile_info = false;
    options->profile_name = NULL;
    
    static struct option long_options[] = {
        {"list", no_argument, 0, 'l'},
        {"select", required_argument, 0, 's'},
        {"info", required_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "ls:i:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'l': options->list_profiles = true; break;
            case 's': options->select_profile = optarg; break;
            case 'i': options->show_profile_info = true; options->profile_name = optarg; break;
        }
    }
}

static void parse_lookup_options(int argc, char** argv, lookup_options_t* options) {
    options->pattern_name = NULL;
    options->pattern_id = NULL;
    options->category = BUG_CATEGORY_UNKNOWN;
    options->priority = BUG_PRIORITY_UNKNOWN;
    options->list_all = false;
    options->show_details = false;
    
    static struct option long_options[] = {
        {"pattern", required_argument, 0, 'p'},
        {"list-all", no_argument, 0, 'l'},
        {"details", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    
    int opt;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "p:ld", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p': options->pattern_name = optarg; break;
            case 'l': options->list_all = true; break;
            case 'd': options->show_details = true; break;
        }
    }
}

int main(int argc, char** argv) {
    // Initialize CLI
    crrss_cli_context_t* ctx = crrss_cli_initialize();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize CRRSS\n");
        return 1;
    }
    
    // Try to load configuration from .crrssrc
    crrss_cli_load_config(ctx, ".crrssrc");
    
    // Parse command
    crrss_command_t cmd = crrss_cli_parse_command(ctx, argc, argv);
    
    crrss_status_t status = CRRSS_SUCCESS;
    
    switch (cmd) {
        case CMD_QUERY: {
            query_options_t options;
            parse_query_options(argc, argv, &options);
            status = crrss_cli_execute_query(ctx, &options);
            break;
        }
        
        case CMD_STATS: {
            stats_options_t options;
            parse_stats_options(argc, argv, &options);
            status = crrss_cli_execute_stats(ctx, &options);
            break;
        }
        
        case CMD_MSM: {
            msm_options_t options;
            parse_msm_options(argc, argv, &options);
            status = crrss_cli_execute_msm(ctx, &options);
            break;
        }
        
        case CMD_STP: {
            stp_options_t options;
            parse_stp_options(argc, argv, &options);
            status = crrss_cli_execute_stp(ctx, &options);
            break;
        }
        
        case CMD_VALIDATE: {
            validate_options_t options;
            parse_validate_options(argc, argv, &options);
            status = crrss_cli_execute_validate(ctx, &options);
            break;
        }
        
        case CMD_CONSULT: {
            consult_options_t options;
            parse_consult_options(argc, argv, &options);
            status = crrss_cli_execute_consult(ctx, &options);
            break;
        }
        
        case CMD_CONFIGURE: {
            configure_options_t options;
            parse_configure_options(argc, argv, &options);
            status = crrss_cli_execute_configure(ctx, &options);
            break;
        }
        
        case CMD_PROFILE: {
            profile_options_t options;
            parse_profile_options(argc, argv, &options);
            status = crrss_cli_execute_profile(ctx, &options);
            break;
        }
        
        case CMD_LOOKUP: {
            lookup_options_t options;
            parse_lookup_options(argc, argv, &options);
            status = crrss_cli_execute_lookup(ctx, &options);
            break;
        }
        
        case CMD_INTERACTIVE: {
            status = crrss_cli_interactive_mode(ctx);
            break;
        }
        
        case CMD_HELP:
            crrss_cli_print_help();
            break;
        
        case CMD_VERSION:
            crrss_cli_print_version();
            break;
        
        case CMD_UNKNOWN:
        default:
            fprintf(stderr, "Error: Unknown command\n");
            crrss_cli_print_help();
            status = CRRSS_ERROR_INVALID_PARAM;
            break;
    }
    
    // Cleanup
    crrss_cli_shutdown(ctx);
    
    return (status == CRRSS_SUCCESS) ? 0 : 1;
}
