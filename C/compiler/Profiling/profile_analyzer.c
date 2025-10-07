
#include "profile_analyzer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BOTTLENECK_THRESHOLD 0.05  // 5% of total time
#define CACHE_HIT_RATE_THRESHOLD 0.90  // 90% hit rate
#define BRANCH_TAKEN_RATE_THRESHOLD 0.80  // 80% taken rate

OptimizationReport* profile_analyzer_generate_suggestions(const ProfileData *data) {
    if (!data) {
        return NULL;
    }

    OptimizationReport *report = calloc(1, sizeof(OptimizationReport));
    if (!report) {
        return NULL;
    }

    // Allocate suggestions array
    report->suggestions = calloc(100, sizeof(OptimizationSuggestion));
    if (!report->suggestions) {
        free(report);
        return NULL;
    }

    size_t suggestion_idx = 0;

    // Analyze each function
    for (size_t i = 0; i < data->function_count && suggestion_idx < 100; i++) {
        const FunctionStats *stats = &data->function_stats[i];

        // Check if function is a bottleneck
        if (profile_analyzer_is_bottleneck(stats, data)) {
            // Small, frequently called functions -> inline
            if (stats->call_count > 1000 && stats->avg_time_ns < 1000) {
                OptimizationSuggestion *sugg = &report->suggestions[suggestion_idx++];
                sugg->type = OPT_RECOMMEND_INLINE;
                sugg->function_id = stats->function_id;
                strncpy(sugg->function_name, stats->function_name, 63);
                sugg->confidence = 0.85;
                snprintf(sugg->reason, 255, 
                        "Function called %lu times with avg time %.2f ns - good inline candidate",
                        stats->call_count, stats->avg_time_ns);
            }
            // Long-running functions -> optimize
            else if (stats->avg_time_ns > 100000) {
                OptimizationSuggestion *sugg = &report->suggestions[suggestion_idx++];
                sugg->type = OPT_RECOMMEND_LOOP_UNROLL;
                sugg->function_id = stats->function_id;
                strncpy(sugg->function_name, stats->function_name, 63);
                sugg->confidence = 0.70;
                snprintf(sugg->reason, 255,
                        "Function takes %.2f ms on average - consider loop unrolling",
                        stats->avg_time_ns / 1000000.0);
            }
        }
    }

    // Check cache performance
    if (profile_analyzer_has_cache_issues(data)) {
        OptimizationSuggestion *sugg = &report->suggestions[suggestion_idx++];
        sugg->type = OPT_RECOMMEND_CACHE_OPTIMIZE;
        sugg->function_id = 0;
        strcpy(sugg->function_name, "GLOBAL");
        sugg->confidence = 0.90;
        snprintf(sugg->reason, 255,
                "Cache hit rate is %.2f%% - consider data structure reorganization",
                data->cache_stats.hit_rate * 100.0);
    }

    // Check branch prediction
    if (profile_analyzer_has_branch_issues(data)) {
        OptimizationSuggestion *sugg = &report->suggestions[suggestion_idx++];
        sugg->type = OPT_RECOMMEND_BRANCH_PREDICT;
        sugg->function_id = 0;
        strcpy(sugg->function_name, "GLOBAL");
        sugg->confidence = 0.75;
        snprintf(sugg->reason, 255,
                "Branch taken rate is %.2f%% - consider branch prediction hints",
                data->branch_stats.taken_rate * 100.0);
    }

    // Check memory allocation patterns
    if (data->memory_stats.total_allocations > 10000) {
        OptimizationSuggestion *sugg = &report->suggestions[suggestion_idx++];
        sugg->type = OPT_RECOMMEND_MEMORY_POOL;
        sugg->function_id = 0;
        strcpy(sugg->function_name, "GLOBAL");
        sugg->confidence = 0.80;
        snprintf(sugg->reason, 255,
                "High allocation count (%lu) - consider memory pooling",
                data->memory_stats.total_allocations);
    }

    report->suggestion_count = suggestion_idx;
    return report;
}

void profile_analyzer_free_report(OptimizationReport *report) {
    if (!report) return;
    free(report->suggestions);
    free(report);
}

void profile_analyzer_print_report(const OptimizationReport *report) {
    if (!report) return;

    printf("\n=== Optimization Recommendations ===\n");
    printf("Total Suggestions: %zu\n\n", report->suggestion_count);

    for (size_t i = 0; i < report->suggestion_count; i++) {
        const OptimizationSuggestion *sugg = &report->suggestions[i];
        
        const char *type_str = "UNKNOWN";
        switch (sugg->type) {
            case OPT_RECOMMEND_INLINE: type_str = "INLINE"; break;
            case OPT_RECOMMEND_LOOP_UNROLL: type_str = "LOOP_UNROLL"; break;
            case OPT_RECOMMEND_VECTORIZE: type_str = "VECTORIZE"; break;
            case OPT_RECOMMEND_CACHE_OPTIMIZE: type_str = "CACHE_OPTIMIZE"; break;
            case OPT_RECOMMEND_BRANCH_PREDICT: type_str = "BRANCH_PREDICT"; break;
            case OPT_RECOMMEND_MEMORY_POOL: type_str = "MEMORY_POOL"; break;
            default: break;
        }

        printf("[%zu] %s (confidence: %.0f%%)\n", i + 1, type_str, sugg->confidence * 100.0);
        printf("    Function: %s\n", sugg->function_name);
        printf("    Reason: %s\n\n", sugg->reason);
    }
}

bool profile_analyzer_is_bottleneck(const FunctionStats *stats, const ProfileData *data) {
    if (!stats || !data || data->total_execution_time_ns == 0) {
        return false;
    }

    double time_percentage = (double)stats->total_time_ns / data->total_execution_time_ns;
    return time_percentage >= BOTTLENECK_THRESHOLD;
}

bool profile_analyzer_has_cache_issues(const ProfileData *data) {
    if (!data) return false;
    return data->cache_stats.hit_rate < CACHE_HIT_RATE_THRESHOLD;
}

bool profile_analyzer_has_branch_issues(const ProfileData *data) {
    if (!data) return false;
    
    // Branch prediction works best when branches are highly predictable
    // Either very high or very low taken rates are good
    double taken_rate = data->branch_stats.taken_rate;
    return (taken_rate > 0.20 && taken_rate < 0.80);
}
