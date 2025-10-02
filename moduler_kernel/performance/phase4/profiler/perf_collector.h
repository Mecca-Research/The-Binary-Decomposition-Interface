/**
 * @file perf_collector.h
 * @brief Performance counter collection using Linux perf
 */

#ifndef PHASE4_PERF_COLLECTOR_H
#define PHASE4_PERF_COLLECTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Performance counter types
typedef enum {
    PERF_COUNTER_CYCLES,
    PERF_COUNTER_INSTRUCTIONS,
    PERF_COUNTER_CACHE_REFERENCES,
    PERF_COUNTER_CACHE_MISSES,
    PERF_COUNTER_BRANCH_INSTRUCTIONS,
    PERF_COUNTER_BRANCH_MISSES,
    PERF_COUNTER_PAGE_FAULTS,
    PERF_COUNTER_CONTEXT_SWITCHES,
} perf_counter_type_t;

// Performance counter data
typedef struct {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_references;
    uint64_t cache_misses;
    uint64_t branch_instructions;
    uint64_t branch_misses;
    uint64_t page_faults;
    uint64_t context_switches;
    double ipc;  // Instructions per cycle
    double cache_miss_rate;
    double branch_miss_rate;
} perf_counters_t;

/**
 * @brief Initialize performance counter collection
 * @return 0 on success, negative on error
 */
int perf_collector_init(void);

/**
 * @brief Shutdown performance counter collection
 */
void perf_collector_shutdown(void);

/**
 * @brief Start collecting performance counters
 * @return 0 on success, negative on error
 */
int perf_collector_start(void);

/**
 * @brief Stop collecting and read performance counters
 * @param counters Output counter data
 * @return 0 on success, negative on error
 */
int perf_collector_stop(perf_counters_t* counters);

/**
 * @brief Read current performance counters (without stopping)
 * @param counters Output counter data
 * @return 0 on success, negative on error
 */
int perf_collector_read(perf_counters_t* counters);

#ifdef __cplusplus
}
#endif

#endif
