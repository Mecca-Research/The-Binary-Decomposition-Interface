
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
// Helper Functions
// ============================================================================

/**
 * @brief Determine appropriate HAM tier based on allocation size
 * @param size Size in bytes
 * @return Appropriate HamTier
 */
static HamTier determine_tier_from_size(size_t size) {
    if (size <= 4096) {
        return HAM_ACTIVE;          // Small allocations
    } else if (size <= 64 * 1024) {
        return HAM_ACTIVE;          // Medium allocations
    } else if (size <= 1024 * 1024) {
        return HAM_DORMANT;         // Large allocations
    } else {
        return HAM_ARCHIVE;         // Very large allocations
    }
}

/**
 * @brief Wrapper for HAM allocation with proper API
 * @param size Size in bytes
 * @param out_id Output region ID
 * @param out_ptr Output pointer
 * @return 0 on success, negative on failure
 */
static int ham_alloc_wrapper(size_t size, RegionId *out_id, void **out_ptr) {
    HamTier tier = determine_tier_from_size(size);
    return ham_alloc(out_id, tier, size, out_ptr);
}

// ============================================================================
// Test: HAM Initialization
// ============================================================================

REDTEAM_TEST(ham_initialization, "ham",
             "Test HAM initialization and cleanup") {
    
    // Initialize HAM
    int init_result = ham_init();
    REDTEAM_ASSERT(init_result == 0, "HAM initialization failed");
    
    // Cleanup HAM
    ham_shutdown();
    
    // Re-initialize
    init_result = ham_init();
    REDTEAM_ASSERT(init_result == 0, "HAM re-initialization failed");
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Tier Allocation
// ============================================================================

REDTEAM_TEST(ham_tier_allocation, "ham",
             "Test allocation across different HAM tiers") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    // Test allocations of different sizes to trigger different tiers
    size_t tier_sizes[] = {
        64,             // Small tier
        1024,           // Medium tier
        64 * 1024,      // Large tier
        1024 * 1024,    // Very large tier
    };
    
    for (size_t i = 0; i < sizeof(tier_sizes) / sizeof(tier_sizes[0]); i++) {
        RegionId region_id;
        void *ptr = NULL;
        
        result = ham_alloc_wrapper(tier_sizes[i], &region_id, &ptr);
        
        if (result == 0 && ptr != NULL) {
            memset(ptr, 0xEE, tier_sizes[i]);
            ham_free(region_id);
        }
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Tier Transition
// ============================================================================

REDTEAM_TEST(ham_tier_transition, "ham",
             "Test tier transition mechanisms") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    const uint32_t num_allocs = 100;
    RegionId region_ids[num_allocs];
    void *allocations[num_allocs];
    
    // Initialize arrays
    for (uint32_t i = 0; i < num_allocs; i++) {
        region_ids[i] = 0;
        allocations[i] = NULL;
    }
    
    // Allocate in one tier
    for (uint32_t i = 0; i < num_allocs; i++) {
        result = ham_alloc_wrapper(1024, &region_ids[i], &allocations[i]);
        if (result != 0) {
            allocations[i] = NULL;
        }
    }
    
    // Free half to potentially trigger tier transition
    for (uint32_t i = 0; i < num_allocs / 2; i++) {
        if (allocations[i] != NULL) {
            ham_free(region_ids[i]);
            allocations[i] = NULL;
            region_ids[i] = 0;
        }
    }
    
    // Allocate different sizes
    for (uint32_t i = 0; i < num_allocs / 2; i++) {
        result = ham_alloc_wrapper(64 * 1024, &region_ids[i], &allocations[i]);
        if (result != 0) {
            allocations[i] = NULL;
        }
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i] != NULL) {
            ham_free(region_ids[i]);
        }
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Motif Deduplication
// ============================================================================

REDTEAM_TEST(ham_motif_dedup, "ham",
             "Test motif deduplication") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    const uint32_t num_allocs = 50;
    RegionId region_ids[num_allocs];
    void *allocations[num_allocs];
    
    // Initialize arrays
    for (uint32_t i = 0; i < num_allocs; i++) {
        region_ids[i] = 0;
        allocations[i] = NULL;
    }
    
    // Allocate and fill with same pattern (should trigger deduplication)
    for (uint32_t i = 0; i < num_allocs; i++) {
        result = ham_alloc_wrapper(4096, &region_ids[i], &allocations[i]);
        
        if (result == 0 && allocations[i] != NULL) {
            memset(allocations[i], 0xAA, 4096);
        }
    }
    
    // Get global stats to check deduplication
    const HamGlobalStats *stats = ham_get_global_stats();
    if (stats != NULL) {
        redteam_log("Deduplication checks: %lu, hits: %lu, saved: %lu bytes",
                    (unsigned long)stats->dedup_checks,
                    (unsigned long)stats->dedup_hits,
                    (unsigned long)stats->dedup_saved_bytes);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i] != NULL) {
            ham_free(region_ids[i]);
        }
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Hash Collision Handling
// ============================================================================

