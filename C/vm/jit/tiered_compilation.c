
#include "tiered_compilation.h"
#include <stdlib.h>
#include <string.h>

TieredCompilationManager* tiered_compilation_create(
    JITCompiler* jit_compiler,
    HotPathDetector* hot_path_detector
) {
    if (!jit_compiler || !hot_path_detector) return NULL;
    
    TieredCompilationManager* manager = (TieredCompilationManager*)calloc(1, sizeof(TieredCompilationManager));
    if (!manager) return NULL;
    
    manager->jit_compiler = jit_compiler;
    manager->hot_path_detector = hot_path_detector;
    manager->policy = TIER_POLICY_BALANCED;
    
    // Set default thresholds based on policy
    manager->interpreter_to_baseline = 100;
    manager->baseline_to_optimized = 1000;
    manager->deopt_threshold = 10000;
    
    return manager;
}

void tiered_compilation_destroy(TieredCompilationManager* manager) {
    if (!manager) return;
    free(manager);
}

bool tiered_compilation_make_decision(
    TieredCompilationManager* manager,
    uint32_t function_id,
    uint64_t execution_count,
    uint64_t execution_time_ns,
    CompilationDecision* decision
) {
    if (!manager || !decision) return false;
    
    decision->function_id = function_id;
    decision->execution_count = execution_count;
    decision->should_compile = false;
    decision->should_deoptimize = false;
    
    // Determine current tier (simplified - would query actual state)
    JITTier current_tier = JIT_TIER_INTERPRETER;
    if (execution_count >= manager->baseline_to_optimized) {
        current_tier = JIT_TIER_BASELINE;
    }
    
    decision->current_tier = current_tier;
    decision->target_tier = current_tier;
    
    // Check for tier upgrade
    if (current_tier == JIT_TIER_INTERPRETER && 
        execution_count >= manager->interpreter_to_baseline) {
        decision->target_tier = JIT_TIER_BASELINE;
        decision->should_compile = true;
    } else if (current_tier == JIT_TIER_BASELINE && 
               execution_count >= manager->baseline_to_optimized) {
        decision->target_tier = JIT_TIER_OPTIMIZED;
        decision->should_compile = true;
    }
    
    // Calculate cost-benefit ratio
    if (decision->should_compile) {
        decision->cost_benefit_ratio = tiered_compilation_calculate_benefit(
            manager, function_id, current_tier, decision->target_tier, execution_count
        );
        
        // Only compile if benefit is positive
        if (decision->cost_benefit_ratio < 1.0) {
            decision->should_compile = false;
        }
    }
    
    manager->compilation_decisions++;
    
    return true;
}

bool tiered_compilation_execute_decision(
    TieredCompilationManager* manager,
    const CompilationDecision* decision
) {
    if (!manager || !decision) return false;
    
    if (decision->should_compile) {
        // In production: Actually compile the function
        manager->tier_transitions++;
        return true;
    }
    
    if (decision->should_deoptimize) {
        // In production: Deoptimize the function
        manager->deoptimizations++;
        return true;
    }
    
    return false;
}

void tiered_compilation_set_policy(TieredCompilationManager* manager, TierPolicy policy) {
    if (!manager) return;
    
    manager->policy = policy;
    
    // Adjust thresholds based on policy
    switch (policy) {
        case TIER_POLICY_AGGRESSIVE:
            manager->interpreter_to_baseline = 50;
            manager->baseline_to_optimized = 500;
            break;
        case TIER_POLICY_BALANCED:
            manager->interpreter_to_baseline = 100;
            manager->baseline_to_optimized = 1000;
            break;
        case TIER_POLICY_CONSERVATIVE:
            manager->interpreter_to_baseline = 200;
            manager->baseline_to_optimized = 2000;
            break;
    }
}

void tiered_compilation_set_thresholds(
    TieredCompilationManager* manager,
    uint64_t interpreter_to_baseline,
    uint64_t baseline_to_optimized
) {
    if (!manager) return;
    
    manager->interpreter_to_baseline = interpreter_to_baseline;
    manager->baseline_to_optimized = baseline_to_optimized;
}

void tiered_compilation_get_stats(
    const TieredCompilationManager* manager,
    uint64_t* tier_transitions,
    uint64_t* deoptimizations,
    uint64_t* compilation_decisions
) {
    if (!manager) return;
    
    if (tier_transitions) *tier_transitions = manager->tier_transitions;
    if (deoptimizations) *deoptimizations = manager->deoptimizations;
    if (compilation_decisions) *compilation_decisions = manager->compilation_decisions;
}

void tiered_compilation_reset_stats(TieredCompilationManager* manager) {
    if (!manager) return;
    
    manager->tier_transitions = 0;
    manager->deoptimizations = 0;
    manager->compilation_decisions = 0;
}

double tiered_compilation_calculate_benefit(
    const TieredCompilationManager* manager,
    uint32_t function_id,
    JITTier current_tier,
    JITTier target_tier,
    uint64_t execution_count
) {
    if (!manager) return 0.0;
    
    // Simplified cost-benefit calculation
    // In production: Use actual profiling data
    
    double compilation_cost = 1000.0; // nanoseconds
    double speedup = 1.0;
    
    switch (target_tier) {
        case JIT_TIER_BASELINE:
            compilation_cost = 1000.0;
            speedup = 2.0;
            break;
        case JIT_TIER_OPTIMIZED:
            compilation_cost = 10000.0;
            speedup = 5.0;
            break;
        default:
            break;
    }
    
    double expected_savings = execution_count * 100.0 * (speedup - 1.0);
    return expected_savings / compilation_cost;
}
