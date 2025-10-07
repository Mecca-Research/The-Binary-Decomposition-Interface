
#ifndef BDI_FEEDBACK_COLLECTOR_H
#define BDI_FEEDBACK_COLLECTOR_H

#include "../Profiling/profile_data.h"
#include "../ModelFormat/bdi_model.h"
#include <stdbool.h>

// Feedback entry
typedef struct {
    char source_file[256];
    ProfileData *profile;
    double performance_score;
    time_t timestamp;
} FeedbackEntry;

// Feedback database
typedef struct {
    FeedbackEntry *entries;
    size_t entry_count;
    size_t entry_capacity;
} FeedbackDatabase;

// Initialize feedback collector
bool feedback_collector_init(void);

// Cleanup feedback collector
void feedback_collector_cleanup(void);

// Create feedback database
FeedbackDatabase* feedback_collector_create_db(void);

// Free feedback database
void feedback_collector_free_db(FeedbackDatabase *db);

// Add feedback entry
bool feedback_collector_add(FeedbackDatabase *db, const char *source_file,
                           const ProfileData *profile, double performance_score);

// Get feedback for file
const FeedbackEntry* feedback_collector_get(const FeedbackDatabase *db,
                                           const char *source_file, size_t *count);

// Save feedback database
bool feedback_collector_save(const FeedbackDatabase *db, const char *filename);

// Load feedback database
FeedbackDatabase* feedback_collector_load(const char *filename);

#endif // BDI_FEEDBACK_COLLECTOR_H
