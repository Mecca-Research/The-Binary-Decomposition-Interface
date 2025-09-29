
/**
 * @file x86_tlb_mgmt.h
 * @brief x86 TLB Management Interface
 * 
 * Provides comprehensive Translation Lookaside Buffer (TLB) management including:
 * - TLB invalidation strategies
 * - TLB performance optimization
 * - TLB miss handling
 * - TLB entry management
 * - Multi-level TLB support
 * 
 * Based on technical foundation from Assembly Language for x86 Processors 7th Edition
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef X86_TLB_MGMT_H
#define X86_TLB_MGMT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// TLB CONSTANTS AND DEFINITIONS
// =============================================================================

#define X86_TLB_ENTRY_COUNT_L1      64      ///< Typical L1 TLB entry count
#define X86_TLB_ENTRY_COUNT_L2      512     ///< Typical L2 TLB entry count
#define X86_TLB_ASSOCIATIVITY       4       ///< Typical TLB associativity
#define X86_TLB_PAGE_SIZE_4KB       0       ///< 4KB page size
#define X86_TLB_PAGE_SIZE_2MB       1       ///< 2MB page size
#define X86_TLB_PAGE_SIZE_1GB       2       ///< 1GB page size

/**
 * @brief TLB invalidation types
 */
typedef enum {
    X86_TLB_INVALIDATE_PAGE = 0,    ///< Invalidate single page
    X86_TLB_INVALIDATE_GLOBAL,      ///< Invalidate all global pages
    X86_TLB_INVALIDATE_NON_GLOBAL,  ///< Invalidate all non-global pages
    X86_TLB_INVALIDATE_ALL,         ///< Invalidate entire TLB
    X86_TLB_INVALIDATE_CONTEXT      ///< Invalidate context-specific entries
} x86_tlb_invalidate_type_t;

/**
 * @brief TLB entry types
 */
typedef enum {
    X86_TLB_ENTRY_INSTRUCTION = 0,  ///< Instruction TLB entry
    X86_TLB_ENTRY_DATA,             ///< Data TLB entry
    X86_TLB_ENTRY_UNIFIED           ///< Unified TLB entry
} x86_tlb_entry_type_t;

/**
 * @brief TLB performance metrics
 */
typedef struct {
    uint64_t hits;                  ///< TLB hit count
    uint64_t misses;                ///< TLB miss count
    uint64_t invalidations;         ///< TLB invalidation count
    uint64_t flushes;               ///< TLB flush count
    double hit_rate;                ///< TLB hit rate percentage
    double miss_rate;               ///< TLB miss rate percentage
} x86_tlb_stats_t;

/**
 * @brief TLB entry descriptor
 */
typedef struct {
    uint32_t virtual_addr;          ///< Virtual address (page-aligned)
    uint32_t physical_addr;         ///< Physical address (page-aligned)
    uint32_t flags;                 ///< Page flags
    uint16_t asid;                  ///< Address Space ID (if supported)
    uint8_t page_size;              ///< Page size (0=4KB, 1=2MB, 2=1GB)
    uint8_t entry_type;             ///< Entry type (instruction/data/unified)
    bool global;                    ///< Global page flag
    bool valid;                     ///< Entry is valid
    uint64_t timestamp;             ///< Last access timestamp
} x86_tlb_entry_t;

/**
 * @brief TLB configuration
 */
typedef struct {
    size_t l1_itlb_entries;         ///< L1 instruction TLB entries
    size_t l1_dtlb_entries;         ///< L1 data TLB entries
    size_t l2_tlb_entries;          ///< L2 unified TLB entries
    size_t associativity;           ///< TLB associativity
    bool supports_global_pages;     ///< Global page support
    bool supports_large_pages;      ///< Large page support
    bool supports_asid;             ///< Address Space ID support
    bool hardware_managed;          ///< Hardware-managed TLB
} x86_tlb_config_t;

/**
 * @brief TLB management context
 */
typedef struct {
    x86_tlb_config_t config;        ///< TLB configuration
    x86_tlb_stats_t stats;          ///< Performance statistics
    x86_tlb_entry_t *entries;       ///< TLB entry cache (software model)
    size_t entry_count;             ///< Number of cached entries
    uint64_t access_counter;        ///< Access counter for LRU
    bool initialized;               ///< Initialization status
    uint16_t current_asid;          ///< Current Address Space ID
} x86_tlb_context_t;

/**
 * @brief TLB invalidation request
 */
typedef struct {
    x86_tlb_invalidate_type_t type; ///< Invalidation type
    uint32_t virtual_addr;          ///< Virtual address (for page invalidation)
    uint16_t asid;                  ///< Address Space ID (if applicable)
    bool flush_global;              ///< Include global pages
    bool flush_instruction;         ///< Flush instruction TLB
    bool flush_data;                ///< Flush data TLB
} x86_tlb_invalidate_request_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize x86 TLB management system
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_mgmt_initialize(void);

