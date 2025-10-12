/**
 * @file tdt.c
 * @brief Test-Driven Timmy Profile - Core implementation
 * 
 * Phase 2 Stage 2 Implementation
 */

#define _POSIX_C_SOURCE 199309L

#include "tdt.h"
#include "tdt_generator.h"
#include "tdt_coverage.h"
#include "tdt_templates.h"
#include "tdt_integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ==================== TDT Context Structure ====================

struct tdt_context {
    tdt_config_t config;
    bool initialized;
    
    // Statistics
    tdt_statistics_t stats;
    
    // Integration contexts
    void* msm_ctx;
    void* stp_ctx;
    void* bpme_ctx;
    
    // Analysis state
    tdt_generation_result_t last_generation_result;
    tdt_file_coverage_t* coverage_cache;
    uint32_t coverage_cache_count;
    uint32_t coverage_cache_capacity;
    
    // Timing
    struct timespec start_time;
};

// ==================== String Conversion Functions ====================

const char* tdt_strategy_to_string(tdt_generation_strategy_t strategy) {
    switch (strategy) {
        case TDT_STRATEGY_PATTERN_BASED: return "Pattern-Based";
        case TDT_STRATEGY_COVERAGE_DRIVEN: return "Coverage-Driven";
        case TDT_STRATEGY_SPECIFICATION_BASED: return "Specification-Based";
        case TDT_STRATEGY_ALL: return "All Strategies";
        default: return "Unknown";
    }
}

const char* tdt_framework_to_string(tdt_framework_t framework) {
    switch (framework) {
        case TDT_FRAMEWORK_CUSTOM: return "Custom CRRSS";
        case TDT_FRAMEWORK_UNITY: return "Unity";
        case TDT_FRAMEWORK_CHECK: return "Check";
        case TDT_FRAMEWORK_ALL: return "All Frameworks";
        default: return "Unknown";
    }
}

const char* tdt_test_type_to_string(tdt_test_type_t test_type) {
    switch (test_type) {
        case TDT_TEST_TYPE_UNIT: return "Unit Test";
        case TDT_TEST_TYPE_INTEGRATION: return "Integration Test";
        case TDT_TEST_TYPE_BOTH: return "Unit & Integration Tests";
        default: return "Unknown";
    }
}

// ==================== Initialization & Shutdown ====================

tdt_context_t* tdt_init(const tdt_config_t* config) {
    if (!config) {
        fprintf(stderr, "[TDT] Error: NULL configuration\n");
        return NULL;
    }
    
    tdt_context_t* ctx = calloc(1, sizeof(tdt_context_t));
    if (!ctx) {
        fprintf(stderr, "[TDT] Error: Failed to allocate context\n");
        return NULL;
    }
    
    // Copy configuration
    memcpy(&ctx->config, config, sizeof(tdt_config_t));
    
    // Initialize statistics
    memset(&ctx->stats, 0, sizeof(tdt_statistics_t));
    
    // Allocate coverage cache
    ctx->coverage_cache_capacity = 100;
    ctx->coverage_cache = calloc(ctx->coverage_cache_capacity, 
                                  sizeof(tdt_file_coverage_t));
    if (!ctx->coverage_cache) {
        fprintf(stderr, "[TDT] Warning: Failed to allocate coverage cache\n");
        ctx->coverage_cache_capacity = 0;
    }
    
    // Mark as initialized
    ctx->initialized = true;
    clock_gettime(CLOCK_MONOTONIC, &ctx->start_time);
    
    printf("[TDT] Initialized with strategy: %s, framework: %s\n",
           tdt_strategy_to_string(config->strategy),
           tdt_framework_to_string(config->framework));
    
    return ctx;
}

