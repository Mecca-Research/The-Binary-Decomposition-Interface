
/**
 * @file rers.c
 * @brief Enhanced Runtime Error Replay System (RERS) - Main Implementation
 */

#include "rers.h"
#include "rers_replay.h"
#include "rers_learning.h"
#include "rers_patterns.h"
#include "rers_integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief RERS system structure
 */
struct rers_system {
    rers_config_t config;
    rers_stats_t stats;
    rers_replay_engine_t *replay_engine;
    rers_learning_system_t *learning_system;
    rers_pattern_db_t *pattern_db;
    rers_integration_layer_t *integration_layer;
    bool initialized;
};

/**
 * @brief Default RERS configuration
 */
static const rers_config_t default_config = {
    .enable_replay = true,
    .enable_learning = true,
    .enable_patterns = true,
    .enable_integration = true,
    .max_patterns = RERS_MAX_PATTERNS,
    .max_replay_depth = 10,
    .learning_threshold = 5
};

/* Initialize RERS system */
rers_error_t rers_init(const rers_config_t *config, rers_system_t **system) {
    if (!system) {
        return RERS_ERROR_INVALID_PARAM;
    }

    /* Allocate system structure */
    rers_system_t *sys = calloc(1, sizeof(rers_system_t));
    if (!sys) {
        return RERS_ERROR_NO_MEMORY;
    }

    /* Use provided config or defaults */
    if (config) {
        sys->config = *config;
    } else {
        sys->config = default_config;
    }

    /* Initialize statistics */
    memset(&sys->stats, 0, sizeof(rers_stats_t));

    /* Initialize replay engine */
    if (sys->config.enable_replay) {
        rers_replay_config_t replay_cfg = {
            .max_depth = sys->config.max_replay_depth,
            .enable_segfault = true,
            .enable_assertion = true,
            .enable_memory_leak = true,
            .enable_logic_error = true
        };
        
        if (rers_replay_init(&replay_cfg, &sys->replay_engine) != RERS_SUCCESS) {
            free(sys);
            return RERS_ERROR_COMPONENT_FAILED;
        }
    }

    /* Initialize learning system */
    if (sys->config.enable_learning) {
        rers_learning_config_t learning_cfg = {
            .max_hierarchy_depth = 5,
            .priority_levels = 4,
            .learning_threshold = sys->config.learning_threshold
        };
        
        if (rers_learning_init(&learning_cfg, &sys->learning_system) != RERS_SUCCESS) {
            if (sys->replay_engine) {
                rers_replay_shutdown(sys->replay_engine);
            }
            free(sys);
            return RERS_ERROR_COMPONENT_FAILED;
        }
    }

    /* Initialize pattern database */
    if (sys->config.enable_patterns) {
        rers_pattern_config_t pattern_cfg = {
            .max_patterns = sys->config.max_patterns,
            .enable_fuzzy_match = true
        };
        
        if (rers_pattern_init(&pattern_cfg, &sys->pattern_db) != RERS_SUCCESS) {
            if (sys->learning_system) {
                rers_learning_shutdown(sys->learning_system);
            }
            if (sys->replay_engine) {
                rers_replay_shutdown(sys->replay_engine);
            }
            free(sys);
            return RERS_ERROR_COMPONENT_FAILED;
        }
    }

    /* Initialize integration layer */
    if (sys->config.enable_integration) {
        rers_integration_config_t integration_cfg = {
            .max_profiles = RERS_MAX_PROFILES,
            .enable_coordination = true
        };
        
        if (rers_integration_init(&integration_cfg, &sys->integration_layer) != RERS_SUCCESS) {
            if (sys->pattern_db) {
                rers_pattern_shutdown(sys->pattern_db);
            }
            if (sys->learning_system) {
                rers_learning_shutdown(sys->learning_system);
            }
            if (sys->replay_engine) {
                rers_replay_shutdown(sys->replay_engine);
            }
            free(sys);
            return RERS_ERROR_COMPONENT_FAILED;
        }
    }

    sys->initialized = true;
    *system = sys;
    return RERS_SUCCESS;
}

/* Shutdown RERS system */
void rers_shutdown(rers_system_t *system) {
    if (!system) {
        return;
    }

    if (system->integration_layer) {
        rers_integration_shutdown(system->integration_layer);
    }
    if (system->pattern_db) {
        rers_pattern_shutdown(system->pattern_db);
    }
    if (system->learning_system) {
        rers_learning_shutdown(system->learning_system);
    }
    if (system->replay_engine) {
        rers_replay_shutdown(system->replay_engine);
    }

    system->initialized = false;
    free(system);
}

/* Get RERS statistics */
rers_error_t rers_get_stats(rers_system_t *system, rers_stats_t *stats) {
    if (!system || !stats) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (!system->initialized) {
        return RERS_ERROR_NOT_INITIALIZED;
    }

    *stats = system->stats;
    return RERS_SUCCESS;
}

/* Reset RERS statistics */
rers_error_t rers_reset_stats(rers_system_t *system) {
    if (!system) {
        return RERS_ERROR_INVALID_PARAM;
    }

    if (!system->initialized) {
        return RERS_ERROR_NOT_INITIALIZED;
    }

    memset(&system->stats, 0, sizeof(rers_stats_t));
    return RERS_SUCCESS;
}

/* Get RERS version string */
const char *rers_get_version(void) {
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d",
             RERS_VERSION_MAJOR, RERS_VERSION_MINOR, RERS_VERSION_PATCH);
    return version;
}

/* Get error description string */
const char *rers_get_error_string(rers_error_t error) {
    static const char *error_strings[] = {
        "Success",
        "Invalid parameter",
        "Memory allocation failed",
        "RERS not initialized",
        "RERS already initialized",
        "Component operation failed",
        "Pattern not found",
        "Replay operation failed"
    };

    if (error >= 0 && error < RERS_ERROR_MAX) {
        return error_strings[error];
    }
    return "Unknown error";
}
