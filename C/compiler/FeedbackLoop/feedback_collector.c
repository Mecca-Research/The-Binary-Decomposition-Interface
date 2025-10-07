
#include "feedback_collector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_DB_CAPACITY 100

static bool feedback_collector_initialized = false;

bool feedback_collector_init(void) {
    if (feedback_collector_initialized) {
        return true;
    }
    feedback_collector_initialized = true;
    return true;
}

void feedback_collector_cleanup(void) {
    feedback_collector_initialized = false;
}

FeedbackDatabase* feedback_collector_create_db(void) {
    FeedbackDatabase *db = calloc(1, sizeof(FeedbackDatabase));
    if (!db) {
        return NULL;
    }

    db->entries = calloc(INITIAL_DB_CAPACITY, sizeof(FeedbackEntry));
    if (!db->entries) {
        free(db);
        return NULL;
    }

    db->entry_capacity = INITIAL_DB_CAPACITY;
    db->entry_count = 0;

    return db;
}

void feedback_collector_free_db(FeedbackDatabase *db) {
    if (!db) return;

    for (size_t i = 0; i < db->entry_count; i++) {
        if (db->entries[i].profile) {
            profile_data_free(db->entries[i].profile);
        }
    }

    free(db->entries);
    free(db);
}

bool feedback_collector_add(FeedbackDatabase *db, const char *source_file,
                           const ProfileData *profile, double performance_score) {
    if (!db || !source_file) {
        return false;
    }

    // Expand capacity if needed
    if (db->entry_count >= db->entry_capacity) {
        size_t new_capacity = db->entry_capacity * 2;
        FeedbackEntry *new_entries = realloc(db->entries,
                                            new_capacity * sizeof(FeedbackEntry));
        if (!new_entries) {
            return false;
        }
        db->entries = new_entries;
        db->entry_capacity = new_capacity;
    }

    // Add entry
    FeedbackEntry *entry = &db->entries[db->entry_count++];
    strncpy(entry->source_file, source_file, sizeof(entry->source_file) - 1);
    entry->performance_score = performance_score;
    entry->timestamp = time(NULL);

    // Copy profile data
    if (profile) {
        entry->profile = malloc(sizeof(ProfileData));
        if (entry->profile) {
            memcpy(entry->profile, profile, sizeof(ProfileData));
        }
    }

    return true;
}

const FeedbackEntry* feedback_collector_get(const FeedbackDatabase *db,
                                           const char *source_file, size_t *count) {
    if (!db || !source_file || !count) {
        return NULL;
    }

    *count = 0;

    // Find matching entries
    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].source_file, source_file) == 0) {
            (*count)++;
        }
    }

    if (*count == 0) {
        return NULL;
    }

    // Return first matching entry (simplified)
    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].source_file, source_file) == 0) {
            return &db->entries[i];
        }
    }

    return NULL;
}

bool feedback_collector_save(const FeedbackDatabase *db, const char *filename) {
    if (!db || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Write entry count
    fwrite(&db->entry_count, sizeof(size_t), 1, fp);

    // Write entries (without profile data for simplicity)
    for (size_t i = 0; i < db->entry_count; i++) {
        fwrite(db->entries[i].source_file, sizeof(char), 256, fp);
        fwrite(&db->entries[i].performance_score, sizeof(double), 1, fp);
        fwrite(&db->entries[i].timestamp, sizeof(time_t), 1, fp);
    }

    fclose(fp);
    return true;
}

FeedbackDatabase* feedback_collector_load(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    FeedbackDatabase *db = calloc(1, sizeof(FeedbackDatabase));
    if (!db) {
        fclose(fp);
        return NULL;
    }

    // Read entry count
    fread(&db->entry_count, sizeof(size_t), 1, fp);

    if (db->entry_count > 0) {
        db->entries = calloc(db->entry_count, sizeof(FeedbackEntry));
        if (!db->entries) {
            free(db);
            fclose(fp);
            return NULL;
        }

        // Read entries
        for (size_t i = 0; i < db->entry_count; i++) {
            fread(db->entries[i].source_file, sizeof(char), 256, fp);
            fread(&db->entries[i].performance_score, sizeof(double), 1, fp);
            fread(&db->entries[i].timestamp, sizeof(time_t), 1, fp);
        }
    }

    db->entry_capacity = db->entry_count;

    fclose(fp);
    return db;
}