REDTEAM_TEST(ham_hash_collision, "ham",
             "Test hash collision handling in motif deduplication") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    const uint32_t num_patterns = 100;
    RegionId region_ids[num_patterns];
    void *allocations[num_patterns];
    
    // Initialize arrays
    for (uint32_t i = 0; i < num_patterns; i++) {
        region_ids[i] = 0;
        allocations[i] = NULL;
    }
    
    // Create allocations with different patterns
    for (uint32_t i = 0; i < num_patterns; i++) {
        result = ham_alloc_wrapper(4096, &region_ids[i], &allocations[i]);
        
        if (result == 0 && allocations[i] != NULL) {
            // Fill with unique pattern
            uint8_t *bytes = (uint8_t *)allocations[i];
            for (size_t j = 0; j < 4096; j++) {
                bytes[j] = (uint8_t)(i + j);
            }
        }
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_patterns; i++) {
        if (allocations[i] != NULL) {
            ham_free(region_ids[i]);
        }
    }
    
    ham_shutdown();
    
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
        RegionId region_id;
        void *ptr = NULL;
        
        int result = ham_alloc_wrapper(size, &region_id, &ptr);
        
        bool success = (result == 0 && ptr != NULL);
        thread_stats_record_op(ctx->stats, success);
        
        if (success) {
            memset(ptr, 0xFF, size);
            ham_free(region_id);
        }
    }
    
    return NULL;
}

REDTEAM_TEST(ham_concurrent_ops, "ham",
             "Test concurrent HAM operations") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
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
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Region Management
// ============================================================================

