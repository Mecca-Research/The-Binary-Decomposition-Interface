
/**
 * @file attention.h
 * @brief Attention-guided memory allocation policy
 * 
 * Tracks object class access patterns and guides allocation decisions.
 * Migrates hot objects to frequently-accessing NUMA nodes.
 */

#ifndef PHASE2_ATTENTION_H
#define PHASE2_ATTENTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum object classes tracked
#define ATTENTION_MAX_CLASSES 256

// Exponential moving average alpha (0.1 = 10% weight to new samples)
#define ATTENTION_EMA_ALPHA 0.1

// Migration threshold (accesses before considering migration)
#define ATTENTION_MIGRATION_THRESHOLD 1000

/**
 * @brief Object class access statistics
 */
typedef struct {
    uint64_t access_count[NUMA_MAX_NODES];  // Accesses per NUMA node
    double affinity_score[NUMA_MAX_NODES];  // EMA affinity scores
    uint32_t preferred_node;                // Current preferred node
    uint64_t migration_count;               // Number of migrations
    uint64_t last_migration_time;           // Timestamp of last migration
} attention_class_stats_t;

/**
 * @brief Attention policy configuration
 */
typedef struct {
    uint64_t migration_threshold;    // Accesses before migration
    uint64_t migration_cooldown;     // Cooldown period (ms)
    double migration_cost_factor;    // Cost factor for migration
    double ema_alpha;                // EMA smoothing factor
    bool enable_migration;           // Enable automatic migration
} attention_config_t;

/**
 * @brief Initialize attention-guided allocation
 * 
 * @param config Configuration (NULL = defaults)
 * @return 0 on success, -1 on failure
 */
int attention_init(const attention_config_t* config);

/**
 * @brief Record object access
 * 
 * @param class_id Object class ID
 * @param node NUMA node where access occurred
 */
void attention_record_access(uint32_t class_id, uint32_t node);

/**
 * @brief Get preferred NUMA node for object class
 * 
 * @param class_id Object class ID
 * @return Preferred NUMA node, or -1 if unknown
 */
int attention_get_preferred_node(uint32_t class_id);

/**
 * @brief Check if object class should migrate
 * 
 * @param class_id Object class ID
 * @param current_node Current NUMA node
 * @param target_node Output: target node for migration
 * @return true if should migrate, false otherwise
 */
bool attention_should_migrate(uint32_t class_id, uint32_t current_node, uint32_t* target_node);

/**
 * @brief Record object migration
 * 
 * @param class_id Object class ID
 * @param from_node Source node
 * @param to_node Destination node
 */
void attention_record_migration(uint32_t class_id, uint32_t from_node, uint32_t to_node);

/**
 * @brief Get statistics for object class
 * 
 * @param class_id Object class ID
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int attention_get_stats(uint32_t class_id, attention_class_stats_t* stats);

/**
 * @brief Reset statistics for object class
 * 
 * @param class_id Object class ID
 */
void attention_reset_stats(uint32_t class_id);

/**
 * @brief Print attention statistics
 * 
 * @param class_id Object class ID
 */
void attention_print_stats(uint32_t class_id);

/**
 * @brief Destroy attention system
 */
void attention_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_ATTENTION_H
