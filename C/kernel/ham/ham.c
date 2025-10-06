
/**
 * @file ham.c
 * @brief Hierarchical Adaptive Memory Implementation for BDI Kernel
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#include "ham.h"
#include "../pmm.h"
#include "../vmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ============================================================================
// Global State
// ============================================================================

static HamRegion g_ham_regions[HAM_MAX_REGIONS] = {0};
static atomic_uint_fast32_t g_region_count = 0;
static HamGlobalStats g_ham_stats = {0};
static MotifDictionary g_motif_dict = {0};
static bool g_ham_initialized = false;
static atomic_flag g_ham_lock = ATOMIC_FLAG_INIT;

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Get current timestamp
 */
static inline uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/**
 * @brief Calculate entropy of data
 */
static float calculate_entropy(const void *data, size_t size) {
    if (data == nullptr || size == 0) {
        return 0.0f;
    }
    
    uint32_t counts[256] = {0};
    const uint8_t *bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i++) {
        counts[bytes[i]]++;
    }
    
    float entropy = 0.0f;
    for (uint32_t i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            float p = (float)counts[i] / size;
            entropy -= p * log2f(p);
        }
    }
    
    return entropy / 8.0f;  // Normalize to 0-1 range
}

/**
 * @brief Find region by ID
 */
static HamRegion* find_region(RegionId id) {
    if (id == 0) {
        return nullptr;
    }
    
    for (uint32_t i = 0; i < HAM_MAX_REGIONS; i++) {
        if (g_ham_regions[i].id == id) {
            return &g_ham_regions[i];
        }
    }
    
    return nullptr;
}

/**
 * @brief Lock region
 */
static inline void region_lock(HamRegion *region) {
    while (atomic_flag_test_and_set(&region->lock)) {
        // Spin wait
    }
}

/**
 * @brief Unlock region
 */
static inline void region_unlock(HamRegion *region) {
    atomic_flag_clear(&region->lock);
}

// ============================================================================
// HAM Initialization
// ============================================================================

int ham_init(void) {
    if (g_ham_initialized) {
        return 0;
    }
    
    printf("HAM: Initializing Hierarchical Adaptive Memory...\n");
    
    // Initialize motif dictionary
    motif_dict_init(&g_motif_dict);
    
    // Initialize regions
    memset(g_ham_regions, 0, sizeof(g_ham_regions));
    atomic_store(&g_region_count, 0);
    
    // Initialize statistics
    memset(&g_ham_stats, 0, sizeof(g_ham_stats));
    
    g_ham_initialized = true;
    
    printf("HAM: Initialization complete\n");
    return 0;
}

void ham_shutdown(void) {
    if (!g_ham_initialized) {
        return;
    }
    
    printf("HAM: Shutting down...\n");
    
    // Free all regions
    for (uint32_t i = 0; i < HAM_MAX_REGIONS; i++) {
        if (g_ham_regions[i].id != 0) {
            ham_free(g_ham_regions[i].id);
        }
    }
    
    // Free motif dictionary
    motif_dict_free(&g_motif_dict);
    
    g_ham_initialized = false;
    
    printf("HAM: Shutdown complete\n");
}

// ============================================================================
// Region Allocation and Management
// ============================================================================

