
/**
 * @file rers_integration.c
 * @brief RERS Integration Layer Implementation
 */

#include "rers_integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_OUTPUTS_PER_TASK 16

/**
 * @brief Stored profile output
 */
typedef struct {
    rers_profile_output_t output;
    void *data_copy;
    bool valid;
} rers_stored_output_t;

/**
 * @brief Task coordination state
 */
typedef struct {
    rers_task_type_t task;
    rers_stored_output_t outputs[MAX_OUTPUTS_PER_TASK];
    size_t output_count;
} rers_task_state_t;

/**
 * @brief Integration layer structure
 */
struct rers_integration_layer {
    rers_integration_config_t config;
    bool profiles_enabled[RERS_PROFILE_COUNT];
    rers_task_state_t task_states[RERS_TASK_COUNT];
};

/* Profile names */
static const char *profile_names[] = {
    "MSM (Multi-State Machine)",
    "STP (Self-Testing Protocol)",
    "BPME (Bug Pattern Matching Engine)",
    "TDT (Test-Driven Thinking)"
};

/* Task names */
static const char *task_names[] = {
    "Error Analysis",
    "Pattern Matching",
    "Test Generation",
    "State Tracking",
    "Bug Classification"
};

/* Profile-to-Task mapping (which profiles are best for which tasks) */
static const rers_profile_type_t task_profiles[][RERS_PROFILE_COUNT] = {
    /* ERROR_ANALYSIS */      { RERS_PROFILE_BPME, RERS_PROFILE_MSM, RERS_PROFILE_STP, RERS_PROFILE_TDT },
    /* PATTERN_MATCHING */    { RERS_PROFILE_BPME, RERS_PROFILE_TDT, RERS_PROFILE_STP, RERS_PROFILE_MSM },
    /* TEST_GENERATION */     { RERS_PROFILE_TDT, RERS_PROFILE_STP, RERS_PROFILE_BPME, RERS_PROFILE_MSM },
    /* STATE_TRACKING */      { RERS_PROFILE_MSM, RERS_PROFILE_STP, RERS_PROFILE_BPME, RERS_PROFILE_TDT },
    /* BUG_CLASSIFICATION */  { RERS_PROFILE_BPME, RERS_PROFILE_MSM, RERS_PROFILE_TDT, RERS_PROFILE_STP }
};

/* Initialize integration layer */
rers_error_t rers_integration_init(const rers_integration_config_t *config,
                                   rers_integration_layer_t **layer) {
    if (!config || !layer) {
        return RERS_ERROR_INVALID_PARAM;
    }

    rers_integration_layer_t *integ = calloc(1, sizeof(rers_integration_layer_t));
    if (!integ) {
        return RERS_ERROR_NO_MEMORY;
    }

    integ->config = *config;

    /* Enable all profiles by default */
    for (size_t i = 0; i < RERS_PROFILE_COUNT; i++) {
        integ->profiles_enabled[i] = true;
    }

    /* Initialize task states */
    for (size_t i = 0; i < RERS_TASK_COUNT; i++) {
        integ->task_states[i].task = (rers_task_type_t)i;
        integ->task_states[i].output_count = 0;
    }

    *layer = integ;
    return RERS_SUCCESS;
}

/* Shutdown integration layer */
void rers_integration_shutdown(rers_integration_layer_t *layer) {
    if (!layer) {
        return;
    }

    /* Free stored output data */
    for (size_t i = 0; i < RERS_TASK_COUNT; i++) {
        for (size_t j = 0; j < layer->task_states[i].output_count; j++) {
            if (layer->task_states[i].outputs[j].data_copy) {
                free(layer->task_states[i].outputs[j].data_copy);
            }
        }
    }

    free(layer);
}

