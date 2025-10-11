
/**
 * @file crrss_cli.c
 * @brief CRRSS CLI Implementation
 */

#include "crrss_cli.h"
#include "../bpme/bpme.h"
#include "../sciv/sciv.h"
#include "../memory_layer/memory_integration.h"
#include "../msm/msm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#define CRRSS_VERSION "1.0.0"

struct crrss_cli_context {
    bpme_context_t* bpme;
    sciv_context_t* sciv;
    memory_integration_context_t* memory;
    msm_context_t* msm;
    bool initialized;
};

// ==================== Initialization ====================

crrss_cli_context_t* crrss_cli_initialize(void) {
    crrss_cli_context_t* ctx = (crrss_cli_context_t*)calloc(1, sizeof(crrss_cli_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Initialize BPME
    bpme_config_t bpme_config = {
        .knowledge_base_path = NULL,
        .enable_ml_predictions = false,
        .enable_pattern_matching = true,
        .confidence_threshold = 0.5,
        .max_predictions = 1000
    };
    ctx->bpme = bpme_initialize(&bpme_config);
    
    // Initialize SCIV
    sciv_config_t sciv_config = {
        .enable_strict_mode = false,
        .enable_style_checks = true,
        .enable_performance_checks = true,
        .max_function_complexity = 20,
        .max_function_lines = 200,
        .max_cyclomatic_complexity = 15,
        .coding_standard = "kernel"
    };
    ctx->sciv = sciv_initialize(&sciv_config);
    
    // Initialize Memory Integration
    memory_integration_config_t mem_config = {
        .enable_leak_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .track_allocations = true,
        .max_tracked_allocations = 10000,
        .memory_subsystem_path = NULL
    };
    ctx->memory = memory_integration_initialize(&mem_config);
    
    // Initialize MSM
    msm_config_t msm_config = {
        .tracking_mode = MSM_TRACKING_DETAILED,
        .enable_pointer_tracking = true,
        .enable_allocation_tracking = true,
        .enable_null_check_enforcement = true,
        .enable_buffer_overflow_detection = true,
        .enable_use_after_free_detection = true,
        .enable_double_free_detection = true,
        .enable_leak_detection = true,
        .max_tracked_pointers = 10000,
        .max_tracked_allocations = 10000,
        .max_stack_depth = 10,
        .generate_reports = true,
        .track_allocation_sites = true,
        .report_output_dir = "/tmp/crrss_msm_reports",
        .integrate_with_bpme = true,
        .integrate_with_sciv = true,
        .integrate_with_memory_layer = true
    };
    ctx->msm = msm_initialize(&msm_config);
    
    // Integrate MSM with other components
    if (ctx->msm) {
        msm_integrate_bpme(ctx->msm, ctx->bpme);
        msm_integrate_sciv(ctx->msm, ctx->sciv);
        msm_integrate_memory_layer(ctx->msm, ctx->memory);
    }
    
    ctx->initialized = (ctx->bpme && ctx->sciv && ctx->memory && ctx->msm);
    
    return ctx;
}

void crrss_cli_shutdown(crrss_cli_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->bpme) {
        bpme_shutdown(ctx->bpme);
    }
    if (ctx->sciv) {
        sciv_shutdown(ctx->sciv);
    }
    if (ctx->memory) {
        memory_integration_shutdown(ctx->memory);
    }
    if (ctx->msm) {
        msm_shutdown(ctx->msm);
    }
    
    free(ctx);
}

// ==================== Command Parsing ====================

crrss_command_t crrss_cli_parse_command(
    crrss_cli_context_t* ctx,
    int argc,
    char** argv
) {
    if (argc < 2) {
        return CMD_HELP;
    }
    
    const char* cmd = argv[1];
    
    if (strcmp(cmd, "query") == 0) {
        return CMD_QUERY;
    } else if (strcmp(cmd, "stats") == 0) {
        return CMD_STATS;
    } else if (strcmp(cmd, "analyze") == 0) {
        return CMD_ANALYZE;
    } else if (strcmp(cmd, "validate") == 0) {
        return CMD_VALIDATE;
    } else if (strcmp(cmd, "report") == 0) {
        return CMD_REPORT;
    } else if (strcmp(cmd, "msm") == 0) {
        return CMD_MSM;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        return CMD_HELP;
    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        return CMD_VERSION;
    }
    
    return CMD_UNKNOWN;
}

// ==================== Query Command ====================

crrss_status_t crrss_cli_execute_query(
    crrss_cli_context_t* ctx,
    const query_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Query Results ===\n\n");
    
    bug_prediction_t predictions[1000];
    uint32_t num_predictions = 0;
    
    // Query by priority if specified
    if (options->priority != BUG_PRIORITY_UNKNOWN) {
        printf("Querying bugs with priority: %s\n", 
               bug_priority_to_string(options->priority));
        
        crrss_status_t status = bpme_query_by_priority(
            ctx->bpme, options->priority, predictions, 
            options->max_results, &num_predictions
        );
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u predictions\n\n", num_predictions);
        }
    }
    
    // Query by category if specified
    if (options->category != BUG_CATEGORY_UNKNOWN) {
        printf("Querying bugs in category: %s\n", 
               bug_category_to_string(options->category));
        
        crrss_status_t status = bpme_query_by_category(
            ctx->bpme, options->category, predictions,
            options->max_results, &num_predictions
        );
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u predictions\n\n", num_predictions);
        }
    }
    
    // Query specific file if specified
    if (options->file_path) {
        printf("Analyzing file: %s\n", options->file_path);
        
        crrss_status_t status = bpme_analyze_file(
            ctx->bpme, options->file_path, predictions,
            options->max_results, &num_predictions
        );
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u potential issues\n\n", num_predictions);
            
            if (options->show_details) {
                for (uint32_t i = 0; i < num_predictions; i++) {
                    printf("Issue #%u:\n", i + 1);
                    printf("  File: %s:%u\n", predictions[i].file_path, predictions[i].line_number);
                    printf("  Priority: %s\n", bug_priority_to_string(predictions[i].priority));
                    printf("  Category: %s\n", bug_category_to_string(predictions[i].category));
                    printf("  Risk: %s\n", risk_level_to_string(predictions[i].risk_level));
                    printf("  Confidence: %.2f%%\n", predictions[i].confidence * 100);
                    printf("  Description: %s\n", predictions[i].description);
                    printf("  Recommendation: %s\n\n", predictions[i].recommendation);
                }
            }
        } else {
            printf("Error analyzing file: %s\n", crrss_status_to_string(status));
        }
    }
    
    printf("Query complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== Stats Command ====================

crrss_status_t crrss_cli_execute_stats(
    crrss_cli_context_t* ctx,
    const stats_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Statistics ===\n\n");
    
    // BPME Statistics
    uint32_t total_scans = 0;
    uint32_t bugs_predicted = 0;
    double accuracy = 0.0;
    
    crrss_status_t status = bpme_get_statistics(
        ctx->bpme, &total_scans, &bugs_predicted, &accuracy
    );
    
    if (status == CRRSS_SUCCESS) {
        printf("Bug Prior Mapping Engine:\n");
        printf("  Total Scans: %u\n", total_scans);
        printf("  Bugs Predicted: %u\n", bugs_predicted);
        printf("  Prediction Accuracy: %.2f%%\n\n", accuracy * 100);
    }
    
    // SCIV Statistics
    uint32_t total_validations = 0;
    uint32_t total_issues = 0;
    double avg_compliance = 0.0;
    
    status = sciv_get_statistics(
        ctx->sciv, &total_validations, &total_issues, &avg_compliance
    );
    
    if (status == CRRSS_SUCCESS) {
        printf("Self-Check Internal Validator:\n");
        printf("  Total Validations: %u\n", total_validations);
        printf("  Total Issues Found: %u\n", total_issues);
        printf("  Average Compliance: %.2f%%\n\n", avg_compliance * 100);
    }
    
    // Memory Integration Statistics
    if (options->show_memory_stats) {
        uint64_t total_allocs = 0;
        uint64_t total_frees = 0;
        uint64_t current_usage = 0;
        
        status = memory_integration_get_statistics(
            ctx->memory, MEMORY_SUBSYSTEM_HAM,
            &total_allocs, &total_frees, &current_usage
        );
        
        if (status == CRRSS_SUCCESS) {
            printf("Memory Integration Layer:\n");
            printf("  Total Allocations: %lu bytes\n", total_allocs);
            printf("  Total Frees: %lu bytes\n", total_frees);
            printf("  Current Usage: %lu bytes\n", current_usage);
            
            double efficiency = 0.0;
            memory_integration_calculate_efficiency(ctx->memory, &efficiency);
            printf("  Memory Efficiency: %.2f%%\n\n", efficiency * 100);
        }
    }
    
    // Directory statistics if specified
    if (options->directory) {
        printf("Analyzing directory: %s\n", options->directory);
        
        validation_report_t report = {0};
        status = sciv_validate_directory(ctx->sciv, options->directory, &report);
        
        if (status == CRRSS_SUCCESS) {
            printf("\nDirectory Analysis:\n");
            printf("  Files Validated: %u\n", report.total_files_validated);
            printf("  Issues Found: %u\n", report.total_issues_found);
            printf("  Errors: %u\n", report.errors);
            printf("  Warnings: %u\n", report.warnings);
            printf("  Compliance Score: %.2f%%\n", report.compliance_score * 100);
        }
    }
    
    printf("\nStatistics complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== MSM Command ====================

crrss_status_t crrss_cli_execute_msm(
    crrss_cli_context_t* ctx,
    const msm_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Memory Safety Maniac (MSM) Analysis ===\n\n");
    
    crrss_status_t status = CRRSS_SUCCESS;
    uint32_t total_issues = 0;
    msm_issue_t issues[1000];
    uint32_t num_issues = 0;
    
    // Analyze file if specified
    if (options->file_path) {
        printf("Analyzing file: %s\n", options->file_path);
        
        status = msm_analyze_file(ctx->msm, options->file_path, 
                                  issues, options->max_issues, &num_issues);
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u memory safety issues\n\n", num_issues);
            total_issues = num_issues;
            
            // Display issues by type
            printf("Issue Breakdown:\n");
            
            uint32_t use_after_free = 0, double_free = 0, memory_leaks = 0;
            uint32_t null_deref = 0, buffer_overflow = 0, missing_null_checks = 0;
            
            for (uint32_t i = 0; i < num_issues; i++) {
                switch (issues[i].issue_type) {
                    case MSM_ISSUE_USE_AFTER_FREE: use_after_free++; break;
                    case MSM_ISSUE_DOUBLE_FREE: double_free++; break;
                    case MSM_ISSUE_MEMORY_LEAK: memory_leaks++; break;
                    case MSM_ISSUE_NULL_DEREF: null_deref++; break;
                    case MSM_ISSUE_BUFFER_OVERFLOW: buffer_overflow++; break;
                    case MSM_ISSUE_MISSING_NULL_CHECK: missing_null_checks++; break;
                    default: break;
                }
            }
            
            printf("  Use-After-Free:     %u\n", use_after_free);
            printf("  Double-Free:        %u\n", double_free);
            printf("  Memory Leaks:       %u\n", memory_leaks);
            printf("  NULL Dereferences:  %u\n", null_deref);
            printf("  Buffer Overflows:   %u\n", buffer_overflow);
            printf("  Missing NULL Checks: %u\n\n", missing_null_checks);
            
            // Show detailed issues
            printf("Detailed Issues:\n");
            for (uint32_t i = 0; i < num_issues; i++) {
                printf("\n[%u] %s\n", i + 1, msm_issue_type_to_string(issues[i].issue_type));
                printf("    Priority: %s\n", bug_priority_to_string(issues[i].priority));
                printf("    Risk: %s\n", risk_level_to_string(issues[i].risk_level));
                if (issues[i].file_path) {
                    printf("    Location: %s:%u\n", issues[i].file_path, issues[i].line_number);
                }
                if (issues[i].function_name) {
                    printf("    Function: %s\n", issues[i].function_name);
                }
                if (issues[i].description) {
                    printf("    Description: %s\n", issues[i].description);
                }
                if (issues[i].recommendation) {
                    printf("    Recommendation: %s\n", issues[i].recommendation);
                }
            }
        } else {
            printf("Error analyzing file: %s\n", crrss_status_to_string(status));
        }
    }
    
    // Analyze directory if specified
    if (options->directory) {
        printf("\nAnalyzing directory: %s\n", options->directory);
        
        msm_report_t report;
        memset(&report, 0, sizeof(msm_report_t));
        
        status = msm_analyze_directory(ctx->msm, options->directory, &report);
        
        if (status == CRRSS_SUCCESS) {
            printf("Directory Analysis Complete\n\n");
            printf("Statistics:\n");
            printf("  Files Analyzed: %u\n", report.statistics.files_analyzed);
            printf("  Total Issues: %u\n", report.statistics.total_issues_detected);
            printf("  Memory Leaks: %u\n", report.statistics.memory_leaks_detected);
            printf("  Use-After-Free: %u\n", report.statistics.use_after_free_detected);
            printf("  Double-Free: %u\n", report.statistics.double_free_detected);
            printf("  NULL Dereferences: %u\n", report.statistics.null_deref_detected);
            printf("  Buffer Overflows: %u\n", report.statistics.buffer_overflow_detected);
            printf("  Missing NULL Checks: %u\n\n", report.statistics.missing_null_checks);
            
            printf("Safety Score: %.2f/1.0\n", report.safety_score);
            printf("Overall Risk: %s\n\n", risk_level_to_string(report.overall_risk));
            
            total_issues = report.issue_count;
            
            // Free report resources
            if (report.issues) free(report.issues);
            if (report.leak_records) free(report.leak_records);
        }
    }
    
    // Generate report if requested
    if (options->generate_report && options->report_output) {
        printf("Generating MSM report...\n");
        
        msm_report_t report;
        memset(&report, 0, sizeof(msm_report_t));
        
        status = msm_generate_report(ctx->msm, &report);
        
        if (status == CRRSS_SUCCESS) {
            status = msm_export_report(ctx->msm, &report, 
                                      options->report_output,
                                      options->report_format);
            
            if (status == CRRSS_SUCCESS) {
                printf("Report saved to: %s\n", options->report_output);
            } else {
                printf("Error saving report: %s\n", crrss_status_to_string(status));
            }
            
            // Free report resources
            if (report.issues) free(report.issues);
            if (report.leak_records) free(report.leak_records);
        }
    }
    
    // Display MSM statistics
    printf("\n=== MSM Statistics ===\n");
    msm_statistics_t stats;
    msm_get_statistics(ctx->msm, &stats);
    
    printf("Allocations Tracked: %lu\n", stats.total_allocations_tracked);
    printf("Deallocations Tracked: %lu\n", stats.total_deallocations_tracked);
    printf("Current Allocations: %lu\n", stats.current_allocations);
    printf("Pointers Tracked: %lu\n", stats.total_pointers_tracked);
    printf("Peak Memory: %lu bytes\n", stats.peak_memory_tracked);
    printf("Files Analyzed: %u\n", stats.files_analyzed);
    printf("Total Issues Detected: %u\n\n", stats.total_issues_detected);
    
    // Calculate and display safety score
    double safety_score = 0.0;
    msm_calculate_safety_score(ctx->msm, &safety_score);
    printf("Overall Safety Score: %.2f/1.0\n", safety_score);
    
    if (safety_score >= 0.9) {
        printf("Assessment: EXCELLENT - Very few memory safety issues detected\n");
    } else if (safety_score >= 0.7) {
        printf("Assessment: GOOD - Some issues detected, review recommended\n");
    } else if (safety_score >= 0.5) {
        printf("Assessment: FAIR - Multiple issues detected, action required\n");
    } else {
        printf("Assessment: POOR - Critical issues detected, immediate action required\n");
    }
    
    printf("\nMSM Analysis complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== Help and Version ====================

void crrss_cli_print_help(void) {
    printf("CRRSS - Code Review, Reliability, and Static Safety System\n");
    printf("Version %s\n\n", CRRSS_VERSION);
    printf("Usage: crrss <command> [options]\n\n");
    printf("Commands:\n");
    printf("  query     Query bug predictions and risk assessments\n");
    printf("  stats     Display codebase statistics and system health\n");
    printf("  msm       Memory Safety Maniac - comprehensive memory safety analysis\n");
    printf("  analyze   Analyze files or directories for bugs\n");
    printf("  validate  Validate code against standards\n");
    printf("  report    Generate detailed reports\n");
    printf("  help      Display this help message\n");
    printf("  version   Display version information\n\n");
    printf("Query Options:\n");
    printf("  -p, --priority <level>    Filter by priority (P0, P1, P2, P3)\n");
    printf("  -c, --category <cat>      Filter by category (memory, concurrency, etc.)\n");
    printf("  -f, --file <path>         Query specific file\n");
    printf("  -d, --details             Show detailed information\n");
    printf("  -n, --max-results <num>   Maximum results to display\n\n");
    printf("Stats Options:\n");
    printf("  -d, --directory <path>    Analyze directory\n");
    printf("  -m, --memory              Show memory statistics\n");
    printf("  -v, --validation          Show validation statistics\n");
    printf("  --format <fmt>            Output format (text, json, csv)\n\n");
    printf("MSM Options:\n");
    printf("  -f, --file <path>         Analyze specific file\n");
    printf("  -d, --directory <path>    Analyze directory recursively\n");
    printf("  --report <path>           Generate and save report\n");
    printf("  --format <fmt>            Report format (text, json, html)\n");
    printf("  --max-issues <num>        Maximum issues to report (default: 1000)\n\n");
    printf("Examples:\n");
    printf("  crrss query -p P0 -d\n");
    printf("  crrss query -f kernel/memory.c\n");
    printf("  crrss stats -d moduler_kernel/\n");
    printf("  crrss stats -m\n");
    printf("  crrss msm -f kernel/memory.c\n");
    printf("  crrss msm -d moduler_kernel/ --report msm_report.txt\n\n");
}

void crrss_cli_print_version(void) {
    printf("CRRSS Version %s\n", CRRSS_VERSION);
    printf("Code Review, Reliability, and Static Safety System\n");
    printf("Part of the Binary Decomposition Interface (BDI) Project\n\n");
    printf("Components:\n");
    printf("  - Bug Prior Mapping Engine (BPME)\n");
    printf("  - Self-Check Internal Validator (SCIV)\n");
    printf("  - Memory Integration Layer\n");
    printf("  - Memory Safety Maniac Profile (MSM)\n\n");
    printf("Copyright (c) 2025 BDI Development Team\n");
}
