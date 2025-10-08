
#include "../include/curriculum.h"
#include "../include/progress.h"
#include "../include/accuracy_gates.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_PROCESSES 1024

// Curriculum process structure
struct curriculum_process {
    char process_id[256];
    curriculum_phase_t current_phase;
    progress_tracker_t *tracker;
    accuracy_gate_t *gates[PHASE_MAX];
    bool in_session;
    time_t session_start;
    uint32_t current_topic;
    uint32_t current_difficulty;
    pthread_mutex_t lock;
};

// Curriculum controller structure
struct curriculum_controller {
    char data_dir[1024];
    curriculum_process_t *processes[MAX_PROCESSES];
    uint32_t num_processes;
    pthread_mutex_t lock;
};

curriculum_controller_t *curriculum_init(const char *data_dir) {
    if (!data_dir) return NULL;
    
    curriculum_controller_t *ctrl = calloc(1, sizeof(curriculum_controller_t));
    if (!ctrl) return NULL;
    
    strncpy(ctrl->data_dir, data_dir, sizeof(ctrl->data_dir) - 1);
    pthread_mutex_init(&ctrl->lock, NULL);
    
    printf("Curriculum controller initialized with data dir: %s\n", data_dir);
    return ctrl;
}

void curriculum_destroy(curriculum_controller_t *ctrl) {
    if (!ctrl) return;
    
    pthread_mutex_lock(&ctrl->lock);
    
    for (uint32_t i = 0; i < ctrl->num_processes; i++) {
        if (ctrl->processes[i]) {
            curriculum_unregister_process(ctrl, ctrl->processes[i]);
        }
    }
    
    pthread_mutex_unlock(&ctrl->lock);
    pthread_mutex_destroy(&ctrl->lock);
    free(ctrl);
}

curriculum_process_t *curriculum_register_process(curriculum_controller_t *ctrl, const char *process_id) {
    if (!ctrl || !process_id) return NULL;
    
    pthread_mutex_lock(&ctrl->lock);
    
    if (ctrl->num_processes >= MAX_PROCESSES) {
        pthread_mutex_unlock(&ctrl->lock);
        return NULL;
    }
    
    curriculum_process_t *process = calloc(1, sizeof(curriculum_process_t));
    if (!process) {
        pthread_mutex_unlock(&ctrl->lock);
        return NULL;
    }
    
    strncpy(process->process_id, process_id, sizeof(process->process_id) - 1);
    process->current_phase = PHASE_0_FOUNDATIONS;
    process->tracker = progress_tracker_create(process_id);
    pthread_mutex_init(&process->lock, NULL);
    
    // Initialize accuracy gates for all phases
    for (int i = 0; i < PHASE_MAX; i++) {
        process->gates[i] = accuracy_gate_create(i);
    }
    
    ctrl->processes[ctrl->num_processes++] = process;
    
    pthread_mutex_unlock(&ctrl->lock);
    
    printf("Registered process: %s\n", process_id);
    return process;
}

void curriculum_unregister_process(curriculum_controller_t *ctrl, curriculum_process_t *process) {
    if (!ctrl || !process) return;
    
    pthread_mutex_lock(&process->lock);
    
    // End any active session before cleanup
    if (process->in_session) {
        progress_tracker_end_session(process->tracker, process->current_phase);
    }
    
    if (process->tracker) {
        progress_tracker_destroy(process->tracker);
    }
    
    for (int i = 0; i < PHASE_MAX; i++) {
        if (process->gates[i]) {
            accuracy_gate_destroy(process->gates[i]);
        }
    }
    
    pthread_mutex_unlock(&process->lock);
    pthread_mutex_destroy(&process->lock);
    free(process);
}

int curriculum_start_session(curriculum_process_t *process) {
    if (!process) return -1;
    
    pthread_mutex_lock(&process->lock);
    
    if (process->in_session) {
        pthread_mutex_unlock(&process->lock);
        return -1;
    }
    
    process->in_session = true;
    process->session_start = time(NULL);
    
    // Start session tracking in progress tracker
    progress_tracker_start_session(process->tracker, process->current_phase);
    
    pthread_mutex_unlock(&process->lock);
    return 0;
}

int curriculum_end_session(curriculum_process_t *process) {
    if (!process) return -1;
    
    pthread_mutex_lock(&process->lock);
    
    if (!process->in_session) {
        pthread_mutex_unlock(&process->lock);
        return -1;
    }
    
    process->in_session = false;
    
    // End session tracking in progress tracker (increments num_sessions!)
    progress_tracker_end_session(process->tracker, process->current_phase);
    
    // Save progress
    char filename[1024];
    snprintf(filename, sizeof(filename), "/tmp/progress_%s.dat", process->process_id);
    progress_tracker_save(process->tracker, filename);
    
    pthread_mutex_unlock(&process->lock);
    return 0;
}

