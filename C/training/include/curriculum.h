
#ifndef CURRICULUM_H
#define CURRICULUM_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Phase definitions
typedef enum {
    PHASE_0_FOUNDATIONS = 0,
    PHASE_1_ELEMENTARY = 1,
    PHASE_2_INTERMEDIATE = 2,
    PHASE_3_ADVANCED = 3,
    PHASE_4_EXPERT = 4,
    PHASE_5_MASTER = 5,
    PHASE_6_VIRTUOSO = 6,
    PHASE_7_TRANSCENDENT = 7,
    PHASE_MAX = 8
} curriculum_phase_t;

// Topic types
typedef enum {
    TOPIC_MATH = 0,
    TOPIC_LOGIC = 1,
    TOPIC_LANGUAGE = 2,
    TOPIC_CODE = 3,
    TOPIC_MAX = 4
} topic_type_t;

// AI process handle
typedef struct curriculum_process curriculum_process_t;

// Curriculum controller
typedef struct curriculum_controller curriculum_controller_t;

// Initialize curriculum system
curriculum_controller_t *curriculum_init(const char *data_dir);
void curriculum_destroy(curriculum_controller_t *ctrl);

// Process management
curriculum_process_t *curriculum_register_process(curriculum_controller_t *ctrl, const char *process_id);
void curriculum_unregister_process(curriculum_controller_t *ctrl, curriculum_process_t *process);

// Training session
int curriculum_start_session(curriculum_process_t *process);
int curriculum_end_session(curriculum_process_t *process);
int curriculum_get_next_example(curriculum_process_t *process, void **input, size_t *input_size, void **output, size_t *output_size);
int curriculum_submit_answer(curriculum_process_t *process, const void *answer, size_t answer_size, bool *correct);

// Phase management
curriculum_phase_t curriculum_get_current_phase(const curriculum_process_t *process);
bool curriculum_can_advance_phase(const curriculum_process_t *process);
int curriculum_advance_phase(curriculum_process_t *process);
int curriculum_set_phase(curriculum_process_t *process, curriculum_phase_t phase); // Manual override

// Progress queries
double curriculum_get_phase_accuracy(const curriculum_process_t *process, curriculum_phase_t phase);
double curriculum_get_topic_accuracy(const curriculum_process_t *process, topic_type_t topic);
double curriculum_get_overall_accuracy(const curriculum_process_t *process);
uint64_t curriculum_get_total_attempts(const curriculum_process_t *process);
uint64_t curriculum_get_total_correct(const curriculum_process_t *process);

// Persistence
int curriculum_save_progress(const curriculum_process_t *process, const char *filename);
int curriculum_load_progress(curriculum_process_t *process, const char *filename);

#endif // CURRICULUM_H
