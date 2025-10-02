#include "../profiler/perf_collector.h"
#include "../profiler/ai_optimizer.h"
#include <stdio.h>
#include <unistd.h>

static void do_work(void) {
    volatile long sum = 0;
    for (long i = 0; i < 10000000; i++) {
        sum += i;
    }
}

int main(void) {
    printf("Testing performance collector...\n");
    
    if (perf_collector_init() < 0) {
        fprintf(stderr, "Failed to initialize perf collector (may need root)\n");
        return 1;
    }
    
    perf_collector_start();
    do_work();
    
    perf_counters_t counters;
    perf_collector_stop(&counters);
    
    printf("Cycles: %lu\n", counters.cycles);
    printf("Instructions: %lu\n", counters.instructions);
    printf("IPC: %.2f\n", counters.ipc);
    printf("Cache miss rate: %.2f%%\n", counters.cache_miss_rate * 100);
    printf("Branch miss rate: %.2f%%\n", counters.branch_miss_rate * 100);
    
    // Test AI optimizer
    ai_optimizer_init();
    optimization_recommendation_t recs[10];
    int num_recs = ai_optimizer_analyze(&counters, recs, 10);
    
    printf("\nAI Optimizer Recommendations: %d\n", num_recs);
    for (int i = 0; i < num_recs; i++) {
        printf("  [P%d] %s\n", recs[i].priority, recs[i].description);
        printf("       Expected improvement: %.1f%%\n", recs[i].expected_improvement);
    }
    
    ai_optimizer_shutdown();
    perf_collector_shutdown();
    
    printf("\nPerformance collector test: PASS\n");
    return 0;
}
