
// ===================================================================
// BDI Modular Kernel Test Suite
// Comprehensive testing for all modular components
// ===================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../uabi/uops.h"
#include "../capgraph/capability.h"
#include "../orchestrator/orchestrator.h"
#include "../attention_mm/attention_mm.h"

// ===================================================================
// Test Framework
// ===================================================================

typedef struct {
    const char* name;
    bool (*test_func)(void);
} test_case_t;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    if (!(condition)) { \
        printf("  FAIL: %s\n", message); \
        return false; \
    } \
} while(0)

#define RUN_TEST(test_func) do { \
    printf("Running %s...\n", #test_func); \
    tests_run++; \
    if (test_func()) { \
        printf("  PASS\n"); \
        tests_passed++; \
    } else { \
        printf("  FAIL\n"); \
        tests_failed++; \
    } \
} while(0)

// ===================================================================
// Capability Detection Tests
// ===================================================================

bool test_capability_detection(void) {
    bdi_caps_t caps;
    
    // Test capability detection
    bdi_probe_caps(&caps);
    
    TEST_ASSERT(caps.architecture != BDI_ARCH_UNKNOWN, "Architecture should be detected");
    TEST_ASSERT(caps.memory.numa_nodes > 0, "Should detect at least one NUMA node");
    TEST_ASSERT(caps.memory.total_memory_mb > 0, "Should detect system memory");
    TEST_ASSERT(caps.memory.cache_line_size > 0, "Should detect cache line size");
    
    // Test capability validation
    TEST_ASSERT(bdi_validate_caps(&caps), "Detected capabilities should be valid");
    
    // Test capability digest
    uint64_t digest1 = bdi_caps_digest(&caps);
    uint64_t digest2 = bdi_caps_digest(&caps);
    TEST_ASSERT(digest1 == digest2, "Capability digest should be stable");
    TEST_ASSERT(digest1 != 0, "Capability digest should not be zero");
    
    // Test capability queries
    bool has_some_simd = bdi_has_capability(&caps, "avx2") || 
                        bdi_has_capability(&caps, "neon") ||
                        bdi_has_capability(&caps, "rvv");
    
    // Test string representation
    char buffer[2048];
    bdi_caps_to_string(&caps, buffer, sizeof(buffer));
    TEST_ASSERT(strlen(buffer) > 0, "Capability string should not be empty");
    TEST_ASSERT(strstr(buffer, "BDI Capability Summary") != NULL, 
                "Should contain capability summary header");
    
    return true;
}

// ===================================================================
// μABI Tests
// ===================================================================

bool test_uabi_initialization(void) {
    bdi_caps_t caps;
    bdi_probe_caps(&caps);
    
    // Initialize μABI
    bdi_init_uops(&caps);
    
    // Test that basic operations are available
    TEST_ASSERT(bdi_uops.memcpy_fast != NULL, "memcpy_fast should be initialized");
    TEST_ASSERT(bdi_uops.memset_fast != NULL, "memset_fast should be initialized");
    TEST_ASSERT(bdi_uops.lfence != NULL, "lfence should be initialized");
    TEST_ASSERT(bdi_uops.ticks != NULL, "ticks should be initialized");
    
    // Test version information
    bdi_uabi_version_t version;
    bdi_get_uabi_version(&version);
    TEST_ASSERT(version.major == 0 && version.minor == 1, "Version should be 0.1");
    TEST_ASSERT(version.arch != NULL, "Architecture string should be set");
    
    return true;
}

bool test_uabi_operations(void) {
    // This test is performed by bdi_verify_uops()
    TEST_ASSERT(bdi_verify_uops(), "μABI self-tests should pass");
    return true;
}

bool test_uabi_performance_hints(void) {
    char test_data[1024];
    
    // Test prefetch hints (should not crash)
    bdi_prefetch(test_data, BDI_HINT_TEMPORAL);
    bdi_prefetch(test_data, BDI_HINT_NON_TEMPORAL);
    bdi_prefetch(test_data, BDI_HINT_STREAMING);
    
    // Test cache operations (should not crash)
    bdi_cache_flush(test_data, sizeof(test_data));
    bdi_cache_invalidate(test_data, sizeof(test_data));
    
    return true;
}

// ===================================================================
// Attention Memory Manager Tests
// ===================================================================

