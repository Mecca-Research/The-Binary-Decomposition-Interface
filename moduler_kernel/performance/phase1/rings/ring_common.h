
/**
 * @file ring_common.h
 * @brief Common definitions for lock-free ring buffers
 * 
 * Provides shared types, constants, and utilities for SPSC and MPSC rings.
 */

#ifndef PHASE1_RING_COMMON_H
#define PHASE1_RING_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cache line size for alignment (x86-64)
#define CACHE_LINE_SIZE 64

// Alignment macro
#define CACHE_ALIGNED _Alignas(CACHE_LINE_SIZE)

// Ring buffer status codes
typedef enum {
    RING_SUCCESS = 0,
    RING_ERROR_FULL = -1,
    RING_ERROR_EMPTY = -2,
    RING_ERROR_INVALID_PARAM = -3,
    RING_ERROR_ALLOCATION = -4
} ring_status_t;

// Ring buffer statistics
typedef struct {
    uint64_t total_enqueues;
    uint64_t total_dequeues;
    uint64_t failed_enqueues;
    uint64_t failed_dequeues;
    uint64_t current_size;
    uint64_t peak_size;
} ring_stats_t;

// Memory ordering helpers
static inline void memory_fence_full(void) {
    atomic_thread_fence(memory_order_seq_cst);
}

static inline void memory_fence_acquire(void) {
    atomic_thread_fence(memory_order_acquire);
}

static inline void memory_fence_release(void) {
    atomic_thread_fence(memory_order_release);
}

// Power-of-2 check
static inline bool is_power_of_2(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Next power of 2
static inline size_t next_power_of_2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

#ifdef __cplusplus
}
#endif

#endif // PHASE1_RING_COMMON_H
