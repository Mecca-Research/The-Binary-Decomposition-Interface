
/**
 * @file thread_utils.h
 * @brief Thread Utilities for Multi-threaded Red-Team Testing
 * @details Provides thread pool management, CPU pinning, synchronization,
 *          and thread-safe statistics for concurrent testing.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include "../../../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

// ============================================================================
// Thread Configuration
// ============================================================================

#define MAX_THREADS 256

typedef void *(*thread_func_t)(void *arg);

// ============================================================================
// Thread Pool
// ============================================================================

typedef struct {
    pthread_t threads[MAX_THREADS];
    uint32_t thread_count;
    bool initialized;
} thread_pool_t;

// ============================================================================
// Thread-Safe Statistics
// ============================================================================

typedef struct {
    atomic_uint_fast64_t operations;
    atomic_uint_fast64_t successes;
    atomic_uint_fast64_t failures;
    atomic_uint_fast64_t bytes_allocated;
    atomic_uint_fast64_t bytes_freed;
    atomic_uint_fast64_t peak_memory;
} thread_safe_stats_t;

// ============================================================================
// Barrier
// ============================================================================

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint32_t count;
    uint32_t waiting;
    uint32_t generation;
} barrier_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize thread pool
 * @param pool Thread pool
 * @param count Number of threads
 * @return true on success
 */
bool thread_pool_init(thread_pool_t *pool, uint32_t count);

/**
 * @brief Create threads in pool
 * @param pool Thread pool
 * @param func Thread function
 * @param args Array of arguments (one per thread)
 * @return true on success
 */
bool thread_pool_create(thread_pool_t *pool, thread_func_t func, void **args);

/**
 * @brief Wait for all threads to complete
 * @param pool Thread pool
 * @return true on success
 */
bool thread_pool_join(thread_pool_t *pool);

/**
 * @brief Cleanup thread pool
 * @param pool Thread pool
 */
void thread_pool_cleanup(thread_pool_t *pool);

/**
 * @brief Pin thread to CPU
 * @param cpu_id CPU ID
 * @return true on success
 */
bool thread_pin_to_cpu(uint32_t cpu_id);

/**
 * @brief Get current CPU ID
 * @return CPU ID
 */
uint32_t thread_get_cpu_id(void);

/**
 * @brief Initialize barrier
 * @param barrier Barrier
 * @param count Number of threads
 * @return true on success
 */
bool barrier_init(barrier_t *barrier, uint32_t count);

/**
 * @brief Wait at barrier
 * @param barrier Barrier
 * @return true on success
 */
bool barrier_wait(barrier_t *barrier);

/**
 * @brief Destroy barrier
 * @param barrier Barrier
 */
void barrier_destroy(barrier_t *barrier);

/**
 * @brief Initialize thread-safe statistics
 * @param stats Statistics structure
 */
void thread_stats_init(thread_safe_stats_t *stats);

/**
 * @brief Record operation
 * @param stats Statistics structure
 * @param success Operation success
 */
void thread_stats_record_op(thread_safe_stats_t *stats, bool success);

/**
 * @brief Record allocation
 * @param stats Statistics structure
 * @param bytes Bytes allocated
 */
void thread_stats_record_alloc(thread_safe_stats_t *stats, uint64_t bytes);

/**
 * @brief Record free
 * @param stats Statistics structure
 * @param bytes Bytes freed
 */
void thread_stats_record_free(thread_safe_stats_t *stats, uint64_t bytes);

/**
 * @brief Get statistics snapshot
 * @param stats Statistics structure
 * @param operations Output: operations count
 * @param successes Output: successes count
 * @param failures Output: failures count
 */
void thread_stats_get(thread_safe_stats_t *stats, uint64_t *operations,
                     uint64_t *successes, uint64_t *failures);

/**
 * @brief Print thread-safe statistics
 * @param stats Statistics structure
 */
void thread_stats_print(thread_safe_stats_t *stats);

#endif // THREAD_UTILS_H
