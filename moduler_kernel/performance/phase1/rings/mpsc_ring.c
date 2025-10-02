
/**
 * @file mpsc_ring.c
 * @brief Implementation of lock-free MPSC ring buffer
 */

#include "mpsc_ring.h"
#include <stdlib.h>
#include <string.h>

// Maximum CAS retry attempts before giving up
#define MAX_CAS_RETRIES 1000

mpsc_ring_t* mpsc_ring_create(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    
    // Round up to power of 2
    if (!is_power_of_2(capacity)) {
        capacity = next_power_of_2(capacity);
    }
    
    // Allocate ring structure + data array
    size_t total_size = sizeof(mpsc_ring_t) + capacity * sizeof(void*);
    
    // BUG FIX 3 (P1): Round total_size up to multiple of CACHE_LINE_SIZE
    // C17 §7.22.3.1 requires size to be an integer multiple of alignment
    total_size = ((total_size + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
    
    mpsc_ring_t* ring = aligned_alloc(CACHE_LINE_SIZE, total_size);
    if (!ring) {
        return NULL;
    }
    
    // Allocate sequence array
    // BUG FIX 3 (P1): Round sequence array size up to multiple of CACHE_LINE_SIZE
    size_t seq_size = capacity * sizeof(atomic_size_t);
    seq_size = ((seq_size + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
    
    ring->sequences = aligned_alloc(CACHE_LINE_SIZE, seq_size);
    if (!ring->sequences) {
        free(ring);
        return NULL;
    }
    
    // Initialize
    memset(ring, 0, total_size);
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
    atomic_init(&ring->cas_failures, 0);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    
    // Initialize sequences
    for (size_t i = 0; i < capacity; i++) {
        atomic_init(&ring->sequences[i], i);
    }
    
    return ring;
}

void mpsc_ring_destroy(mpsc_ring_t* ring) {
    if (ring) {
        if (ring->sequences) {
            free(ring->sequences);
        }
        free(ring);
    }
}

ring_status_t mpsc_ring_enqueue(mpsc_ring_t* ring, void* element) {
    if (!ring) {
        return RING_ERROR_INVALID_PARAM;
    }
    
    size_t retries = 0;
    
    while (retries < MAX_CAS_RETRIES) {
        // Load current head
        size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
        size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
        
        // Check if full
        size_t available = ring->capacity - (head - tail);
        if (available == 0) {
            ring->stats.failed_enqueues++;
            return RING_ERROR_FULL;
        }
        
        // Get slot index
        size_t index = head & ring->mask;
        
        // Check sequence number (ensure slot is ready)
        size_t seq = atomic_load_explicit(&ring->sequences[index], memory_order_acquire);
        if (seq != head) {
            // Slot not ready, retry
            retries++;
            atomic_fetch_add_explicit(&ring->cas_failures, 1, memory_order_relaxed);
            continue;
        }
        
        // Try to claim slot with CAS
        size_t expected = head;
        if (atomic_compare_exchange_weak_explicit(&ring->head, &expected, head + 1,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire)) {
            // Successfully claimed slot, write data
            ring->data[index] = element;
            
            // Update sequence to mark slot as written
            atomic_store_explicit(&ring->sequences[index], head + 1, memory_order_release);
            
            // Update statistics
            ring->stats.total_enqueues++;
            size_t current_size = (head + 1) - tail;
            ring->stats.current_size = current_size;
            if (current_size > ring->stats.peak_size) {
                ring->stats.peak_size = current_size;
            }
            
            return RING_SUCCESS;
        }
        
        // CAS failed, retry
        retries++;
        atomic_fetch_add_explicit(&ring->cas_failures, 1, memory_order_relaxed);
    }
    
    // Too many retries, give up
    ring->stats.failed_enqueues++;
    return RING_ERROR_FULL;
}

ring_status_t mpsc_ring_dequeue(mpsc_ring_t* ring, void** element) {
    if (!ring || !element) {
        return RING_ERROR_INVALID_PARAM;
    }
    
    // Load head with acquire ordering
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    
    // Check if empty
    if (head == tail) {
        ring->stats.failed_dequeues++;
        return RING_ERROR_EMPTY;
    }
    
    // Get slot index
    size_t index = tail & ring->mask;
    
    // Wait for slot to be written (sequence == tail + 1)
    size_t seq;
    do {
        seq = atomic_load_explicit(&ring->sequences[index], memory_order_acquire);
    } while (seq != tail + 1);
    
    // Read data
    *element = ring->data[index];
    
    // Update sequence to mark slot as read
    atomic_store_explicit(&ring->sequences[index], tail + ring->capacity, memory_order_release);
    
    // Update tail
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
    
    // Update statistics
    ring->stats.total_dequeues++;
    ring->stats.current_size = head - (tail + 1);
    
    return RING_SUCCESS;
}

bool mpsc_ring_is_empty(const mpsc_ring_t* ring) {
    if (!ring) {
        return true;
    }
    
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    return head == tail;
}

size_t mpsc_ring_size(const mpsc_ring_t* ring) {
    if (!ring) {
        return 0;
    }
    
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    return head - tail;
}

size_t mpsc_ring_capacity(const mpsc_ring_t* ring) {
    return ring ? ring->capacity : 0;
}

void mpsc_ring_get_stats(const mpsc_ring_t* ring, ring_stats_t* stats) {
    if (ring && stats) {
        *stats = ring->stats;
    }
}

uint64_t mpsc_ring_get_cas_failures(const mpsc_ring_t* ring) {
    if (!ring) {
        return 0;
    }
    return atomic_load_explicit(&ring->cas_failures, memory_order_relaxed);
}

void mpsc_ring_reset_stats(mpsc_ring_t* ring) {
    if (ring) {
        memset(&ring->stats, 0, sizeof(ring_stats_t));
        atomic_store_explicit(&ring->cas_failures, 0, memory_order_relaxed);
    }
}
