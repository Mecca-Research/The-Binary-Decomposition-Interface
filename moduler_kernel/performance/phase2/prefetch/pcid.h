
/**
 * @file pcid.h
 * @brief PCID/ASID management for TLB efficiency
 * 
 * Manages Process-Context Identifiers to avoid full TLB flushes.
 * Supports selective invalidation with INVPCID instruction.
 */

#ifndef PHASE2_PCID_H
#define PHASE2_PCID_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PCID space size (12-bit = 4096 contexts)
#define PCID_SPACE_SIZE 4096

// Reserved PCIDs
#define PCID_KERNEL 0
#define PCID_INVALID 0xFFFF

/**
 * @brief PCID statistics
 */
typedef struct {
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t evictions;
    uint64_t tlb_flushes_avoided;
    uint64_t selective_invalidations;
    uint64_t full_invalidations;
    uint32_t current_allocated;
    uint32_t peak_allocated;
} pcid_stats_t;

/**
 * @brief PCID configuration
 */
typedef struct {
    bool enable_pcid;           // Enable PCID support
    bool enable_invpcid;        // Enable INVPCID instruction
    uint32_t eviction_threshold; // Evict when this many PCIDs allocated
} pcid_config_t;

/**
 * @brief Initialize PCID manager
 * 
 * @param config Configuration (NULL = defaults)
 * @return 0 on success, -1 on failure
 */
int pcid_init(const pcid_config_t* config);

/**
 * @brief Allocate PCID for context
 * 
 * @param context_id Context identifier
 * @return PCID, or PCID_INVALID on failure
 */
uint16_t pcid_alloc(uint64_t context_id);

/**
 * @brief Free PCID
 * 
 * @param pcid PCID to free
 */
void pcid_free(uint16_t pcid);

/**
 * @brief Get PCID for context
 * 
 * @param context_id Context identifier
 * @return PCID, or PCID_INVALID if not allocated
 */
uint16_t pcid_get(uint64_t context_id);

/**
 * @brief Invalidate TLB entry for PCID
 * 
 * @param pcid PCID
 * @param addr Virtual address (0 = all addresses)
 */
void pcid_invalidate(uint16_t pcid, uintptr_t addr);

/**
 * @brief Invalidate all TLB entries for PCID
 * 
 * @param pcid PCID
 */
void pcid_invalidate_all(uint16_t pcid);

/**
 * @brief Flush all TLB entries
 */
void pcid_flush_all(void);

/**
 * @brief Check if PCID is supported
 * 
 * @return true if supported, false otherwise
 */
bool pcid_is_supported(void);

/**
 * @brief Check if INVPCID is supported
 * 
 * @return true if supported, false otherwise
 */
bool pcid_is_invpcid_supported(void);

/**
 * @brief Get PCID statistics
 * 
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int pcid_get_stats(pcid_stats_t* stats);

/**
 * @brief Reset PCID statistics
 */
void pcid_reset_stats(void);

/**
 * @brief Print PCID statistics
 */
void pcid_print_stats(void);

/**
 * @brief Destroy PCID manager
 */
void pcid_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_PCID_H
