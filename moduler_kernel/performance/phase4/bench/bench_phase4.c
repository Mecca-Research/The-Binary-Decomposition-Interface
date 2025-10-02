#include "../integration/phase4_init.h"
#include "../profiler/perf_collector.h"
#include <stdio.h>
#include <time.h>

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(void) {
    printf("=== BDI Phase 4 Benchmark ===\n\n");
    
    double start = get_time();
    
    if (phase4_init() < 0) {
        fprintf(stderr, "Failed to initialize Phase 4\n");
        return 1;
    }
    
    double init_time = get_time() - start;
    printf("Phase 4 initialization: %.3f ms\n\n", init_time * 1000);
    
    // Benchmark with performance monitoring
    printf("Running benchmark with performance monitoring...\n");
    perf_collector_start();
    
    start = get_time();
    volatile long sum = 0;
    for (long i = 0; i < 100000000; i++) {
        sum += i;
    }
    double elapsed = get_time() - start;
    
    perf_counters_t counters;
    perf_collector_stop(&counters);
    
    printf("Benchmark completed in %.3f ms\n", elapsed * 1000);
    printf("Performance metrics:\n");
    printf("  Cycles: %lu\n", counters.cycles);
    printf("  Instructions: %lu\n", counters.instructions);
    printf("  IPC: %.2f\n", counters.ipc);
    printf("  Cache miss rate: %.2f%%\n", counters.cache_miss_rate * 100);
    
    phase4_shutdown();
    
    printf("\nPhase 4 benchmark complete\n");
    return 0;
}
