
// ===================================================================
// Phase 5.4: HAM NUMA Awareness Implementation
// ===================================================================
#include "ham_numa.h"
#include <stdlib.h>
#include <string.h>

// --- NUMA Manager ---

NumaManager* numa_manager_create(uint32_t num_nodes) {
    if (num_nodes == 0) return NULL;
    
    NumaManager* manager = malloc(sizeof(NumaManager));
    if (!manager) return NULL;
    
    manager->num_nodes = num_nodes;
    manager->region_count = 0;
    manager->regions = NULL;
    
    manager->affinities = malloc(sizeof(NumaAffinity) * num_nodes);
    if (!manager->affinities) {
        free(manager);
        return NULL;
    }
    
    // Initialize affinities
    for (uint32_t i = 0; i < num_nodes; i++) {
        manager->affinities[i].node_id = i;
        manager->affinities[i].affinity_score = 1.0f / (float)num_nodes;
    }
    
    return manager;
}

void numa_manager_free(NumaManager* manager) {
    if (!manager) return;
    
    free(manager->affinities);
    free(manager->regions);
    free(manager);
}

int numa_manager_add_region(NumaManager* manager, HamRegion* region) {
    if (!manager || !region) return -1;
    
    size_t new_count = manager->region_count + 1;
    HamRegion** new_regions = realloc(manager->regions, sizeof(HamRegion*) * new_count);
    
    if (!new_regions) return -1;
    
    manager->regions = new_regions;
    manager->regions[manager->region_count] = region;
    manager->region_count = new_count;
    
    return 0;
}

float ham_compute_numa_affinity(const HamRegion* region, uint32_t node_id) {
    if (!region) return 0.0f;
    
    // Compute affinity based on:
    // 1. Access frequency
    // 2. Memory tier
    // 3. Distance to node
    
    float affinity = 0.0f;
    
    // Higher access count = higher affinity
    affinity += (float)region->stats.access_count / 10000.0f;
    
    // Critical tier = higher affinity
    switch (region->tier) {
        case HAM_CRITICAL:
            affinity += 1.0f;
            break;
        case HAM_ACTIVE:
            affinity += 0.5f;
            break;
        case HAM_DORMANT:
            affinity += 0.2f;
            break;
        case HAM_ARCHIVE:
            affinity += 0.1f;
            break;
    }
    
    // Normalize
    if (affinity > 1.0f) affinity = 1.0f;
    
    return affinity;
}

uint32_t find_best_numa_node(const NumaManager* manager, const HamRegion* region) {
    if (!manager || !region) return 0;
    
    uint32_t best_node = 0;
    float best_affinity = 0.0f;
    
    for (uint32_t i = 0; i < manager->num_nodes; i++) {
        float affinity = ham_compute_numa_affinity(region, i);
        
        if (affinity > best_affinity) {
            best_affinity = affinity;
            best_node = i;
        }
    }
    
    return best_node;
}

bool should_migrate(const HamRegion* region, uint32_t current_node, uint32_t target_node) {
    if (!region || current_node == target_node) return false;
    
    // Migrate if:
    // 1. Target node has significantly better affinity
    // 2. Region is frequently accessed
    
    float current_affinity = ham_compute_numa_affinity(region, current_node);
    float target_affinity = ham_compute_numa_affinity(region, target_node);
    
    // Require 20% improvement to justify migration cost
    return (target_affinity > current_affinity * 1.2f);
}

int ham_migrate_to_numa_node(HamRegion* region, uint32_t target_node) {
    if (!region) return -1;
    
    // TODO: Implement actual NUMA migration
    // For now, just update metadata
    (void)target_node;
    
    return 0;
}

int numa_manager_optimize(NumaManager* manager) {
    if (!manager) return -1;
    
    // Optimize placement of all regions
    for (size_t i = 0; i < manager->region_count; i++) {
        HamRegion* region = manager->regions[i];
        
        uint32_t best_node = find_best_numa_node(manager, region);
        uint32_t current_node = 0;  // TODO: Track current node
        
        if (should_migrate(region, current_node, best_node)) {
            ham_migrate_to_numa_node(region, best_node);
        }
    }
    
    return 0;
}
