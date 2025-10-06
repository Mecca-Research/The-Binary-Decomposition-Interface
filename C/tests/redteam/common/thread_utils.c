
/**
 * @file thread_utils.c
 * @brief Thread Utilities Implementation
 */

#include "thread_utils.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>

// ============================================================================
// Thread Pool Implementation
// ============================================================================

bool thread_pool_init(thread_pool_t *pool, uint32_t count) {
    if (!pool || count == 0 || count > MAX_THREADS) {
        return false;
    }
    
    memset(pool, 0, sizeof(thread_pool_t));
    pool->thread_count = count;
    pool->initialized = true;
    
    return true;
}

bool thread_pool_create(thread_pool_t *pool, thread_func_t func, void **args) {
    if (!pool || !pool->initialized || !func) {
        return false;
    }
    
    for (uint32_t i = 0; i < pool->thread_count; i++) {
        void *arg = args ? args[i] : NULL;
        if (pthread_create(&pool->threads[i], NULL, func, arg) != 0) {
            // Cleanup already created threads
            for (uint32_t j = 0; j < i; j++) {
                pthread_cancel(pool->threads[j]);
                pthread_join(pool->threads[j], NULL);
            }
            return false;
        }
    }
    
    return true;
}

bool thread_pool_join(thread_pool_t *pool) {
    if (!pool || !pool->initialized) {
        return false;
    }
    
    bool success = true;
    for (uint32_t i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            success = false;
        }
    }
    
    return success;
}

void thread_pool_cleanup(thread_pool_t *pool) {
    if (pool) {
        pool->initialized = false;
    }
}

bool thread_pin_to_cpu(uint32_t cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

uint32_t thread_get_cpu_id(void) {
    return sched_getcpu();
}

// ============================================================================
// Barrier Implementation
// ============================================================================

bool barrier_init(barrier_t *barrier, uint32_t count) {
    if (!barrier || count == 0) {
        return false;
    }
    
    barrier->count = count;
    barrier->waiting = 0;
    barrier->generation = 0;
    
    if (pthread_mutex_init(&barrier->mutex, NULL) != 0) {
        return false;
    }
    
    if (pthread_cond_init(&barrier->cond, NULL) != 0) {
        pthread_mutex_destroy(&barrier->mutex);
        return false;
    }
    
    return true;
}

bool barrier_wait(barrier_t *barrier) {
    if (!barrier) {
        return false;
    }
    
    pthread_mutex_lock(&barrier->mutex);
    
    uint32_t gen = barrier->generation;
    barrier->waiting++;
    
    if (barrier->waiting == barrier->count) {
        // Last thread to arrive
        barrier->waiting = 0;
        barrier->generation++;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return true;
    }
    
    // Wait for all threads
    while (gen == barrier->generation) {
        pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }
    
    pthread_mutex_unlock(&barrier->mutex);
    return true;
}

void barrier_destroy(barrier_t *barrier) {
    if (barrier) {
        pthread_mutex_destroy(&barrier->mutex);
        pthread_cond_destroy(&barrier->cond);
    }
}

// ============================================================================
// Thread-Safe Statistics Implementation
// ============================================================================

void thread_stats_init(thread_safe_stats_t *stats) {
    if (stats) {
        atomic_init(&stats->operations, 0);
        atomic_init(&stats->successes, 0);
        atomic_init(&stats->failures, 0);
        atomic_init(&stats->bytes_allocated, 0);
        atomic_init(&stats->bytes_freed, 0);
        atomic_init(&stats->peak_memory, 0);
    }
}

void thread_stats_record_op(thread_safe_stats_t *stats, bool success) {
    if (stats) {
        atomic_fetch_add(&stats->operations, 1);
        if (success) {
            atomic_fetch_add(&stats->successes, 1);
        } else {
            atomic_fetch_add(&stats->failures, 1);
        }
    }
}

void thread_stats_record_alloc(thread_safe_stats_t *stats, uint64_t bytes) {
    if (stats) {
        atomic_fetch_add(&stats->bytes_allocated, bytes);
        
        // Update peak memory
        uint64_t current = atomic_load(&stats->bytes_allocated) - 
                          atomic_load(&stats->bytes_freed);
        uint64_t peak = atomic_load(&stats->peak_memory);
        
        while (current > peak) {
            if (atomic_compare_exchange_weak(&stats->peak_memory, &peak, current)) {
                break;
            }
        }
    }
}

void thread_stats_record_free(thread_safe_stats_t *stats, uint64_t bytes) {
    if (stats) {
        atomic_fetch_add(&stats->bytes_freed, bytes);
    }
}

void thread_stats_get(thread_safe_stats_t *stats, uint64_t *operations,
                     uint64_t *successes, uint64_t *failures) {
    if (stats) {
        if (operations) *operations = atomic_load(&stats->operations);
        if (successes) *successes = atomic_load(&stats->successes);
        if (failures) *failures = atomic_load(&stats->failures);
    }
}

void thread_stats_print(thread_safe_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    uint64_t ops = atomic_load(&stats->operations);
    uint64_t succ = atomic_load(&stats->successes);
    uint64_t fail = atomic_load(&stats->failures);
    uint64_t alloc = atomic_load(&stats->bytes_allocated);
    uint64_t freed = atomic_load(&stats->bytes_freed);
    uint64_t peak = atomic_load(&stats->peak_memory);
    
    printf("Thread Statistics:\n");
    printf("  Operations:  %lu\n", ops);
    printf("  Successes:   %lu (%.1f%%)\n", succ, 100.0 * succ / ops);
    printf("  Failures:    %lu (%.1f%%)\n", fail, 100.0 * fail / ops);
    printf("  Allocated:   %lu bytes\n", alloc);
    printf("  Freed:       %lu bytes\n", freed);
    printf("  Peak Memory: %lu bytes\n", peak);
    printf("  Current:     %ld bytes\n", (int64_t)(alloc - freed));
}
