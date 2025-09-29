
/**
 * @file x86_cache_hints.h
 * @brief x86 Cache Hints and Memory Optimization Interface
 * 
 * Provides comprehensive x86 cache management and optimization including:
 * - Cache hierarchy management (L1, L2, L3)
 * - Memory access pattern optimization
 * - Cache line alignment and prefetching
 * - Write-through and write-back policies
 * - Cache coherency protocols
 * 
 * Based on technical foundation from Assembly Language for x86 Processors 7th Edition
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef X86_CACHE_HINTS_H
#define X86_CACHE_HINTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CACHE CONSTANTS AND DEFINITIONS
// =============================================================================

#define X86_CACHE_LINE_SIZE_L1      64      ///< Typical L1 cache line size
#define X86_CACHE_LINE_SIZE_L2      64      ///< Typical L2 cache line size
#define X86_CACHE_LINE_SIZE_L3      64      ///< Typical L3 cache line size
#define X86_CACHE_ALIGNMENT         64      ///< Cache line alignment
#define X86_PREFETCH_DISTANCE       256     ///< Default prefetch distance

/**
 * @brief Cache levels
 */
typedef enum {
    X86_CACHE_L1_INSTRUCTION = 0,   ///< L1 instruction cache
    X86_CACHE_L1_DATA,              ///< L1 data cache
    X86_CACHE_L2_UNIFIED,           ///< L2 unified cache
    X86_CACHE_L3_UNIFIED,           ///< L3 unified cache
    X86_CACHE_LEVEL_COUNT           ///< Total cache levels
} x86_cache_level_t;

/**
 * @brief Cache policies
 */
typedef enum {
    X86_CACHE_POLICY_WRITE_BACK = 0,    ///< Write-back policy
    X86_CACHE_POLICY_WRITE_THROUGH,     ///< Write-through policy
    X86_CACHE_POLICY_WRITE_COMBINING,   ///< Write-combining policy
    X86_CACHE_POLICY_UNCACHEABLE,       ///< Uncacheable policy
    X86_CACHE_POLICY_WRITE_PROTECTED    ///< Write-protected policy
} x86_cache_policy_t;

/**
 * @brief Memory access patterns
 */
typedef enum {
    X86_ACCESS_PATTERN_SEQUENTIAL = 0,  ///< Sequential access pattern
    X86_ACCESS_PATTERN_RANDOM,          ///< Random access pattern
    X86_ACCESS_PATTERN_TEMPORAL,        ///< Temporal locality pattern
    X86_ACCESS_PATTERN_SPATIAL,         ///< Spatial locality pattern
    X86_ACCESS_PATTERN_STREAMING        ///< Streaming access pattern
} x86_access_pattern_t;

/**
 * @brief Prefetch hints
 */
typedef enum {
    X86_PREFETCH_NTA = 0,       ///< Non-temporal access (bypass cache)
    X86_PREFETCH_T0,            ///< Temporal access to all cache levels
    X86_PREFETCH_T1,            ///< Temporal access to L2 and L3 only
    X86_PREFETCH_T2             ///< Temporal access to L3 only
} x86_prefetch_hint_t;

/**
 * @brief Cache configuration for a single level
 */
typedef struct {
    size_t size;                ///< Cache size in bytes
    size_t line_size;           ///< Cache line size in bytes
    size_t associativity;       ///< Cache associativity
    size_t sets;                ///< Number of cache sets
    x86_cache_policy_t policy;  ///< Cache policy
    bool write_allocate;        ///< Write-allocate on miss
    bool inclusive;             ///< Inclusive of lower levels
    uint32_t latency_cycles;    ///< Access latency in cycles
} x86_cache_level_config_t;

/**
 * @brief Complete cache hierarchy configuration
 */
typedef struct {
    x86_cache_level_config_t levels[X86_CACHE_LEVEL_COUNT];
    bool coherency_enabled;     ///< Cache coherency enabled
    bool prefetch_enabled;      ///< Hardware prefetching enabled
    size_t prefetch_distance;   ///< Prefetch distance in bytes
    uint32_t false_sharing_threshold; ///< False sharing detection threshold
} x86_cache_config_t;

