
#include "../include/accuracy_gates.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct accuracy_gate {
    curriculum_phase_t phase;
    accuracy_gate_config_t config;
};

// Default gate configurations for each phase
static const accuracy_gate_config_t default_configs[PHASE_MAX] = {
    {0.90, 100, 3, false, false},  // Phase 0 -> 1
    {0.92, 150, 3, false, false},  // Phase 1 -> 2
    {0.94, 200, 4, false, false},  // Phase 2 -> 3
    {0.95, 250, 4, false, false},  // Phase 3 -> 4
    {0.96, 300, 5, false, false},  // Phase 4 -> 5
    {0.97, 350, 5, false, false},  // Phase 5 -> 6
    {0.98, 400, 6, false, false},  // Phase 6 -> 7
    {0.99, 500, 6, false, false},  // Phase 7 -> ∞
};

accuracy_gate_t *accuracy_gate_create(curriculum_phase_t phase) {
    if (phase >= PHASE_MAX) return NULL;
    
    accuracy_gate_t *gate = malloc(sizeof(accuracy_gate_t));
    if (!gate) return NULL;
    
    gate->phase = phase;
    gate->config = default_configs[phase];
    
    return gate;
}

void accuracy_gate_destroy(accuracy_gate_t *gate) {
    if (!gate) return;
    free(gate);
}

void accuracy_gate_set_config(accuracy_gate_t *gate, const accuracy_gate_config_t *config) {
    if (!gate || !config) return;
    gate->config = *config;
}

void accuracy_gate_get_config(const accuracy_gate_t *gate, accuracy_gate_config_t *config) {
    if (!gate || !config) return;
    *config = gate->config;
}

bool accuracy_gate_check(const accuracy_gate_t *gate, const progress_tracker_t *tracker, 
                        curriculum_phase_t phase) {
    if (!gate || !tracker || phase >= PHASE_MAX) return false;
    
    // Get phase metrics
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(tracker, phase, &metrics);
    
    // Check all conditions
    if (!accuracy_gate_check_sample_size(gate, metrics.total_attempts)) {
        return false;
    }
    
    if (!accuracy_gate_check_accuracy(gate, metrics.accuracy)) {
        return false;
    }
    
    if (!accuracy_gate_check_consistency(gate, tracker, phase)) {
        return false;
    }
    
    if (!accuracy_gate_check_regression(gate, tracker, phase)) {
        return false;
    }
    
    return true;
}

double accuracy_gate_get_required_accuracy(curriculum_phase_t phase) {
    if (phase >= PHASE_MAX) return 1.0;
    return default_configs[phase].required_accuracy;
}

uint64_t accuracy_gate_get_minimum_samples(curriculum_phase_t phase) {
    if (phase >= PHASE_MAX) return 0;
    return default_configs[phase].minimum_samples;
}

bool accuracy_gate_check_accuracy(const accuracy_gate_t *gate, double accuracy) {
    if (!gate) return false;
    
    double threshold = gate->config.required_accuracy;
    if (gate->config.adaptive_threshold) {
        threshold = accuracy_gate_get_adaptive_threshold(gate);
    }
    
    return accuracy >= threshold;
}

bool accuracy_gate_check_sample_size(const accuracy_gate_t *gate, uint64_t samples) {
    if (!gate) return false;
    return samples >= gate->config.minimum_samples;
}

bool accuracy_gate_check_consistency(const accuracy_gate_t *gate, const progress_tracker_t *tracker, 
                                    curriculum_phase_t phase) {
    if (!gate || !tracker) return false;
    
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(tracker, phase, &metrics);
    
    // Check if we have enough sessions
    if (metrics.num_sessions < gate->config.consistency_sessions) {
        return false;
    }
    
    // For now, assume consistency if we have enough sessions
    // A more sophisticated implementation would track per-session accuracy
    return true;
}

bool accuracy_gate_check_regression(const accuracy_gate_t *gate, const progress_tracker_t *tracker, 
                                   curriculum_phase_t phase) {
    if (!gate || !tracker) return false;
    
    if (gate->config.allow_regression) {
        return true;
    }
    
    // Check if recent performance has declined
    // For now, simplified implementation
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(tracker, phase, &metrics);
    
    return metrics.accuracy >= gate->config.required_accuracy * 0.95;
}

void accuracy_gate_adjust_threshold(accuracy_gate_t *gate, double difficulty_factor) {
    if (!gate) return;
    
    gate->config.required_accuracy *= difficulty_factor;
    
    // Clamp to reasonable range
    if (gate->config.required_accuracy > 0.99) {
        gate->config.required_accuracy = 0.99;
    }
    if (gate->config.required_accuracy < 0.50) {
        gate->config.required_accuracy = 0.50;
    }
}

double accuracy_gate_get_adaptive_threshold(const accuracy_gate_t *gate) {
    if (!gate) return 0.0;
    
    // For now, return the configured threshold
    // A more sophisticated implementation would adjust based on difficulty
    return gate->config.required_accuracy;
}
