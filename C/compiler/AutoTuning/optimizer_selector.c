
#include "optimizer_selector.h"
#include <stdlib.h>
#include <string.h>

OptimizationStrategy optimizer_selector_select_strategy(const ProfileData *profile) {
    if (!profile) {
        return OPT_STRATEGY_BALANCED;
    }

    // Analyze profile characteristics
    double cache_hit_rate = profile->cache_stats.hit_rate;
    uint64_t total_time = profile->total_execution_time_ns;
    size_t function_count = profile->function_count;

    // If cache performance is poor, focus on cache optimization
    if (cache_hit_rate < 0.80) {
        return OPT_STRATEGY_CONSERVATIVE;
    }

    // If many small functions, aggressive inlining
    if (function_count > 100) {
        return OPT_STRATEGY_AGGRESSIVE;
    }

    // If execution time is high, focus on speed
    if (total_time > 1000000000ULL) {  // > 1 second
        return OPT_STRATEGY_SPEED;
    }

    // Default to ML-guided optimization
    return OPT_STRATEGY_ML_GUIDED;
}

OptimizationFlags optimizer_selector_get_flags(OptimizationStrategy strategy) {
    OptimizationFlags flags = {0};

    switch (strategy) {
        case OPT_STRATEGY_AGGRESSIVE:
            flags.enable_inlining = true;
            flags.enable_loop_unrolling = true;
            flags.enable_vectorization = true;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = true;
            flags.enable_instruction_scheduling = true;
            flags.inline_threshold = 1000;
            flags.unroll_factor = 8;
            break;

        case OPT_STRATEGY_BALANCED:
            flags.enable_inlining = true;
            flags.enable_loop_unrolling = true;
            flags.enable_vectorization = true;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = false;
            flags.enable_instruction_scheduling = true;
            flags.inline_threshold = 500;
            flags.unroll_factor = 4;
            break;

        case OPT_STRATEGY_CONSERVATIVE:
            flags.enable_inlining = true;
            flags.enable_loop_unrolling = false;
            flags.enable_vectorization = false;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = false;
            flags.enable_instruction_scheduling = false;
            flags.inline_threshold = 200;
            flags.unroll_factor = 2;
            break;

        case OPT_STRATEGY_SIZE:
            flags.enable_inlining = false;
            flags.enable_loop_unrolling = false;
            flags.enable_vectorization = false;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = false;
            flags.enable_instruction_scheduling = false;
            flags.inline_threshold = 50;
            flags.unroll_factor = 1;
            break;

        case OPT_STRATEGY_SPEED:
            flags.enable_inlining = true;
            flags.enable_loop_unrolling = true;
            flags.enable_vectorization = true;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = true;
            flags.enable_instruction_scheduling = true;
            flags.inline_threshold = 2000;
            flags.unroll_factor = 16;
            break;

        case OPT_STRATEGY_ML_GUIDED:
            flags.enable_inlining = true;
            flags.enable_loop_unrolling = true;
            flags.enable_vectorization = true;
            flags.enable_constant_folding = true;
            flags.enable_dead_code_elimination = true;
            flags.enable_register_allocation_ml = true;
            flags.enable_instruction_scheduling = true;
            flags.inline_threshold = 750;
            flags.unroll_factor = 6;
            break;
    }

    return flags;
}

OptimizationFlags optimizer_selector_ml_select(const ProfileData *profile, const BDIModel *model) {
    // TODO: Implement ML-based selection using trained model
    // For now, fall back to heuristic selection
    OptimizationStrategy strategy = optimizer_selector_select_strategy(profile);
    return optimizer_selector_get_flags(strategy);
}

double optimizer_selector_predict_benefit(const ProfileData *profile, 
                                         const OptimizationFlags *flags) {
    if (!profile || !flags) {
        return 0.0;
    }

    double benefit = 0.0;

    // Estimate benefit from each optimization
    if (flags->enable_inlining && profile->function_count > 50) {
        benefit += 0.15;  // 15% improvement
    }

    if (flags->enable_loop_unrolling) {
        benefit += 0.10;  // 10% improvement
    }

    if (flags->enable_vectorization) {
        benefit += 0.20;  // 20% improvement
    }

    if (flags->enable_constant_folding) {
        benefit += 0.05;  // 5% improvement
    }

    if (flags->enable_dead_code_elimination) {
        benefit += 0.08;  // 8% improvement
    }

    if (flags->enable_register_allocation_ml) {
        benefit += 0.12;  // 12% improvement
    }

    if (flags->enable_instruction_scheduling) {
        benefit += 0.10;  // 10% improvement
    }

    // Cap total benefit at 60%
    if (benefit > 0.60) {
        benefit = 0.60;
    }

    return benefit;
}
