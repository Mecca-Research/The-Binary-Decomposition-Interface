#include "include/progress.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    progress_tracker_t *tracker = progress_tracker_create("debug");
    
    printf("Starting Phase 0 session...\n");
    progress_tracker_start_session(tracker, PHASE_0_FOUNDATIONS);
    time_t p0_start = time(NULL);
    
    for (int i = 0; i < 50; i++) {
        progress_tracker_record_attempt(tracker, PHASE_0_FOUNDATIONS, TOPIC_MATH, true);
    }
    
    sleep(1);
    
    printf("Starting Phase 1 session (Phase 0 still active)...\n");
    progress_tracker_start_session(tracker, PHASE_1_ELEMENTARY);
    time_t p1_start = time(NULL);
    
    for (int i = 0; i < 30; i++) {
        progress_tracker_record_attempt(tracker, PHASE_1_ELEMENTARY, TOPIC_LOGIC, true);
    }
    
    sleep(1);
    
    printf("Ending Phase 0 session...\n");
    progress_tracker_end_session(tracker, PHASE_0_FOUNDATIONS);
    time_t p0_end = time(NULL);
    
    topic_metrics_t math_metrics, logic_metrics;
    progress_tracker_get_topic_metrics(tracker, TOPIC_MATH, &math_metrics);
    progress_tracker_get_topic_metrics(tracker, TOPIC_LOGIC, &logic_metrics);
    
    printf("After Phase 0 ends:\n");
    printf("  Phase 0 duration: %ld\n", p0_end - p0_start);
    printf("  TOPIC_MATH time: %ld\n", math_metrics.time_spent);
    printf("  TOPIC_LOGIC time: %ld\n", logic_metrics.time_spent);
    
    sleep(1);
    
    printf("Ending Phase 1 session...\n");
    progress_tracker_end_session(tracker, PHASE_1_ELEMENTARY);
    time_t p1_end = time(NULL);
    
    progress_tracker_get_topic_metrics(tracker, TOPIC_MATH, &math_metrics);
    progress_tracker_get_topic_metrics(tracker, TOPIC_LOGIC, &logic_metrics);
    
    printf("After Phase 1 ends:\n");
    printf("  Phase 1 duration: %ld\n", p1_end - p1_start);
    printf("  TOPIC_MATH time: %ld\n", math_metrics.time_spent);
    printf("  TOPIC_LOGIC time: %ld\n", logic_metrics.time_spent);
    
    progress_tracker_destroy(tracker);
    return 0;
}
