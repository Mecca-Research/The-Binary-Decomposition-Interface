/**
 * @file ai_optimizer.h
 * @brief AI-driven optimization recommendation engine
 */

#ifndef PHASE4_AI_OPTIMIZER_H
#define PHASE4_AI_OPTIMIZER_H

#include "perf_collector.h"

#ifdef __cplusplus
extern "C" {
#endif

// Optimization recommendation
typedef struct {
    char description[256];
    char location[128];
    double expected_improvement;
    int priority;
} optimization_recommendation_t;

/**
 * @brief Initialize AI optimizer
 * @return 0 on success, negative on error
 */
int ai_optimizer_init(void);

/**
 * @brief Shutdown AI optimizer
 */
void ai_optimizer_shutdown(void);

/**
 * @brief Analyze performance data and generate recommendations
 * @param counters Performance counter data
 * @param recommendations Output array of recommendations
 * @param max_recommendations Maximum recommendations to generate
 * @return Number of recommendations generated
 */
int ai_optimizer_analyze(const perf_counters_t* counters,
                        optimization_recommendation_t* recommendations,
                        size_t max_recommendations);

#ifdef __cplusplus
}
#endif

#endif
