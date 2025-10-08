
#include "../include/curriculum.h"
#include "../include/progress.h"
#include "../include/accuracy_gates.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void test_curriculum_init() {
    printf("Testing curriculum initialization...\n");
    
    curriculum_controller_t *ctrl = curriculum_init("/tmp/training_data");
    assert(ctrl != NULL);
    
    curriculum_destroy(ctrl);
    printf("✓ Curriculum initialization test passed\n");
}

void test_process_registration() {
    printf("Testing process registration...\n");
    
    curriculum_controller_t *ctrl = curriculum_init("/tmp/training_data");
    assert(ctrl != NULL);
    
    curriculum_process_t *process = curriculum_register_process(ctrl, "test_process_1");
    assert(process != NULL);
    
    curriculum_phase_t phase = curriculum_get_current_phase(process);
    assert(phase == PHASE_0_FOUNDATIONS);
    
    curriculum_destroy(ctrl);
    printf("✓ Process registration test passed\n");
}

void test_training_session() {
    printf("Testing training session...\n");
    
    curriculum_controller_t *ctrl = curriculum_init("/tmp/training_data");
    curriculum_process_t *process = curriculum_register_process(ctrl, "test_process_2");
    
    int result = curriculum_start_session(process);
    assert(result == 0);
    
    void *input = NULL, *output = NULL;
    size_t input_size = 0, output_size = 0;
    
    result = curriculum_get_next_example(process, &input, &input_size, &output, &output_size);
    assert(result == 0);
    assert(input != NULL);
    assert(output != NULL);
    
    bool correct = false;
    result = curriculum_submit_answer(process, output, output_size, &correct);
    assert(result == 0);
    
    free(input);
    free(output);
    
    result = curriculum_end_session(process);
    assert(result == 0);
    
    curriculum_destroy(ctrl);
    printf("✓ Training session test passed\n");
}

void test_progress_tracking() {
    printf("Testing progress tracking...\n");
    
    progress_tracker_t *tracker = progress_tracker_create("test_tracker");
    assert(tracker != NULL);
    
    // Record some attempts
    for (int i = 0; i < 100; i++) {
        bool correct = (i % 5) != 0; // 80% correct
        progress_tracker_record_attempt(tracker, PHASE_0_FOUNDATIONS, TOPIC_MATH, correct, 1);
    }
    
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(tracker, PHASE_0_FOUNDATIONS, &metrics);
    
    assert(metrics.total_attempts == 100);
    assert(metrics.correct_answers == 80);
    assert(metrics.accuracy >= 0.79 && metrics.accuracy <= 0.81);
    
    progress_tracker_destroy(tracker);
    printf("✓ Progress tracking test passed\n");
}

void test_accuracy_gates() {
    printf("Testing accuracy gates...\n");
    
    accuracy_gate_t *gate = accuracy_gate_create(PHASE_0_FOUNDATIONS);
    assert(gate != NULL);
    
    double required = accuracy_gate_get_required_accuracy(PHASE_0_FOUNDATIONS);
    assert(required == 0.90);
    
    uint64_t min_samples = accuracy_gate_get_minimum_samples(PHASE_0_FOUNDATIONS);
    assert(min_samples == 100);
    
    assert(accuracy_gate_check_accuracy(gate, 0.95) == true);
    assert(accuracy_gate_check_accuracy(gate, 0.85) == false);
    
    assert(accuracy_gate_check_sample_size(gate, 150) == true);
    assert(accuracy_gate_check_sample_size(gate, 50) == false);
    
    accuracy_gate_destroy(gate);
    printf("✓ Accuracy gates test passed\n");
}

void test_phase_advancement() {
    printf("Testing phase advancement...\n");
    
    curriculum_controller_t *ctrl = curriculum_init("/tmp/training_data");
    curriculum_process_t *process = curriculum_register_process(ctrl, "test_process_3");
    
    // Manually set phase for testing
    curriculum_set_phase(process, PHASE_1_ELEMENTARY);
    
    curriculum_phase_t phase = curriculum_get_current_phase(process);
    assert(phase == PHASE_1_ELEMENTARY);
    
    curriculum_destroy(ctrl);
    printf("✓ Phase advancement test passed\n");
}

void test_persistence() {
    printf("Testing progress persistence...\n");
    
    progress_tracker_t *tracker = progress_tracker_create("test_persistence");
    
    // Record some data
    for (int i = 0; i < 50; i++) {
        progress_tracker_record_attempt(tracker, PHASE_0_FOUNDATIONS, TOPIC_MATH, true, 1);
    }
    
    // Save
    int result = progress_tracker_save(tracker, "/tmp/test_progress.dat");
    assert(result == 0);
    
    // Create new tracker and load
    progress_tracker_t *tracker2 = progress_tracker_create("test_persistence");
    result = progress_tracker_load(tracker2, "/tmp/test_progress.dat");
    assert(result == 0);
    
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(tracker2, PHASE_0_FOUNDATIONS, &metrics);
    assert(metrics.total_attempts == 50);
    
    progress_tracker_destroy(tracker);
    progress_tracker_destroy(tracker2);
    printf("✓ Persistence test passed\n");
}

int main() {
    printf("=== Running Curriculum System Tests ===\n\n");
    
    test_curriculum_init();
    test_process_registration();
    test_training_session();
    test_progress_tracking();
    test_accuracy_gates();
    test_phase_advancement();
    test_persistence();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