int ham_alloc(RegionId *out_id, HamTier tier, size_t size_bytes, void **out_ptr) {
    if (out_id == nullptr || size_bytes == 0 || 
        size_bytes < HAM_REGION_MIN_SIZE || size_bytes > HAM_REGION_MAX_SIZE) {
        return -1;
    }
    
    if (tier >= HAM_TIER_COUNT) {
        return -1;
    }
    
    // Find free slot
    uint32_t slot = HAM_MAX_REGIONS;
    for (uint32_t i = 0; i < HAM_MAX_REGIONS; i++) {
        if (g_ham_regions[i].id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot >= HAM_MAX_REGIONS) {
        printf("HAM: No free region slots\n");
        return -1;
    }
    
    HamRegion *region = &g_ham_regions[slot];
    
    // Allocate region ID
    RegionId id = atomic_fetch_add(&g_region_count, 1) + 1;
    
    // Initialize region
    region->id = id;
    region->tier = tier;
    region->capacity_bytes = size_bytes;
    region->used_bytes = 0;
    region->numa_node = numa_get_current_node();
    atomic_flag_clear(&region->lock);
    
    // Allocate backing memory based on tier
    if (tier == HAM_CRITICAL || tier == HAM_ACTIVE) {
        // Allocate from PMM
        uint32_t num_pages = (size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
        region->pages = pmm_alloc_pages(GFP_KERNEL | GFP_ZERO, 0);
        
        if (region->pages == nullptr) {
            printf("HAM: Failed to allocate pages for region\n");
            region->id = 0;
            return -1;
        }
        
        region->base = region->pages->virtual_addr;
        region->num_pages = num_pages;
    } else if (tier == HAM_DORMANT) {
        // Allocate compressed storage
        region->base = calloc(1, size_bytes);
        if (region->base == nullptr) {
            region->id = 0;
            return -1;
        }
        region->num_pages = 0;
    } else {
        // ARCHIVE tier - no memory allocated yet
        region->base = nullptr;
        region->num_pages = 0;
    }
    
    // Set archive path
    snprintf(region->path, HAM_REGION_PATH_MAX, 
             "/tmp/ham_region_%llu.bin", (unsigned long long)id);
    
    // Initialize statistics
    atomic_store(&region->stats.access_count, 0);
    atomic_store(&region->stats.read_count, 0);
    atomic_store(&region->stats.write_count, 0);
    region->stats.last_access_cycle = get_timestamp();
    region->stats.creation_time = get_timestamp();
    region->stats.entropy_score = 0.0f;
    region->stats.compression_ratio = 1.0f;
    
    // Initialize state
    region->persistent = false;
    region->compressed = false;
    region->pinned = (tier == HAM_CRITICAL);
    region->zero_copy_enabled = false;
    region->interned_motif = nullptr;
    
    // Update global statistics
    atomic_fetch_add(&g_ham_stats.total_regions, 1);
    atomic_fetch_add(&g_ham_stats.active_regions, 1);
    atomic_fetch_add(&g_ham_stats.total_memory, size_bytes);
    atomic_fetch_add(&g_ham_stats.tier_regions[tier], 1);
    atomic_fetch_add(&g_ham_stats.tier_memory[tier], size_bytes);
    
    *out_id = id;
    if (out_ptr != nullptr) {
        *out_ptr = region->base;
    }
    
    printf("HAM: Allocated region %llu (%zu bytes) in tier %d on NUMA node %u\n",
           (unsigned long long)id, size_bytes, tier, region->numa_node);
    
    return 0;
}

int ham_free(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    // Release motif if interned
    if (region->interned_motif != nullptr) {
        motif_dict_release(&g_motif_dict, region->interned_motif->hash);
        region->interned_motif = nullptr;
    }
    
    // Free backing memory
    if (region->pages != nullptr) {
        pmm_free_pages(region->pages, 0);
        region->pages = nullptr;
    } else if (region->base != nullptr) {
        free(region->base);
    }
    
    region->base = nullptr;
    
    // Update statistics
    atomic_fetch_sub(&g_ham_stats.active_regions, 1);
    atomic_fetch_sub(&g_ham_stats.total_memory, region->capacity_bytes);
    atomic_fetch_sub(&g_ham_stats.tier_regions[region->tier], 1);
    atomic_fetch_sub(&g_ham_stats.tier_memory[region->tier], region->capacity_bytes);
    
    region_unlock(region);
    
    // Clear region
    region->id = 0;
    
    printf("HAM: Freed region %llu\n", (unsigned long long)id);
    return 0;
}

int ham_resize(RegionId id, size_t new_size) {
    HamRegion *region = find_region(id);
    if (region == nullptr || new_size == 0) {
        return -1;
    }
    
    region_lock(region);
    
    if (new_size == region->capacity_bytes) {
        region_unlock(region);
        return 0;
    }
    
    // Simplified resize - reallocate
    void *new_base = realloc(region->base, new_size);
    if (new_base == nullptr) {
        region_unlock(region);
        return -1;
    }
    
    size_t old_size = region->capacity_bytes;
    region->base = new_base;
    region->capacity_bytes = new_size;
    
    // Update statistics
    if (new_size > old_size) {
        atomic_fetch_add(&g_ham_stats.total_memory, new_size - old_size);
        atomic_fetch_add(&g_ham_stats.tier_memory[region->tier], new_size - old_size);
    } else {
        atomic_fetch_sub(&g_ham_stats.total_memory, old_size - new_size);
        atomic_fetch_sub(&g_ham_stats.tier_memory[region->tier], old_size - new_size);
    }
    
    region_unlock(region);
    
    printf("HAM: Resized region %llu from %zu to %zu bytes\n",
           (unsigned long long)id, old_size, new_size);
    
    return 0;
}

// ============================================================================
// Persistence Operations
// ============================================================================

int ham_persist(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr || region->base == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    FILE *f = fopen(region->path, "wb");
    if (f == nullptr) {
        perror("HAM: Failed to open file for persistence");
        region_unlock(region);
        return -1;
    }
    
    size_t written = fwrite(region->base, 1, region->capacity_bytes, f);
    fclose(f);
    
    if (written != region->capacity_bytes) {
        printf("HAM: Incomplete write during persistence\n");
        region_unlock(region);
        return -1;
    }
    
    region->persistent = true;
    atomic_fetch_add(&region->stats.persist_count, 1);
    
    region_unlock(region);
    
    printf("HAM: Persisted region %llu to %s\n", 
           (unsigned long long)id, region->path);
    
    return 0;
}

int ham_load(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    // Allocate memory if not present
    if (region->base == nullptr) {
        region->base = malloc(region->capacity_bytes);
        if (region->base == nullptr) {
            region_unlock(region);
            return -1;
        }
    }
    
    FILE *f = fopen(region->path, "rb");
    if (f == nullptr) {
        // File doesn't exist yet - not an error
        region_unlock(region);
        return 0;
    }
    
    size_t read = fread(region->base, 1, region->capacity_bytes, f);
    fclose(f);
    
    if (read != region->capacity_bytes) {
        printf("HAM: Incomplete read during load\n");
    }
    
    atomic_fetch_add(&region->stats.load_count, 1);
    
    region_unlock(region);
    
    printf("HAM: Loaded region %llu from %s\n",
           (unsigned long long)id, region->path);
    
    return 0;
}

// ============================================================================
// Intelligence and Lifecycle Operations
// ============================================================================

int ham_update_stats(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    atomic_fetch_add(&region->stats.access_count, 1);
    region->stats.last_access_cycle = get_timestamp();
    
    // Calculate entropy if data present
    if (region->base != nullptr && region->capacity_bytes > 0) {
        region->stats.entropy_score = calculate_entropy(region->base, 
                                                        region->capacity_bytes);
    }
    
    region_unlock(region);
    
    return 0;
}

int ham_promote(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    HamTier old_tier = region->tier;
    HamTier new_tier = old_tier;
    
    // Determine promotion target
    if (old_tier == HAM_ARCHIVE) {
        new_tier = HAM_DORMANT;
    } else if (old_tier == HAM_DORMANT) {
        new_tier = HAM_ACTIVE;
    } else if (old_tier == HAM_ACTIVE) {
        new_tier = HAM_CRITICAL;
    } else {
        region_unlock(region);
        return 0;  // Already at highest tier
    }
    
    // Perform promotion
    region->tier = new_tier;
    atomic_fetch_add(&region->stats.promotion_count, 1);
    atomic_fetch_add(&g_ham_stats.promotions, 1);
    
    // Update tier statistics
    atomic_fetch_sub(&g_ham_stats.tier_regions[old_tier], 1);
    atomic_fetch_sub(&g_ham_stats.tier_memory[old_tier], region->capacity_bytes);
    atomic_fetch_add(&g_ham_stats.tier_regions[new_tier], 1);
    atomic_fetch_add(&g_ham_stats.tier_memory[new_tier], region->capacity_bytes);
    
    region_unlock(region);
    
    printf("HAM: Promoted region %llu from tier %d to tier %d\n",
           (unsigned long long)id, old_tier, new_tier);
    
    return 0;
}

int ham_demote(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr || region->pinned) {
        return -1;
    }
    
    region_lock(region);
    
    HamTier old_tier = region->tier;
    HamTier new_tier = old_tier;
    
    // Determine demotion target
    if (old_tier == HAM_CRITICAL) {
        new_tier = HAM_ACTIVE;
    } else if (old_tier == HAM_ACTIVE) {
        new_tier = HAM_DORMANT;
    } else if (old_tier == HAM_DORMANT) {
        new_tier = HAM_ARCHIVE;
    } else {
        region_unlock(region);
        return 0;  // Already at lowest tier
    }
    
    // Perform demotion
    region->tier = new_tier;
    atomic_fetch_add(&region->stats.demotion_count, 1);
    atomic_fetch_add(&g_ham_stats.demotions, 1);
    
    // Update tier statistics
    atomic_fetch_sub(&g_ham_stats.tier_regions[old_tier], 1);
    atomic_fetch_sub(&g_ham_stats.tier_memory[old_tier], region->capacity_bytes);
    atomic_fetch_add(&g_ham_stats.tier_regions[new_tier], 1);
    atomic_fetch_add(&g_ham_stats.tier_memory[new_tier], region->capacity_bytes);
    
    // Persist if demoting to archive
    if (new_tier == HAM_ARCHIVE) {
        ham_persist(id);
    }
    
    region_unlock(region);
    
    printf("HAM: Demoted region %llu from tier %d to tier %d\n",
           (unsigned long long)id, old_tier, new_tier);
    
    return 0;
}

int ham_demote_check(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr || region->pinned) {
        return -1;
    }
    
    region_lock(region);
    
    uint64_t access_count = atomic_load(&region->stats.access_count);
    bool should_demote = false;
    
    // Policy: demote based on access patterns
    if (region->tier == HAM_ACTIVE && access_count < HAM_COLD_THRESHOLD) {
        should_demote = true;
    } else if (region->tier == HAM_DORMANT && access_count < HAM_DORMANT_THRESHOLD) {
        should_demote = true;
    }
    
    region_unlock(region);
    
    if (should_demote) {
        return ham_demote(id);
    }
    
    return 0;
}

