
#ifndef BDI_OPTIMIZER_SELECTOR_H
#define BDI_OPTIMIZER_SELECTOR_H

#include "../Profiling/profile_data.h"
#include "../ModelFormat/bdi_model.h"
#include <stdbool.h>

// Optimization strategies
typedef enum {
    OPT_STRATEGY_AGGRESSIVE,
    OPT_STRATEGY_BALANCED,
    OPT_STRATEGY_CONSERVATIVE,
    OPT_STRATEGY_SIZE,
    OPT_STRATEGY_SPEED,
    OPT_STRATEGY_ML_GUIDED
} OptimizationStrategy;

// Optimization flags
typedef struct {
    bool enable_inlining;
    bool enable_loop_unrolling;
    bool enable_vectorization;
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_register_allocation_ml;
    bool enable_instruction_scheduling;
    int inline_threshold;
    int unroll_factor;
} OptimizationFlags;

// Select optimization strategy based on profile
OptimizationStrategy optimizer_selector_select_strategy(const ProfileData *profile);

// Get optimization flags for strategy
OptimizationFlags optimizer_selector_get_flags(OptimizationStrategy strategy);

// ML-based optimization selection
OptimizationFlags optimizer_selector_ml_select(const ProfileData *profile, const BDIModel *model);

// Predict optimization benefit
double optimizer_selector_predict_benefit(const ProfileData *profile, 
                                         const OptimizationFlags *flags);

#endif // BDI_OPTIMIZER_SELECTOR_H