/**
 * @brief Shutdown x86 TLB management system
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_mgmt_shutdown(void);

/**
 * @brief Configure TLB parameters
 * @param config TLB configuration structure
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_configure(const x86_tlb_config_t *config);

/**
 * @brief Invalidate TLB entry for specific page
 * @param virtual_addr Virtual address to invalidate
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_invalidate_page(uint32_t virtual_addr);

/**
 * @brief Invalidate TLB entries for address range
 * @param start_addr Start of virtual address range
 * @param end_addr End of virtual address range
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_invalidate_range(uint32_t start_addr, uint32_t end_addr);

/**
 * @brief Flush entire TLB
 * @param flush_global Include global pages in flush
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_flush_all(bool flush_global);

/**
 * @brief Flush TLB for specific context/ASID
 * @param asid Address Space ID to flush
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_flush_context(uint16_t asid);

/**
 * @brief Process TLB invalidation request
 * @param request Invalidation request descriptor
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_process_invalidation(const x86_tlb_invalidate_request_t *request);

/**
 * @brief Prefetch TLB entry for virtual address
 * @param virtual_addr Virtual address to prefetch
 * @param entry_type Type of TLB entry to prefetch
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_prefetch_entry(uint32_t virtual_addr, x86_tlb_entry_type_t entry_type);

/**
 * @brief Add TLB entry to software cache
 * @param entry TLB entry to add
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_add_entry(const x86_tlb_entry_t *entry);

/**
 * @brief Remove TLB entry from software cache
 * @param virtual_addr Virtual address of entry to remove
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_remove_entry(uint32_t virtual_addr);

/**
 * @brief Lookup TLB entry in software cache
 * @param virtual_addr Virtual address to lookup
 * @param entry Pointer to store found entry
 * @return Status code (0 = found, negative = not found)
 */
int x86_tlb_lookup_entry(uint32_t virtual_addr, x86_tlb_entry_t *entry);

/**
 * @brief Update TLB statistics
 * @param hit True for TLB hit, false for miss
 */
void x86_tlb_update_stats(bool hit);

/**
 * @brief Get TLB performance statistics
 * @return Pointer to TLB statistics structure
 */
const x86_tlb_stats_t *x86_tlb_get_stats(void);

/**
 * @brief Reset TLB performance statistics
 */
void x86_tlb_reset_stats(void);

/**
 * @brief Get TLB configuration
 * @return Pointer to TLB configuration structure
 */
const x86_tlb_config_t *x86_tlb_get_config(void);

/**
 * @brief Get TLB management context
 * @return Pointer to TLB context structure
 */
x86_tlb_context_t *x86_tlb_get_context(void);

/**
 * @brief Set current Address Space ID
 * @param asid Address Space ID to set
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_set_asid(uint16_t asid);

/**
 * @brief Get current Address Space ID
 * @return Current ASID
 */
uint16_t x86_tlb_get_asid(void);

/**
 * @brief Optimize TLB usage for specific access pattern
 * @param pattern Access pattern hint
 * @return Status code (0 = success, negative = error)
 */
int x86_tlb_optimize_for_pattern(uint32_t pattern);

/**
 * @brief Handle TLB miss event
 * @param virtual_addr Virtual address that caused the miss
 * @param entry_type Type of TLB entry that missed
 * @return Status code (0 = handled, negative = error)
 */
int x86_tlb_handle_miss(uint32_t virtual_addr, x86_tlb_entry_type_t entry_type);

/**
 * @brief Validate TLB entry consistency
 * @param virtual_addr Virtual address to validate
 * @return True if TLB entry is consistent with page tables
 */
bool x86_tlb_validate_entry(uint32_t virtual_addr);

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Check if address is TLB-aligned
 * @param addr Address to check
 * @param page_size Page size (0=4KB, 1=2MB, 2=1GB)
 * @return True if aligned
 */
static inline bool x86_tlb_is_aligned(uint32_t addr, uint8_t page_size)
{
    switch (page_size) {
        case X86_TLB_PAGE_SIZE_4KB:
            return (addr & 0xFFF) == 0;
        case X86_TLB_PAGE_SIZE_2MB:
            return (addr & 0x1FFFFF) == 0;
        case X86_TLB_PAGE_SIZE_1GB:
            return (addr & 0x3FFFFFFF) == 0;
        default:
            return false;
    }
}

/**
 * @brief Get page size in bytes
 * @param page_size Page size code
 * @return Page size in bytes
 */
static inline size_t x86_tlb_get_page_size_bytes(uint8_t page_size)
{
    switch (page_size) {
        case X86_TLB_PAGE_SIZE_4KB:
            return 4096;
        case X86_TLB_PAGE_SIZE_2MB:
            return 2097152;
        case X86_TLB_PAGE_SIZE_1GB:
            return 1073741824;
        default:
            return 4096;
    }
}

/**
 * @brief Calculate TLB hit rate
 * @param hits Number of hits
 * @param misses Number of misses
 * @return Hit rate as percentage
 */
static inline double x86_tlb_calculate_hit_rate(uint64_t hits, uint64_t misses)
{
    uint64_t total = hits + misses;
    return (total > 0) ? ((double)hits / (double)total) * 100.0 : 0.0;
}

/**
 * @brief Check if TLB supports feature
 * @param config TLB configuration
 * @param feature Feature to check
 * @return True if feature is supported
 */
static inline bool x86_tlb_supports_feature(const x86_tlb_config_t *config, uint32_t feature)
{
    if (config == NULL) return false;
    
    switch (feature) {
        case 0: return config->supports_global_pages;
        case 1: return config->supports_large_pages;
        case 2: return config->supports_asid;
        case 3: return config->hardware_managed;
        default: return false;
    }
}

#ifdef __cplusplus
}
#endif

#endif // X86_TLB_MGMT_H
