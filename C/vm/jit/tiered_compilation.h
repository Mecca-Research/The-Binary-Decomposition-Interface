
/**
 * @file tiered_compilation.h
 * @brief Tiered Compilation Manager
 * @details This file provides the tiered compilation functionality for just-in-time compilation and optimization.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef TIERED_COMPILATION_H
#define TIERED_COMPILATION_H

#include <stdint.h>
#include <stdbool.h>
#include "jit_compiler.h"
#include "hot_path.h"

// Tiered compilation policy
typedef enum {
    TIER_POLICY_AGGRESSIVE = 0,  // Compile quickly, optimize aggressively
    TIER_POLICY_BALANCED = 1,    // Balance compilation time and optimization
    TIER_POLICY_CONSERVATIVE = 2 // Optimize only proven hot paths
} TierPolicy;

// Compilation decision
typedef struct {
    uint32_t function_id;
    JITTier current_tier;
    JITTier target_tier;
    bool should_compile;
    bool should_deoptimize;
    uint64_t execution_count;
    double cost_benefit_ratio;
} CompilationDecision;

// Tiered compilation manager
typedef struct {
    JITCompiler* jit_compiler;
    HotPathDetector* hot_path_detector;
    
    TierPolicy policy;
    
    // Tier transition thresholds
    uint64_t interpreter_to_baseline;
    uint64_t baseline_to_optimized;
    
    // Deoptimization thresholds
    uint64_t deopt_threshold;
    
    // Statistics
    uint64_t tier_transitions;
    uint64_t deoptimizations;
    uint64_t compilation_decisions;
} TieredCompilationManager;

// Tiered compilation API
TieredCompilationManager* tiered_compilation_create(
    JITCompiler* jit_compiler,
    HotPathDetector* hot_path_detector
);

void tiered_compilation_destroy(TieredCompilationManager* manager);

bool tiered_compilation_make_decision(
    TieredCompilationManager* manager,
    uint32_t function_id,
    uint64_t execution_count,
    uint64_t execution_time_ns,
    CompilationDecision* decision
);

bool tiered_compilation_execute_decision(
    TieredCompilationManager* manager,
    const CompilationDecision* decision
);

// Policy configuration
void tiered_compilation_set_policy(TieredCompilationManager* manager, TierPolicy policy);
void tiered_compilation_set_thresholds(
    TieredCompilationManager* manager,
    uint64_t interpreter_to_baseline,
    uint64_t baseline_to_optimized
);

// Statistics
void tiered_compilation_get_stats(
    const TieredCompilationManager* manager,
    uint64_t* tier_transitions,
    uint64_t* deoptimizations,
    uint64_t* compilation_decisions
);

void tiered_compilation_reset_stats(TieredCompilationManager* manager);

// Cost-benefit analysis
double tiered_compilation_calculate_benefit(
    const TieredCompilationManager* manager,
    uint32_t function_id,
    JITTier current_tier,
    JITTier target_tier,
    uint64_t execution_count
);

#endif // TIERED_COMPILATION_H
