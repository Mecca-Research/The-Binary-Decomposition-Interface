
#ifndef BDI_PROFILE_ANALYZER_H
#define BDI_PROFILE_ANALYZER_H

#include "profile_data.h"
#include <stdbool.h>

// Optimization recommendations
typedef enum {
    OPT_RECOMMEND_INLINE,
    OPT_RECOMMEND_LOOP_UNROLL,
    OPT_RECOMMEND_VECTORIZE,
    OPT_RECOMMEND_CACHE_OPTIMIZE,
    OPT_RECOMMEND_BRANCH_PREDICT,
    OPT_RECOMMEND_MEMORY_POOL,
    OPT_RECOMMEND_NONE
} OptimizationRecommendation;

typedef struct {
    OptimizationRecommendation type;
    uint64_t function_id;
    char function_name[64];
    double confidence;
    char reason[256];
} OptimizationSuggestion;

typedef struct {
    OptimizationSuggestion *suggestions;
    size_t suggestion_count;
} OptimizationReport;

// Analyze profile and generate optimization suggestions
OptimizationReport* profile_analyzer_generate_suggestions(const ProfileData *data);

// Free optimization report
void profile_analyzer_free_report(OptimizationReport *report);

// Print optimization report
void profile_analyzer_print_report(const OptimizationReport *report);

// Detect performance bottlenecks
bool profile_analyzer_is_bottleneck(const FunctionStats *stats, const ProfileData *data);

// Detect cache-unfriendly patterns
bool profile_analyzer_has_cache_issues(const ProfileData *data);

// Detect branch misprediction issues
bool profile_analyzer_has_branch_issues(const ProfileData *data);

#endif // BDI_PROFILE_ANALYZER_H
