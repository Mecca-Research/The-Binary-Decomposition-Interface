
#ifndef BDI_PROFILE_DATA_H
#define BDI_PROFILE_DATA_H

#include "profiler.h"
#include <stdint.h>

// Aggregated profile statistics
typedef struct {
    uint64_t function_id;
    char function_name[64];
    uint64_t call_count;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
} FunctionStats;

typedef struct {
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t bytes_allocated;
    uint64_t bytes_freed;
    uint64_t peak_memory_usage;
    uint64_t current_memory_usage;
} MemoryStats;

typedef struct {
    uint64_t cache_hits;
    uint64_t cache_misses;
    double hit_rate;
} CacheStats;

typedef struct {
    uint64_t branches_taken;
    uint64_t branches_not_taken;
    double taken_rate;
} BranchStats;

// Complete profile data
typedef struct {
    FunctionStats *function_stats;
    size_t function_count;
    MemoryStats memory_stats;
    CacheStats cache_stats;
    BranchStats branch_stats;
    uint64_t total_execution_time_ns;
} ProfileData;

// Analyze profile session and generate statistics
ProfileData* profile_data_analyze(const ProfileSession *session);

// Free profile data
void profile_data_free(ProfileData *data);

// Print profile data summary
void profile_data_print_summary(const ProfileData *data);

// Get hottest functions (most time spent)
FunctionStats* profile_data_get_hottest_functions(const ProfileData *data, size_t *count);

#endif // BDI_PROFILE_DATA_H
