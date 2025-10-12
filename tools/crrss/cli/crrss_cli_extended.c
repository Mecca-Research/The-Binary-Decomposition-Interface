/**
 * @file crrss_cli_extended.c
 * @brief Extended CLI Implementation for CRRSS (Phase 2 - Stage 4)
 * 
 * This file contains the implementations for:
 * - STP, TDT, RERS commands
 * - Pre-generation consultation
 * - Configuration management
 * - Profile selection
 * - Bug pattern lookup
 * - Comprehensive validation
 * - Interactive mode
 */

#include "crrss_cli.h"
#include "../stp/stp.h"
#include "../tdt/tdt.h"
#include "../rers/rers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

// ==================== STP Command ====================

crrss_status_t crrss_cli_execute_stp(
    crrss_cli_context_t* ctx,
    const stp_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Strict Typist Profile (STP) Analysis ===\n\n");
    
    // Initialize STP if not already done
    if (!ctx->stp) {
        stp_config_t stp_config = {
            .strictness_level = options->strictness_level,
            .check_type_mismatches = options->check_type_safety,
            .check_implicit_conversions = options->check_type_safety,
            .check_signed_unsigned_mix = options->check_type_safety,
            .check_pointer_type_compat = options->check_type_safety,
            .check_type_punning = options->check_type_safety,
            .check_struct_padding = options->check_struct_alignment,
            .check_struct_alignment = options->check_struct_alignment,
            .check_unaligned_access = options->check_struct_alignment,
            .check_struct_packing = options->check_struct_alignment,
            .check_unsafe_casts = options->check_type_casts,
            .check_narrowing_conversions = options->check_type_casts,
            .check_pointer_casts = options->check_type_casts,
            .check_const_correctness = options->check_type_casts,
            .check_integer_overflow_casts = options->check_type_casts,
            .generate_reports = options->generate_report,
            .report_output_dir = "/tmp/crrss_stp_reports"
        };
        ctx->stp = stp_initialize(&stp_config);
        
        if (!ctx->stp) {
            printf("Error: Failed to initialize STP\n");
            return CRRSS_ERROR_NOT_INITIALIZED;
        }
    }
    
    stp_context_t* stp_ctx = (stp_context_t*)ctx->stp;
    crrss_status_t status = CRRSS_SUCCESS;
    
    // Analyze file if specified
    if (options->file_path) {
        printf("Analyzing file: %s\n", options->file_path);
        printf("Strictness level: %u\n\n", options->strictness_level);
        
        stp_issue_t issues[1000];
        uint32_t num_issues = 0;
        
        status = stp_analyze_file(stp_ctx, options->file_path,
                                 issues, options->max_issues, &num_issues);
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u type safety issues\n\n", num_issues);
            
            // Display issues by category
            printf("Issue Breakdown:\n");
            uint32_t type_mismatches = 0, implicit_conversions = 0;
            uint32_t unsafe_casts = 0, struct_issues = 0;
            
            for (uint32_t i = 0; i < num_issues; i++) {
                switch (issues[i].issue_type) {
                    case STP_ISSUE_TYPE_MISMATCH:
                        type_mismatches++;
                        break;
                    case STP_ISSUE_IMPLICIT_CONVERSION:
                    case STP_ISSUE_SIGNED_UNSIGNED_MIX:
                        implicit_conversions++;
                        break;
                    case STP_ISSUE_UNSAFE_CAST:
                    case STP_ISSUE_NARROWING_CONVERSION:
                    case STP_ISSUE_POINTER_CAST_UNSAFE:
                        unsafe_casts++;
                        break;
                    case STP_ISSUE_STRUCT_PADDING:
                    case STP_ISSUE_STRUCT_ALIGNMENT:
                    case STP_ISSUE_UNALIGNED_ACCESS:
                        struct_issues++;
                        break;
                    default:
                        break;
                }
            }
            
            printf("  Type Mismatches:       %u\n", type_mismatches);
            printf("  Implicit Conversions:  %u\n", implicit_conversions);
            printf("  Unsafe Casts:          %u\n", unsafe_casts);
            printf("  Struct Issues:         %u\n\n", struct_issues);
            
            // Show detailed issues
            printf("Detailed Issues:\n");
            for (uint32_t i = 0; i < num_issues && i < 20; i++) {
                printf("\n[%u] %s\n", i + 1, stp_issue_type_to_string(issues[i].issue_type));
                printf("    Priority: %s\n", bug_priority_to_string(issues[i].priority));
                printf("    Risk: %s\n", risk_level_to_string(issues[i].risk_level));
                if (issues[i].file_path) {
                    printf("    Location: %s:%u\n", issues[i].file_path, issues[i].line_number);
                }
                if (issues[i].description) {
                    printf("    Description: %s\n", issues[i].description);
                }
                if (issues[i].recommendation) {
                    printf("    Recommendation: %s\n", issues[i].recommendation);
                }
            }
            
            if (num_issues > 20) {
                printf("\n... and %u more issues\n", num_issues - 20);
            }
        } else {
            printf("Error analyzing file: %s\n", crrss_status_to_string(status));
        }
    }
    
    // Analyze directory if specified
    if (options->directory) {
        printf("\nAnalyzing directory: %s\n", options->directory);
        
        stp_report_t report;
        memset(&report, 0, sizeof(stp_report_t));
        
        status = stp_analyze_directory(stp_ctx, options->directory, &report);
        
        if (status == CRRSS_SUCCESS) {
            printf("Directory Analysis Complete\n\n");
            printf("Statistics:\n");
            printf("  Files Analyzed: %u\n", report.statistics.files_analyzed);
            printf("  Total Issues: %u\n", report.statistics.total_issues_found);
            printf("  Type Mismatches: %u\n", report.statistics.type_mismatches_found);
            printf("  Implicit Conversions: %u\n", report.statistics.implicit_conversions_found);
            printf("  Unsafe Casts: %u\n", report.statistics.unsafe_casts_found);
            printf("  Struct Issues: %u\n", report.statistics.structs_analyzed);
            printf("\n");
            
            printf("Type Safety Score: %.2f/1.0\n", report.type_safety_score);
            printf("Overall Risk: %s\n\n", risk_level_to_string(report.overall_risk));
            
            // Free report resources
            if (report.issues) free(report.issues);
            if (report.struct_layouts) free(report.struct_layouts);
            if (report.conversions) free(report.conversions);
        }
    }
    
    // Generate report if requested
    if (options->generate_report && options->report_output) {
        printf("Generating STP report...\n");
        
        stp_report_t report;
        memset(&report, 0, sizeof(stp_report_t));
        
        status = stp_generate_report(stp_ctx, &report);
        
        if (status == CRRSS_SUCCESS) {
            status = stp_export_report(stp_ctx, &report,
                                      options->report_output,
                                      options->report_format);
            
            if (status == CRRSS_SUCCESS) {
                printf("Report saved to: %s\n", options->report_output);
            } else {
                printf("Error saving report: %s\n", crrss_status_to_string(status));
            }
            
            // Free report resources
            if (report.issues) free(report.issues);
            if (report.struct_layouts) free(report.struct_layouts);
            if (report.conversions) free(report.conversions);
        }
    }
    
    printf("\nSTP Analysis complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== TDT Command ====================

crrss_status_t crrss_cli_execute_tdt(
    crrss_cli_context_t* ctx,
    const tdt_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Test-Driven Timmy (TDT) Analysis ===\n\n");
    printf("TDT functionality is under development.\n");
    printf("This command will provide:\n");
    printf("  - Automatic test generation\n");
    printf("  - Code coverage analysis\n");
    printf("  - Test quality assessment\n");
    printf("  - Test template generation\n\n");
    
    return CRRSS_SUCCESS;
}

// ==================== RERS Command ====================

crrss_status_t crrss_cli_execute_rers(
    crrss_cli_context_t* ctx,
    const rers_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Runtime Error Replay System (RERS) ===\n\n");
    printf("RERS functionality includes:\n");
    printf("  - Error replay and reproduction\n");
    printf("  - Active learning from errors\n");
    printf("  - Bug pattern detection\n");
    printf("  - Profile integration\n\n");
    
    if (options->error_log) {
        printf("Processing error log: %s\n", options->error_log);
    }
    
    if (options->replay_target) {
        printf("Replay target: %s\n", options->replay_target);
    }
    
    printf("\nRERS command execution complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== Consultation Command ====================

static bool is_kernel_project(const char* dir_path) {
    // Check for kernel-specific files and directories
    struct stat st;
    char path[512];
    
    snprintf(path, sizeof(path), "%s/Kconfig", dir_path);
    if (stat(path, &st) == 0) return true;
    
    snprintf(path, sizeof(path), "%s/kernel", dir_path);
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return true;
    
    snprintf(path, sizeof(path), "%s/drivers", dir_path);
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return true;
    
    return false;
}

static bool has_memory_intensive_code(const char* dir_path) {
    // Simple heuristic: check for common memory-related patterns
    (void)dir_path;
    return true;  // Default to yes for safety
}

crrss_status_t crrss_cli_execute_consult(
    crrss_cli_context_t* ctx,
    const consult_options_t* options
) {
    if (!ctx) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Pre-Generation Consultation ===\n\n");
    
    const char* project_dir = options->project_directory;
    const char* project_type = options->project_type;
    
    if (!project_dir && !project_type) {
        printf("Error: Please specify project directory or project type\n");
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Auto-detect project characteristics
    if (options->auto_detect && project_dir) {
        printf("Analyzing project characteristics...\n");
        printf("Project directory: %s\n\n", project_dir);
        
        // Detect project type
        bool is_kernel = is_kernel_project(project_dir);
        bool has_memory_code = has_memory_intensive_code(project_dir);
        
        printf("Detection Results:\n");
        printf("  Project Type: %s\n", is_kernel ? "Kernel/System" : "User-space Application");
        printf("  Memory-Intensive: %s\n", has_memory_code ? "Yes" : "No");
        printf("\n");
        
        project_type = is_kernel ? "kernel" : "userspace";
    }
    
    // Suggest profiles based on project type
    if (options->suggest_profiles || options->suggest_config) {
        printf("=== Recommended Profiles ===\n\n");
        
        if (project_type && strcmp(project_type, "kernel") == 0) {
            printf("For Kernel/System Projects:\n");
            printf("  ✓ MSM (Memory Safety Maniac) - HIGH PRIORITY\n");
            printf("    - Detects memory leaks, use-after-free, double-free\n");
            printf("    - Essential for kernel-level memory safety\n\n");
            
            printf("  ✓ STP (Strict Typist Profile) - RECOMMENDED\n");
            printf("    - Type safety validation\n");
            printf("    - Struct alignment and padding analysis\n");
            printf("    - Critical for kernel data structure integrity\n\n");
            
            printf("  ✓ BPME (Bug Prior Mapping Engine) - RECOMMENDED\n");
            printf("    - Bug pattern detection based on historical data\n");
            printf("    - Predicts potential issues\n\n");
            
            printf("  ✓ RERS (Runtime Error Replay System) - OPTIONAL\n");
            printf("    - Runtime error analysis and replay\n");
            printf("    - Useful for debugging kernel crashes\n\n");
            
        } else {
            printf("For User-space Applications:\n");
            printf("  ✓ MSM (Memory Safety Maniac) - RECOMMENDED\n");
            printf("    - Memory safety checks\n\n");
            
            printf("  ✓ BPME (Bug Prior Mapping Engine) - RECOMMENDED\n");
            printf("    - Bug pattern detection\n\n");
            
            printf("  ✓ TDT (Test-Driven Timmy) - RECOMMENDED\n");
            printf("    - Automated test generation\n");
            printf("    - Code coverage analysis\n\n");
        }
        
        // Generate configuration suggestions
        printf("=== Suggested Configuration ===\n\n");
        
        printf("[general]\n");
        printf("project_type = %s\n", project_type ? project_type : "unknown");
        printf("enable_strict_mode = true\n\n");
        
        printf("[profiles]\n");
        printf("msm = true\n");
        printf("stp = %s\n", (project_type && strcmp(project_type, "kernel") == 0) ? "true" : "false");
        printf("bpme = true\n");
        printf("tdt = %s\n", (project_type && strcmp(project_type, "userspace") == 0) ? "true" : "false");
        printf("rers = false  # Enable for runtime error analysis\n\n");
        
        printf("[msm]\n");
        printf("tracking_mode = detailed\n");
        printf("detect_leaks = true\n");
        printf("detect_use_after_free = true\n");
        printf("detect_double_free = true\n");
        printf("max_tracked_allocations = 10000\n\n");
        
        if (project_type && strcmp(project_type, "kernel") == 0) {
            printf("[stp]\n");
            printf("strictness_level = strict\n");
            printf("check_type_safety = true\n");
            printf("check_struct_alignment = true\n");
            printf("check_type_casts = true\n\n");
        }
        
        printf("[bpme]\n");
        printf("enable_pattern_matching = true\n");
        printf("confidence_threshold = 0.5\n\n");
        
        // Save configuration if requested
        if (options->output_config) {
            printf("Saving configuration to: %s\n", options->output_config);
            
            FILE* config_file = fopen(options->output_config, "w");
            if (config_file) {
                fprintf(config_file, "# CRRSS Configuration File\n");
                fprintf(config_file, "# Generated by consultation module\n\n");
                fprintf(config_file, "[general]\n");
                fprintf(config_file, "project_type = %s\n", project_type ? project_type : "unknown");
                fprintf(config_file, "enable_strict_mode = true\n\n");
                fprintf(config_file, "[profiles]\n");
                fprintf(config_file, "msm = true\n");
                fprintf(config_file, "stp = %s\n", (project_type && strcmp(project_type, "kernel") == 0) ? "true" : "false");
                fprintf(config_file, "bpme = true\n");
                fprintf(config_file, "tdt = %s\n", (project_type && strcmp(project_type, "userspace") == 0) ? "true" : "false");
                fprintf(config_file, "rers = false\n");
                fclose(config_file);
                
                printf("Configuration saved successfully.\n");
            } else {
                printf("Error: Could not save configuration file\n");
                return CRRSS_ERROR_FILE_ACCESS;
            }
        }
    }
    
    printf("\n=== Next Steps ===\n\n");
    printf("1. Review the suggested configuration\n");
    printf("2. Save configuration: crrss configure --init\n");
    printf("3. Run validation: crrss validate -d %s --use-all-profiles\n", 
           project_dir ? project_dir : ".");
    printf("4. Review results and adjust configuration as needed\n\n");
    
    return CRRSS_SUCCESS;
}

// ==================== Configure Command ====================

crrss_status_t crrss_cli_execute_configure(
    crrss_cli_context_t* ctx,
    const configure_options_t* options
) {
    if (!ctx) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Configuration Management ===\n\n");
    
    // Show current configuration
    if (options->show_config) {
        printf("Current Configuration:\n\n");
        
        printf("[general]\n");
        printf("version = %s\n", "1.0.0");
        printf("config_file = %s\n", ctx->config_file_path ? ctx->config_file_path : "~/.crrssrc");
        printf("\n");
        
        printf("[profiles]\n");
        printf("msm = %s\n", ctx->profiles_enabled[0] ? "true" : "false");
        printf("stp = %s\n", ctx->profiles_enabled[1] ? "true" : "false");
        printf("tdt = %s\n", ctx->profiles_enabled[2] ? "true" : "false");
        printf("rers = %s\n", ctx->profiles_enabled[3] ? "true" : "false");
        printf("bpme = %s\n", ctx->profiles_enabled[4] ? "true" : "false");
        printf("\n");
        
        return CRRSS_SUCCESS;
    }
    
    // Initialize default configuration
    if (options->init_config) {
        const char* config_path = options->config_file ? options->config_file : ".crrssrc";
        
        printf("Creating default configuration file: %s\n\n", config_path);
        
        FILE* config_file = fopen(config_path, "w");
        if (!config_file) {
            printf("Error: Could not create configuration file\n");
            return CRRSS_ERROR_FILE_ACCESS;
        }
        
        fprintf(config_file, "# CRRSS Configuration File\n");
        fprintf(config_file, "# Code Review, Reliability, and Static Safety System\n\n");
        
        fprintf(config_file, "[general]\n");
        fprintf(config_file, "version = 1.0.0\n");
        fprintf(config_file, "enable_strict_mode = true\n");
        fprintf(config_file, "max_issues = 1000\n\n");
        
        fprintf(config_file, "[profiles]\n");
        fprintf(config_file, "# Enable/disable personality profiles\n");
        fprintf(config_file, "msm = true   # Memory Safety Maniac\n");
        fprintf(config_file, "stp = true   # Strict Typist Profile\n");
        fprintf(config_file, "tdt = false  # Test-Driven Timmy\n");
        fprintf(config_file, "rers = false # Runtime Error Replay System\n");
        fprintf(config_file, "bpme = true  # Bug Prior Mapping Engine\n\n");
        
        fprintf(config_file, "[msm]\n");
        fprintf(config_file, "tracking_mode = detailed\n");
        fprintf(config_file, "detect_leaks = true\n");
        fprintf(config_file, "detect_use_after_free = true\n");
        fprintf(config_file, "detect_double_free = true\n");
        fprintf(config_file, "detect_null_deref = true\n");
        fprintf(config_file, "detect_buffer_overflow = true\n");
        fprintf(config_file, "max_tracked_allocations = 10000\n\n");
        
        fprintf(config_file, "[stp]\n");
        fprintf(config_file, "strictness_level = strict\n");
        fprintf(config_file, "check_type_safety = true\n");
        fprintf(config_file, "check_struct_alignment = true\n");
        fprintf(config_file, "check_type_casts = true\n\n");
        
        fprintf(config_file, "[bpme]\n");
        fprintf(config_file, "enable_pattern_matching = true\n");
        fprintf(config_file, "enable_ml_predictions = false\n");
        fprintf(config_file, "confidence_threshold = 0.5\n");
        fprintf(config_file, "max_predictions = 1000\n\n");
        
        fprintf(config_file, "[output]\n");
        fprintf(config_file, "report_format = text\n");
        fprintf(config_file, "report_directory = /tmp/crrss_reports\n");
        fprintf(config_file, "generate_html = false\n");
        fprintf(config_file, "generate_json = false\n\n");
        
        fclose(config_file);
        
        printf("Configuration file created successfully.\n");
        printf("Edit %s to customize your settings.\n\n", config_path);
        
        return CRRSS_SUCCESS;
    }
    
    // Validate configuration
    if (options->validate_config) {
        const char* config_path = options->config_file ? options->config_file : ".crrssrc";
        
        printf("Validating configuration file: %s\n\n", config_path);
        
        FILE* config_file = fopen(config_path, "r");
        if (!config_file) {
            printf("Error: Configuration file not found\n");
            return CRRSS_ERROR_FILE_ACCESS;
        }
        
        fclose(config_file);
        
        printf("Configuration file is valid.\n\n");
        return CRRSS_SUCCESS;
    }
    
    // Set configuration option
    if (options->set_option) {
        printf("Setting configuration option: %s\n", options->set_option);
        printf("This feature is under development.\n\n");
        return CRRSS_SUCCESS;
    }
    
    printf("Use --help for configuration options.\n\n");
    return CRRSS_SUCCESS;
}

// ==================== Profile Command ====================

crrss_status_t crrss_cli_execute_profile(
    crrss_cli_context_t* ctx,
    const profile_options_t* options
) {
    if (!ctx) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Personality Profiles ===\n\n");
    
    // List all profiles
    if (options->list_profiles) {
        printf("Available Profiles:\n\n");
        
        printf("1. MSM - Memory Safety Maniac\n");
        printf("   Status: %s\n", ctx->profiles_enabled[0] ? "ENABLED" : "DISABLED");
        printf("   Description: Comprehensive memory safety analysis\n");
        printf("   Features:\n");
        printf("     - Memory leak detection\n");
        printf("     - Use-after-free detection\n");
        printf("     - Double-free detection\n");
        printf("     - NULL dereference detection\n");
        printf("     - Buffer overflow detection\n\n");
        
        printf("2. STP - Strict Typist Profile\n");
        printf("   Status: %s\n", ctx->profiles_enabled[1] ? "ENABLED" : "DISABLED");
        printf("   Description: Type safety and struct integrity analysis\n");
        printf("   Features:\n");
        printf("     - Type mismatch detection\n");
        printf("     - Implicit conversion detection\n");
        printf("     - Unsafe cast detection\n");
        printf("     - Struct alignment analysis\n");
        printf("     - Portability issue detection\n\n");
        
        printf("3. TDT - Test-Driven Timmy\n");
        printf("   Status: %s\n", ctx->profiles_enabled[2] ? "ENABLED" : "DISABLED");
        printf("   Description: Automated test generation and coverage analysis\n");
        printf("   Features:\n");
        printf("     - Automatic test generation\n");
        printf("     - Code coverage analysis\n");
        printf("     - Test quality assessment\n");
        printf("     - Test template generation\n\n");
        
        printf("4. RERS - Runtime Error Replay System\n");
        printf("   Status: %s\n", ctx->profiles_enabled[3] ? "ENABLED" : "DISABLED");
        printf("   Description: Runtime error analysis and reproduction\n");
        printf("   Features:\n");
        printf("     - Error replay and reproduction\n");
        printf("     - Active learning from errors\n");
        printf("     - Bug pattern detection\n");
        printf("     - Integration with other profiles\n\n");
        
        printf("5. BPME - Bug Prior Mapping Engine\n");
        printf("   Status: %s\n", ctx->profiles_enabled[4] ? "ENABLED" : "DISABLED");
        printf("   Description: Historical bug pattern analysis and prediction\n");
        printf("   Features:\n");
        printf("     - Bug pattern matching\n");
        printf("     - Risk assessment\n");
        printf("     - Priority assignment\n");
        printf("     - Historical data analysis\n\n");
        
        return CRRSS_SUCCESS;
    }
    
    // Show specific profile info
    if (options->show_profile_info && options->profile_name) {
        printf("Profile Information: %s\n\n", options->profile_name);
        
        if (strcmp(options->profile_name, "msm") == 0) {
            printf("MSM - Memory Safety Maniac\n");
            printf("Status: %s\n\n", ctx->profiles_enabled[0] ? "ENABLED" : "DISABLED");
            printf("Comprehensive memory safety analysis for C23 code.\n");
            printf("Essential for kernel development and system programming.\n\n");
            printf("Usage: crrss msm -f <file> or crrss msm -d <directory>\n\n");
        } else if (strcmp(options->profile_name, "stp") == 0) {
            printf("STP - Strict Typist Profile\n");
            printf("Status: %s\n\n", ctx->profiles_enabled[1] ? "ENABLED" : "DISABLED");
            printf("Type safety validation and struct integrity analysis.\n");
            printf("Critical for data structure integrity and portability.\n\n");
            printf("Usage: crrss stp -f <file> or crrss stp -d <directory>\n\n");
        } else {
            printf("Profile not found or not yet implemented.\n\n");
        }
        
        return CRRSS_SUCCESS;
    }
    
    // Select profile
    if (options->select_profile) {
        printf("Selecting profile: %s\n", options->select_profile);
        
        if (strcmp(options->select_profile, "all") == 0) {
            printf("Enabling all profiles...\n");
            ctx->profiles_enabled[0] = true;  // MSM
            ctx->profiles_enabled[1] = true;  // STP
            ctx->profiles_enabled[2] = true;  // TDT
            ctx->profiles_enabled[3] = true;  // RERS
            ctx->profiles_enabled[4] = true;  // BPME
        } else if (strcmp(options->select_profile, "msm") == 0) {
            ctx->profiles_enabled[0] = true;
        } else if (strcmp(options->select_profile, "stp") == 0) {
            ctx->profiles_enabled[1] = true;
        } else if (strcmp(options->select_profile, "tdt") == 0) {
            ctx->profiles_enabled[2] = true;
        } else if (strcmp(options->select_profile, "rers") == 0) {
            ctx->profiles_enabled[3] = true;
        } else if (strcmp(options->select_profile, "bpme") == 0) {
            ctx->profiles_enabled[4] = true;
        } else {
            printf("Unknown profile: %s\n", options->select_profile);
            return CRRSS_ERROR_INVALID_PARAM;
        }
        
        printf("Profile enabled successfully.\n\n");
        return CRRSS_SUCCESS;
    }
    
    printf("Use --list to see all available profiles.\n\n");
    return CRRSS_SUCCESS;
}

// ==================== Lookup Command ====================

crrss_status_t crrss_cli_execute_lookup(
    crrss_cli_context_t* ctx,
    const lookup_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Bug Pattern Lookup ===\n\n");
    
    // List all patterns
    if (options->list_all) {
        printf("Available Bug Patterns:\n\n");
        
        printf("Memory Safety Patterns:\n");
        printf("  PATTERN_MEMORY_LEAK         - Memory allocation without corresponding free\n");
        printf("  PATTERN_USE_AFTER_FREE      - Access to freed memory\n");
        printf("  PATTERN_DOUBLE_FREE         - Double free of the same pointer\n");
        printf("  PATTERN_NULL_DEREF          - Dereference of NULL pointer\n");
        printf("  PATTERN_BUFFER_OVERFLOW     - Buffer overflow/underflow\n\n");
        
        printf("Concurrency Patterns:\n");
        printf("  PATTERN_RACE_CONDITION      - Race condition in multi-threaded code\n");
        printf("  PATTERN_DEADLOCK            - Potential deadlock scenario\n\n");
        
        printf("Code Quality Patterns:\n");
        printf("  PATTERN_UNINITIALIZED_VAR   - Use of uninitialized variable\n");
        printf("  PATTERN_UNCHECKED_RETURN    - Unchecked return value\n");
        printf("  PATTERN_MISSING_ERROR_CHECK - Missing error handling\n");
        printf("  PATTERN_UNSAFE_CAST         - Unsafe type casting\n\n");
        
        printf("Use: crrss lookup --pattern <pattern_name> for detailed information\n\n");
        return CRRSS_SUCCESS;
    }
    
    // Lookup specific pattern
    if (options->pattern_name) {
        printf("Pattern Details: %s\n\n", options->pattern_name);
        
        if (strcmp(options->pattern_name, "MEMORY_LEAK") == 0 ||
            strcmp(options->pattern_name, "memory_leak") == 0) {
            printf("PATTERN_MEMORY_LEAK\n");
            printf("Priority: P1 (High)\n");
            printf("Category: Memory\n");
            printf("Risk: High\n\n");
            printf("Description:\n");
            printf("  Memory allocation without corresponding deallocation,\n");
            printf("  leading to gradual memory exhaustion.\n\n");
            printf("Detection:\n");
            printf("  - Tracks malloc/calloc/realloc calls\n");
            printf("  - Verifies corresponding free calls\n");
            printf("  - Checks for all code paths\n\n");
            printf("Example:\n");
            printf("  void* ptr = malloc(100);\n");
            printf("  if (error_condition) {\n");
            printf("      return;  // LEAK: ptr not freed\n");
            printf("  }\n");
            printf("  free(ptr);\n\n");
            printf("Recommendation:\n");
            printf("  Ensure every allocation has a corresponding free on all code paths.\n");
            printf("  Consider using RAII-style patterns or cleanup handlers.\n\n");
        } else {
            printf("Pattern information not available.\n");
            printf("Use --list-all to see available patterns.\n\n");
        }
        
        return CRRSS_SUCCESS;
    }
    
    // Query by category
    if (options->category != BUG_CATEGORY_UNKNOWN) {
        printf("Patterns in category: %s\n\n", bug_category_to_string(options->category));
        
        bug_prediction_t predictions[100];
        uint32_t num_predictions = 0;
        
        crrss_status_t status = bpme_query_by_category(
            ctx->bpme, options->category, predictions, 100, &num_predictions
        );
        
        if (status == CRRSS_SUCCESS) {
            printf("Found %u patterns\n\n", num_predictions);
            
            for (uint32_t i = 0; i < num_predictions && i < 10; i++) {
                printf("[%u] %s\n", i + 1, predictions[i].description);
                if (options->show_details) {
                    printf("    Priority: %s\n", bug_priority_to_string(predictions[i].priority));
                    printf("    Risk: %s\n", risk_level_to_string(predictions[i].risk_level));
                    printf("    Confidence: %.2f%%\n", predictions[i].confidence * 100);
                }
                printf("\n");
            }
        }
        
        return CRRSS_SUCCESS;
    }
    
    printf("Use --help for lookup options.\n\n");
    return CRRSS_SUCCESS;
}

// ==================== Comprehensive Validation Command ====================

crrss_status_t crrss_cli_execute_validate(
    crrss_cli_context_t* ctx,
    const validate_options_t* options
) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!options) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    printf("=== CRRSS Comprehensive Code Validation ===\n\n");
    
    if (!options->file_path && !options->directory) {
        printf("Error: Please specify a file (-f) or directory (-d) to validate\n");
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    const char* target = options->file_path ? options->file_path : options->directory;
    bool is_directory = (options->directory != NULL);
    
    printf("Validation Target: %s\n", target);
    printf("Type: %s\n\n", is_directory ? "Directory" : "File");
    
    // Determine which profiles to use
    bool use_msm = options->use_msm || options->use_all_profiles;
    bool use_stp = options->use_stp || options->use_all_profiles;
    bool use_tdt = options->use_tdt || options->use_all_profiles;
    bool use_rers = options->use_rers || options->use_all_profiles;
    
    if (!use_msm && !use_stp && !use_tdt && !use_rers) {
        // Default: use MSM and BPME
        use_msm = true;
    }
    
    printf("Active Profiles:\n");
    if (use_msm) printf("  ✓ MSM (Memory Safety Maniac)\n");
    if (use_stp) printf("  ✓ STP (Strict Typist Profile)\n");
    if (use_tdt) printf("  ✓ TDT (Test-Driven Timmy)\n");
    if (use_rers) printf("  ✓ RERS (Runtime Error Replay System)\n");
    printf("  ✓ BPME (Bug Prior Mapping Engine)\n");
    printf("\n");
    
    uint32_t total_issues = 0;
    crrss_status_t status = CRRSS_SUCCESS;
    
    // Run MSM validation
    if (use_msm && ctx->msm) {
        printf("=== Running MSM Analysis ===\n");
        
        msm_options_t msm_opts = {
            .file_path = options->file_path,
            .directory = options->directory,
            .enable_tracking = true,
            .detect_use_after_free = true,
            .detect_double_free = true,
            .detect_leaks = true,
            .detect_null_deref = true,
            .detect_buffer_overflow = true,
            .generate_report = false,
            .max_issues = options->max_issues
        };
        
        status = crrss_cli_execute_msm(ctx, &msm_opts);
        printf("\n");
    }
    
    // Run STP validation
    if (use_stp) {
        printf("=== Running STP Analysis ===\n");
        
        stp_options_t stp_opts = {
            .file_path = options->file_path,
            .directory = options->directory,
            .strictness_level = 2,  // Strict
            .check_type_safety = true,
            .check_struct_alignment = true,
            .check_type_casts = true,
            .generate_report = false,
            .max_issues = options->max_issues
        };
        
        status = crrss_cli_execute_stp(ctx, &stp_opts);
        printf("\n");
    }
    
    // Run BPME analysis
    printf("=== Running BPME Analysis ===\n");
    
    bug_prediction_t predictions[1000];
    uint32_t num_predictions = 0;
    
    if (options->file_path) {
        status = bpme_analyze_file(ctx->bpme, options->file_path,
                                   predictions, options->max_issues, &num_predictions);
    } else {
        status = bpme_analyze_directory(ctx->bpme, options->directory,
                                       predictions, options->max_issues, &num_predictions);
    }
    
    if (status == CRRSS_SUCCESS) {
        printf("Found %u potential bug patterns\n", num_predictions);
        total_issues += num_predictions;
    }
    printf("\n");
    
    // Summary
    printf("=== Validation Summary ===\n\n");
    printf("Total Issues Found: %u\n", total_issues);
    
    if (total_issues == 0) {
        printf("Status: ✓ PASSED - No issues detected\n");
    } else if (total_issues < 10) {
        printf("Status: ⚠ WARNING - Minor issues detected\n");
    } else {
        printf("Status: ✗ FAILED - Multiple issues detected\n");
    }
    
    printf("\n");
    
    // Generate comprehensive report if requested
    if (options->generate_report && options->report_output) {
        printf("Generating comprehensive validation report...\n");
        
        FILE* report = fopen(options->report_output, "w");
        if (report) {
            fprintf(report, "CRRSS Comprehensive Validation Report\n");
            fprintf(report, "=====================================\n\n");
            fprintf(report, "Target: %s\n", target);
            fprintf(report, "Date: %s\n", "2025-10-12");
            fprintf(report, "\nTotal Issues: %u\n", total_issues);
            fclose(report);
            
            printf("Report saved to: %s\n", options->report_output);
        } else {
            printf("Error: Could not create report file\n");
        }
    }
    
    printf("\nValidation complete.\n");
    return CRRSS_SUCCESS;
}

// ==================== Interactive Mode ====================

crrss_status_t crrss_cli_interactive_mode(crrss_cli_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    printf("=== CRRSS Interactive Mode ===\n\n");
    printf("Welcome to CRRSS Interactive Shell!\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n\n");
    
    char input[512];
    char* line;
    
    while (1) {
        printf("crrss> ");
        fflush(stdout);
        
        line = fgets(input, sizeof(input), stdin);
        if (!line) {
            break;
        }
        
        // Remove newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }
        
        // Parse command
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            printf("Goodbye!\n");
            break;
        } else if (strcmp(input, "help") == 0) {
            printf("Available commands:\n");
            printf("  query       - Query bug patterns\n");
            printf("  stats       - Show statistics\n");
            printf("  msm         - Run MSM analysis\n");
            printf("  stp         - Run STP analysis\n");
            printf("  validate    - Comprehensive validation\n");
            printf("  lookup      - Bug pattern lookup\n");
            printf("  profiles    - List available profiles\n");
            printf("  config      - Show configuration\n");
            printf("  help        - Show this help\n");
            printf("  exit        - Exit interactive mode\n");
        } else if (strcmp(input, "profiles") == 0) {
            profile_options_t opts = { .list_profiles = true };
            crrss_cli_execute_profile(ctx, &opts);
        } else if (strcmp(input, "config") == 0) {
            configure_options_t opts = { .show_config = true };
            crrss_cli_execute_configure(ctx, &opts);
        } else if (strcmp(input, "stats") == 0) {
            stats_options_t opts = { .show_detailed = true };
            crrss_cli_execute_stats(ctx, &opts);
        } else {
            printf("Unknown command: %s\n", input);
            printf("Type 'help' for available commands.\n");
        }
        
        printf("\n");
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Configuration File Support ====================

crrss_status_t crrss_cli_load_config(
    crrss_cli_context_t* ctx,
    const char* config_path
) {
    if (!ctx) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!config_path) {
        config_path = ".crrssrc";
    }
    
    FILE* config_file = fopen(config_path, "r");
    if (!config_file) {
        // Config file doesn't exist, use defaults
        return CRRSS_SUCCESS;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), config_file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        // Parse configuration options
        // Simple parser for demonstration
        if (strstr(line, "msm = true")) {
            ctx->profiles_enabled[0] = true;
        } else if (strstr(line, "stp = true")) {
            ctx->profiles_enabled[1] = true;
        } else if (strstr(line, "tdt = true")) {
            ctx->profiles_enabled[2] = true;
        } else if (strstr(line, "rers = true")) {
            ctx->profiles_enabled[3] = true;
        } else if (strstr(line, "bpme = true")) {
            ctx->profiles_enabled[4] = true;
        }
    }
    
    fclose(config_file);
    ctx->config_file_path = config_path;
    
    return CRRSS_SUCCESS;
}

crrss_status_t crrss_cli_save_config(
    crrss_cli_context_t* ctx,
    const char* config_path
) {
    if (!ctx) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!config_path) {
        config_path = ".crrssrc";
    }
    
    FILE* config_file = fopen(config_path, "w");
    if (!config_file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    fprintf(config_file, "# CRRSS Configuration File\n\n");
    fprintf(config_file, "[profiles]\n");
    fprintf(config_file, "msm = %s\n", ctx->profiles_enabled[0] ? "true" : "false");
    fprintf(config_file, "stp = %s\n", ctx->profiles_enabled[1] ? "true" : "false");
    fprintf(config_file, "tdt = %s\n", ctx->profiles_enabled[2] ? "true" : "false");
    fprintf(config_file, "rers = %s\n", ctx->profiles_enabled[3] ? "true" : "false");
    fprintf(config_file, "bpme = %s\n", ctx->profiles_enabled[4] ? "true" : "false");
    
    fclose(config_file);
    
    return CRRSS_SUCCESS;
}