// ============================================================================
// Compression and Deduplication
// ============================================================================

int ham_compress(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr || region->base == nullptr || region->compressed) {
        return -1;
    }
    
    region_lock(region);
    
    // Simple compression simulation (in real implementation, use zlib/lz4)
    size_t compressed_size = region->capacity_bytes / 2;  // Assume 50% compression
    void *compressed_data = malloc(compressed_size);
    
    if (compressed_data == nullptr) {
        region_unlock(region);
        return -1;
    }
    
    // Simulate compression
    memcpy(compressed_data, region->base, compressed_size);
    
    // Replace original data
    free(region->base);
    region->base = compressed_data;
    region->compressed = true;
    region->stats.compression_ratio = (float)compressed_size / region->capacity_bytes;
    
    atomic_fetch_add(&g_ham_stats.compressions, 1);
    atomic_fetch_add(&g_ham_stats.compressed_memory, compressed_size);
    
    region_unlock(region);
    
    printf("HAM: Compressed region %llu (ratio: %.2f)\n",
           (unsigned long long)id, region->stats.compression_ratio);
    
    return 0;
}

int ham_decompress(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr || !region->compressed) {
        return -1;
    }
    
    region_lock(region);
    
    // Decompress (simplified)
    void *decompressed_data = malloc(region->capacity_bytes);
    if (decompressed_data == nullptr) {
        region_unlock(region);
        return -1;
    }
    
    // Simulate decompression
    size_t compressed_size = region->capacity_bytes * region->stats.compression_ratio;
    memcpy(decompressed_data, region->base, compressed_size);
    
    free(region->base);
    region->base = decompressed_data;
    region->compressed = false;
    
    atomic_fetch_add(&g_ham_stats.decompressions, 1);
    atomic_fetch_sub(&g_ham_stats.compressed_memory, compressed_size);
    
    region_unlock(region);
    
    printf("HAM: Decompressed region %llu\n", (unsigned long long)id);
    
    return 0;
}