int curriculum_get_next_example(curriculum_process_t *process, void **input, size_t *input_size, void **output, size_t *output_size) {
    if (!process || !input || !input_size || !output || !output_size) return -1;
    
    pthread_mutex_lock(&process->lock);
    
    if (!process->in_session) {
        pthread_mutex_unlock(&process->lock);
        return -1;
    }
    
    // Select content based on current phase
    // This is a simplified implementation - actual implementation would load from training tables
    
    *input = malloc(16);
    *output = malloc(16);
    *input_size = 16;
    *output_size = 16;
    
    // Generate sample data based on phase
    uint32_t *in_data = (uint32_t *)*input;
    uint32_t *out_data = (uint32_t *)*output;
    
    in_data[0] = process->current_phase;
    in_data[1] = process->current_topic;
    in_data[2] = rand() % 100;
    in_data[3] = rand() % 100;
    
    out_data[0] = in_data[2] + in_data[3]; // Simple addition for now
    
    pthread_mutex_unlock(&process->lock);
    return 0;
}

int curriculum_submit_answer(curriculum_process_t *process, const void *answer, size_t answer_size, bool *correct) {
    if (!process || !answer || !correct) return -1;
    
    pthread_mutex_lock(&process->lock);
    
    if (!process->in_session) {
        pthread_mutex_unlock(&process->lock);
        return -1;
    }
    
    // Check answer (simplified)
    *correct = (rand() % 100) < 80; // 80% correct for testing
    
    // Record attempt
    time_t duration = time(NULL) - process->session_start;
    progress_tracker_record_attempt(process->tracker, process->current_phase, 
                                    process->current_topic, *correct, duration);
    
    pthread_mutex_unlock(&process->lock);
    return 0;
}

curriculum_phase_t curriculum_get_current_phase(const curriculum_process_t *process) {
    if (!process) return PHASE_0_FOUNDATIONS;
    return process->current_phase;
}

bool curriculum_can_advance_phase(const curriculum_process_t *process) {
    if (!process) return false;
    
    if (process->current_phase >= PHASE_7_TRANSCENDENT) {
        return false;
    }
    
    // Check accuracy gate
    accuracy_gate_t *gate = process->gates[process->current_phase];
    return accuracy_gate_check(gate, process->tracker, process->current_phase);
}

int curriculum_advance_phase(curriculum_process_t *process) {
    if (!process) return -1;
    
    pthread_mutex_lock(&process->lock);
    
    if (!curriculum_can_advance_phase(process)) {
        pthread_mutex_unlock(&process->lock);
        return -1;
    }
    
    process->current_phase++;
    printf("Process %s advanced to phase %d\n", process->process_id, process->current_phase);
    
    pthread_mutex_unlock(&process->lock);
    return 0;
}

int curriculum_set_phase(curriculum_process_t *process, curriculum_phase_t phase) {
    if (!process || phase >= PHASE_MAX) return -1;
    
    pthread_mutex_lock(&process->lock);
    process->current_phase = phase;
    pthread_mutex_unlock(&process->lock);
    
    return 0;
}

double curriculum_get_phase_accuracy(const curriculum_process_t *process, curriculum_phase_t phase) {
    if (!process) return 0.0;
    
    phase_metrics_t metrics;
    progress_tracker_get_phase_metrics(process->tracker, phase, &metrics);
    return metrics.accuracy;
}

double curriculum_get_topic_accuracy(const curriculum_process_t *process, topic_type_t topic) {
    if (!process) return 0.0;
    
    topic_metrics_t metrics;
    progress_tracker_get_topic_metrics(process->tracker, topic, &metrics);
    return metrics.accuracy;
}

double curriculum_get_overall_accuracy(const curriculum_process_t *process) {
    if (!process) return 0.0;
    
    overall_metrics_t metrics;
    progress_tracker_get_overall_metrics(process->tracker, &metrics);
    return metrics.overall_accuracy;
}

uint64_t curriculum_get_total_attempts(const curriculum_process_t *process) {
    if (!process) return 0;
    
    overall_metrics_t metrics;
    progress_tracker_get_overall_metrics(process->tracker, &metrics);
    return metrics.total_attempts;
}

uint64_t curriculum_get_total_correct(const curriculum_process_t *process) {
    if (!process) return 0;
    
    overall_metrics_t metrics;
    progress_tracker_get_overall_metrics(process->tracker, &metrics);
    return metrics.total_correct;
}

int curriculum_save_progress(const curriculum_process_t *process, const char *filename) {
    if (!process || !filename) return -1;
    return progress_tracker_save(process->tracker, filename);
}

int curriculum_load_progress(curriculum_process_t *process, const char *filename) {
    if (!process || !filename) return -1;
    return progress_tracker_load(process->tracker, filename);
}
