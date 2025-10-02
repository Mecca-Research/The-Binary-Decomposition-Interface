
/**
 * @file test_spsc_ring.c
 * @brief Unit tests for SPSC ring buffer
 */

#include "../rings/spsc_ring.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

// Test basic operations
void test_spsc_basic(void) {
    printf("Testing SPSC basic operations...\n");
    
    spsc_ring_t* ring = spsc_ring_create(16);
    assert(ring != NULL);
    assert(spsc_ring_capacity(ring) == 16);
    assert(spsc_ring_is_empty(ring));
    assert(!spsc_ring_is_full(ring));
    
    // Enqueue elements
    for (int i = 0; i < 10; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        assert(spsc_ring_enqueue(ring, data) == RING_SUCCESS);
    }
    
    assert(spsc_ring_size(ring) == 10);
    assert(!spsc_ring_is_empty(ring));
    assert(!spsc_ring_is_full(ring));
    
    // Dequeue elements
    for (int i = 0; i < 10; i++) {
        void* data = NULL;
        assert(spsc_ring_dequeue(ring, &data) == RING_SUCCESS);
        assert((uintptr_t)data == (uintptr_t)(i + 1));
    }
    
    assert(spsc_ring_is_empty(ring));
    
    spsc_ring_destroy(ring);
    printf("SPSC basic operations: PASSED\n");
}

// Test full condition
void test_spsc_full(void) {
    printf("Testing SPSC full condition...\n");
    
    spsc_ring_t* ring = spsc_ring_create(8);
    assert(ring != NULL);
    
    // Fill ring
    for (int i = 0; i < 8; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        assert(spsc_ring_enqueue(ring, data) == RING_SUCCESS);
    }
    
    assert(spsc_ring_is_full(ring));
    
    // Try to enqueue when full
    void* data = (void*)999;
    assert(spsc_ring_enqueue(ring, data) == RING_ERROR_FULL);
    
    spsc_ring_destroy(ring);
    printf("SPSC full condition: PASSED\n");
}

// Producer thread
void* producer_thread(void* arg) {
    spsc_ring_t* ring = (spsc_ring_t*)arg;
    
    for (int i = 0; i < 10000; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        while (spsc_ring_enqueue(ring, data) != RING_SUCCESS) {
            // Retry if full
        }
    }
    
    return NULL;
}

// Consumer thread
void* consumer_thread(void* arg) {
    spsc_ring_t* ring = (spsc_ring_t*)arg;
    
    for (int i = 0; i < 10000; i++) {
        void* data = NULL;
        while (spsc_ring_dequeue(ring, &data) != RING_SUCCESS) {
            // Retry if empty
        }
        assert((uintptr_t)data == (uintptr_t)(i + 1));
    }
    
    return NULL;
}

// Test concurrent access
void test_spsc_concurrent(void) {
    printf("Testing SPSC concurrent access...\n");
    
    spsc_ring_t* ring = spsc_ring_create(64);
    assert(ring != NULL);
    
    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer_thread, ring);
    pthread_create(&cons, NULL, consumer_thread, ring);
    
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    
    assert(spsc_ring_is_empty(ring));
    
    spsc_ring_destroy(ring);
    printf("SPSC concurrent access: PASSED\n");
}

int main(void) {
    printf("=== SPSC Ring Buffer Tests ===\n");
    
    test_spsc_basic();
    test_spsc_full();
    test_spsc_concurrent();
    
    printf("=== All SPSC tests PASSED ===\n");
    return 0;
}
