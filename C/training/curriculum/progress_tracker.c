
#include "../include/progress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct progress_tracker {
    char process_id[256];
    phase_metrics_t phase_metrics[PHASE_MAX];
    topic_metrics_t topic_metrics[TOPIC_MAX];
    overall_metrics_t overall_metrics;
    pthread_mutex_t lock;
};

progress_tracker_t *progress_tracker_create(const char *process_id) {
    if (!process_id) return NULL;
    
    progress_tracker_t *tracker = calloc(1, sizeof(progress_tracker_t));
    if (!tracker) return NULL;
    
    strncpy(tracker->process_id, process_id, sizeof(tracker->process_id) - 1);
    pthread_mutex_init(&tracker->lock, NULL);
    
    tracker->overall_metrics.started_at = time(NULL);
    tracker->overall_metrics.last_updated = time(NULL);
    
    return tracker;
}

void progress_tracker_destroy(progress_tracker_t *tracker) {
    if (!tracker) return;
    
    pthread_mutex_destroy(&tracker->lock);
    free(tracker);
}

void progress_tracker_record_attempt(progress_tracker_t *tracker, curriculum_phase_t phase, 
                                     topic_type_t topic, bool correct, time_t duration) {
    if (!tracker || phase >= PHASE_MAX || topic >= TOPIC_MAX) return;
    
    pthread_mutex_lock(&tracker->lock);
    
    // Update phase metrics
    phase_metrics_t *pm = &tracker->phase_metrics[phase];
    pm->total_attempts++;
    if (correct) {
        pm->correct_answers++;
    } else {
        pm->incorrect_answers++;
    }
    pm->accuracy = (double)pm->correct_answers / pm->total_attempts;
    pm->time_spent += duration;
    pm->last_attempt = time(NULL);
    
    if (pm->first_attempt == 0) {
        pm->first_attempt = time(NULL);
    }
    
    // Update topic metrics
    topic_metrics_t *tm = &tracker->topic_metrics[topic];
    tm->total_attempts++;
    if (correct) {
        tm->correct_answers++;
    }
    tm->accuracy = (double)tm->correct_answers / tm->total_attempts;
    tm->time_spent += duration;
    
    // Update overall metrics
    overall_metrics_t *om = &tracker->overall_metrics;
    om->total_attempts++;
    if (correct) {
        om->total_correct++;
    }
    om->overall_accuracy = (double)om->total_correct / om->total_attempts;
    om->total_time += duration;
    om->last_updated = time(NULL);
    
    // Calculate learning velocity
    om->learning_velocity = progress_tracker_calculate_learning_velocity(tracker);
    om->mastery_level = progress_tracker_calculate_mastery_level(tracker);
    
    pthread_mutex_unlock(&tracker->lock);
}

void progress_tracker_get_phase_metrics(const progress_tracker_t *tracker, curriculum_phase_t phase, 
                                       phase_metrics_t *metrics) {
    if (!tracker || !metrics || phase >= PHASE_MAX) return;
    
    memcpy(metrics, &tracker->phase_metrics[phase], sizeof(phase_metrics_t));
}

void progress_tracker_get_topic_metrics(const progress_tracker_t *tracker, topic_type_t topic, 
                                       topic_metrics_t *metrics) {
    if (!tracker || !metrics || topic >= TOPIC_MAX) return;
    
    memcpy(metrics, &tracker->topic_metrics[topic], sizeof(topic_metrics_t));
}

void progress_tracker_get_overall_metrics(const progress_tracker_t *tracker, overall_metrics_t *metrics) {
    if (!tracker || !metrics) return;
    
    memcpy(metrics, &tracker->overall_metrics, sizeof(overall_metrics_t));
}

int progress_tracker_save(const progress_tracker_t *tracker, const char *filename) {
    if (!tracker || !filename) return -1;
    
    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;
    
    pthread_mutex_lock((pthread_mutex_t *)&tracker->lock);
    
    fwrite(tracker->process_id, sizeof(tracker->process_id), 1, fp);
    fwrite(tracker->phase_metrics, sizeof(tracker->phase_metrics), 1, fp);
    fwrite(tracker->topic_metrics, sizeof(tracker->topic_metrics), 1, fp);
    fwrite(&tracker->overall_metrics, sizeof(tracker->overall_metrics), 1, fp);
    
    pthread_mutex_unlock((pthread_mutex_t *)&tracker->lock);
    
    fclose(fp);
    return 0;
}

int progress_tracker_load(progress_tracker_t *tracker, const char *filename) {
    if (!tracker || !filename) return -1;
    
    FILE *fp = fopen(filename, "rb");
    if (!fp) return -1;
    
    pthread_mutex_lock(&tracker->lock);
    
    fread(tracker->process_id, sizeof(tracker->process_id), 1, fp);
    fread(tracker->phase_metrics, sizeof(tracker->phase_metrics), 1, fp);
    fread(tracker->topic_metrics, sizeof(tracker->topic_metrics), 1, fp);
    fread(&tracker->overall_metrics, sizeof(tracker->overall_metrics), 1, fp);
    
    pthread_mutex_unlock(&tracker->lock);
    
    fclose(fp);
    return 0;
}

int progress_tracker_checkpoint(const progress_tracker_t *tracker) {
    if (!tracker) return -1;
    
    char filename[1024];
    snprintf(filename, sizeof(filename), "/tmp/checkpoint_%s.dat", tracker->process_id);
    return progress_tracker_save(tracker, filename);
}

int progress_tracker_restore(progress_tracker_t *tracker) {
    if (!tracker) return -1;
    
    char filename[1024];
    snprintf(filename, sizeof(filename), "/tmp/checkpoint_%s.dat", tracker->process_id);
    return progress_tracker_load(tracker, filename);
}

double progress_tracker_calculate_learning_velocity(const progress_tracker_t *tracker) {
    if (!tracker) return 0.0;
    
    time_t elapsed = time(NULL) - tracker->overall_metrics.started_at;
    if (elapsed == 0) return 0.0;
    
    return (double)tracker->overall_metrics.total_correct / elapsed;
}

uint32_t progress_tracker_calculate_mastery_level(const progress_tracker_t *tracker) {
    if (!tracker) return 0;
    
    double accuracy = tracker->overall_metrics.overall_accuracy;
    
    if (accuracy >= 0.99) return 7;
    if (accuracy >= 0.95) return 6;
    if (accuracy >= 0.90) return 5;
    if (accuracy >= 0.85) return 4;
    if (accuracy >= 0.80) return 3;
    if (accuracy >= 0.75) return 2;
    if (accuracy >= 0.70) return 1;
    return 0;
}

void progress_tracker_get_strengths_weaknesses(const progress_tracker_t *tracker, 
                                               topic_type_t *strengths, topic_type_t *weaknesses) {
    if (!tracker || !strengths || !weaknesses) return;
    
    double max_accuracy = 0.0;
    double min_accuracy = 1.0;
    
    for (int i = 0; i < TOPIC_MAX; i++) {
        double accuracy = tracker->topic_metrics[i].accuracy;
        
        if (accuracy > max_accuracy) {
            max_accuracy = accuracy;
            *strengths = i;
        }
        
        if (accuracy < min_accuracy && tracker->topic_metrics[i].total_attempts > 0) {
            min_accuracy = accuracy;
            *weaknesses = i;
        }
    }
}
