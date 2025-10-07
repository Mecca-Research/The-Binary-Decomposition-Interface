
#include "profile_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint64_t function_id;
    char function_name[64];
    uint64_t enter_time;
    bool is_active;
} FunctionCallStack;

ProfileData* profile_data_analyze(const ProfileSession *session) {
    if (!session || !session->events) {
        return NULL;
    }

    ProfileData *data = calloc(1, sizeof(ProfileData));
    if (!data) {
        return NULL;
    }

    // Allocate function stats array (estimate max functions)
    data->function_stats = calloc(1000, sizeof(FunctionStats));
    if (!data->function_stats) {
        free(data);
        return NULL;
    }

    // Track function call stack for timing
    FunctionCallStack *call_stack = calloc(100, sizeof(FunctionCallStack));
    int stack_depth = 0;

    // Process events
    for (size_t i = 0; i < session->event_count; i++) {
        const ProfileEvent *event = &session->events[i];

        switch (event->type) {
            case PROFILE_EVENT_FUNCTION_ENTER:
                if (stack_depth < 100) {
                    call_stack[stack_depth].function_id = event->function_id;
                    strncpy(call_stack[stack_depth].function_name, event->name, 63);
                    call_stack[stack_depth].enter_time = event->timestamp_ns;
                    call_stack[stack_depth].is_active = true;
                    stack_depth++;
                }
                break;

            case PROFILE_EVENT_FUNCTION_EXIT:
                if (stack_depth > 0) {
                    stack_depth--;
                    FunctionCallStack *call = &call_stack[stack_depth];
                    
                    if (call->is_active && call->function_id == event->function_id) {
                        uint64_t duration = event->timestamp_ns - call->enter_time;
                        
                        // Find or create function stats
                        FunctionStats *stats = NULL;
                        for (size_t j = 0; j < data->function_count; j++) {
                            if (data->function_stats[j].function_id == event->function_id) {
                                stats = &data->function_stats[j];
                                break;
                            }
                        }
                        
                        if (!stats && data->function_count < 1000) {
                            stats = &data->function_stats[data->function_count++];
                            stats->function_id = event->function_id;
                            strncpy(stats->function_name, call->function_name, 63);
                            stats->min_time_ns = UINT64_MAX;
                        }
                        
                        if (stats) {
                            stats->call_count++;
                            stats->total_time_ns += duration;
                            if (duration < stats->min_time_ns) stats->min_time_ns = duration;
                            if (duration > stats->max_time_ns) stats->max_time_ns = duration;
                            stats->avg_time_ns = (double)stats->total_time_ns / stats->call_count;
                        }
                    }
                }
                break;

            case PROFILE_EVENT_MEMORY_ALLOC:
                data->memory_stats.total_allocations++;
                data->memory_stats.bytes_allocated += event->size;
                data->memory_stats.current_memory_usage += event->size;
                if (data->memory_stats.current_memory_usage > data->memory_stats.peak_memory_usage) {
                    data->memory_stats.peak_memory_usage = data->memory_stats.current_memory_usage;
                }
                break;

            case PROFILE_EVENT_MEMORY_FREE:
                data->memory_stats.total_frees++;
                break;

            case PROFILE_EVENT_CACHE_HIT:
                data->cache_stats.cache_hits++;
                break;

            case PROFILE_EVENT_CACHE_MISS:
                data->cache_stats.cache_misses++;
                break;

            case PROFILE_EVENT_BRANCH_TAKEN:
                data->branch_stats.branches_taken++;
                break;

            case PROFILE_EVENT_BRANCH_NOT_TAKEN:
                data->branch_stats.branches_not_taken++;
                break;

            default:
                break;
        }
    }

    // Calculate derived statistics
    uint64_t total_cache = data->cache_stats.cache_hits + data->cache_stats.cache_misses;
    if (total_cache > 0) {
        data->cache_stats.hit_rate = (double)data->cache_stats.cache_hits / total_cache;
    }

    uint64_t total_branches = data->branch_stats.branches_taken + data->branch_stats.branches_not_taken;
    if (total_branches > 0) {
        data->branch_stats.taken_rate = (double)data->branch_stats.branches_taken / total_branches;
    }

    data->total_execution_time_ns = session->end_time_ns - session->start_time_ns;

    free(call_stack);
    return data;
}

void profile_data_free(ProfileData *data) {
    if (!data) return;
    free(data->function_stats);
    free(data);
}

void profile_data_print_summary(const ProfileData *data) {
    if (!data) return;

    printf("\n=== Profile Summary ===\n");
    printf("Total Execution Time: %.3f ms\n", data->total_execution_time_ns / 1000000.0);
    
    printf("\n--- Memory Statistics ---\n");
    printf("Total Allocations: %lu\n", data->memory_stats.total_allocations);
    printf("Total Frees: %lu\n", data->memory_stats.total_frees);
    printf("Bytes Allocated: %lu\n", data->memory_stats.bytes_allocated);
    printf("Peak Memory Usage: %lu bytes\n", data->memory_stats.peak_memory_usage);
    
    printf("\n--- Cache Statistics ---\n");
    printf("Cache Hits: %lu\n", data->cache_stats.cache_hits);
    printf("Cache Misses: %lu\n", data->cache_stats.cache_misses);
    printf("Hit Rate: %.2f%%\n", data->cache_stats.hit_rate * 100.0);
    
    printf("\n--- Branch Statistics ---\n");
    printf("Branches Taken: %lu\n", data->branch_stats.branches_taken);
    printf("Branches Not Taken: %lu\n", data->branch_stats.branches_not_taken);
    printf("Taken Rate: %.2f%%\n", data->branch_stats.taken_rate * 100.0);
    
    printf("\n--- Top 10 Functions by Time ---\n");
    size_t count;
    FunctionStats *hottest = profile_data_get_hottest_functions(data, &count);
    for (size_t i = 0; i < count && i < 10; i++) {
        printf("%2zu. %s: %.3f ms (%lu calls, avg %.3f us)\n",
               i + 1,
               hottest[i].function_name,
               hottest[i].total_time_ns / 1000000.0,
               hottest[i].call_count,
               hottest[i].avg_time_ns / 1000.0);
    }
    free(hottest);
}

static int compare_function_stats(const void *a, const void *b) {
    const FunctionStats *fa = a;
    const FunctionStats *fb = b;
    if (fa->total_time_ns > fb->total_time_ns) return -1;
    if (fa->total_time_ns < fb->total_time_ns) return 1;
    return 0;
}

FunctionStats* profile_data_get_hottest_functions(const ProfileData *data, size_t *count) {
    if (!data || !count) return NULL;

    FunctionStats *sorted = malloc(data->function_count * sizeof(FunctionStats));
    if (!sorted) return NULL;

    memcpy(sorted, data->function_stats, data->function_count * sizeof(FunctionStats));
    qsort(sorted, data->function_count, sizeof(FunctionStats), compare_function_stats);

    *count = data->function_count;
    return sorted;
}
