
/**
 * @file fault_injection.c
 * @brief Fault Injection Framework Implementation
 */

#include "fault_injection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// Global State
// ============================================================================

static fault_config_t g_fault_configs[FAULT_MAX] = {0};
static fault_stats_t g_fault_stats = {0};
static uint32_t g_operation_counts[FAULT_MAX] = {0};
static bool g_initialized = false;

static const char *g_fault_names[FAULT_MAX] = {
    "NONE",
    "ALLOC_FAIL",
    "NUMA_FAIL",
    "PAGE_FAULT",
    "OOM",
    "CORRUPTION",
    "DOUBLE_FREE",
    "INVALID_FREE",
    "ALIGNMENT_ERROR",
    "TIMEOUT",
    "DEADLOCK",
    "RACE_CONDITION"
};

// ============================================================================
// Implementation
// ============================================================================

void fault_injection_init(uint32_t seed) {
    if (g_initialized) {
        return;
    }
    
    memset(&g_fault_configs, 0, sizeof(g_fault_configs));
    memset(&g_fault_stats, 0, sizeof(g_fault_stats));
    memset(&g_operation_counts, 0, sizeof(g_operation_counts));
    
    srand(seed);
    g_initialized = true;
}

void fault_injection_cleanup(void) {
    g_initialized = false;
}

bool fault_injection_enable(fault_type_t type, double probability,
                           const char *description) {
    if (type >= FAULT_MAX) {
        return false;
    }
    
    g_fault_configs[type].type = type;
    g_fault_configs[type].probability = probability;
    g_fault_configs[type].enabled = true;
    g_fault_configs[type].description = description;
    g_fault_configs[type].max_faults = UINT32_MAX;
    
    return true;
}

void fault_injection_disable(fault_type_t type) {
    if (type < FAULT_MAX) {
        g_fault_configs[type].enabled = false;
    }
}

void fault_injection_disable_all(void) {
    for (int i = 0; i < FAULT_MAX; i++) {
        g_fault_configs[i].enabled = false;
    }
}

bool fault_injection_should_fail(fault_type_t type) {
    if (!g_initialized || type >= FAULT_MAX) {
        return false;
    }
    
    g_fault_stats.total_checks++;
    
    fault_config_t *config = &g_fault_configs[type];
    if (!config->enabled) {
        return false;
    }
    
    // Check if we've hit max faults
    if (g_fault_stats.faults_by_type[type] >= config->max_faults) {
        return false;
    }
    
    // Check trigger count
    g_operation_counts[type]++;
    if (config->trigger_count > 0 && 
        g_operation_counts[type] < config->trigger_count) {
        return false;
    }
    
    // Probability check
    double random = (double)rand() / RAND_MAX;
    if (random < config->probability) {
        g_fault_stats.faults_injected++;
        g_fault_stats.faults_by_type[type]++;
        g_fault_stats.operations_affected++;
        return true;
    }
    
    return false;
}

void fault_injection_set_trigger(fault_type_t type, uint32_t count) {
    if (type < FAULT_MAX) {
        g_fault_configs[type].trigger_count = count;
    }
}

void fault_injection_set_max_faults(fault_type_t type, uint32_t max) {
    if (type < FAULT_MAX) {
        g_fault_configs[type].max_faults = max;
    }
}

fault_stats_t fault_injection_get_stats(void) {
    return g_fault_stats;
}

void fault_injection_reset_stats(void) {
    memset(&g_fault_stats, 0, sizeof(g_fault_stats));
    memset(&g_operation_counts, 0, sizeof(g_operation_counts));
}

void fault_injection_print_stats(void) {
    printf("\n");
    printf("========================================\n");
    printf("Fault Injection Statistics\n");
    printf("========================================\n");
    printf("Total checks:      %lu\n", g_fault_stats.total_checks);
    printf("Faults injected:   %lu\n", g_fault_stats.faults_injected);
    printf("Operations affected: %lu\n", g_fault_stats.operations_affected);
    printf("\nFaults by type:\n");
    
    for (int i = 0; i < FAULT_MAX; i++) {
        if (g_fault_stats.faults_by_type[i] > 0) {
            printf("  %-20s: %lu\n", g_fault_names[i], 
                   g_fault_stats.faults_by_type[i]);
        }
    }
    
    printf("========================================\n");
}

const char *fault_injection_get_type_name(fault_type_t type) {
    if (type < FAULT_MAX) {
        return g_fault_names[type];
    }
    return "UNKNOWN";
}
