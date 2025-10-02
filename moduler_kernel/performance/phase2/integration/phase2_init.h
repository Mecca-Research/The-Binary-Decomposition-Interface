
/**
 * @file phase2_init.h
 * @brief Phase 2 initialization and integration
 * 
 * Provides unified initialization for all Phase 2 subsystems.
 * Integrates with Phase 1 components.
 */

#ifndef PHASE2_INIT_H
#define PHASE2_INIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Phase 2 configuration
 */
typedef struct {
    // NUMA configuration
    bool enable_numa;
    bool enable_attention;
    uint64_t attention_threshold;
    
    // Prefetch configuration
    bool enable_huge_pages;
    bool enable_pcid;
    bool enable_prefetch;
    
    // Scheduler configuration
    bool enable_tickless;
    bool enable_timer_wheel;
    
    // Integration
    bool integrate_phase1;
} phase2_config_t;

/**
 * @brief Initialize Phase 2
 * 
 * Initializes all Phase 2 subsystems in correct order.
 * 
 * @param config Configuration (NULL = defaults)
 * @return 0 on success, -1 on failure
 */
int phase2_init(const phase2_config_t* config);

/**
 * @brief Check if Phase 2 is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool phase2_is_initialized(void);

/**
 * @brief Get Phase 2 configuration
 * 
 * @return Pointer to configuration
 */
const phase2_config_t* phase2_get_config(void);

/**
 * @brief Print Phase 2 status
 */
void phase2_print_status(void);

/**
 * @brief Print all Phase 2 statistics
 */
void phase2_print_all_stats(void);

/**
 * @brief Reset all Phase 2 statistics
 */
void phase2_reset_all_stats(void);

/**
 * @brief Destroy Phase 2
 * 
 * Cleans up all Phase 2 subsystems.
 */
void phase2_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_INIT_H
