
/**
 * @file test_ham_redteam.c
 * @brief Red-Team Tests for Hierarchical Adaptive Memory (HAM)
 * @details Vtable fuzzing, tier transition testing, motif deduplication,
 *          concurrent operation safety, and region management validation.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fuzzing_utils.h"
#include "../common/thread_utils.h"
#include "../../../kernel/ham/ham.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test: HAM Initialization
// ============================================================================

REDTEAM_TEST(ham_initialization, "ham",
             "Test HAM initialization and cleanup") {
    
    // Initialize HAM
    bool init_result = ham_init();
    REDTEAM_ASSERT(init_result, "HAM initialization failed");
    
    // Cleanup HAM
    ham_cleanup();
    
    // Re-initialize
    init_result = ham_init();
    REDTEAM_ASSERT(init_result, "HAM re-initialization failed");
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Tier Allocation
// ============================================================================

REDTEAM_TEST(ham_tier_allocation, "ham",
             "Test allocation across different HAM tiers") {
    
    ham_init();
    
    // Test allocations of different sizes to trigger different tiers
    size_t tier_sizes[] = {
        64,             // Small tier
        1024,           // Medium tier
        64 * 1024,      // Large tier
        1024 * 1024,    // Very large tier
    };
    
    for (size_t i = 0; i < sizeof(tier_sizes) / sizeof(tier_sizes[0]); i++) {
        void *ptr = ham_alloc(tier_sizes[i]);
        
        if (ptr) {
            memset(ptr, 0xEE, tier_sizes[i]);
            ham_free(ptr);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Tier Transition
// ============================================================================

REDTEAM_TEST(ham_tier_transition, "ham",
             "Test tier transition mechanisms") {
    
    ham_init();
    
    const uint32_t num_allocs = 100;
    void *allocations[num_allocs];
    
    // Allocate in one tier
    for (uint32_t i = 0; i < num_allocs; i++) {
        allocations[i] = ham_alloc(1024);
    }
    
    // Free half to potentially trigger tier transition
    for (uint32_t i = 0; i < num_allocs / 2; i++) {
        if (allocations[i]) {
            ham_free(allocations[i]);
            allocations[i] = NULL;
        }
    }
    
    // Allocate different sizes
    for (uint32_t i = 0; i < num_allocs / 2; i++) {
        allocations[i] = ham_alloc(64 * 1024);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            ham_free(allocations[i]);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Motif Deduplication
// ============================================================================

REDTEAM_TEST(ham_motif_dedup, "ham",
             "Test motif deduplication") {
    
    ham_init();
    
    const uint32_t num_allocs = 50;
    void *allocations[num_allocs];
    
    // Allocate and fill with same pattern (should trigger deduplication)
    for (uint32_t i = 0; i < num_allocs; i++) {
        allocations[i] = ham_alloc(4096);
        
        if (allocations[i]) {
            memset(allocations[i], 0xAA, 4096);
        }
    }
    
    // TODO: Verify deduplication occurred
    // This would require HAM to expose deduplication statistics
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            ham_free(allocations[i]);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Hash Collision Handling
// ============================================================================

REDTEAM_TEST(ham_hash_collision, "ham",
             "Test hash collision handling in motif deduplication") {
    
    ham_init();
    
    const uint32_t num_patterns = 100;
    void *allocations[num_patterns];
    
    // Create allocations with different patterns
    for (uint32_t i = 0; i < num_patterns; i++) {
        allocations[i] = ham_alloc(4096);
        
        if (allocations[i]) {
            // Fill with unique pattern
            uint8_t *bytes = (uint8_t *)allocations[i];
            for (size_t j = 0; j < 4096; j++) {
                bytes[j] = (uint8_t)(i + j);
            }
        }
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_patterns; i++) {
        if (allocations[i]) {
            ham_free(allocations[i]);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Concurrent HAM Operations
// ============================================================================

typedef struct {
    uint32_t thread_id;
    uint32_t iterations;
    thread_safe_stats_t *stats;
} ham_thread_context_t;

static void *ham_concurrent_worker(void *arg) {
    ham_thread_context_t *ctx = (ham_thread_context_t *)arg;
    
    for (uint32_t i = 0; i < ctx->iterations; i++) {
        size_t size = fuzz_random_size(64, 8192);
        void *ptr = ham_alloc(size);
        
        bool success = (ptr != NULL);
        thread_stats_record_op(ctx->stats, success);
        
        if (ptr) {
            memset(ptr, 0xFF, size);
            ham_free(ptr);
        }
    }
    
    return NULL;
}

REDTEAM_TEST(ham_concurrent_ops, "ham",
             "Test concurrent HAM operations") {
    
    ham_init();
    
    const uint32_t num_threads = 8;
    const uint32_t iterations = 500;
    
    thread_pool_t pool;
    thread_safe_stats_t stats;
    ham_thread_context_t contexts[num_threads];
    void *thread_args[num_threads];
    
    thread_pool_init(&pool, num_threads);
    thread_stats_init(&stats);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        contexts[i].iterations = iterations;
        contexts[i].stats = &stats;
        thread_args[i] = &contexts[i];
    }
    
    REDTEAM_ASSERT(thread_pool_create(&pool, ham_concurrent_worker, thread_args),
                  "Failed to create threads");
    
    REDTEAM_ASSERT(thread_pool_join(&pool), "Failed to join threads");
    
    uint64_t operations, successes, failures;
    thread_stats_get(&stats, &operations, &successes, &failures);
    
    redteam_log("HAM concurrent ops: %lu total, %lu success",
                operations, successes);
    
    thread_pool_cleanup(&pool);
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Region Management
// ============================================================================

REDTEAM_TEST(ham_region_management, "ham",
             "Test HAM region allocation and management") {
    
    ham_init();
    
    const uint32_t num_regions = 100;
    
    for (uint32_t i = 0; i < num_regions; i++) {
        size_t size = fuzz_random_size(4096, 64 * 1024);
        void *ptr = ham_alloc(size);
        
        if (ptr) {
            // Immediately free to stress region management
            ham_free(ptr);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Compression Stress
// ============================================================================

REDTEAM_TEST(ham_compression_stress, "ham",
             "Stress test HAM compression") {
    
    ham_init();
    
    const uint32_t num_allocs = 50;
    void *allocations[num_allocs];
    
    // Allocate and fill with compressible data
    for (uint32_t i = 0; i < num_allocs; i++) {
        allocations[i] = ham_alloc(8192);
        
        if (allocations[i]) {
            // Fill with highly compressible pattern
            uint32_t *words = (uint32_t *)allocations[i];
            for (size_t j = 0; j < 8192 / sizeof(uint32_t); j++) {
                words[j] = 0x12345678;
            }
        }
    }
    
    // TODO: Verify compression occurred
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i]) {
            ham_free(allocations[i]);
        }
    }
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Entropy Analysis
// ============================================================================

REDTEAM_TEST(ham_entropy_analysis, "ham",
             "Test HAM entropy analysis") {
    
    ham_init();
    
    // Allocate with low entropy data
    void *low_entropy = ham_alloc(4096);
    if (low_entropy) {
        memset(low_entropy, 0x00, 4096);
    }
    
    // Allocate with high entropy data
    void *high_entropy = ham_alloc(4096);
    if (high_entropy) {
        fuzz_fill_random(high_entropy, 4096);
    }
    
    // TODO: Verify HAM handles different entropy levels appropriately
    
    if (low_entropy) ham_free(low_entropy);
    if (high_entropy) ham_free(high_entropy);
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Test: HAM Statistics
// ============================================================================

REDTEAM_TEST(ham_statistics, "ham",
             "Test HAM statistics tracking") {
    
    ham_init();
    
    // Perform various operations
    for (uint32_t i = 0; i < 100; i++) {
        size_t size = fuzz_random_size(64, 4096);
        void *ptr = ham_alloc(size);
        
        if (ptr) {
            memset(ptr, 0xBB, size);
            ham_free(ptr);
        }
    }
    
    // TODO: Retrieve and verify HAM statistics
    // ham_stats_t stats = ham_get_stats();
    
    ham_cleanup();
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "ham_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_ham_initialization);
    redteam_register_test(&test_case_ham_tier_allocation);
    redteam_register_test(&test_case_ham_tier_transition);
    redteam_register_test(&test_case_ham_motif_dedup);
    redteam_register_test(&test_case_ham_hash_collision);
    redteam_register_test(&test_case_ham_concurrent_ops);
    redteam_register_test(&test_case_ham_region_management);
    redteam_register_test(&test_case_ham_compression_stress);
    redteam_register_test(&test_case_ham_entropy_analysis);
    redteam_register_test(&test_case_ham_statistics);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