void tdt_cleanup(tdt_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Free coverage cache
    if (ctx->coverage_cache) {
        for (uint32_t i = 0; i < ctx->coverage_cache_count; i++) {
            // Free individual coverage data
            if (ctx->coverage_cache[i].line_coverage) {
                free(ctx->coverage_cache[i].line_coverage);
            }
            if (ctx->coverage_cache[i].branch_coverage) {
                free(ctx->coverage_cache[i].branch_coverage);
            }
            if (ctx->coverage_cache[i].function_coverage) {
                free(ctx->coverage_cache[i].function_coverage);
            }
        }
        free(ctx->coverage_cache);
    }
    
    // Free generation result data
    if (ctx->last_generation_result.test_cases) {
        // Free dynamically allocated test names before freeing test_cases array
        for (uint32_t i = 0; i < ctx->last_generation_result.test_case_count; i++) {
            if (ctx->last_generation_result.test_cases[i].test_name) {
                free((void*)ctx->last_generation_result.test_cases[i].test_name);
            }
        }
        free(ctx->last_generation_result.test_cases);
    }
    if (ctx->last_generation_result.generated_test_files) {
        free(ctx->last_generation_result.generated_test_files);
    }
    
    printf("[TDT] Cleanup complete. Tests generated: %u\n",
           ctx->stats.total_tests_generated);
    
    free(ctx);
}

crrss_status_t tdt_reset(tdt_context_t* ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    // Reset statistics
    memset(&ctx->stats, 0, sizeof(tdt_statistics_t));
    
    // Clear coverage cache
    for (uint32_t i = 0; i < ctx->coverage_cache_count; i++) {
        if (ctx->coverage_cache[i].line_coverage) {
            free(ctx->coverage_cache[i].line_coverage);
            ctx->coverage_cache[i].line_coverage = NULL;
        }
        if (ctx->coverage_cache[i].branch_coverage) {
            free(ctx->coverage_cache[i].branch_coverage);
            ctx->coverage_cache[i].branch_coverage = NULL;
        }
        if (ctx->coverage_cache[i].function_coverage) {
            free(ctx->coverage_cache[i].function_coverage);
            ctx->coverage_cache[i].function_coverage = NULL;
        }
    }
    ctx->coverage_cache_count = 0;
    
    // Reset timing
    clock_gettime(CLOCK_MONOTONIC, &ctx->start_time);
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_configure(tdt_context_t* ctx, const tdt_config_t* config) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!config) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Update configuration
    memcpy(&ctx->config, config, sizeof(tdt_config_t));
    
    return CRRSS_SUCCESS;
}

// ==================== Test Generation ====================

