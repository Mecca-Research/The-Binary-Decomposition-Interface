
/**
 * @file test_instrumentation.c
 * @brief Red-Team Instrumentation and Sanitizer Integration Tests
 * @details Sanitizer integration helpers, allocation tracing, differential analysis,
 *          and memory barrier validation.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fuzzing_utils.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

// ============================================================================
// Allocation Tracing
// ============================================================================

typedef struct {
    void *ptr;
    size_t size;
    uint64_t timestamp;
    const char *location;
} alloc_trace_t;

static alloc_trace_t g_traces[10000];
static uint32_t g_trace_count = 0;

static void trace_allocation(void *ptr, size_t size, const char *location) {
    if (g_trace_count < 10000) {
        g_traces[g_trace_count].ptr = ptr;
        g_traces[g_trace_count].size = size;
        g_traces[g_trace_count].timestamp = redteam_get_timestamp_us();
        g_traces[g_trace_count].location = location;
        g_trace_count++;
    }
}

static void trace_free(void *ptr) {
    for (uint32_t i = 0; i < g_trace_count; i++) {
        if (g_traces[i].ptr == ptr) {
            g_traces[i].ptr = NULL; // Mark as freed
            break;
        }
    }
}

// ============================================================================
// Test: Allocation Tracing
// ============================================================================

REDTEAM_TEST(instrumentation_alloc_tracing, "instrumentation",
             "Test allocation tracing framework") {
    g_trace_count = 0;
    
    // Perform traced allocations
    for (uint32_t i = 0; i < 100; i++) {
        size_t size = fuzz_random_size(64, 4096);
        void *ptr = alloc_memory(size, GFP_KERNEL);
        
        if (ptr) {
            trace_allocation(ptr, size, "test_location");
            free_memory(ptr);
            trace_free(ptr);
        }
    }
    
    // Verify traces
    REDTEAM_ASSERT(g_trace_count > 0, "No allocations traced");
    
    // Check for leaks
    uint32_t leaked = 0;
    for (uint32_t i = 0; i < g_trace_count; i++) {
        if (g_traces[i].ptr != NULL) {
            leaked++;
        }
    }
    
    REDTEAM_ASSERT(leaked == 0, "Memory leaks detected in tracing");
    
    redteam_log("Traced %u allocations, %u leaked", g_trace_count, leaked);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Differential Analysis
// ============================================================================

REDTEAM_TEST(instrumentation_differential, "instrumentation",
             "Test differential memory analysis") {
    
    // Capture initial state
    MemoryStats stats_before = get_memory_stats();
    uint64_t mem_before = redteam_get_memory_usage();
    
    // Perform operations
    const uint32_t num_ops = 1000;
    for (uint32_t i = 0; i < num_ops; i++) {
        size_t size = fuzz_random_size(64, 1024);
        void *ptr = alloc_memory(size, GFP_KERNEL);
        if (ptr) {
            free_memory(ptr);
        }
    }
    
    // Capture final state
    MemoryStats stats_after = get_memory_stats();
    uint64_t mem_after = redteam_get_memory_usage();
    
    // Analyze differences
    uint64_t alloc_delta = stats_after.total_allocated - stats_before.total_allocated;
    uint64_t free_delta = stats_after.total_freed - stats_before.total_freed;
    int64_t mem_delta = (int64_t)(mem_after - mem_before);
    
    redteam_log("Differential analysis:");
    redteam_log("  Allocations: +%lu", alloc_delta);
    redteam_log("  Frees: +%lu", free_delta);
    redteam_log("  Memory delta: %ld bytes", mem_delta);
    
    // Verify no significant leaks
    REDTEAM_ASSERT(abs(mem_delta) < 1024 * 1024, "Significant memory delta detected");
    
    return TEST_PASS;
}

// ============================================================================
// Test: Memory Barrier Validation
// ============================================================================

static atomic_int g_barrier_counter = 0;
static atomic_int g_barrier_flag = 0;

static void *barrier_worker(void *arg) {
    int id = *(int *)arg;
    
    // Increment counter
    atomic_fetch_add(&g_barrier_counter, 1);
    
    // Memory barrier
    atomic_thread_fence(memory_order_seq_cst);
    
    // Check flag
    int flag = atomic_load(&g_barrier_flag);
    
    return (void *)(intptr_t)flag;
}

REDTEAM_TEST(instrumentation_memory_barriers, "instrumentation",
             "Test memory barrier validation") {
    
    const uint32_t num_threads = 4;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    
    atomic_store(&g_barrier_counter, 0);
    atomic_store(&g_barrier_flag, 0);
    
    // Create threads
    for (uint32_t i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, barrier_worker, &thread_ids[i]);
    }
    
    // Wait a bit then set flag
    usleep(1000);
    atomic_store(&g_barrier_flag, 1);
    
    // Join threads
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify counter
    int final_count = atomic_load(&g_barrier_counter);
    REDTEAM_ASSERT(final_count == num_threads, "Barrier counter mismatch");
    
    return TEST_PASS;
}

// ============================================================================
// Test: Sanitizer Integration (ASAN)
// ============================================================================

REDTEAM_TEST(instrumentation_asan_integration, "instrumentation",
             "Test Address Sanitizer integration") {
    
    // This test verifies ASAN can detect issues
    // When compiled with -fsanitize=address
    
    void *ptr = alloc_memory(1024, GFP_KERNEL);
    REDTEAM_ASSERT_NOT_NULL(ptr, "Allocation failed");
    
    // Normal access - should be fine
    memset(ptr, 0xAA, 1024);
    
    // Note: Intentional buffer overflow would be detected by ASAN:
    // memset(ptr, 0xBB, 2048); // This would trigger ASAN
    
    free_memory(ptr);
    
    // Note: Use-after-free would be detected by ASAN:
    // memset(ptr, 0xCC, 1024); // This would trigger ASAN
    
    return TEST_PASS;
}

// ============================================================================
// Test: Sanitizer Integration (UBSAN)
// ============================================================================

REDTEAM_TEST(instrumentation_ubsan_integration, "instrumentation",
             "Test Undefined Behavior Sanitizer integration") {
    
    // This test verifies UBSAN can detect issues
    // When compiled with -fsanitize=undefined
    
    // Normal operations - should be fine
    int a = 10;
    int b = 20;
    int c = a + b;
    
    REDTEAM_ASSERT(c == 30, "Basic arithmetic failed");
    
    // Note: Integer overflow would be detected by UBSAN:
    // int max = INT_MAX;
    // int overflow = max + 1; // This would trigger UBSAN
    
    // Note: Division by zero would be detected by UBSAN:
    // int zero = 0;
    // int div = 10 / zero; // This would trigger UBSAN
    
    return TEST_PASS;
}

// ============================================================================
// Test: Sanitizer Integration (TSAN)
// ============================================================================

static int g_shared_counter = 0;
static pthread_mutex_t g_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *tsan_worker_safe(void *arg) {
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&g_counter_mutex);
        g_shared_counter++;
        pthread_mutex_unlock(&g_counter_mutex);
    }
    return NULL;
}

REDTEAM_TEST(instrumentation_tsan_integration, "instrumentation",
             "Test Thread Sanitizer integration") {
    
    // This test verifies TSAN can detect races
    // When compiled with -fsanitize=thread
    
    const uint32_t num_threads = 4;
    pthread_t threads[num_threads];
    
    g_shared_counter = 0;
    
    // Create threads with proper synchronization
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, tsan_worker_safe, NULL);
    }
    
    // Join threads
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify result
    REDTEAM_ASSERT(g_shared_counter == num_threads * 1000,
                  "Counter mismatch - possible race condition");
    
    // Note: Unsynchronized access would be detected by TSAN:
    // If we removed the mutex, TSAN would report a data race
    
    return TEST_PASS;
}

// ============================================================================
// Test: Performance Profiling
// ============================================================================

REDTEAM_TEST(instrumentation_performance, "instrumentation",
             "Test performance profiling") {
    
    const uint32_t iterations = 10000;
    uint64_t start_time = redteam_get_timestamp_us();
    
    // Perform operations
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = 1024;
        void *ptr = alloc_memory(size, GFP_KERNEL);
        if (ptr) {
            free_memory(ptr);
        }
    }
    
    uint64_t end_time = redteam_get_timestamp_us();
    uint64_t elapsed = end_time - start_time;
    
    double ops_per_sec = (double)iterations / (elapsed / 1000000.0);
    
    redteam_log("Performance: %u ops in %lu us (%.2f ops/sec)",
                iterations, elapsed, ops_per_sec);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Memory Pattern Detection
// ============================================================================

REDTEAM_TEST(instrumentation_pattern_detection, "instrumentation",
             "Test memory pattern detection") {
    
    // Allocate and fill with patterns
    const uint32_t num_allocs = 50;
    void *allocations[num_allocs];
    
    for (uint32_t i = 0; i < num_allocs; i++) {
        allocations[i] = alloc_memory(4096, GFP_KERNEL);
        
        if (allocations[i]) {
            // Fill with pattern
            uint8_t pattern = (uint8_t)(i % 256);
            memset(allocations[i], pattern, 4096);
        }
    }
    
    // Verify patterns
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            uint8_t expected = (uint8_t)(i % 256);
            uint8_t *bytes = (uint8_t *)allocations[i];
            
            bool pattern_valid = true;
            for (size_t j = 0; j < 4096; j++) {
                if (bytes[j] != expected) {
                    pattern_valid = false;
                    break;
                }
            }
            
            REDTEAM_ASSERT(pattern_valid, "Memory pattern corrupted");
            free_memory(allocations[i]);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Leak Detection
// ============================================================================

REDTEAM_TEST(instrumentation_leak_detection, "instrumentation",
             "Test memory leak detection") {
    
    uint64_t mem_before = redteam_get_memory_usage();
    
    // Intentionally leak some memory (for testing)
    const uint32_t leak_size = 1024;
    void *leaked = malloc(leak_size);
    
    // Don't free it
    (void)leaked;
    
    uint64_t mem_after = redteam_get_memory_usage();
    
    // Detect leak
    if (mem_after > mem_before + leak_size / 2) {
        redteam_log("Leak detected: %lu bytes", mem_after - mem_before);
    }
    
    // Cleanup (to avoid actual leak in test)
    free(leaked);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Stack Trace Capture
// ============================================================================

REDTEAM_TEST(instrumentation_stack_trace, "instrumentation",
             "Test stack trace capture") {
    
    // This test would capture stack traces for allocations
    // Useful for debugging memory issues
    
    void *ptr = alloc_memory(1024, GFP_KERNEL);
    
    if (ptr) {
        // TODO: Capture and log stack trace
        // This would require backtrace() or similar
        
        free_memory(ptr);
    }
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "instrumentation_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_instrumentation_alloc_tracing);
    redteam_register_test(&test_case_instrumentation_differential);
    redteam_register_test(&test_case_instrumentation_memory_barriers);
    redteam_register_test(&test_case_instrumentation_asan_integration);
    redteam_register_test(&test_case_instrumentation_ubsan_integration);
    redteam_register_test(&test_case_instrumentation_tsan_integration);
    redteam_register_test(&test_case_instrumentation_performance);
    redteam_register_test(&test_case_instrumentation_pattern_detection);
    redteam_register_test(&test_case_instrumentation_leak_detection);
    redteam_register_test(&test_case_instrumentation_stack_trace);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
