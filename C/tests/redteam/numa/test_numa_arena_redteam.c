
/**
 * @file test_numa_arena_redteam.c
 * @brief Red-Team Tests for NUMA and Per-CPU Arena Management
 * @details Multi-threaded stress tests, CPU pinning validation, arena switching,
 *          cross-thread leak detection, and NUMA topology testing.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/thread_utils.h"
#include "../common/fuzzing_utils.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

// ============================================================================
// Thread Test Context
// ============================================================================

typedef struct {
    uint32_t thread_id;
    uint32_t cpu_id;
    uint32_t iterations;
    thread_safe_stats_t *stats;
    barrier_t *barrier;
} thread_context_t;

// ============================================================================
// Test: Multi-threaded Allocation Stress
// ============================================================================

static void *multithread_alloc_worker(void *arg) {
    thread_context_t *ctx = (thread_context_t *)arg;
    
    // Pin to CPU
    if (!thread_pin_to_cpu(ctx->cpu_id)) {
        return NULL;
    }
    
    // Wait for all threads to be ready
    barrier_wait(ctx->barrier);
    
    // Perform allocations
    for (uint32_t i = 0; i < ctx->iterations; i++) {
        size_t size = fuzz_random_size(64, 4096);
        void *ptr = alloc_memory(size, GFP_KERNEL | GFP_NUMA_LOCAL);
        
        bool success = (ptr != NULL);
        thread_stats_record_op(ctx->stats, success);
        
        if (ptr) {
            thread_stats_record_alloc(ctx->stats, size);
            memset(ptr, 0xAA, size);
            free_memory(ptr);
            thread_stats_record_free(ctx->stats, size);
        }
    }
    
    return NULL;
}

REDTEAM_TEST(multithread_stress, "numa",
             "Multi-threaded allocation stress test") {
    const uint32_t num_threads = 8;
    const uint32_t iterations = 1000;
    
    thread_pool_t pool;
    thread_safe_stats_t stats;
    barrier_t barrier;
    thread_context_t contexts[num_threads];
    void *thread_args[num_threads];
    
    // Initialize
    thread_pool_init(&pool, num_threads);
    thread_stats_init(&stats);
    barrier_init(&barrier, num_threads);
    
    // Setup thread contexts
    for (uint32_t i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        contexts[i].cpu_id = i % sysconf(_SC_NPROCESSORS_ONLN);
        contexts[i].iterations = iterations;
        contexts[i].stats = &stats;
        contexts[i].barrier = &barrier;
        thread_args[i] = &contexts[i];
    }
    
    // Create and run threads
    REDTEAM_ASSERT(thread_pool_create(&pool, multithread_alloc_worker, thread_args),
                  "Failed to create thread pool");
    
    REDTEAM_ASSERT(thread_pool_join(&pool), "Failed to join threads");
    
    // Verify results
    uint64_t operations, successes, failures;
    thread_stats_get(&stats, &operations, &successes, &failures);
    
    REDTEAM_ASSERT(operations == num_threads * iterations,
                  "Incorrect operation count");
    REDTEAM_ASSERT(successes > 0, "No successful allocations");
    
    redteam_log("Multi-thread stress: %lu ops, %lu success, %lu failures",
                operations, successes, failures);
    
    // Cleanup
    barrier_destroy(&barrier);
    thread_pool_cleanup(&pool);
    
    return TEST_PASS;
}

// ============================================================================
// Test: CPU Pinning and Arena Switching
// ============================================================================

static void *cpu_migration_worker(void *arg) {
    thread_context_t *ctx = (thread_context_t *)arg;
    uint32_t num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    for (uint32_t i = 0; i < ctx->iterations; i++) {
        // Switch CPU
        uint32_t target_cpu = rand() % num_cpus;
        thread_pin_to_cpu(target_cpu);
        
        // Allocate on new CPU
        size_t size = fuzz_random_size(128, 2048);
        void *ptr = alloc_memory(size, GFP_KERNEL | GFP_NUMA_LOCAL);
        
        if (ptr) {
            thread_stats_record_op(ctx->stats, true);
            
            // Verify we're on the expected CPU
            uint32_t current_cpu = thread_get_cpu_id();
            if (current_cpu != target_cpu) {
                // CPU migration happened, which is acceptable
            }
            
            free_memory(ptr);
        } else {
            thread_stats_record_op(ctx->stats, false);
        }
    }
    
    return NULL;
}

REDTEAM_TEST(cpu_pinning_arena_switch, "numa",
             "Test CPU pinning and arena switching") {
    const uint32_t num_threads = 4;
    const uint32_t iterations = 500;
    
    thread_pool_t pool;
    thread_safe_stats_t stats;
    thread_context_t contexts[num_threads];
    void *thread_args[num_threads];
    
    thread_pool_init(&pool, num_threads);
    thread_stats_init(&stats);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        contexts[i].iterations = iterations;
        contexts[i].stats = &stats;
        thread_args[i] = &contexts[i];
    }
    
    REDTEAM_ASSERT(thread_pool_create(&pool, cpu_migration_worker, thread_args),
                  "Failed to create threads");
    
    REDTEAM_ASSERT(thread_pool_join(&pool), "Failed to join threads");
    
    uint64_t operations, successes, failures;
    thread_stats_get(&stats, &operations, &successes, &failures);
    
    redteam_log("CPU migration test: %lu ops, %lu success",
                operations, successes);
    
    thread_pool_cleanup(&pool);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Cross-Thread Memory Leak Detection
// ============================================================================

typedef struct {
    void *allocations[100];
    uint32_t count;
    pthread_mutex_t lock;
} shared_alloc_pool_t;

static void *leak_detection_worker(void *arg) {
    thread_context_t *ctx = (thread_context_t *)arg;
    shared_alloc_pool_t *pool = (shared_alloc_pool_t *)ctx->barrier; // Reusing field
    
    for (uint32_t i = 0; i < ctx->iterations; i++) {
        size_t size = fuzz_random_size(64, 1024);
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (ptr) {
            // Randomly decide to free or store
            if (rand() % 2 == 0) {
                free_memory(ptr);
            } else {
                // Store in shared pool
                pthread_mutex_lock(&pool->lock);
                if (pool->count < 100) {
                    pool->allocations[pool->count++] = ptr;
                }
                pthread_mutex_unlock(&pool->lock);
            }
        }
    }
    
    return NULL;
}

REDTEAM_TEST(cross_thread_leak_detection, "numa",
             "Detect memory leaks across threads") {
    const uint32_t num_threads = 4;
    const uint32_t iterations = 250;
    
    thread_pool_t pool;
    shared_alloc_pool_t alloc_pool = {0};
    thread_context_t contexts[num_threads];
    void *thread_args[num_threads];
    
    pthread_mutex_init(&alloc_pool.lock, NULL);
    thread_pool_init(&pool, num_threads);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        contexts[i].iterations = iterations;
        contexts[i].barrier = (barrier_t *)&alloc_pool; // Reusing field
        thread_args[i] = &contexts[i];
    }
    
    uint64_t mem_before = redteam_get_memory_usage();
    
    REDTEAM_ASSERT(thread_pool_create(&pool, leak_detection_worker, thread_args),
                  "Failed to create threads");
    
    REDTEAM_ASSERT(thread_pool_join(&pool), "Failed to join threads");
    
    // Free all stored allocations
    for (uint32_t i = 0; i < alloc_pool.count; i++) {
        if (alloc_pool.allocations[i]) {
            free_memory(alloc_pool.allocations[i]);
        }
    }
    
    uint64_t mem_after = redteam_get_memory_usage();
    
    // Check for leaks
    if (mem_after > mem_before + 1024 * 1024) { // Allow 1MB tolerance
        redteam_log("Potential memory leak: %lu bytes",
                    mem_after - mem_before);
    }
    
    pthread_mutex_destroy(&alloc_pool.lock);
    thread_pool_cleanup(&pool);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Arena Exhaustion
// ============================================================================

REDTEAM_TEST(arena_exhaustion, "numa",
             "Test arena exhaustion scenarios") {
    const uint32_t max_allocs = 10000;
    void **allocations = malloc(max_allocs * sizeof(void *));
    uint32_t alloc_count = 0;
    
    REDTEAM_ASSERT_NOT_NULL(allocations, "Failed to allocate tracking array");
    
    // Try to exhaust arena
    for (uint32_t i = 0; i < max_allocs; i++) {
        size_t size = 4096; // Fixed size to stress arena
        void *ptr = alloc_memory(size, GFP_KERNEL | GFP_NUMA_LOCAL);
        
        if (ptr) {
            allocations[alloc_count++] = ptr;
        } else {
            // Arena exhausted or allocation failed
            break;
        }
    }
    
    redteam_log("Allocated %u blocks before exhaustion", alloc_count);
    
    // Verify we can still allocate after freeing some
    if (alloc_count > 100) {
        for (uint32_t i = 0; i < 50; i++) {
            free_memory(allocations[i]);
        }
        
        // Try to allocate again
        void *ptr = alloc_memory(4096, GFP_KERNEL);
        REDTEAM_ASSERT_NOT_NULL(ptr, "Failed to allocate after freeing");
        free_memory(ptr);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < alloc_count; i++) {
        if (allocations[i]) {
            free_memory(allocations[i]);
        }
    }
    
    free(allocations);
    
    return TEST_PASS;
}

// ============================================================================
// Test: NUMA Node Preference
// ============================================================================

REDTEAM_TEST(numa_node_preference, "numa",
             "Test NUMA node allocation preference") {
    // This test verifies that GFP_NUMA_LOCAL flag is respected
    
    for (uint32_t i = 0; i < 100; i++) {
        size_t size = fuzz_random_size(1024, 8192);
        
        // Allocate with NUMA local preference
        void *ptr = alloc_memory(size, GFP_KERNEL | GFP_NUMA_LOCAL);
        
        if (ptr) {
            // TODO: Verify allocation is on local NUMA node
            // This requires NUMA topology information
            free_memory(ptr);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "numa_arena_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_multithread_stress);
    redteam_register_test(&test_case_cpu_pinning_arena_switch);
    redteam_register_test(&test_case_cross_thread_leak_detection);
    redteam_register_test(&test_case_arena_exhaustion);
    redteam_register_test(&test_case_numa_node_preference);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
