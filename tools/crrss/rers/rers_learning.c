
/**
 * @file rers_learning.c
 * @brief RERS Active Learning System Implementation
 */

#include "rers_learning.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_LEARNED_BUGS 512
#define MAX_COMPONENT_NAME 128
#define MAX_DESCRIPTION 512

/**
 * @brief Learned bug entry
 */
typedef struct {
    rers_bug_info_t info;
    char component_copy[MAX_COMPONENT_NAME];
    char description_copy[MAX_DESCRIPTION];
    bool valid;
} rers_learned_bug_t;

/**
 * @brief Learning system structure
 */
struct rers_learning_system {
    rers_learning_config_t config;
    rers_learned_bug_t bugs[MAX_LEARNED_BUGS];
    size_t bug_count;
    uint64_t next_bug_id;
    
    /* Priority-based indices */
    size_t priority_counts[RERS_PRIORITY_COUNT];
    
    /* Hierarchy-based indices */
    size_t level_counts[RERS_HIERARCHY_COUNT];
};

/* Priority names */
static const char *priority_names[] = {
    "Critical",
    "High",
    "Medium",
    "Low"
};

/* Hierarchy level names */
static const char *level_names[] = {
    "Error Type",
    "Component",
    "Subsystem",
    "System",
    "Global"
};

/* Initialize learning system */
rers_error_t rers_learning_init(const rers_learning_config_t *config,
                                rers_learning_system_t **system) {
    if (!config || !system) {
        return RERS_ERROR_INVALID_PARAM;
    }

    rers_learning_system_t *sys = calloc(1, sizeof(rers_learning_system_t));
    if (!sys) {
        return RERS_ERROR_NO_MEMORY;
    }

    sys->config = *config;
    sys->bug_count = 0;
    sys->next_bug_id = 1;

    memset(sys->priority_counts, 0, sizeof(sys->priority_counts));
    memset(sys->level_counts, 0, sizeof(sys->level_counts));

    *system = sys;
    return RERS_SUCCESS;
}

/* Shutdown learning system */
void rers_learning_shutdown(rers_learning_system_t *system) {
    if (!system) {
        return;
    }
    free(system);
}

/* Learn from a new bug */
rers_error_t rers_learning_learn(rers_learning_system_t *system,
                                 const rers_bug_info_t *bug_info) {
    if (!system || !bug_info) {
        return RERS_ERROR_INVALID_PARAM;
    }

    /* Check if we have space */
    if (system->bug_count >= MAX_LEARNED_BUGS) {
        return RERS_ERROR_COMPONENT_FAILED;
    }

    /* Check if bug already exists (by error type and component) */
    for (size_t i = 0; i < system->bug_count; i++) {
        if (system->bugs[i].valid &&
            system->bugs[i].info.error_type == bug_info->error_type &&
            system->bugs[i].info.component && bug_info->component &&
            strcmp(system->bugs[i].info.component, bug_info->component) == 0) {
            
            /* Update existing bug */
            system->bugs[i].info.occurrence_count++;
            system->bugs[i].info.last_seen = (uint64_t)time(NULL);
            
            /* Update priority if higher */
            if (bug_info->priority < system->bugs[i].info.priority) {
                /* Decrease old priority count */
                if (system->priority_counts[system->bugs[i].info.priority] > 0) {
                    system->priority_counts[system->bugs[i].info.priority]--;
                }
                
                /* Update priority */
                system->bugs[i].info.priority = bug_info->priority;
                system->priority_counts[bug_info->priority]++;
            }
            
            return RERS_SUCCESS;
        }
    }

    /* Create new bug entry */
    rers_learned_bug_t *bug = &system->bugs[system->bug_count];
    bug->info = *bug_info;
    bug->info.bug_id = system->next_bug_id++;
    bug->info.occurrence_count = 1;
    bug->info.first_seen = (uint64_t)time(NULL);
    bug->info.last_seen = bug->info.first_seen;
    bug->valid = true;

    /* Copy component name */
    if (bug_info->component) {
        strncpy(bug->component_copy, bug_info->component, 
                sizeof(bug->component_copy) - 1);
        bug->component_copy[sizeof(bug->component_copy) - 1] = '\0';
        bug->info.component = bug->component_copy;
    }

    /* Copy description */
    if (bug_info->description) {
        strncpy(bug->description_copy, bug_info->description,
                sizeof(bug->description_copy) - 1);
        bug->description_copy[sizeof(bug->description_copy) - 1] = '\0';
        bug->info.description = bug->description_copy;
    }

    /* Update indices */
    if (bug_info->priority < RERS_PRIORITY_COUNT) {
        system->priority_counts[bug_info->priority]++;
    }
    
    if (bug_info->level < RERS_HIERARCHY_COUNT) {
        system->level_counts[bug_info->level]++;
    }

    system->bug_count++;
    return RERS_SUCCESS;
}

/* Get bugs by priority */
rers_error_t rers_learning_get_by_priority(rers_learning_system_t *system,
                                           rers_priority_t priority,
                                           uint64_t *bugs,
                                           size_t max_bugs,
                                           size_t *count) {
    if (!system || !bugs || !count) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (priority >= RERS_PRIORITY_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    *count = 0;
    for (size_t i = 0; i < system->bug_count && *count < max_bugs; i++) {
        if (system->bugs[i].valid && 
            system->bugs[i].info.priority == priority) {
            bugs[*count] = system->bugs[i].info.bug_id;
            (*count)++;
        }
    }

    return RERS_SUCCESS;
}

/* Get bugs by hierarchy level */
rers_error_t rers_learning_get_by_level(rers_learning_system_t *system,
                                        rers_hierarchy_level_t level,
                                        uint64_t *bugs,
                                        size_t max_bugs,
                                        size_t *count) {
    if (!system || !bugs || !count) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (level >= RERS_HIERARCHY_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    *count = 0;
    for (size_t i = 0; i < system->bug_count && *count < max_bugs; i++) {
        if (system->bugs[i].valid && 
            system->bugs[i].info.level == level) {
            bugs[*count] = system->bugs[i].info.bug_id;
            (*count)++;
        }
    }

    return RERS_SUCCESS;
}

/* Get total number of learned bugs */
size_t rers_learning_get_count(rers_learning_system_t *system) {
    if (!system) {
        return 0;
    }
    return system->bug_count;
}

/* Get priority name string */
const char *rers_learning_get_priority_name(rers_priority_t priority) {
    if (priority >= 0 && priority < RERS_PRIORITY_COUNT) {
        return priority_names[priority];
    }
    return "Unknown";
}

/* Get hierarchy level name string */
const char *rers_learning_get_level_name(rers_hierarchy_level_t level) {
    if (level >= 0 && level < RERS_HIERARCHY_COUNT) {
        return level_names[level];
    }
    return "Unknown";
}