int ham_intern_check(RegionId id, MotifDictionary *dict) {
    HamRegion *region = find_region(id);
    if (region == nullptr || region->base == nullptr || dict == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    atomic_fetch_add(&g_ham_stats.dedup_checks, 1);
    
    // Only intern DORMANT regions
    if (region->tier != HAM_DORMANT || region->interned_motif != nullptr) {
        region_unlock(region);
        return 0;
    }
    
    printf("HAM: Checking region %llu for deduplication\n", (unsigned long long)id);
    
    Motif *motif = motif_dict_intern(dict, region->base, region->capacity_bytes);
    if (motif != nullptr && motif->ref_count > 1) {
        // Found duplicate!
        printf("HAM: Found duplicate! Deduplicating region %llu\n", 
               (unsigned long long)id);
        
        free(region->base);
        region->base = motif->data;
        region->interned_motif = motif;
        
        atomic_fetch_add(&g_ham_stats.dedup_hits, 1);
        atomic_fetch_add(&g_ham_stats.dedup_saved_bytes, region->capacity_bytes);
    } else if (motif != nullptr) {
        region->interned_motif = motif;
    }
    
    region_unlock(region);
    
    return 0;
}

// ============================================================================
// Zero-Copy Integration
// ============================================================================

int ham_enable_zero_copy(RegionId id) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    region->zero_copy_enabled = true;
    region_unlock(region);
    
    printf("HAM: Enabled zero-copy for region %llu\n", (unsigned long long)id);
    return 0;
}