/**
 * @brief Cache performance statistics
 */
typedef struct {
    uint64_t hits[X86_CACHE_LEVEL_COUNT];       ///< Cache hits per level
    uint64_t misses[X86_CACHE_LEVEL_COUNT];     ///< Cache misses per level
    uint64_t evictions[X86_CACHE_LEVEL_COUNT];  ///< Cache evictions per level
    uint64_t prefetch_hits;                     ///< Prefetch hits
    uint64_t prefetch_misses;                   ///< Prefetch misses
    uint64_t false_sharing_events;              ///< False sharing events
    double hit_rates[X86_CACHE_LEVEL_COUNT];    ///< Hit rates per level
    double miss_rates[X86_CACHE_LEVEL_COUNT];   ///< Miss rates per level
} x86_cache_stats_t;

/**
 * @brief Memory region cache hint
 */
typedef struct {
    void *start_addr;           ///< Start address of region
    size_t size;                ///< Size of region
    x86_cache_policy_t policy;  ///< Cache policy for region
    x86_access_pattern_t pattern; ///< Expected access pattern
    x86_prefetch_hint_t prefetch; ///< Prefetch hint
    bool align_to_cache_line;   ///< Align to cache line boundary
    bool avoid_false_sharing;   ///< Avoid false sharing
} x86_cache_hint_t;

/**
 * @brief Cache management context
 */
typedef struct {
    x86_cache_config_t config;      ///< Cache configuration
    x86_cache_stats_t stats;        ///< Performance statistics
    x86_cache_hint_t *hints;        ///< Active cache hints
    size_t hint_count;              ///< Number of active hints
    size_t hint_capacity;           ///< Hint array capacity
    bool initialized;               ///< Initialization status
    uint64_t access_counter;        ///< Access counter
} x86_cache_context_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize x86 cache hints and optimization system
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_hints_initialize(void);

/**
 * @brief Shutdown x86 cache hints and optimization system
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_hints_shutdown(void);

/**
 * @brief Configure cache hierarchy
 * @param config Cache configuration structure
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_configure(const x86_cache_config_t *config);

/**
 * @brief Add cache hint for memory region
 * @param hint Cache hint descriptor
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_add_hint(const x86_cache_hint_t *hint);

/**
 * @brief Remove cache hint for memory region
 * @param start_addr Start address of region
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_remove_hint(void *start_addr);

/**
 * @brief Optimize memory layout for cache efficiency
 * @param data Pointer to data structure
 * @param size Size of data structure
 * @param access_pattern Expected access pattern
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_optimize_layout(void *data, size_t size, x86_access_pattern_t access_pattern);

/**
 * @brief Prefetch data into cache
 * @param addr Address to prefetch
 * @param hint Prefetch hint
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_prefetch(const void *addr, x86_prefetch_hint_t hint);

/**
 * @brief Prefetch data range into cache
 * @param start_addr Start address
 * @param end_addr End address
 * @param hint Prefetch hint
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_prefetch_range(const void *start_addr, const void *end_addr, x86_prefetch_hint_t hint);

/**
 * @brief Flush cache line containing address
 * @param addr Address to flush
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_flush_line(const void *addr);

/**
 * @brief Flush cache range
 * @param start_addr Start address
 * @param size Size of range to flush
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_flush_range(const void *start_addr, size_t size);

/**
 * @brief Invalidate cache line containing address
 * @param addr Address to invalidate
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_invalidate_line(const void *addr);

/**
 * @brief Set cache policy for memory region
 * @param start_addr Start address of region
 * @param size Size of region
 * @param policy Cache policy to set
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_set_policy(void *start_addr, size_t size, x86_cache_policy_t policy);

/**
 * @brief Detect and mitigate false sharing
 * @param addr1 First address
 * @param addr2 Second address
 * @return True if false sharing detected
 */
bool x86_cache_detect_false_sharing(const void *addr1, const void *addr2);

