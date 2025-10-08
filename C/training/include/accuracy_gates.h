
#ifndef ACCURACY_GATES_H
#define ACCURACY_GATES_H

#include <stdint.h>
#include <stdbool.h>
#include "curriculum.h"
#include "progress.h"

// Accuracy gate configuration
typedef struct {
    double required_accuracy;
    uint64_t minimum_samples;
    uint32_t consistency_sessions;
    bool allow_regression;
    bool adaptive_threshold;
} accuracy_gate_config_t;

// Accuracy gate
typedef struct accuracy_gate accuracy_gate_t;

// Create/destroy accuracy gate
accuracy_gate_t *accuracy_gate_create(curriculum_phase_t phase);
void accuracy_gate_destroy(accuracy_gate_t *gate);

// Configure gate
void accuracy_gate_set_config(accuracy_gate_t *gate, const accuracy_gate_config_t *config);
void accuracy_gate_get_config(const accuracy_gate_t *gate, accuracy_gate_config_t *config);

// Check if gate is passed
bool accuracy_gate_check(const accuracy_gate_t *gate, const progress_tracker_t *tracker, curriculum_phase_t phase);

// Get gate requirements
double accuracy_gate_get_required_accuracy(curriculum_phase_t phase);
uint64_t accuracy_gate_get_minimum_samples(curriculum_phase_t phase);

// Check specific conditions
bool accuracy_gate_check_accuracy(const accuracy_gate_t *gate, double accuracy);
bool accuracy_gate_check_sample_size(const accuracy_gate_t *gate, uint64_t samples);
bool accuracy_gate_check_consistency(const accuracy_gate_t *gate, const progress_tracker_t *tracker, curriculum_phase_t phase);
bool accuracy_gate_check_regression(const accuracy_gate_t *gate, const progress_tracker_t *tracker, curriculum_phase_t phase);

// Adaptive thresholds
void accuracy_gate_adjust_threshold(accuracy_gate_t *gate, double difficulty_factor);
double accuracy_gate_get_adaptive_threshold(const accuracy_gate_t *gate);

#endif // ACCURACY_GATES_H
