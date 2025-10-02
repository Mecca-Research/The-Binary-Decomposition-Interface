
/**
 * @file spsc_ring.h
 * @brief Lock-free Single Producer Single Consumer (SPSC) ring buffer
 * 
 * High-performance, wait-free ring buffer for single producer and single consumer.
 * Uses atomic operations with acquire/release semantics for synchronization.
 * Cache-line aligned to prevent false sharing.
 */

#ifndef PHASE1_SPSC_RING_H
#define PHASE1_SPSC_RING_H

#include "ring_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct spsc_ring spsc_ring_t;

/**
 * @brief SPSC ring buffer structure
 * 
 * Memory layout optimized to avoid false sharing:
 * - Producer cache line: head index
 * - Consumer cache line: tail index
 * - Shared metadata: capacity, mask (read-only after init)
 * - Data array: cache-line aligned
 */
struct spsc_ring {
    // Producer cache line (64 bytes)
    CACHE_ALIGNED atomic_size_t head;  // Producer writes here
    char pad1[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    
    // Consumer cache line (64 bytes)
    CACHE_ALIGNED atomic_size_t tail;  // Consumer writes here
    char pad2[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    
    // Shared metadata (read-only after initialization)
    size_t capacity;  // Must be power of 2
    size_t mask;      // capacity - 1 (for fast modulo)
    
    // Statistics
    ring_stats_t stats;
    
    // Data array (flexible array member, cache-line aligned)
    CACHE_ALIGNED void* data[];
};

/**
 * @brief Create a new SPSC ring buffer
 * 
 * @param capacity Number of elements (will be rounded up to power of 2)
 * @return Pointer to ring buffer, or NULL on failure
 */
spsc_ring_t* spsc_ring_create(size_t capacity);

/**
 * @brief Destroy SPSC ring buffer
 * 
 * @param ring Ring buffer to destroy
 */
void spsc_ring_destroy(spsc_ring_t* ring);

/**
 * @brief Enqueue element (producer only)
 * 
 * Wait-free operation. Returns immediately if ring is full.
 * 
 * @param ring Ring buffer
 * @param element Element to enqueue
 * @return RING_SUCCESS or RING_ERROR_FULL
 */
ring_status_t spsc_ring_enqueue(spsc_ring_t* ring, void* element);

/**
 * @brief Dequeue element (consumer only)
 * 
 * Wait-free operation. Returns immediately if ring is empty.
 * 
 * @param ring Ring buffer
 * @param element Output parameter for dequeued element
 * @return RING_SUCCESS or RING_ERROR_EMPTY
 */
ring_status_t spsc_ring_dequeue(spsc_ring_t* ring, void** element);

/**
 * @brief Check if ring is empty (consumer only)
 * 
 * @param ring Ring buffer
 * @return true if empty, false otherwise
 */
bool spsc_ring_is_empty(const spsc_ring_t* ring);

/**
 * @brief Check if ring is full (producer only)
 * 
 * @param ring Ring buffer
 * @return true if full, false otherwise
 */
bool spsc_ring_is_full(const spsc_ring_t* ring);

/**
 * @brief Get current size (approximate, may be stale)
 * 
 * @param ring Ring buffer
 * @return Current number of elements
 */
size_t spsc_ring_size(const spsc_ring_t* ring);

/**
 * @brief Get ring capacity
 * 
 * @param ring Ring buffer
 * @return Maximum number of elements
 */
size_t spsc_ring_capacity(const spsc_ring_t* ring);

/**
 * @brief Get ring statistics
 * 
 * @param ring Ring buffer
 * @param stats Output parameter for statistics
 */
void spsc_ring_get_stats(const spsc_ring_t* ring, ring_stats_t* stats);

/**
 * @brief Reset ring statistics
 * 
 * @param ring Ring buffer
 */
void spsc_ring_reset_stats(spsc_ring_t* ring);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_SPSC_RING_H
