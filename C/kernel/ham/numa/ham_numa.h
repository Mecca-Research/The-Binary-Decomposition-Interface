
// ===================================================================
// Phase 5.4: HAM NUMA Awareness
// DESC: NUMA-aware memory allocation and migration
// ===================================================================
#ifndef AEON_HAM_NUMA_H
#define AEON_HAM_NUMA_H

#include "../../c23_compat.h"
#include "../../ham/ham.h"
#include <stdint.h>

// --- NUMA Affinity ---
typedef struct {
    uint32_t node_id;
    float affinity_score;  // 0.0 to 1.0
} NumaAffinity;

// --- NUMA Manager ---
typedef struct {
    uint32_t num_nodes;
    NumaAffinity* affinities;
    HamRegion** regions;
    size_t region_count;
} NumaManager;

// --- NUMA API ---
[[nodiscard]] NumaManager* numa_manager_create(uint32_t num_nodes);
void numa_manager_free(NumaManager* manager);
[[nodiscard]] int numa_manager_add_region(NumaManager* manager, HamRegion* region);
[[nodiscard]] float ham_compute_numa_affinity(const HamRegion* region, uint32_t node_id);
[[nodiscard]] int ham_migrate_to_numa_node(HamRegion* region, uint32_t target_node);
[[nodiscard]] int numa_manager_optimize(NumaManager* manager);

// --- Utility Functions ---
[[nodiscard]] uint32_t find_best_numa_node(const NumaManager* manager, const HamRegion* region);
[[nodiscard]] bool should_migrate(const HamRegion* region, uint32_t current_node, uint32_t target_node);

#endif // AEON_HAM_NUMA_H