/* Submit profile output for coordination */
rers_error_t rers_integration_submit_output(rers_integration_layer_t *layer,
                                            const rers_profile_output_t *output) {
    if (!layer || !output) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (output->task >= RERS_TASK_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    rers_task_state_t *task_state = &layer->task_states[output->task];

    /* Check if we have space */
    if (task_state->output_count >= MAX_OUTPUTS_PER_TASK) {
        return RERS_ERROR_COMPONENT_FAILED;
    }

    /* Store output */
    rers_stored_output_t *stored = &task_state->outputs[task_state->output_count];
    stored->output = *output;
    stored->valid = true;

    /* Copy data if present */
    if (output->data && output->data_size > 0) {
        stored->data_copy = malloc(output->data_size);
        if (stored->data_copy) {
            memcpy(stored->data_copy, output->data, output->data_size);
            stored->output.data = stored->data_copy;
        }
    }

    stored->output.timestamp = (uint64_t)time(NULL);
    task_state->output_count++;

    return RERS_SUCCESS;
}

/* Coordinate profiles for a task */
rers_error_t rers_integration_coordinate(rers_integration_layer_t *layer,
                                         rers_task_type_t task,
                                         rers_coordination_result_t *result) {
    if (!layer || !result) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (task >= RERS_TASK_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    memset(result, 0, sizeof(rers_coordination_result_t));
    result->task = task;

    rers_task_state_t *task_state = &layer->task_states[task];

    /* Determine primary profile for this task */
    result->primary_profile = task_profiles[task][0];

    /* Aggregate confidence from all profile outputs */
    float total_confidence = 0.0f;
    uint32_t confidence_count = 0;

    for (size_t i = 0; i < task_state->output_count; i++) {
        if (!task_state->outputs[i].valid) {
            continue;
        }

        rers_profile_output_t *output = &task_state->outputs[i].output;
        
        /* Mark profile as used */
        result->profiles_used |= (1 << output->profile);
        
        /* Aggregate confidence */
        total_confidence += output->confidence;
        confidence_count++;
    }

    /* Calculate overall confidence */
    if (confidence_count > 0) {
        result->overall_confidence = total_confidence / confidence_count;
    }

    /* Generate recommendation based on task and profiles */
    static char recommendation_buffer[256];
    snprintf(recommendation_buffer, sizeof(recommendation_buffer),
             "Task '%s' coordinated using %u profile(s) with %.1f%% confidence. "
             "Primary profile: %s",
             task_names[task],
             confidence_count,
             result->overall_confidence * 100.0f,
             profile_names[result->primary_profile]);
    result->recommendation = recommendation_buffer;

    return RERS_SUCCESS;
}

/* Get active profiles for a task */
rers_error_t rers_integration_get_active_profiles(rers_integration_layer_t *layer,
                                                  rers_task_type_t task,
                                                  rers_profile_type_t *profiles,
                                                  size_t max_profiles,
                                                  size_t *count) {
    if (!layer || !profiles || !count) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (task >= RERS_TASK_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    *count = 0;

    /* Return profiles that are enabled and suitable for this task */
    for (size_t i = 0; i < RERS_PROFILE_COUNT && *count < max_profiles; i++) {
        rers_profile_type_t profile = task_profiles[task][i];
        if (layer->profiles_enabled[profile]) {
            profiles[*count] = profile;
            (*count)++;
        }
    }

    return RERS_SUCCESS;
}

/* Enable/disable a profile */
rers_error_t rers_integration_set_profile_enabled(rers_integration_layer_t *layer,
                                                  rers_profile_type_t profile,
                                                  bool enabled) {
    if (!layer) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (profile >= RERS_PROFILE_COUNT) {
        return RERS_ERROR_INVALID_PARAM;
    }

    layer->profiles_enabled[profile] = enabled;
    return RERS_SUCCESS;
}

/* Get profile type name string */
const char *rers_integration_get_profile_name(rers_profile_type_t profile) {
    if (profile >= 0 && profile < RERS_PROFILE_COUNT) {
        return profile_names[profile];
    }
    return "Unknown Profile";
}

/* Get task type name string */
const char *rers_integration_get_task_name(rers_task_type_t task) {
    if (task >= 0 && task < RERS_TASK_COUNT) {
        return task_names[task];
    }
    return "Unknown Task";
}
