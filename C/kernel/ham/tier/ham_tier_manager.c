
// ===================================================================
// Phase 5.4: HAM Automatic Tier Management Implementation
// ===================================================================
#include "ham_tier_manager.h"
#include <stdlib.h>
#include <string.h>

// --- Tier Manager ---

HamTierManager* ham_tier_manager_create(HamPolicy policy) {
    HamTierManager* manager = malloc(sizeof(HamTierManager));
    if (!manager) return NULL;
    
    manager->region_count = 0;
    manager->regions = NULL;
    manager->access_patterns = NULL;
    manager->policy = policy;
    
    return manager;
}

void ham_tier_manager_free(HamTierManager* manager) {
    if (!manager) return;
    
    if (manager->access_patterns) {
        for (size_t i = 0; i < manager->region_count; i++) {
            access_pattern_free(manager->access_patterns[i]);
        }
        free(manager->access_patterns);
    }
    
    free(manager->regions);
    free(manager);
}

int ham_tier_manager_add_region(HamTierManager* manager, HamRegion* region) {
    if (!manager || !region) return -1;
    
    // Reallocate arrays
    size_t new_count = manager->region_count + 1;
    
    HamRegion** new_regions = realloc(manager->regions, sizeof(HamRegion*) * new_count);
    if (!new_regions) return -1;
    
    AccessPattern** new_patterns = realloc(manager->access_patterns, sizeof(AccessPattern*) * new_count);
    if (!new_patterns) {
        free(new_regions);
        return -1;
    }
    
    manager->regions = new_regions;
    manager->access_patterns = new_patterns;
    
    manager->regions[manager->region_count] = region;
    manager->access_patterns[manager->region_count] = access_pattern_create(256);
    
    manager->region_count = new_count;
    
    return 0;
}

bool should_promote(const HamRegion* region, const AccessPattern* pattern, const HamPolicy* policy) {
    if (!region || !pattern || !policy) return false;
    
    // Promote if:
    // 1. Access frequency is high (low entropy in access pattern)
    // 2. Current tier is not already CRITICAL
    
    if (region->tier == HAM_CRITICAL) return false;
    
    float access_entropy = pattern->entropy_score;
    
    // Low entropy = regular access pattern = hot data
    if (access_entropy < policy->promotion_threshold) {
        return true;
    }
    
    // High access count
    if (region->stats.access_count > 1000) {
        return true;
    }
    
    return false;
}

bool should_demote(const HamRegion* region, const AccessPattern* pattern, const HamPolicy* policy) {
    if (!region || !pattern || !policy) return false;
    
    // Demote if:
    // 1. Access frequency is low
    // 2. Last access was long ago
    // 3. Current tier is not already ARCHIVE
    
    if (region->tier == HAM_ARCHIVE) return false;
    
    float access_entropy = pattern->entropy_score;
    
    // High entropy = irregular access = cold data
    if (access_entropy > policy->demotion_threshold) {
        return true;
    }
    
    // Low access count
    if (region->stats.access_count < 10) {
        return true;
    }
    
    return false;
}

int ham_auto_promote(HamTierManager* manager, RegionId region_id) {
    if (!manager) return -1;
    
    // Find region
    HamRegion* region = NULL;
    size_t region_idx = 0;
    
    for (size_t i = 0; i < manager->region_count; i++) {
        if (manager->regions[i]->id == region_id) {
            region = manager->regions[i];
            region_idx = i;
            break;
        }
    }
    
    if (!region) return -1;
    
    // Promote tier
    switch (region->tier) {
        case HAM_ARCHIVE:
            region->tier = HAM_DORMANT;
            break;
        case HAM_DORMANT:
            region->tier = HAM_ACTIVE;
            break;
        case HAM_ACTIVE:
            region->tier = HAM_CRITICAL;
            break;
        case HAM_CRITICAL:
            // Already at highest tier
            break;
    }
    
    return 0;
}

int ham_auto_demote(HamTierManager* manager, RegionId region_id) {
    if (!manager) return -1;
    
    // Find region
    HamRegion* region = NULL;
    size_t region_idx = 0;
    
    for (size_t i = 0; i < manager->region_count; i++) {
        if (manager->regions[i]->id == region_id) {
            region = manager->regions[i];
            region_idx = i;
            break;
        }
    }
    
    if (!region) return -1;
    
    // Demote tier
    switch (region->tier) {
        case HAM_CRITICAL:
            region->tier = HAM_ACTIVE;
            break;
        case HAM_ACTIVE:
            region->tier = HAM_DORMANT;
            break;
        case HAM_DORMANT:
            region->tier = HAM_ARCHIVE;
            break;
        case HAM_ARCHIVE:
            // Already at lowest tier
            break;
    }
    
    return 0;
}

int ham_tier_manager_update(HamTierManager* manager, uint64_t current_cycle) {
    if (!manager) return -1;
    
    // Update all regions
    for (size_t i = 0; i < manager->region_count; i++) {
        HamRegion* region = manager->regions[i];
        AccessPattern* pattern = manager->access_patterns[i];
        
        // Record access if recent
        if (region->stats.last_access_cycle + manager->policy.access_window >= current_cycle) {
            access_pattern_record(pattern, region->stats.last_access_cycle);
        }
        
        // Check for promotion
        if (should_promote(region, pattern, &manager->policy)) {
            ham_auto_promote(manager, region->id);
        }
        
        // Check for demotion
        if (should_demote(region, pattern, &manager->policy)) {
            ham_auto_demote(manager, region->id);
        }
    }
    
    return 0;
}
