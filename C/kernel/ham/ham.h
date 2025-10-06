
/**
 * @file ham.h
 * @brief Hierarchical Adaptive Memory (HAM) for BDI Kernel
 * @details Implements intelligent memory management with region lifecycle,
 *          promotion/demotion, persistence, and motif deduplication.
 *          Integrates with PMM/VMM for zero-copy handoff to schedulers, IPC, and filesystems.
 * 
 * Phase 1: Memory & HAM Readiness
 * - Region lifecycle management (promotion/demotion)
 * - Persistence layer for HAM regions
 * - Motif deduplication for memory efficiency
 * - Integration with general allocator flow
 * - Zero-copy handoff to higher tiers
 * - NUMA-aware region placement
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#ifndef BDI_KERNEL_HAM_H
#define BDI_KERNEL_HAM_H

#include "../c23_compat.h"
#include "../memory.h"
#include "../motif/motif.h"
#include "graph.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

// ============================================================================
// HAM Configuration
// ============================================================================

#define HAM_MAX_REGIONS         1024
#define HAM_REGION_MIN_SIZE     (4 * 1024)          // 4KB
#define HAM_REGION_MAX_SIZE     (1024 * 1024 * 1024) // 1GB
#define HAM_ARCHIVE_PATH_MAX    256

// Access pattern thresholds
#define HAM_HOT_THRESHOLD       100     // Accesses for promotion
#define HAM_COLD_THRESHOLD      10      // Accesses for demotion
#define HAM_DORMANT_THRESHOLD   5       // Accesses for archival

// Entropy thresholds for compression
#define HAM_HIGH_ENTROPY        0.8f    // High randomness, hard to compress
#define HAM_LOW_ENTROPY         0.3f    // Low randomness, good for compression

// ============================================================================
// HAM Tier Definitions
// ============================================================================

/**
 * @brief HAM memory tiers
 */
typedef enum {
    HAM_CRITICAL,       // Hot working set, pinned in fastest memory
    HAM_ACTIVE,         // Near-term use, general purpose
    HAM_DORMANT,        // Cold, compressible data
    HAM_ARCHIVE,        // Persistent storage
    HAM_TIER_COUNT
} HamTier;

// ============================================================================
// HAM Statistics
// ============================================================================

/**
 * @brief Statistics for a memory region
 */
typedef struct {
    atomic_uint_fast64_t access_count;
    atomic_uint_fast64_t read_count;
    atomic_uint_fast64_t write_count;
    uint64_t last_access_cycle;
    uint64_t creation_time;
    float entropy_score;            // 0.0 to 1.0+
    float compression_ratio;        // Actual size / original size
    
    // Lifecycle statistics
    atomic_uint_fast32_t promotion_count;
    atomic_uint_fast32_t demotion_count;
    atomic_uint_fast32_t persist_count;
    atomic_uint_fast32_t load_count;
} HamStats;

/**
 * @brief Global HAM statistics
 */
typedef struct {
    atomic_uint_fast64_t total_regions;
    atomic_uint_fast64_t active_regions;
    atomic_uint_fast64_t total_memory;
    atomic_uint_fast64_t compressed_memory;
    
    // Per-tier statistics
    atomic_uint_fast64_t tier_regions[HAM_TIER_COUNT];
    atomic_uint_fast64_t tier_memory[HAM_TIER_COUNT];
    
    // Lifecycle operations
    atomic_uint_fast64_t promotions;
    atomic_uint_fast64_t demotions;
    atomic_uint_fast64_t compressions;
    atomic_uint_fast64_t decompressions;
    
    // Deduplication statistics
    atomic_uint_fast64_t dedup_checks;
    atomic_uint_fast64_t dedup_hits;
    atomic_uint_fast64_t dedup_saved_bytes;
} HamGlobalStats;

// ============================================================================
// HAM Region Structure
// ============================================================================

/**
 * @brief HAM memory region
 */
typedef struct HamRegion {
    RegionId id;
    HamTier tier;
    size_t capacity_bytes;
    size_t used_bytes;
    void *base;                     // Mapped host pointer
    uint32_t numa_node;             // NUMA node placement
    char path[HAM_REGION_PATH_MAX]; // Archive path
    
    // Memory management
    page_frame_t *pages;            // Physical pages backing this region
    uint32_t num_pages;
    atomic_flag lock;               // Region lock
    
    // Intelligence fields
    HamStats stats;
    Motif *interned_motif;          // If not nullptr, region is compressed
    
    // Lifecycle state
    bool persistent;                // Has been persisted
    bool compressed;                // Currently compressed
    bool pinned;                    // Cannot be evicted
    bool zero_copy_enabled;         // Can be handed off without copy
    
    // Integration hooks
    void *scheduler_data;           // Scheduler-specific data
    void *ipc_data;                 // IPC-specific data
    void *fs_data;                  // Filesystem-specific data
    
    struct HamRegion *next;         // Free list linkage
} HamRegion;

// ============================================================================
// HAM Virtual Table (Interface)
// ============================================================================

/**
 * @brief HAM operations interface
 */