bool test_attention_mm_basic(void) {
    bdi_attention_config_t config = {
        .attention_learning_rate = 0.1f,
        .recency_decay_rate = 0.9f,
        .hotness_learning_rate = 0.2f,
        .regularization_factor = 0.01f,
        .weight_attention = 0.6f,
        .weight_recency = 0.3f,
        .weight_hotness = 0.1f,
        .eviction_threshold = 0.3f,
        .update_frequency = 100,
        .gc_frequency = 1000,
        .enable_prefetching = true,
        .enable_numa_balancing = false
    };
    
    bdi_attention_mm_t* mm = bdi_attention_mm_create(&config);
    TEST_ASSERT(mm != NULL, "Memory manager should be created");
    
    // Test basic allocation
    void* ptr1 = bdi_attention_alloc(mm, 1024, 0);
    TEST_ASSERT(ptr1 != NULL, "Should allocate memory");
    
    void* ptr2 = bdi_attention_alloc_with_hint(mm, 2048, BDI_PAGE_CRITICAL, 
                                              0.8f, BDI_ACCESS_SEQUENTIAL);
    TEST_ASSERT(ptr2 != NULL, "Should allocate memory with hints");
    
    // Test attention score management
    TEST_ASSERT(bdi_set_attention_score(mm, ptr1, 0.7f), "Should set attention score");
    float score = bdi_get_attention_score(mm, ptr1);
    TEST_ASSERT(score == 0.7f, "Should retrieve correct attention score");
    
    // Test attention updates
    TEST_ASSERT(bdi_update_attention_hint(mm, ptr1, 1.5f), "Should update attention");
    
    // Test memory access tracking
    bdi_track_memory_access(mm, ptr1, false);
    bdi_track_memory_access(mm, ptr2, true);
    bdi_track_cache_miss(mm, ptr1);
    
    // Test attention tick
    bdi_attention_tick(mm);
    
    // Test statistics
    bdi_attention_mm_stats_t stats;
    bdi_get_attention_mm_stats(mm, &stats);
    TEST_ASSERT(stats.total_allocations == 2, "Should track allocations");
    TEST_ASSERT(stats.bytes_allocated == 1024 + 2048, "Should track allocated bytes");
    
    // Test batch updates
    bdi_batch_attention_update_start(mm);
    bdi_batch_attention_update_add(mm, ptr1, 0.5f);
    bdi_batch_attention_update_add(mm, ptr2, 1.2f);
    bdi_batch_attention_update_commit(mm);
    
    // Test freeing
    bdi_attention_free(mm, ptr1);
    bdi_attention_free(mm, ptr2);
    
    bdi_get_attention_mm_stats(mm, &stats);
    TEST_ASSERT(stats.total_frees == 2, "Should track frees");
    
    bdi_attention_mm_destroy(mm);
    return true;
}

bool test_attention_mm_learning(void) {
    bdi_attention_mm_t* mm = bdi_attention_mm_create(NULL);
    TEST_ASSERT(mm != NULL, "Memory manager should be created");
    
    void* ptr = bdi_attention_alloc_with_hint(mm, 4096, 0, 0.5f, BDI_ACCESS_RANDOM);
    TEST_ASSERT(ptr != NULL, "Should allocate memory");
    
    float initial_score = bdi_get_attention_score(mm, ptr);
    
    // Simulate high activity
    for (int i = 0; i < 10; i++) {
        bdi_update_attention_hint(mm, ptr, 2.0f);
        bdi_track_memory_access(mm, ptr, i % 2 == 0);
    }
    
    float after_activity = bdi_get_attention_score(mm, ptr);
    TEST_ASSERT(after_activity > initial_score, "Attention should increase with activity");
    
    // Simulate decay
    for (int i = 0; i < 20; i++) {
        bdi_attention_tick(mm);
    }
    
    float after_decay = bdi_get_attention_score(mm, ptr);
    TEST_ASSERT(after_decay < after_activity, "Attention should decay over time");
    
    bdi_attention_free(mm, ptr);
    bdi_attention_mm_destroy(mm);
    return true;
}

// ===================================================================
// Orchestrator Tests
// ===================================================================

bool test_orchestrator_creation(void) {
    bdi_orchestrator_t* orch = bdi_orchestrator_create();
    TEST_ASSERT(orch != NULL, "Orchestrator should be created");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_PROBE, "Should start in PROBE phase");
    TEST_ASSERT(!orch->initialization_complete, "Should not be initialized yet");
    
    bdi_orchestrator_destroy(orch);
    return true;
}

