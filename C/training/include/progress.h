
#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "curriculum.h"

// Per-phase metrics
typedef struct {
    uint64_t total_attempts;
    uint64_t correct_answers;
    uint64_t incorrect_answers;
    double accuracy;
    time_t time_spent;
    uint32_t num_sessions;
    time_t first_attempt;
    time_t last_attempt;
} phase_metrics_t;

// Per-topic metrics
typedef struct {
    uint64_t total_attempts;
    uint64_t correct_answers;
    double accuracy;
    time_t time_spent;
} topic_metrics_t;

// Overall metrics
typedef struct {
    uint64_t total_attempts;
    uint64_t total_correct;
    double overall_accuracy;
    time_t total_time;
    double learning_velocity;
    uint32_t mastery_level;
    time_t started_at;
    time_t last_updated;
} overall_metrics_t;

// Progress tracker
typedef struct progress_tracker progress_tracker_t;

// Create/destroy progress tracker
progress_tracker_t *progress_tracker_create(const char *process_id);
void progress_tracker_destroy(progress_tracker_t *tracker);

// Record attempts
void progress_tracker_record_attempt(progress_tracker_t *tracker, curriculum_phase_t phase, topic_type_t topic, bool correct, time_t duration);

// Query metrics
void progress_tracker_get_phase_metrics(const progress_tracker_t *tracker, curriculum_phase_t phase, phase_metrics_t *metrics);
void progress_tracker_get_topic_metrics(const progress_tracker_t *tracker, topic_type_t topic, topic_metrics_t *metrics);
void progress_tracker_get_overall_metrics(const progress_tracker_t *tracker, overall_metrics_t *metrics);

// Persistence
int progress_tracker_save(const progress_tracker_t *tracker, const char *filename);
int progress_tracker_load(progress_tracker_t *tracker, const char *filename);

// Checkpointing
int progress_tracker_checkpoint(const progress_tracker_t *tracker);
int progress_tracker_restore(progress_tracker_t *tracker);

// Statistics
double progress_tracker_calculate_learning_velocity(const progress_tracker_t *tracker);
uint32_t progress_tracker_calculate_mastery_level(const progress_tracker_t *tracker);
void progress_tracker_get_strengths_weaknesses(const progress_tracker_t *tracker, topic_type_t *strengths, topic_type_t *weaknesses);

#endif // PROGRESS_H
