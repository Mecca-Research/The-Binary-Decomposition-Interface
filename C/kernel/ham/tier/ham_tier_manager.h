
// ===================================================================
// Phase 5.4: HAM Automatic Tier Management
// DESC: Automatic promotion and demotion of memory regions
// ===================================================================
#ifndef AEON_HAM_TIER_MANAGER_H
#define AEON_HAM_TIER_MANAGER_H

#include "../../c23_compat.h"
#include "../../ham/ham.h"
#include "../entropy/ham_entropy.h"
#include <stdint.h>

// --- HAM Policy ---
typedef struct {
    float promotion_threshold;
    float demotion_threshold;
    uint64_t access_window;  // Cycles to consider for tier decisions
    bool auto_compression;
} HamPolicy;

// --- Tier Manager ---
typedef struct {
    HamRegion** regions;
    size_t region_count;
    HamPolicy policy;
    AccessPattern** access_patterns;
} HamTierManager;

// --- Tier Manager API ---
[[nodiscard]] HamTierManager* ham_tier_manager_create(HamPolicy policy);
void ham_tier_manager_free(HamTierManager* manager);
[[nodiscard]] int ham_tier_manager_add_region(HamTierManager* manager, HamRegion* region);
[[nodiscard]] int ham_auto_promote(HamTierManager* manager, RegionId region_id);
[[nodiscard]] int ham_auto_demote(HamTierManager* manager, RegionId region_id);
[[nodiscard]] int ham_tier_manager_update(HamTierManager* manager, uint64_t current_cycle);

// --- Utility Functions ---
[[nodiscard]] bool should_promote(const HamRegion* region, const AccessPattern* pattern, const HamPolicy* policy);
[[nodiscard]] bool should_demote(const HamRegion* region, const AccessPattern* pattern, const HamPolicy* policy);

#endif // AEON_HAM_TIER_MANAGER_H
