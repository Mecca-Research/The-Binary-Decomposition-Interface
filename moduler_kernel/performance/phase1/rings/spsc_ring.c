
/**
 * @file spsc_ring.c
 * @brief Implementation of lock-free SPSC ring buffer
 */

#include "spsc_ring.h"
#include <stdlib.h>
#include <string.h>

spsc_ring_t* spsc_ring_create(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    
    // Round up to power of 2
    if (!is_power_of_2(capacity)) {
        capacity = next_power_of_2(capacity);
    }
    
    // Allocate ring structure + data array
    size_t total_size = sizeof(spsc_ring_t) + capacity * sizeof(void*);
    
    // BUG FIX 2 (P1): Round total_size up to multiple of CACHE_LINE_SIZE
    // C17 §7.22.3.1 requires size to be an integer multiple of alignment
    // Without this, aligned_alloc can fail or invoke undefined behavior
    total_size = ((total_size + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
    
    spsc_ring_t* ring = aligned_alloc(CACHE_LINE_SIZE, total_size);
    if (!ring) {
        return NULL;
    }
    
    // Initialize
    memset(ring, 0, total_size);
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    
    return ring;
}

void spsc_ring_destroy(spsc_ring_t* ring) {
    if (ring) {
        free(ring);
    }
}

ring_status_t spsc_ring_enqueue(spsc_ring_t* ring, void* element) {
    if (!ring) {
        return RING_ERROR_INVALID_PARAM;
    }
    
    // Load tail with acquire ordering (see consumer's writes)
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    
    // Check if full: (head - tail) == capacity
    size_t available = ring->capacity - (head - tail);
    if (available == 0) {
        ring->stats.failed_enqueues++;
        return RING_ERROR_FULL;
    }
    
    // Write data at head position
    ring->data[head & ring->mask] = element;
    
    // Update head with release ordering (make write visible to consumer)
    atomic_store_explicit(&ring->head, head + 1, memory_order_release);
    
    // Update statistics
    ring->stats.total_enqueues++;
    size_t current_size = (head + 1) - tail;
    ring->stats.current_size = current_size;
    if (current_size > ring->stats.peak_size) {
        ring->stats.peak_size = current_size;
    }
    
    return RING_SUCCESS;
}

ring_status_t spsc_ring_dequeue(spsc_ring_t* ring, void** element) {
    if (!ring || !element) {
        return RING_ERROR_INVALID_PARAM;
    }
    
    // Load head with acquire ordering (see producer's writes)
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    
    // Check if empty: head == tail
    if (head == tail) {
        ring->stats.failed_dequeues++;
        return RING_ERROR_EMPTY;
    }
    
    // Read data at tail position
    *element = ring->data[tail & ring->mask];
    
    // Update tail with release ordering (make read visible to producer)
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    
    // Update statistics
    ring->stats.total_dequeues++;
    ring->stats.current_size = head - (tail + 1);
    
    return RING_SUCCESS;
}

bool spsc_ring_is_empty(const spsc_ring_t* ring) {
    if (!ring) {
        return true;
    }
    
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    
    return head == tail;
}

bool spsc_ring_is_full(const spsc_ring_t* ring) {
    if (!ring) {
        return true;
    }
    
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    
    return (head - tail) == ring->capacity;
}

size_t spsc_ring_size(const spsc_ring_t* ring) {
    if (!ring) {
        return 0;
    }
    
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    return head - tail;
}

size_t spsc_ring_capacity(const spsc_ring_t* ring) {
    return ring ? ring->capacity : 0;
}

void spsc_ring_get_stats(const spsc_ring_t* ring, ring_stats_t* stats) {
    if (ring && stats) {
        *stats = ring->stats;
    }
}

void spsc_ring_reset_stats(spsc_ring_t* ring) {
    if (ring) {
        memset(&ring->stats, 0, sizeof(ring_stats_t));
    }
}
