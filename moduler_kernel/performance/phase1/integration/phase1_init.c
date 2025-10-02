
/**
 * @file phase1_init.c
 * @brief Implementation of Phase 1 initialization
 */

#include "phase1_init.h"
#include "../rings/spsc_ring.h"
#include "../rings/mpsc_ring.h"
#include "../fibers/fiber_scheduler.h"
#include "../arena/shared_arena.h"
#include "../ipc/graph_call.h"
#include <stdlib.h>
#include <stdbool.h>

// Global state
static bool g_initialized = false;
static phase1_config_t g_config;
static fiber_scheduler_t** g_schedulers = NULL;
static shared_arena_t* g_arena = NULL;
static graph_call_port_t** g_ports = NULL;

phase1_status_t phase1_init(const phase1_config_t* config) {
    if (g_initialized) {
        return PHASE1_SUCCESS;
    }
    
    if (!config || config->num_cores == 0) {
        return PHASE1_ERROR_INVALID_CONFIG;
    }
    
    g_config = *config;
    
    // Create per-core fiber schedulers
    g_schedulers = calloc(config->num_cores, sizeof(fiber_scheduler_t*));
    if (!g_schedulers) {
        return PHASE1_ERROR_INITIALIZATION;
    }
    
    for (uint32_t i = 0; i < config->num_cores; i++) {
        g_schedulers[i] = fiber_scheduler_create(i);
        if (!g_schedulers[i]) {
            phase1_shutdown();
            return PHASE1_ERROR_INITIALIZATION;
        }
    }
    
    // Create shared arena
    g_arena = shared_arena_create(config->arena_size);
    if (!g_arena) {
        phase1_shutdown();
        return PHASE1_ERROR_INITIALIZATION;
    }
    
    // Create per-core graph call ports
    g_ports = calloc(config->num_cores, sizeof(graph_call_port_t*));
    if (!g_ports) {
        phase1_shutdown();
        return PHASE1_ERROR_INITIALIZATION;
    }
    
    for (uint32_t i = 0; i < config->num_cores; i++) {
        g_ports[i] = graph_call_port_create(i, config->ring_capacity);
        if (!g_ports[i]) {
            phase1_shutdown();
            return PHASE1_ERROR_INITIALIZATION;
        }
    }
    
    g_initialized = true;
    return PHASE1_SUCCESS;
}

phase1_status_t phase1_shutdown(void) {
    if (!g_initialized) {
        return PHASE1_ERROR_NOT_INITIALIZED;
    }
    
    // Destroy graph call ports
    if (g_ports) {
        for (uint32_t i = 0; i < g_config.num_cores; i++) {
            if (g_ports[i]) {
                graph_call_port_destroy(g_ports[i]);
            }
        }
        free(g_ports);
        g_ports = NULL;
    }
    
    // Destroy shared arena
    if (g_arena) {
        shared_arena_destroy(g_arena);
        g_arena = NULL;
    }
    
    // Destroy fiber schedulers
    if (g_schedulers) {
        for (uint32_t i = 0; i < g_config.num_cores; i++) {
            if (g_schedulers[i]) {
                fiber_scheduler_destroy(g_schedulers[i]);
            }
        }
        free(g_schedulers);
        g_schedulers = NULL;
    }
    
    g_initialized = false;
    return PHASE1_SUCCESS;
}

bool phase1_is_initialized(void) {
    return g_initialized;
}

phase1_config_t phase1_get_default_config(void) {
    phase1_config_t config = {
        .num_cores = 4,
        .ring_capacity = 1024,
        .arena_size = 64 * 1024 * 1024,  // 64MB
        .fiber_stack_size = 64 * 1024,    // 64KB
        .enable_performance_counters = true
    };
    return config;
}