crrss_status_t tdt_analyze_file(tdt_context_t* ctx, const char* file_path) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Check if file exists
    FILE* file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "[TDT] Error: Cannot open file: %s\n", file_path);
        return CRRSS_ERROR_FILE_ACCESS;
    }
    fclose(file);
    
    ctx->stats.files_analyzed++;
    
    if (ctx->config.verbose_output) {
        printf("[TDT] Analyzing file: %s\n", file_path);
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_generate_tests(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    struct timespec gen_start;
    clock_gettime(CLOCK_MONOTONIC, &gen_start);
    
    // Initialize result
    memset(result, 0, sizeof(tdt_generation_result_t));
    
    // Analyze file first
    crrss_status_t status = tdt_analyze_file(ctx, file_path);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Generate tests based on strategy
    switch (ctx->config.strategy) {
        case TDT_STRATEGY_PATTERN_BASED:
            status = tdt_generate_pattern_based_tests(ctx, file_path, result);
            break;
        case TDT_STRATEGY_COVERAGE_DRIVEN:
            status = tdt_generate_coverage_driven_tests(ctx, file_path, result);
            break;
        case TDT_STRATEGY_SPECIFICATION_BASED:
            status = tdt_generate_specification_based_tests(ctx, file_path, result);
            break;
        case TDT_STRATEGY_ALL:
            // Use all strategies
            status = tdt_generate_pattern_based_tests(ctx, file_path, result);
            if (status == CRRSS_SUCCESS) {
                tdt_generation_result_t coverage_result;
                if (tdt_generate_coverage_driven_tests(ctx, file_path, &coverage_result) == CRRSS_SUCCESS) {
                    result->tests_generated += coverage_result.tests_generated;
                }
            }
            break;
        default:
            return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (status == CRRSS_SUCCESS) {
        result->success = true;
        ctx->stats.total_tests_generated += result->tests_generated;
        
        // Calculate generation time
        struct timespec gen_end;
        clock_gettime(CLOCK_MONOTONIC, &gen_end);
        double gen_time = (gen_end.tv_sec - gen_start.tv_sec) +
                         (gen_end.tv_nsec - gen_start.tv_nsec) / 1e9;
        ctx->stats.generation_time_seconds += gen_time;
        
        if (ctx->config.verbose_output) {
            printf("[TDT] Generated %u tests for %s in %.3f seconds\n",
                   result->tests_generated, file_path, gen_time);
        }
    }
    
    return status;
}

crrss_status_t tdt_generate_function_tests(
    tdt_context_t* ctx,
    const char* file_path,
    const char* function_name,
    tdt_generation_result_t* result)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !function_name || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Analyze function
    tdt_function_analysis_t analysis;
    crrss_status_t status = tdt_analyze_function(ctx, file_path, 
                                                   function_name, &analysis);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Initialize result
    memset(result, 0, sizeof(tdt_generation_result_t));
    
    // Allocate test cases array
    uint32_t max_tests = analysis.recommended_test_count;
    if (max_tests == 0) {
        max_tests = 10; // Default
    }
    
    result->test_cases = calloc(max_tests, sizeof(tdt_test_case_t));
    if (!result->test_cases) {
        return CRRSS_ERROR_MEMORY_ALLOCATION;
    }
    result->max_test_cases = max_tests;
    
    // Generate unit test
    tdt_test_case_t test_case;
    status = tdt_generate_function_unit_test(ctx, &analysis, &test_case);
    if (status == CRRSS_SUCCESS) {
        result->test_cases[result->test_case_count++] = test_case;
        result->tests_generated++;
        result->unit_tests_generated++;
    }
    
    // Generate edge case tests
    if (ctx->config.generate_edge_case_tests) {
        uint32_t num_edge_tests = 0;
        status = tdt_generate_edge_case_tests(ctx, &analysis,
                                               &result->test_cases[result->test_case_count],
                                               max_tests - result->test_case_count,
                                               &num_edge_tests);
        if (status == CRRSS_SUCCESS) {
            result->test_case_count += num_edge_tests;
            result->tests_generated += num_edge_tests;
            result->edge_case_tests_generated += num_edge_tests;
        }
    }
    
    // Generate error handling tests
    if (ctx->config.generate_error_handling_tests && analysis.has_error_handling) {
        uint32_t num_error_tests = 0;
        status = tdt_generate_error_handling_tests(ctx, &analysis,
                                                     &result->test_cases[result->test_case_count],
                                                     max_tests - result->test_case_count,
                                                     &num_error_tests);
        if (status == CRRSS_SUCCESS) {
            result->test_case_count += num_error_tests;
            result->tests_generated += num_error_tests;
            result->error_handling_tests_generated += num_error_tests;
        }
    }
    
    result->success = true;
    ctx->stats.total_tests_generated += result->tests_generated;
    ctx->stats.functions_analyzed++;
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_generate_directory_tests(
    tdt_context_t* ctx,
    const char* dir_path,
    tdt_generation_result_t* result)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!dir_path || !result) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Initialize result
    memset(result, 0, sizeof(tdt_generation_result_t));
    result->success = true;
    
    // TODO: Implement directory traversal and test generation
    // For now, return success with zero tests generated
    
    if (ctx->config.verbose_output) {
        printf("[TDT] Directory test generation for: %s\n", dir_path);
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Coverage Analysis ====================

crrss_status_t tdt_analyze_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_file_coverage_t* coverage)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !coverage) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    return tdt_coverage_analyze_file(ctx, file_path, coverage);
}

crrss_status_t tdt_calculate_line_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !coverage_percent) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    return tdt_coverage_calculate_line(ctx, file_path, coverage_percent);
}

crrss_status_t tdt_calculate_branch_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !coverage_percent) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    return tdt_coverage_calculate_branch(ctx, file_path, coverage_percent);
}

crrss_status_t tdt_calculate_function_coverage(
    tdt_context_t* ctx,
    const char* file_path,
    double* coverage_percent)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !coverage_percent) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    return tdt_coverage_calculate_function(ctx, file_path, coverage_percent);
}

