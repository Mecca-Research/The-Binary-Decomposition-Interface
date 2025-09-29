
/*
 * Master Memory Manager - Phase 4 Self-Healing System
 * Automatic problem detection and resolution
 * Part of the LEGENDARY BDI BUILD
 */

#ifndef MMM_SELF_HEALING_H
#define MMM_SELF_HEALING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Self-healing actions
typedef enum {
    MMM_HEALING_RESTART_COMPONENT = 1,
    MMM_HEALING_REALLOCATE_MEMORY,
    MMM_HEALING_ADJUST_PARAMETERS,
    MMM_HEALING_FAILOVER,
    MMM_HEALING_SCALE_RESOURCES,
    MMM_HEALING_CLEAR_CACHE,
    MMM_HEALING_RESET_CONNECTION,
    MMM_HEALING_CUSTOM_ACTION
} mmm_healing_action_t;

// Learning data
typedef struct {
    uint32_t data_points;
    double *features;
    double *labels;
    struct timespec collection_time;
} mmm_learning_data_t;

// Function declarations

/**
 * Trigger self-healing
 * @param issue_type Type of issue to heal
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_trigger_self_healing(uint32_t issue_type);

/**
 * Update learning model
 * @param data Learning data
 * @return MMM_SUCCESS on success, error code on failure
 */
int mmm_update_learning_model(mmm_learning_data_t *data);

#endif /* MMM_SELF_HEALING_H */
