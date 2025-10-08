
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

// Session management
// A "session" is a continuous training period for a specific phase.
// Sessions must be explicitly started and ended to track num_sessions correctly.
// The num_sessions counter is incremented when progress_tracker_end_session() is called.
// This is critical for consistency gates to function properly.
//
// Time tracking is done per-session, not per-attempt. When a session ends, the session
// duration is added to:
// - Phase time metrics (for the current phase)
// - Topic time metrics (for all topics used during the session)
// - Overall time metrics
//
// This ensures all time metrics grow linearly with actual elapsed time, preventing
// the quadratic growth bug that occurred when cumulative duration was added per-attempt.
//
// Session lifecycle:
// 1. Call progress_tracker_start_session() to begin a training session
// 2. Record training attempts using progress_tracker_record_attempt()
// 3. Call progress_tracker_end_session() to complete the session (increments num_sessions and updates time metrics)
//
// Example usage:
//   progress_tracker_start_session(tracker, PHASE_0_FOUNDATIONS);
//   for (int i = 0; i < 100; i++) {
//       bool correct = train_and_check();
//       progress_tracker_record_attempt(tracker, PHASE_0_FOUNDATIONS, TOPIC_MATH, correct);
//   }
//   progress_tracker_end_session(tracker, PHASE_0_FOUNDATIONS);  // num_sessions++, time metrics updated
void progress_tracker_start_session(progress_tracker_t *tracker, uint8_t phase);
void progress_tracker_end_session(progress_tracker_t *tracker, uint8_t phase);
bool progress_tracker_is_session_active(progress_tracker_t *tracker, uint8_t phase);

/**
 * Record a training attempt for a phase and topic.
 * 
 * Time is tracked per-session, not per-attempt. Topic and overall time
 * metrics are updated when the session ends, ensuring consistency with
 * per-phase time metrics. All time metrics use session duration for
 * linear growth.
 * 
 * @param tracker The progress tracker
 * @param phase The current phase
 * @param topic The topic being trained
 * @param correct Whether the attempt was correct
 */
void progress_tracker_record_attempt(progress_tracker_t *tracker, curriculum_phase_t phase, topic_type_t topic, bool correct);

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