typedef struct {
    // Basic allocation
    int (*alloc)(RegionId *out_id, HamTier tier, size_t size_bytes, void **out_ptr);
    int (*free)(RegionId id);
    int (*resize)(RegionId id, size_t new_size);
    
    // Persistence
    int (*persist)(RegionId id);
    int (*load)(RegionId id);
    int (*sync)(RegionId id);       // Sync to disk without unloading
    
    // Intelligence functions
    int (*update_stats)(RegionId id);
    int (*promote)(RegionId id);    // Promote to higher tier
    int (*demote)(RegionId id);     // Demote to lower tier
    int (*demote_check)(RegionId id); // Check if should demote
    
    // Compression and deduplication
    int (*compress)(RegionId id);
    int (*decompress)(RegionId id);
    int (*intern_check)(RegionId id, MotifDictionary *dict);
    
    // Zero-copy integration
    int (*enable_zero_copy)(RegionId id);
    int (*handoff_to_scheduler)(RegionId id, void *scheduler_ctx);
    int (*handoff_to_ipc)(RegionId id, void *ipc_ctx);
    int (*handoff_to_fs)(RegionId id, void *fs_ctx);
    
    // NUMA operations
    int (*migrate_numa)(RegionId id, uint32_t target_node);
    int (*get_numa_node)(RegionId id, uint32_t *out_node);
} HamVTable;

// ============================================================================
// HAM Interface
// ============================================================================

/**
 * @brief Initialize HAM subsystem
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_init(void);

/**
 * @brief Shutdown HAM subsystem
 */
void ham_shutdown(void);

/**
 * @brief Allocate HAM region
 * @param out_id Output region ID
 * @param tier Initial tier
 * @param size_bytes Size in bytes
 * @param out_ptr Output pointer to region base
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_alloc(RegionId *out_id, HamTier tier, size_t size_bytes, void **out_ptr);

/**
 * @brief Free HAM region
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_free(RegionId id);

/**
 * @brief Resize HAM region
 * @param id Region ID
 * @param new_size New size in bytes
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_resize(RegionId id, size_t new_size);

/**
 * @brief Persist region to storage
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_persist(RegionId id);

/**
 * @brief Load region from storage
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_load(RegionId id);

/**
 * @brief Update region statistics
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_update_stats(RegionId id);

/**
 * @brief Promote region to higher tier
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_promote(RegionId id);

/**
 * @brief Demote region to lower tier
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_demote(RegionId id);

/**
 * @brief Check if region should be demoted
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_demote_check(RegionId id);

/**
 * @brief Compress region
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_compress(RegionId id);

/**
 * @brief Decompress region
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_decompress(RegionId id);

/**
 * @brief Check for deduplication opportunity
 * @param id Region ID
 * @param dict Motif dictionary
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_intern_check(RegionId id, MotifDictionary *dict);

/**
 * @brief Enable zero-copy for region
 * @param id Region ID
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_enable_zero_copy(RegionId id);

/**
 * @brief Handoff region to scheduler without copy
 * @param id Region ID
 * @param scheduler_ctx Scheduler context
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_handoff_to_scheduler(RegionId id, void *scheduler_ctx);

/**
 * @brief Migrate region to different NUMA node
 * @param id Region ID
 * @param target_node Target NUMA node
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_migrate_numa(RegionId id, uint32_t target_node);

/**
 * @brief Get region's NUMA node
 * @param id Region ID
 * @param out_node Output NUMA node
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_get_numa_node(RegionId id, uint32_t *out_node);

/**
 * @brief Get region tier
 * @param id Region ID
 * @return Tier or negative error code
 */
NODISCARD HamTier ham_get_region_tier(RegionId id);

/**
 * @brief Get region base pointer
 * @param id Region ID
 * @return Base pointer or nullptr
 */
NODISCARD void* ham_get_region_base(RegionId id);

/**
 * @brief Get region statistics
 * @param id Region ID
 * @param out_stats Output statistics
 * @return 0 on success, negative error code on failure
 */
NODISCARD int ham_get_region_stats(RegionId id, HamStats *out_stats);

/**
 * @brief Get global HAM statistics
 * @return Pointer to global statistics
 */
NODISCARD const HamGlobalStats* ham_get_global_stats(void);

/**
 * @brief Print HAM statistics
 */
void ham_print_stats(void);

/**
 * @brief Run HAM background maintenance
 * @details Performs promotion/demotion checks, compression, etc.
 */
void ham_maintenance(void);

/**
 * @brief Validate HAM state
 * @return true if valid, false if corruption detected
 */
NODISCARD bool ham_validate(void);

// ============================================================================
// Default HAM Implementation
// ============================================================================

extern HamVTable HAM_DEFAULT_IMPL;

// ============================================================================
// Compile-time Assertions
// ============================================================================

static_assert(sizeof(void*) >= 4, "HAM requires at least 32-bit pointers");
static_assert(sizeof(size_t) >= 4, "size_t must be at least 4 bytes");
static_assert(HAM_TIER_COUNT == 4, "HAM tier count mismatch");

#endif // BDI_KERNEL_HAM_H
