
/**
 * @file bench_phase2.c
 * @brief Comprehensive Phase 2 benchmark suite
 */

#include "phase2_init.h"
#include "numa_topology.h"
#include "per_cpu_arena.h"
#include "attention.h"
#include "huge_pages.h"
#include "pcid.h"
#include "timer_wheel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define BENCH_ITERATIONS 1000000

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static void bench_numa_allocation(void) {
    printf("\n=== NUMA Allocation Benchmark ===\n");
    
    double start = get_time_ms();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        void* ptr = per_cpu_arena_alloc(64);
        if (ptr) {
            per_cpu_arena_free(ptr, 64);
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    double ops_per_sec = BENCH_ITERATIONS / (elapsed / 1000.0);
    double ns_per_op = (elapsed * 1000000.0) / BENCH_ITERATIONS;
    
    printf("  Iterations: %d\n", BENCH_ITERATIONS);
    printf("  Time: %.2f ms\n", elapsed);
    printf("  Throughput: %.2f ops/sec\n", ops_per_sec);
    printf("  Latency: %.2f ns/op\n", ns_per_op);
    
    per_cpu_arena_stats_t stats;
    per_cpu_arena_get_stats(&stats);
    
    printf("\n  Statistics:\n");
    printf("    Local allocations: %lu (%.1f%%)\n", 
           stats.local_allocs,
           100.0 * stats.local_allocs / stats.total_allocs);
    printf("    Remote allocations: %lu (%.1f%%)\n",
           stats.remote_allocs,
           100.0 * stats.remote_allocs / stats.total_allocs);
}

static void bench_huge_pages(void) {
    printf("\n=== Huge Page Benchmark ===\n");
    
    // Allocate 2MB huge pages
    double start = get_time_ms();
    
    void* pages[100];
    int allocated = 0;
    
    for (int i = 0; i < 100; i++) {
        pages[i] = huge_page_alloc(HUGE_PAGE_TYPE_2MB);
        if (pages[i]) {
            allocated++;
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  Allocated: %d x 2MB pages\n", allocated);
    printf("  Time: %.2f ms\n", elapsed);
    printf("  Latency: %.2f ms/page\n", elapsed / allocated);
    
    // Free pages
    for (int i = 0; i < allocated; i++) {
        if (pages[i]) {
            huge_page_free(pages[i], HUGE_PAGE_TYPE_2MB);
        }
    }
    
    huge_page_stats_t stats;
    huge_page_get_stats(&stats);
    
    printf("\n  Statistics:\n");
    printf("    Total 2MB allocations: %lu\n", stats.total_2mb_allocs);
    printf("    Failed allocations: %lu\n", stats.failed_2mb_allocs);
    printf("    Success rate: %.1f%%\n",
           100.0 * stats.total_2mb_allocs / (stats.total_2mb_allocs + stats.failed_2mb_allocs));
}

static void bench_pcid(void) {
    printf("\n=== PCID Benchmark ===\n");
    
    double start = get_time_ms();
    
    uint16_t pcids[1000];
    int allocated = 0;
    
    for (int i = 0; i < 1000; i++) {
        pcids[i] = pcid_alloc(i);
        if (pcids[i] != PCID_INVALID) {
            allocated++;
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  Allocated: %d PCIDs\n", allocated);
    printf("  Time: %.2f ms\n", elapsed);
    printf("  Latency: %.2f µs/PCID\n", (elapsed * 1000.0) / allocated);
    
    // Test invalidation
    start = get_time_ms();
    
    for (int i = 0; i < allocated; i++) {
        pcid_invalidate_all(pcids[i]);
    }
    
    end = get_time_ms();
    elapsed = end - start;
    
    printf("\n  Invalidation:\n");
    printf("    Time: %.2f ms\n", elapsed);
    printf("    Latency: %.2f µs/invalidation\n", (elapsed * 1000.0) / allocated);
    
    // Free PCIDs
    for (int i = 0; i < allocated; i++) {
        pcid_free(pcids[i]);
    }
    
    pcid_stats_t stats;
    pcid_get_stats(&stats);
    
    printf("\n  Statistics:\n");
    printf("    Total allocations: %lu\n", stats.total_allocations);
    printf("    Evictions: %lu\n", stats.evictions);
    printf("    TLB flushes avoided: %lu\n", stats.tlb_flushes_avoided);
}

static int g_timer_count = 0;

static void timer_callback(void* arg) {
    g_timer_count++;
}

static void bench_timer_wheel(void) {
    printf("\n=== Timer Wheel Benchmark ===\n");
    
    timer_wheel_t* wheel = timer_wheel_create();
    
    // Add timers
    double start = get_time_ms();
    
    timer_id_t ids[10000];
    for (int i = 0; i < 10000; i++) {
        ids[i] = timer_wheel_add(wheel, i % 1000, timer_callback, NULL);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  Add 10000 timers:\n");
    printf("    Time: %.2f ms\n", elapsed);
    printf("    Latency: %.2f ns/timer\n", (elapsed * 1000000.0) / 10000);
    
    // Cancel timers
    start = get_time_ms();
    
    for (int i = 0; i < 5000; i++) {
        timer_wheel_cancel(wheel, ids[i]);
    }
    
    end = get_time_ms();
    elapsed = end - start;
    
    printf("\n  Cancel 5000 timers:\n");
    printf("    Time: %.2f ms\n", elapsed);
    printf("    Latency: %.2f ns/timer\n", (elapsed * 1000000.0) / 5000);
    
    // Advance time
    g_timer_count = 0;
    start = get_time_ms();
    
    for (int i = 0; i < 1000; i++) {
        timer_wheel_tick(wheel);
    }
    
    end = get_time_ms();
    elapsed = end - start;
    
    printf("\n  Advance 1000 ticks:\n");
    printf("    Time: %.2f ms\n", elapsed);
    printf("    Latency: %.2f µs/tick\n", (elapsed * 1000.0) / 1000);
    printf("    Timers expired: %d\n", g_timer_count);
    
    timer_wheel_stats_t stats;
    timer_wheel_get_stats(wheel, &stats);
    
    printf("\n  Statistics:\n");
    printf("    Total timers added: %lu\n", stats.total_timers_added);
    printf("    Total timers cancelled: %lu\n", stats.total_timers_cancelled);
    printf("    Total timers expired: %lu\n", stats.total_timers_expired);
    printf("    Total cascades: %lu\n", stats.total_cascades);
    
    timer_wheel_destroy(wheel);
}

static void bench_integrated(void) {
    printf("\n=== Integrated Benchmark ===\n");
    printf("  (NUMA + Huge Pages + Timers)\n\n");
    
    timer_wheel_t* wheel = timer_wheel_create();
    
    double start = get_time_ms();
    
    // Simulate realistic workload
    for (int i = 0; i < 10000; i++) {
        // Allocate memory
        void* ptr = per_cpu_arena_alloc(128);
        
        // Add timer
        timer_wheel_add(wheel, i % 100, timer_callback, NULL);
        
        // Advance time occasionally
        if (i % 100 == 0) {
            timer_wheel_tick(wheel);
        }
        
        // Free memory
        if (ptr) {
            per_cpu_arena_free(ptr, 128);
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  Iterations: 10000\n");
    printf("  Time: %.2f ms\n", elapsed);
    printf("  Throughput: %.2f ops/sec\n", 10000.0 / (elapsed / 1000.0));
    
    timer_wheel_destroy(wheel);
}

int main(int argc, char** argv) {
    printf("Phase 2 Comprehensive Benchmark Suite\n");
    printf("======================================\n");
    
    // Initialize Phase 2
    printf("\nInitializing Phase 2...\n");
    if (phase2_init(NULL) < 0) {
        fprintf(stderr, "Failed to initialize Phase 2\n");
        return 1;
    }
    
    // Run benchmarks
    bench_numa_allocation();
    bench_huge_pages();
    bench_pcid();
    bench_timer_wheel();
    bench_integrated();
    
    // Print summary
    printf("\n=== Summary ===\n");
    phase2_print_all_stats();
    
    // Cleanup
    phase2_destroy();
    
    printf("\nBenchmark complete!\n");
    return 0;
}