REDTEAM_TEST(ham_region_management, "ham",
             "Test HAM region allocation and management") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    const uint32_t num_regions = 100;
    
    for (uint32_t i = 0; i < num_regions; i++) {
        size_t size = fuzz_random_size(4096, 64 * 1024);
        RegionId region_id;
        void *ptr = NULL;
        
        result = ham_alloc_wrapper(size, &region_id, &ptr);
        
        if (result == 0 && ptr != NULL) {
            // Immediately free to stress region management
            ham_free(region_id);
        }
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Compression Stress
// ============================================================================

REDTEAM_TEST(ham_compression_stress, "ham",
             "Stress test HAM compression") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    const uint32_t num_allocs = 50;
    RegionId region_ids[num_allocs];
    void *allocations[num_allocs];
    
    // Initialize arrays
    for (uint32_t i = 0; i < num_allocs; i++) {
        region_ids[i] = 0;
        allocations[i] = NULL;
    }
    
    // Allocate and fill with compressible data
    for (uint32_t i = 0; i < num_allocs; i++) {
        result = ham_alloc_wrapper(8192, &region_ids[i], &allocations[i]);
        
        if (result == 0 && allocations[i] != NULL) {
            // Fill with highly compressible pattern
            uint32_t *words = (uint32_t *)allocations[i];
            for (size_t j = 0; j < 8192 / sizeof(uint32_t); j++) {
                words[j] = 0x12345678;
            }
            
            // Try to compress the region
            ham_compress(region_ids[i]);
        }
    }
    
    // Get compression statistics
    const HamGlobalStats *stats = ham_get_global_stats();
    if (stats != NULL) {
        redteam_log("Compressions: %lu, decompressions: %lu",
                    (unsigned long)stats->compressions,
                    (unsigned long)stats->decompressions);
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_allocs; i++) {
        if (allocations[i] != NULL) {
            ham_free(region_ids[i]);
        }
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Entropy Analysis
// ============================================================================

REDTEAM_TEST(ham_entropy_analysis, "ham",
             "Test HAM entropy analysis") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    RegionId low_entropy_id, high_entropy_id;
    void *low_entropy = NULL;
    void *high_entropy = NULL;
    
    // Allocate with low entropy data
    result = ham_alloc_wrapper(4096, &low_entropy_id, &low_entropy);
    if (result == 0 && low_entropy != NULL) {
        memset(low_entropy, 0x00, 4096);
        
        // Update stats to analyze entropy
        ham_update_stats(low_entropy_id);
        
        // Get region stats
        HamStats stats;
        if (ham_get_region_stats(low_entropy_id, &stats) == 0) {
            redteam_log("Low entropy region: entropy_score=%.2f, compression_ratio=%.2f",
                        stats.entropy_score, stats.compression_ratio);
        }
    }
    
    // Allocate with high entropy data
    result = ham_alloc_wrapper(4096, &high_entropy_id, &high_entropy);
    if (result == 0 && high_entropy != NULL) {
        fuzz_fill_random(high_entropy, 4096);
        
        // Update stats to analyze entropy
        ham_update_stats(high_entropy_id);
        
        // Get region stats
        HamStats stats;
        if (ham_get_region_stats(high_entropy_id, &stats) == 0) {
            redteam_log("High entropy region: entropy_score=%.2f, compression_ratio=%.2f",
                        stats.entropy_score, stats.compression_ratio);
        }
    }
    
    if (low_entropy != NULL) ham_free(low_entropy_id);
    if (high_entropy != NULL) ham_free(high_entropy_id);
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: HAM Statistics
// ============================================================================

REDTEAM_TEST(ham_statistics, "ham",
             "Test HAM statistics tracking") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    // Perform various operations
    for (uint32_t i = 0; i < 100; i++) {
        size_t size = fuzz_random_size(64, 4096);
        RegionId region_id;
        void *ptr = NULL;
        
        result = ham_alloc_wrapper(size, &region_id, &ptr);
        
        if (result == 0 && ptr != NULL) {
            memset(ptr, 0xBB, size);
            ham_free(region_id);
        }
    }
    
    // Retrieve and verify HAM statistics
    const HamGlobalStats *stats = ham_get_global_stats();
    if (stats != NULL) {
        redteam_log("HAM Statistics:");
        redteam_log("  Total regions: %lu", (unsigned long)stats->total_regions);
        redteam_log("  Active regions: %lu", (unsigned long)stats->active_regions);
        redteam_log("  Total memory: %lu bytes", (unsigned long)stats->total_memory);
        redteam_log("  Compressed memory: %lu bytes", (unsigned long)stats->compressed_memory);
        redteam_log("  Promotions: %lu", (unsigned long)stats->promotions);
        redteam_log("  Demotions: %lu", (unsigned long)stats->demotions);
    }
    
    ham_shutdown();
    
    return TEST_PASS;
}

// ============================================================================
// Test: Tier Promotion and Demotion
// ============================================================================

REDTEAM_TEST(ham_tier_promotion_demotion, "ham",
             "Test HAM tier promotion and demotion") {
    
    int result = ham_init();
    REDTEAM_ASSERT(result == 0, "HAM initialization failed");
    
    RegionId region_id;
    void *ptr = NULL;
    
    // Allocate in ACTIVE tier
    result = ham_alloc(&region_id, HAM_ACTIVE, 4096, &ptr);
    REDTEAM_ASSERT(result == 0 && ptr != NULL, "Failed to allocate HAM region");
    
    // Get initial tier
    HamTier initial_tier = ham_get_region_tier(region_id);
    redteam_log("Initial tier: %d", initial_tier);
    
    // Try to promote
    result = ham_promote(region_id);
    if (result == 0) {
        HamTier promoted_tier = ham_get_region_tier(region_id);
        redteam_log("Promoted tier: %d", promoted_tier);
    }
    
    // Try to demote
    result = ham_demote(region_id);
    if (result == 0) {
        HamTier demoted_tier = ham_get_region_tier(region_id);
        redteam_log("Demoted tier: %d", demoted_tier);
    }
    
    ham_free(region_id);
    ham_shutdown();
    
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
    redteam_register_test(&test_case_ham_tier_promotion_demotion);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