bool test_orchestrator_phases(void) {
    bdi_orchestrator_t* orch = bdi_orchestrator_create();
    TEST_ASSERT(orch != NULL, "Orchestrator should be created");
    
    // Test individual phases
    TEST_ASSERT(bdi_orchestrator_probe(orch), "PROBE phase should succeed");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_PROBE, "Should be in PROBE phase");
    TEST_ASSERT(orch->capability_digest != 0, "Should have capability digest");
    
    TEST_ASSERT(bdi_orchestrator_plan(orch, "latency"), "PLAN phase should succeed");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_PLAN, "Should be in PLAN phase");
    
    TEST_ASSERT(bdi_orchestrator_compose(orch), "COMPOSE phase should succeed");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_COMPOSE, "Should be in COMPOSE phase");
    
    TEST_ASSERT(bdi_orchestrator_prove(orch), "PROVE phase should succeed");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_PROVE, "Should be in PROVE phase");
    
    bdi_orchestrator_destroy(orch);
    return true;
}

bool test_orchestrator_boot(void) {
    bdi_orchestrator_t* orch = bdi_orchestrator_create();
    TEST_ASSERT(orch != NULL, "Orchestrator should be created");
    
    // Test full boot sequence
    TEST_ASSERT(bdi_orchestrator_boot(orch, "balanced"), "Boot should succeed");
    TEST_ASSERT(orch->initialization_complete, "Should be initialized after boot");
    TEST_ASSERT(orch->current_phase == BDI_PHASE_RUNNING, "Should be in RUNNING phase");
    TEST_ASSERT(orch->performance_stats.successful_boots == 1, "Should track successful boot");
    TEST_ASSERT(orch->performance_stats.total_boot_time_us > 0, "Should track boot time");
    
    bdi_orchestrator_destroy(orch);
    return true;
}

bool test_orchestrator_profiles(void) {
    bdi_orchestrator_t* orch = bdi_orchestrator_create();
    TEST_ASSERT(orch != NULL, "Orchestrator should be created");
    
    // Test loading different profiles
    TEST_ASSERT(bdi_load_profile(orch, "latency"), "Should load latency profile");
    TEST_ASSERT(strcmp(orch->active_profile.name, "latency") == 0, "Profile name should match");
    TEST_ASSERT(orch->active_profile.weight_latency == 1.0f, "Latency weight should be 1.0");
    
    TEST_ASSERT(bdi_load_profile(orch, "throughput"), "Should load throughput profile");
    TEST_ASSERT(strcmp(orch->active_profile.name, "throughput") == 0, "Profile name should match");
    TEST_ASSERT(orch->active_profile.weight_throughput == 1.0f, "Throughput weight should be 1.0");
    
    TEST_ASSERT(bdi_load_profile(orch, "ai-train"), "Should load AI training profile");
    TEST_ASSERT(strcmp(orch->active_profile.name, "ai-train") == 0, "Profile name should match");
    
    TEST_ASSERT(bdi_load_profile(orch, "balanced"), "Should load balanced profile");
    TEST_ASSERT(strcmp(orch->active_profile.name, "balanced") == 0, "Profile name should match");
    
    bdi_orchestrator_destroy(orch);
    return true;
}

// ===================================================================
// Integration Tests
// ===================================================================

bool test_full_integration(void) {
    printf("  Running full integration test...\n");
    
    // Create orchestrator and boot with AI training profile
    bdi_orchestrator_t* orch = bdi_orchestrator_create();
    TEST_ASSERT(orch != NULL, "Orchestrator should be created");
    
    TEST_ASSERT(bdi_orchestrator_boot(orch, "ai-train"), "Should boot with AI profile");
    
    // Verify μABI is working
    TEST_ASSERT(bdi_verify_uops(), "μABI should be functional after boot");
    
    // Test memory manager integration
    TEST_ASSERT(orch->memory_manager != NULL, "Memory manager should be initialized");
    
    void* test_ptr = bdi_attention_alloc(orch->memory_manager, 8192, BDI_PAGE_CRITICAL);
    TEST_ASSERT(test_ptr != NULL, "Should allocate memory through attention MM");
    
    // Simulate some workload
    for (int i = 0; i < 100; i++) {
        bdi_track_memory_access(orch->memory_manager, test_ptr, i % 3 == 0);
        if (i % 10 == 0) {
            bdi_attention_tick(orch->memory_manager);
        }
    }
    
    // Test μABI operations with allocated memory
    if (bdi_uops.memset_fast) {
        bdi_uops.memset_fast(test_ptr, 0x42, 1024);
    }
    
    if (bdi_uops.vec_add_f32) {
        float* float_ptr = (float*)test_ptr;
        float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float b[4] = {0.5f, 1.5f, 2.5f, 3.5f};
        bdi_uops.vec_add_f32(float_ptr, a, b, 4);
    }
    
    bdi_attention_free(orch->memory_manager, test_ptr);
    
    // Print final statistics
    printf("  Final orchestrator statistics:\n");
    bdi_print_orchestrator_stats(orch);
    
    printf("  Final memory manager statistics:\n");
    bdi_print_attention_mm_stats(orch->memory_manager);
    
    bdi_orchestrator_destroy(orch);
    return true;
}