int ham_handoff_to_scheduler(RegionId id, void *scheduler_ctx) {
    HamRegion *region = find_region(id);
    if (region == nullptr || !region->zero_copy_enabled) {
        return -1;
    }
    
    region_lock(region);
    region->scheduler_data = scheduler_ctx;
    region_unlock(region);
    
    printf("HAM: Handed off region %llu to scheduler (zero-copy)\n",
           (unsigned long long)id);
    
    return 0;
}

// ============================================================================
// NUMA Operations
// ============================================================================

int ham_migrate_numa(RegionId id, uint32_t target_node) {
    HamRegion *region = find_region(id);
    if (region == nullptr) {
        return -1;
    }
    
    region_lock(region);
    
    uint32_t old_node = region->numa_node;
    if (old_node == target_node) {
        region_unlock(region);
        return 0;
    }
    
    // In real implementation, would migrate pages to target node
    region->numa_node = target_node;
    
    region_unlock(region);
    
    printf("HAM: Migrated region %llu from NUMA node %u to %u\n",
           (unsigned long long)id, old_node, target_node);
    
    return 0;
}

int ham_get_numa_node(RegionId id, uint32_t *out_node) {
    HamRegion *region = find_region(id);
    if (region == nullptr || out_node == nullptr) {
        return -1;
    }
    
    *out_node = region->numa_node;
    return 0;
}

// ============================================================================
// Query Functions
// ============================================================================

HamTier ham_get_region_tier(RegionId id) {
    HamRegion *region = find_region(id);
    return region != nullptr ? region->tier : (HamTier)-1;
}

void* ham_get_region_base(RegionId id) {
    HamRegion *region = find_region(id);
    return region != nullptr ? region->base : nullptr;
}

int ham_get_region_stats(RegionId id, HamStats *out_stats) {
    HamRegion *region = find_region(id);
    if (region == nullptr || out_stats == nullptr) {
        return -1;
    }
    
    region_lock(region);
    memcpy(out_stats, &region->stats, sizeof(HamStats));
    region_unlock(region);
    
    return 0;
}

const HamGlobalStats* ham_get_global_stats(void) {
    return &g_ham_stats;
}

// ============================================================================
// Maintenance and Diagnostics
// ============================================================================

