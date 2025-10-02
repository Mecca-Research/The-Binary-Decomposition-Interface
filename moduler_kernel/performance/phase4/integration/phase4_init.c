#include "phase4_init.h"
#include "../pgo/pgo_profile.h"
#include "../intrinsics/cpu_features.h"
#include "../profiler/perf_collector.h"
#include "../profiler/ai_optimizer.h"
#include <stdio.h>

int phase4_init(void) {
    printf("Initializing Phase 4: Build Optimization & AI Tuning\n");
    
    // Initialize PGO
    if (pgo_init() < 0) {
        fprintf(stderr, "Failed to initialize PGO\n");
        return -1;
    }
    
    // Detect CPU features
    uint32_t features = cpu_detect_features();
    printf("CPU features detected: 0x%08x\n", features);
    
    char vendor[13], brand[49];
    cpu_get_vendor(vendor);
    cpu_get_brand(brand);
    printf("CPU: %s - %s\n", vendor, brand);
    
    // Initialize performance collector
    if (perf_collector_init() < 0) {
        fprintf(stderr, "Warning: Failed to initialize perf collector\n");
    }
    
    // Initialize AI optimizer
    if (ai_optimizer_init() < 0) {
        fprintf(stderr, "Failed to initialize AI optimizer\n");
        perf_collector_shutdown();
        pgo_shutdown();
        return -1;
    }
    
    printf("Phase 4 initialized successfully\n");
    return 0;
}

void phase4_shutdown(void) {
    printf("Shutting down Phase 4\n");
    ai_optimizer_shutdown();
    perf_collector_shutdown();
    pgo_shutdown();
}