/**
 * @brief Align data structure to cache line boundary
 * @param data Pointer to data structure
 * @param size Size of data structure
 * @return Aligned pointer or NULL on error
 */
void *x86_cache_align_data(void *data, size_t size);

/**
 * @brief Get cache performance statistics
 * @return Pointer to cache statistics structure
 */
const x86_cache_stats_t *x86_cache_get_stats(void);

/**
 * @brief Reset cache performance statistics
 */
void x86_cache_reset_stats(void);

/**
 * @brief Get cache configuration
 * @return Pointer to cache configuration structure
 */
const x86_cache_config_t *x86_cache_get_config(void);

/**
 * @brief Get cache management context
 * @return Pointer to cache context structure
 */
x86_cache_context_t *x86_cache_get_context(void);

/**
 * @brief Update cache statistics
 * @param level Cache level
 * @param hit True for hit, false for miss
 */
void x86_cache_update_stats(x86_cache_level_t level, bool hit);

/**
 * @brief Optimize for specific access pattern
 * @param pattern Access pattern to optimize for
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_optimize_for_pattern(x86_access_pattern_t pattern);

/**
 * @brief Enable/disable hardware prefetching
 * @param enable True to enable, false to disable
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_set_prefetch_enabled(bool enable);

/**
 * @brief Set prefetch distance
 * @param distance Prefetch distance in bytes
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_set_prefetch_distance(size_t distance);

/**
 * @brief Warm up cache with data
 * @param data Pointer to data to warm up
 * @param size Size of data
 * @return Status code (0 = success, negative = error)
 */
int x86_cache_warm_up(const void *data, size_t size);

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Check if address is cache-line aligned
 * @param addr Address to check
 * @return True if cache-line aligned
 */
static inline bool x86_cache_is_aligned(const void *addr)
{
    return ((uintptr_t)addr & (X86_CACHE_ALIGNMENT - 1)) == 0;
}

/**
 * @brief Align address to cache line boundary
 * @param addr Address to align
 * @return Cache-line aligned address
 */
static inline void *x86_cache_align_address(const void *addr)
{
    uintptr_t aligned = ((uintptr_t)addr + X86_CACHE_ALIGNMENT - 1) & ~(X86_CACHE_ALIGNMENT - 1);
    return (void *)aligned;
}

/**
 * @brief Get cache line containing address
 * @param addr Address
 * @return Start of cache line containing address
 */
static inline void *x86_cache_get_line_start(const void *addr)
{
    uintptr_t line_start = (uintptr_t)addr & ~(X86_CACHE_ALIGNMENT - 1);
    return (void *)line_start;
}

/**
 * @brief Calculate number of cache lines for size
 * @param size Size in bytes
 * @return Number of cache lines
 */
static inline size_t x86_cache_lines_for_size(size_t size)
{
    return (size + X86_CACHE_ALIGNMENT - 1) / X86_CACHE_ALIGNMENT;
}

/**
 * @brief Check if two addresses are in same cache line
 * @param addr1 First address
 * @param addr2 Second address
 * @return True if in same cache line
 */
static inline bool x86_cache_same_line(const void *addr1, const void *addr2)
{
    return x86_cache_get_line_start(addr1) == x86_cache_get_line_start(addr2);
}

/**
 * @brief Calculate cache hit rate
 * @param hits Number of hits
 * @param misses Number of misses
 * @return Hit rate as percentage
 */
static inline double x86_cache_calculate_hit_rate(uint64_t hits, uint64_t misses)
{
    uint64_t total = hits + misses;
    return (total > 0) ? ((double)hits / (double)total) * 100.0 : 0.0;
}

/**
 * @brief Get optimal alignment for data structure
 * @param size Size of data structure
 * @return Optimal alignment in bytes
 */
static inline size_t x86_cache_get_optimal_alignment(size_t size)
{
    if (size <= 8) return 8;
    if (size <= 16) return 16;
    if (size <= 32) return 32;
    return X86_CACHE_ALIGNMENT;
}

#ifdef __cplusplus
}
#endif

#endif // X86_CACHE_HINTS_H