crrss_status_t tdt_identify_coverage_gaps(
    tdt_context_t* ctx,
    const char* file_path,
    tdt_coverage_gap_t* gaps,
    uint32_t max_gaps,
    uint32_t* num_gaps)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!file_path || !gaps || !num_gaps) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    return tdt_coverage_identify_gaps(ctx, file_path, gaps, max_gaps, num_gaps);
}

// ==================== Reporting ====================

crrss_status_t tdt_generate_report(tdt_context_t* ctx, tdt_report_t* report) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!report) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Initialize report
    memset(report, 0, sizeof(tdt_report_t));
    
    // Copy statistics
    memcpy(&report->statistics, &ctx->stats, sizeof(tdt_statistics_t));
    
    // Calculate overall scores
    report->overall_test_quality_score = 0.75; // Placeholder
    report->overall_coverage_score = ctx->stats.average_line_coverage / 100.0;
    
    report->summary = "TDT analysis complete";
    report->recommendations = "Continue generating tests to improve coverage";
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_export_report(
    tdt_context_t* ctx,
    const tdt_report_t* report,
    const char* output_path,
    const char* format)
{
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!report || !output_path || !format) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(output_path, "w");
    if (!file) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Export as text format
    fprintf(file, "=== TDT Test Generation Report ===\n\n");
    fprintf(file, "Statistics:\n");
    fprintf(file, "  Tests Generated: %u\n", report->statistics.total_tests_generated);
    fprintf(file, "  Unit Tests: %u\n", report->statistics.unit_tests_generated);
    fprintf(file, "  Integration Tests: %u\n", report->statistics.integration_tests_generated);
    fprintf(file, "  Files Analyzed: %u\n", report->statistics.files_analyzed);
    fprintf(file, "  Functions Analyzed: %u\n", report->statistics.functions_analyzed);
    fprintf(file, "\nCoverage:\n");
    fprintf(file, "  Average Line Coverage: %.2f%%\n", report->statistics.average_line_coverage);
    fprintf(file, "  Average Branch Coverage: %.2f%%\n", report->statistics.average_branch_coverage);
    fprintf(file, "  Average Function Coverage: %.2f%%\n", report->statistics.average_function_coverage);
    fprintf(file, "\nQuality Metrics:\n");
    fprintf(file, "  Coverage Gaps: %u\n", report->statistics.coverage_gaps_identified);
    fprintf(file, "  Potential Bugs: %u\n", report->statistics.potential_bugs_found);
    fprintf(file, "\nOverall Assessment:\n");
    fprintf(file, "  Test Quality Score: %.2f\n", report->overall_test_quality_score);
    fprintf(file, "  Coverage Score: %.2f\n", report->overall_coverage_score);
    fprintf(file, "\nSummary: %s\n", report->summary);
    fprintf(file, "Recommendations: %s\n", report->recommendations);
    
    fclose(file);
    
    if (ctx->config.verbose_output) {
        printf("[TDT] Report exported to: %s\n", output_path);
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_get_statistics(tdt_context_t* ctx, tdt_statistics_t* stats) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    if (!stats) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &ctx->stats, sizeof(tdt_statistics_t));
    
    return CRRSS_SUCCESS;
}

// ==================== CRRSS Integration ====================

crrss_status_t tdt_integrate_msm(tdt_context_t* ctx, void* msm_ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->msm_ctx = msm_ctx;
    ctx->config.integrate_with_msm = true;
    
    if (ctx->config.verbose_output) {
        printf("[TDT] MSM integration enabled\n");
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_integrate_stp(tdt_context_t* ctx, void* stp_ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->stp_ctx = stp_ctx;
    ctx->config.integrate_with_stp = true;
    
    if (ctx->config.verbose_output) {
        printf("[TDT] STP integration enabled\n");
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t tdt_integrate_bpme(tdt_context_t* ctx, void* bpme_ctx) {
    if (!ctx || !ctx->initialized) {
        return CRRSS_ERROR_NOT_INITIALIZED;
    }
    
    ctx->bpme_ctx = bpme_ctx;
    ctx->config.integrate_with_bpme = true;
    
    if (ctx->config.verbose_output) {
        printf("[TDT] BPME integration enabled\n");
    }
    
    return CRRSS_SUCCESS;
}