void ham_maintenance(void) {
    printf("HAM: Running maintenance...\n");
    
    uint32_t promoted = 0;
    uint32_t demoted = 0;
    uint32_t compressed = 0;
    
    for (uint32_t i = 0; i < HAM_MAX_REGIONS; i++) {
        HamRegion *region = &g_ham_regions[i];
        
        if (region->id == 0) {
            continue;
        }
        
        // Update statistics
        ham_update_stats(region->id);
        
        // Check for promotion
        uint64_t access_count = atomic_load(&region->stats.access_count);
        if (access_count > HAM_HOT_THRESHOLD && region->tier < HAM_CRITICAL) {
            if (ham_promote(region->id) == 0) {
                promoted++;
            }
        }
        
        // Check for demotion
        if (ham_demote_check(region->id) == 0) {
            demoted++;
        }
        
        // Check for compression
        if (region->tier == HAM_DORMANT && !region->compressed &&
            region->stats.entropy_score < HAM_LOW_ENTROPY) {
            if (ham_compress(region->id) == 0) {
                compressed++;
            }
        }
        
        // Check for deduplication
        if (region->tier == HAM_DORMANT && region->interned_motif == nullptr) {
            ham_intern_check(region->id, &g_motif_dict);
        }
    }
    
    printf("HAM: Maintenance complete (promoted: %u, demoted: %u, compressed: %u)\n",
           promoted, demoted, compressed);
}

void ham_print_stats(void) {
    printf("\n=== HAM Statistics ===\n");
    printf("Total Regions:  %llu\n", atomic_load(&g_ham_stats.total_regions));
    printf("Active Regions: %llu\n", atomic_load(&g_ham_stats.active_regions));
    printf("Total Memory:   %llu MB\n", 
           atomic_load(&g_ham_stats.total_memory) / (1024 * 1024));
    printf("Compressed:     %llu MB\n",
           atomic_load(&g_ham_stats.compressed_memory) / (1024 * 1024));
    
    printf("\nPer-Tier Statistics:\n");
    const char *tier_names[] = {"CRITICAL", "ACTIVE", "DORMANT", "ARCHIVE"};
    for (uint32_t i = 0; i < HAM_TIER_COUNT; i++) {
        printf("  %s: %llu regions, %llu MB\n",
               tier_names[i],
               atomic_load(&g_ham_stats.tier_regions[i]),
               atomic_load(&g_ham_stats.tier_memory[i]) / (1024 * 1024));
    }
    
    printf("\nLifecycle Operations:\n");
    printf("  Promotions:     %llu\n", atomic_load(&g_ham_stats.promotions));
    printf("  Demotions:      %llu\n", atomic_load(&g_ham_stats.demotions));
    printf("  Compressions:   %llu\n", atomic_load(&g_ham_stats.compressions));
    printf("  Decompressions: %llu\n", atomic_load(&g_ham_stats.decompressions));
    
    printf("\nDeduplication:\n");
    printf("  Checks:      %llu\n", atomic_load(&g_ham_stats.dedup_checks));
    printf("  Hits:        %llu\n", atomic_load(&g_ham_stats.dedup_hits));
    printf("  Saved:       %llu MB\n",
           atomic_load(&g_ham_stats.dedup_saved_bytes) / (1024 * 1024));
    
    printf("======================\n\n");
}

bool ham_validate(void) {
    if (!g_ham_initialized) {
        return false;
    }
    
    // Validate region consistency
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < HAM_MAX_REGIONS; i++) {
        if (g_ham_regions[i].id != 0) {
            active_count++;
        }
    }
    
    if (active_count != atomic_load(&g_ham_stats.active_regions)) {
        printf("HAM: Validation failed - region count mismatch\n");
        return false;
    }
    
    printf("HAM: Validation passed\n");
    return true;
}

// ============================================================================
// HAM VTable Implementation
// ============================================================================

HamVTable HAM_DEFAULT_IMPL = {
    .alloc = ham_alloc,
    .free = ham_free,
    .resize = ham_resize,
    .persist = ham_persist,
    .load = ham_load,
    .sync = ham_persist,
    .update_stats = ham_update_stats,
    .promote = ham_promote,
    .demote = ham_demote,
    .demote_check = ham_demote_check,
    .compress = ham_compress,
    .decompress = ham_decompress,
    .intern_check = ham_intern_check,
    .enable_zero_copy = ham_enable_zero_copy,
    .handoff_to_scheduler = ham_handoff_to_scheduler,
    .handoff_to_ipc = nullptr,
    .handoff_to_fs = nullptr,
    .migrate_numa = ham_migrate_numa,
    .get_numa_node = ham_get_numa_node
};
