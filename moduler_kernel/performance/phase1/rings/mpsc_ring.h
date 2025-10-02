
/**
 * @file mpsc_ring.h
 * @brief Lock-free Multi Producer Single Consumer (MPSC) ring buffer
 * 
 * High-performance ring buffer for multiple producers and single consumer.
 * Uses CAS (Compare-And-Swap) for producer synchronization.
 * Consumer has exclusive access to tail (no contention).
 */

#ifndef PHASE1_MPSC_RING_H
#define PHASE1_MPSC_RING_H

#include "ring_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct mpsc_ring mpsc_ring_t;

/**
 * @brief MPSC ring buffer structure
 * 
 * Memory layout optimized for multi-producer contention:
 * - Shared head index: multiple producers compete via CAS
 * - Consumer cache line: tail index (exclusive access)
 * - Sequence numbers: track slot ownership
 */
struct mpsc_ring {
    // Shared producer cache line (64 bytes)
    CACHE_ALIGNED atomic_size_t head;  // Multiple producers compete here
    char pad1[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    
    // Consumer cache line (64 bytes)
    CACHE_ALIGNED atomic_size_t tail;  // Consumer writes here
    char pad2[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    
    // Shared metadata
    size_t capacity;
    size_t mask;
    
    // Statistics
    ring_stats_t stats;
    atomic_uint64_t cas_failures;  // Track contention
    
    // Sequence numbers for each slot (track ownership)
    CACHE_ALIGNED atomic_size_t* sequences;
    
    // Data array
    CACHE_ALIGNED void* data[];
};

/**
 * @brief Create a new MPSC ring buffer
 * 
 * @param capacity Number of elements (will be rounded up to power of 2)
 * @return Pointer to ring buffer, or NULL on failure
 */
mpsc_ring_t* mpsc_ring_create(size_t capacity);

/**
 * @brief Destroy MPSC ring buffer
 * 
 * @param ring Ring buffer to destroy
 */
void mpsc_ring_destroy(mpsc_ring_t* ring);

/**
 * @brief Enqueue element (any producer)
 * 
 * Lock-free operation using CAS. May retry on contention.
 * 
 * @param ring Ring buffer
 * @param element Element to enqueue
 * @return RING_SUCCESS or RING_ERROR_FULL
 */
ring_status_t mpsc_ring_enqueue(mpsc_ring_t* ring, void* element);

/**
 * @brief Dequeue element (consumer only)
 * 
 * Wait-free operation. Returns immediately if ring is empty.
 * 
 * @param ring Ring buffer
 * @param element Output parameter for dequeued element
 * @return RING_SUCCESS or RING_ERROR_EMPTY
 */
ring_status_t mpsc_ring_dequeue(mpsc_ring_t* ring, void** element);

/**
 * @brief Check if ring is empty (consumer only)
 * 
 * @param ring Ring buffer
 * @return true if empty, false otherwise
 */
bool mpsc_ring_is_empty(const mpsc_ring_t* ring);

/**
 * @brief Get current size (approximate, may be stale)
 * 
 * @param ring Ring buffer
 * @return Current number of elements
 */
size_t mpsc_ring_size(const mpsc_ring_t* ring);

/**
 * @brief Get ring capacity
 * 
 * @param ring Ring buffer
 * @return Maximum number of elements
 */
size_t mpsc_ring_capacity(const mpsc_ring_t* ring);

/**
 * @brief Get ring statistics
 * 
 * @param ring Ring buffer
 * @param stats Output parameter for statistics
 */
void mpsc_ring_get_stats(const mpsc_ring_t* ring, ring_stats_t* stats);

/**
 * @brief Get CAS failure count (contention metric)
 * 
 * @param ring Ring buffer
 * @return Number of CAS failures
 */
uint64_t mpsc_ring_get_cas_failures(const mpsc_ring_t* ring);

/**
 * @brief Reset ring statistics
 * 
 * @param ring Ring buffer
 */
void mpsc_ring_reset_stats(mpsc_ring_t* ring);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_MPSC_RING_H
