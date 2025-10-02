
/**
 * @file attention.c
 * @brief Attention-guided allocation implementation
 */

#include "attention.h"
#include "numa_topology.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Global attention state
static attention_class_stats_t g_class_stats[ATTENTION_MAX_CLASSES] = {0};
static attention_config_t g_config = {0};
static bool g_initialized = false;

/**
 * @brief Get current timestamp in milliseconds
 */
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int attention_init(const attention_config_t* config) {
    if (g_initialized) {
        return 0;
    }
    
    // Set default configuration
    g_config.migration_threshold = ATTENTION_MIGRATION_THRESHOLD;
    g_config.migration_cooldown = 1000;  // 1 second
    g_config.migration_cost_factor = 0.2;
    g_config.ema_alpha = ATTENTION_EMA_ALPHA;
    g_config.enable_migration = true;
    
    // Override with user config
    if (config) {
        g_config = *config;
    }
    
    // Initialize statistics
    memset(g_class_stats, 0, sizeof(g_class_stats));
    
    // Set initial preferred nodes to local node
    numa_topology_t* topo = numa_topology_get();
    if (topo) {
        for (uint32_t i = 0; i < ATTENTION_MAX_CLASSES; i++) {
            g_class_stats[i].preferred_node = 0;  // Default to node 0
        }
    }
    
    g_initialized = true;
    return 0;
}

void attention_record_access(uint32_t class_id, uint32_t node) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES) {
        return;
    }
    
    numa_topology_t* topo = numa_topology_get();
    if (!topo || node >= topo->num_nodes) {
        return;
    }
    
    attention_class_stats_t* stats = &g_class_stats[class_id];
    
    // Increment access count
    stats->access_count[node]++;
    
    // Update affinity score using exponential moving average
    // affinity[node] = alpha * 1.0 + (1 - alpha) * affinity[node]
    // affinity[other] = (1 - alpha) * affinity[other]
    
    double alpha = g_config.ema_alpha;
    
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        if (i == node) {
            stats->affinity_score[i] = alpha + (1.0 - alpha) * stats->affinity_score[i];
        } else {
            stats->affinity_score[i] = (1.0 - alpha) * stats->affinity_score[i];
        }
    }
}

int attention_get_preferred_node(uint32_t class_id) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES) {
        return -1;
    }
    
    attention_class_stats_t* stats = &g_class_stats[class_id];
    
    // Find node with highest affinity score
    numa_topology_t* topo = numa_topology_get();
    if (!topo) {
        return 0;
    }
    
    uint32_t best_node = 0;
    double best_score = stats->affinity_score[0];
    
    for (uint32_t i = 1; i < topo->num_nodes; i++) {
        if (stats->affinity_score[i] > best_score) {
            best_score = stats->affinity_score[i];
            best_node = i;
        }
    }
    
    stats->preferred_node = best_node;
    return best_node;
}

bool attention_should_migrate(uint32_t class_id, uint32_t current_node, uint32_t* target_node) {
    if (!g_initialized || !g_config.enable_migration || 
        class_id >= ATTENTION_MAX_CLASSES || !target_node) {
        return false;
    }
    
    attention_class_stats_t* stats = &g_class_stats[class_id];
    numa_topology_t* topo = numa_topology_get();
    
    if (!topo || current_node >= topo->num_nodes) {
        return false;
    }
    
    // Check cooldown period
    uint64_t now = get_timestamp_ms();
    if (now - stats->last_migration_time < g_config.migration_cooldown) {
        return false;
    }
    
    // Check if enough accesses have occurred
    uint64_t total_accesses = 0;
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        total_accesses += stats->access_count[i];
    }
    
    if (total_accesses < g_config.migration_threshold) {
        return false;
    }
    
    // Find best target node
    uint32_t best_node = current_node;
    double best_benefit = 0.0;
    
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        if (i == current_node) continue;
        
        // Calculate migration benefit
        // benefit = (affinity[target] - affinity[current]) - migration_cost
        double affinity_gain = stats->affinity_score[i] - stats->affinity_score[current_node];
        
        // Migration cost based on NUMA distance
        int distance = numa_topology_distance(current_node, i);
        double migration_cost = g_config.migration_cost_factor * (distance / 10.0);
        
        double benefit = affinity_gain - migration_cost;
        
        if (benefit > best_benefit) {
            best_benefit = benefit;
            best_node = i;
        }
    }
    
    if (best_node != current_node && best_benefit > 0.1) {
        *target_node = best_node;
        return true;
    }
    
    return false;
}

void attention_record_migration(uint32_t class_id, uint32_t from_node, uint32_t to_node) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES) {
        return;
    }
    
    attention_class_stats_t* stats = &g_class_stats[class_id];
    
    stats->migration_count++;
    stats->last_migration_time = get_timestamp_ms();
    stats->preferred_node = to_node;
    
    // Reset access counts after migration
    memset(stats->access_count, 0, sizeof(stats->access_count));
}

int attention_get_stats(uint32_t class_id, attention_class_stats_t* stats) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES || !stats) {
        return -1;
    }
    
    *stats = g_class_stats[class_id];
    return 0;
}

void attention_reset_stats(uint32_t class_id) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES) {
        return;
    }
    
    memset(&g_class_stats[class_id], 0, sizeof(attention_class_stats_t));
}

void attention_print_stats(uint32_t class_id) {
    if (!g_initialized || class_id >= ATTENTION_MAX_CLASSES) {
        return;
    }
    
    attention_class_stats_t* stats = &g_class_stats[class_id];
    numa_topology_t* topo = numa_topology_get();
    
    if (!topo) {
        return;
    }
    
    printf("Attention Statistics (Class %u):\n", class_id);
    printf("  Preferred Node: %u\n", stats->preferred_node);
    printf("  Migrations: %lu\n", stats->migration_count);
    printf("\n");
    
    printf("  Access Counts:\n");
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        printf("    Node %u: %lu\n", i, stats->access_count[i]);
    }
    printf("\n");
    
    printf("  Affinity Scores:\n");
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        printf("    Node %u: %.4f\n", i, stats->affinity_score[i]);
    }
}

void attention_destroy(void) {
    memset(g_class_stats, 0, sizeof(g_class_stats));
    memset(&g_config, 0, sizeof(g_config));
    g_initialized = false;
}