// ===================================================================
// Performance Tests
// ===================================================================

bool test_performance_benchmarks(void) {
    printf("  Running performance benchmarks...\n");
    
    const size_t test_size = 1024 * 1024; // 1MB
    uint8_t* src = malloc(test_size);
    uint8_t* dst = malloc(test_size);
    
    TEST_ASSERT(src != NULL && dst != NULL, "Should allocate test buffers");
    
    // Initialize test data
    for (size_t i = 0; i < test_size; i++) {
        src[i] = (uint8_t)(i & 0xFF);
    }
    
    // Benchmark memcpy
    if (bdi_uops.memcpy_fast) {
        clock_t start = clock();
        for (int i = 0; i < 100; i++) {
            bdi_uops.memcpy_fast(dst, src, test_size);
        }
        clock_t end = clock();
        
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        double bandwidth_gbps = (100.0 * test_size) / (time_ms * 1000000.0);
        
        printf("    memcpy_fast: %.2f ms for 100x1MB = %.2f GB/s\n", time_ms, bandwidth_gbps);
        
        // Verify correctness
        TEST_ASSERT(memcmp(src, dst, test_size) == 0, "memcpy should be correct");
    }
    
    // Benchmark vector operations
    if (bdi_uops.vec_add_f32) {
        const size_t vec_size = test_size / sizeof(float);
        float* a = (float*)src;
        float* b = (float*)dst;
        float* c = malloc(test_size);
        TEST_ASSERT(c != NULL, "Should allocate result buffer");
        
        clock_t start = clock();
        for (int i = 0; i < 1000; i++) {
            bdi_uops.vec_add_f32(c, a, b, vec_size);
        }
        clock_t end = clock();
        
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        double ops_per_sec = (1000.0 * vec_size) / (time_ms / 1000.0);
        
        printf("    vec_add_f32: %.2f ms for 1000x%zu elements = %.0f ops/s\n", 
               time_ms, vec_size, ops_per_sec);
        
        free(c);
    }
    
    free(src);
    free(dst);
    return true;
}

// ===================================================================
// Main Test Runner
// ===================================================================

int main(int argc, char* argv[]) {
    printf("=== BDI Modular Kernel Test Suite ===\n\n");
    
    // Capability detection tests
    printf("--- Capability Detection Tests ---\n");
    RUN_TEST(test_capability_detection);
    
    // μABI tests
    printf("\n--- μABI Tests ---\n");
    RUN_TEST(test_uabi_initialization);
    RUN_TEST(test_uabi_operations);
    RUN_TEST(test_uabi_performance_hints);
    
    // Attention memory manager tests
    printf("\n--- Attention Memory Manager Tests ---\n");
    RUN_TEST(test_attention_mm_basic);
    RUN_TEST(test_attention_mm_learning);
    
    // Orchestrator tests
    printf("\n--- Orchestrator Tests ---\n");
    RUN_TEST(test_orchestrator_creation);
    RUN_TEST(test_orchestrator_phases);
    RUN_TEST(test_orchestrator_boot);
    RUN_TEST(test_orchestrator_profiles);
    
    // Integration tests
    printf("\n--- Integration Tests ---\n");
    RUN_TEST(test_full_integration);
    
    // Performance tests
    printf("\n--- Performance Tests ---\n");
    RUN_TEST(test_performance_benchmarks);
    
    // Summary
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("All tests PASSED! 🎉\n");
        return 0;
    } else {
        printf("Some tests FAILED! ❌\n");
        return 1;
    }
}
