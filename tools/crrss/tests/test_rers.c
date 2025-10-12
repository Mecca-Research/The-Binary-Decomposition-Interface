/**
 * @file test_rers.c
 * @brief Unit tests for RERS (Runtime Error Replay System)
 */

#include "../rers/rers.h"
#include "../rers/rers_replay.h"
#include "../rers/rers_patterns.h"
#include "../rers/rers_learning.h"
#include "../rers/rers_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test macros
#define TEST(name) \
    static void test_##name(void)

#define RUN_TEST(name) \
    do { \
        printf("Running test_%s...\n", #name); \
        test_##name(); \
        tests_run++; \
    } while(0)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  FAIL: %s\n", message); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b, message) \
    ASSERT((a) == (b), message)

#define ASSERT_NE(a, b, message) \
    ASSERT((a) != (b), message)

#define ASSERT_SUCCESS(status, message) \
    ASSERT((status) == CRRSS_SUCCESS, message)

#define TEST_PASS() \
    do { \
        printf("  PASS\n"); \
        tests_passed++; \
    } while(0)

// ==================== Test: Replay Engine ====================

TEST(replay_engine_init) {
    rers_replay_config_t config = {
        .enable_sandbox = true,
        .enable_timeout = true,
        .timeout_seconds = 30,
        .capture_output = true,
        .restore_state = true,
        .max_iterations = 10,
        .stop_on_success = true,
        .collect_metrics = true
    };
    
    rers_replay_engine_t* engine = rers_replay_init(&config);
    ASSERT_NE(engine, NULL, "Replay engine should initialize");
    
    rers_replay_cleanup(engine);
    TEST_PASS();
}

TEST(error_capture) {
    rers_replay_config_t config = {
        .enable_sandbox = false,
        .enable_timeout = true,
        .timeout_seconds = 30,
        .capture_output = false,
        .restore_state = false,
        .max_iterations = 1,
        .stop_on_success = true,
        .collect_metrics = false
    };
    
    rers_replay_engine_t* engine = rers_replay_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    rers_error_context_t context;
    crrss_status_t status = rers_capture_error_context(
        engine,
        RERS_ERROR_SEGFAULT,
        SIGSEGV,
        NULL,
        &context
    );
    
    ASSERT_SUCCESS(status, "Error capture should succeed");
    ASSERT_EQ(context.error_type, RERS_ERROR_SEGFAULT, "Error type should match");
    ASSERT_EQ(context.severity, RERS_SEVERITY_CRITICAL, "Severity should be critical");
    
    rers_replay_cleanup(engine);
    TEST_PASS();
}

TEST(error_replay) {
    rers_replay_config_t config = {
        .enable_sandbox = false,
        .enable_timeout = true,
        .timeout_seconds = 5,
        .capture_output = false,
        .restore_state = false,
        .max_iterations = 1,
        .stop_on_success = true,
        .collect_metrics = false
    };
    
    rers_replay_engine_t* engine = rers_replay_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    rers_error_context_t context = {
        .error_id = 1,
        .error_type = RERS_ERROR_NULL_DEREF,
        .severity = RERS_SEVERITY_HIGH
    };
    
    rers_replay_result_t result;
    crrss_status_t status = rers_replay(engine, &context, &result);
    
    ASSERT_SUCCESS(status, "Replay should succeed");
    ASSERT_EQ(result.error_id, 1, "Error ID should match");
    ASSERT(result.reproduced, "Error should be reproduced");
    
    rers_replay_cleanup(engine);
    TEST_PASS();
}

TEST(error_injection) {
    rers_replay_config_t config = {
        .enable_sandbox = false,
        .enable_timeout = true,
        .timeout_seconds = 5,
        .capture_output = false,
        .restore_state = false,
        .max_iterations = 1,
        .stop_on_success = true,
        .collect_metrics = false
    };
    
    rers_replay_engine_t* engine = rers_replay_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    rers_injection_params_t params = {
        .error_type = RERS_ERROR_BUFFER_OVERFLOW,
        .target_function = "test_function",
        .target_file = "test.c",
        .target_line = 100,
        .injection_count = 1,
        .injection_probability = 1.0
    };
    
    crrss_status_t status = rers_inject(engine, &params);
    ASSERT_SUCCESS(status, "Injection should succeed");
    
    status = rers_clear_injections(engine);
    ASSERT_SUCCESS(status, "Clear injections should succeed");
    
    rers_replay_cleanup(engine);
    TEST_PASS();
}

// ==================== Test: Pattern Database ====================

TEST(pattern_db_init) {
    rers_pattern_db_t* db = rers_pattern_db_init(1000, NULL);
    ASSERT_NE(db, NULL, "Pattern database should initialize");
    
    uint32_t count;
    crrss_status_t status = rers_pattern_db_count(db, &count);
    ASSERT_SUCCESS(status, "Count should succeed");
    ASSERT_EQ(count, 0, "Initial count should be 0");
    
    rers_pattern_db_cleanup(db);
    TEST_PASS();
}

TEST(pattern_insert_and_retrieve) {
    rers_pattern_db_t* db = rers_pattern_db_init(1000, NULL);
    ASSERT_NE(db, NULL, "Database should initialize");
    
    rers_pattern_entry_t pattern = {
        .pattern_type = PATTERN_MEMORY_LEAK,
        .pattern_name = "Test Pattern",
        .description = "Test description",
        .priority = BUG_PRIORITY_P1_HIGH_HIGH,
        .category = BUG_CATEGORY_MEMORY,
        .risk_level = RISK_LEVEL_HIGH,
        .risk_score = 0.8,
        .occurrence_count = 1,
        .is_active = true
    };
    
    uint32_t pattern_id;
    crrss_status_t status = rers_pattern_db_insert(db, &pattern, &pattern_id);
    ASSERT_SUCCESS(status, "Insert should succeed");
    ASSERT_EQ(pattern_id, 0, "First pattern ID should be 0");
    
    rers_pattern_entry_t retrieved;
    status = rers_pattern_db_get(db, pattern_id, &retrieved);
    ASSERT_SUCCESS(status, "Retrieve should succeed");
    ASSERT_EQ(retrieved.pattern_type, PATTERN_MEMORY_LEAK, "Pattern type should match");
    
    rers_pattern_db_cleanup(db);
    TEST_PASS();
}

TEST(pattern_matching) {
    rers_pattern_db_t* db = rers_pattern_db_init(1000, NULL);
    ASSERT_NE(db, NULL, "Database should initialize");
    
    // Insert a pattern
    rers_pattern_entry_t pattern = {
        .pattern_type = PATTERN_USE_AFTER_FREE,
        .pattern_name = "Use After Free",
        .description = "Use after free pattern",
        .priority = BUG_PRIORITY_P0_CRITICAL_CRITICAL,
        .category = BUG_CATEGORY_MEMORY,
        .risk_level = RISK_LEVEL_CRITICAL,
        .risk_score = 0.95,
        .occurrence_count = 5,
        .is_active = true
    };
    
    uint32_t pattern_id;
    rers_pattern_db_insert(db, &pattern, &pattern_id);
    
    // Match patterns
    bug_pattern_info_t query = {
        .pattern_type = PATTERN_USE_AFTER_FREE,
        .pattern_name = "Use After Free",
        .description = "Use after free",
        .typical_priority = BUG_PRIORITY_P0_CRITICAL_CRITICAL,
        .occurrence_count = 0,
        .risk_score = 0.0
    };
    
    rers_pattern_match_t matches[10];
    uint32_t num_matches;
    crrss_status_t status = rers_pattern_db_match(db, &query, matches, 10, &num_matches);
    
    ASSERT_SUCCESS(status, "Match should succeed");
    ASSERT(num_matches > 0, "Should find at least one match");
    
    rers_pattern_db_cleanup(db);
    TEST_PASS();
}

TEST(pattern_similarity) {
    rers_pattern_db_t* db = rers_pattern_db_init(1000, NULL);
    ASSERT_NE(db, NULL, "Database should initialize");
    
    rers_pattern_entry_t pattern1 = {
        .pattern_type = PATTERN_MEMORY_LEAK,
        .pattern_name = "Memory Leak 1",
        .description = "Test pattern 1",
        .priority = BUG_PRIORITY_P1_HIGH_HIGH,
        .category = BUG_CATEGORY_MEMORY,
        .is_active = true
    };
    
    rers_pattern_entry_t pattern2 = {
        .pattern_type = PATTERN_MEMORY_LEAK,
        .pattern_name = "Memory Leak 2",
        .description = "Test pattern 2",
        .priority = BUG_PRIORITY_P1_HIGH_HIGH,
        .category = BUG_CATEGORY_MEMORY,
        .is_active = true
    };
    
    uint32_t id1, id2;
    rers_pattern_db_insert(db, &pattern1, &id1);
    rers_pattern_db_insert(db, &pattern2, &id2);
    
    double similarity;
    crrss_status_t status = rers_pattern_db_similarity(db, id1, id2, &similarity);
    
    ASSERT_SUCCESS(status, "Similarity calculation should succeed");
    ASSERT(similarity > 0.5, "Similar patterns should have high similarity");
    
    rers_pattern_db_cleanup(db);
    TEST_PASS();
}

// ==================== Test: Learning Engine ====================

TEST(learning_engine_init) {
    rers_learning_config_t config = {
        .strategy = RERS_LEARNING_HYBRID,
        .learning_rate = 0.01,
        .window_size = 100,
        .batch_size = 32,
        .max_iterations = 1000,
        .convergence_threshold = 0.01,
        .enable_online_learning = true,
        .enable_incremental_learning = true
    };
    
    rers_learning_engine_t* engine = rers_learning_init(&config);
    ASSERT_NE(engine, NULL, "Learning engine should initialize");
    
    rers_learning_cleanup(engine);
    TEST_PASS();
}

TEST(learn_from_bug) {
    rers_learning_config_t config = {
        .strategy = RERS_LEARNING_SUPERVISED,
        .learning_rate = 0.01,
        .window_size = 100,
        .batch_size = 32,
        .max_iterations = 1000,
        .convergence_threshold = 0.01,
        .enable_online_learning = true,
        .enable_incremental_learning = false
    };
    
    rers_learning_engine_t* engine = rers_learning_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    bug_prediction_t bug = {
        .file_path = "test.c",
        .line_number = 100,
        .category = BUG_CATEGORY_MEMORY,
        .priority = BUG_PRIORITY_P1_HIGH_HIGH,
        .risk_level = RISK_LEVEL_HIGH,
        .confidence = 0.85,
        .description = "Test bug",
        .recommendation = "Fix it",
        .pattern_detected = PATTERN_MEMORY_LEAK
    };
    
    crrss_status_t status = rers_learn_from_bug(engine, &bug);
    ASSERT_SUCCESS(status, "Learning from bug should succeed");
    
    rers_learning_cleanup(engine);
    TEST_PASS();
}

TEST(pattern_extraction) {
    rers_learning_config_t config = {
        .strategy = RERS_LEARNING_UNSUPERVISED,
        .learning_rate = 0.01,
        .window_size = 100,
        .batch_size = 32,
        .max_iterations = 1000,
        .convergence_threshold = 0.01,
        .enable_online_learning = true,
        .enable_incremental_learning = false
    };
    
    rers_learning_engine_t* engine = rers_learning_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    bug_prediction_t bug = {
        .file_path = "test.c",
        .line_number = 200,
        .category = BUG_CATEGORY_MEMORY,
        .priority = BUG_PRIORITY_P0_CRITICAL_CRITICAL,
        .risk_level = RISK_LEVEL_CRITICAL,
        .confidence = 0.9,
        .pattern_detected = PATTERN_USE_AFTER_FREE
    };
    
    rers_pattern_extraction_t extraction;
    crrss_status_t status = rers_extract_patterns_from_bug(engine, &bug, &extraction);
    
    ASSERT_SUCCESS(status, "Pattern extraction should succeed");
    ASSERT(extraction.success, "Extraction should be successful");
    ASSERT(extraction.patterns_extracted > 0, "Should extract at least one pattern");
    
    rers_learning_cleanup(engine);
    TEST_PASS();
}

TEST(bug_ranking) {
    rers_learning_config_t config = {
        .strategy = RERS_LEARNING_HYBRID,
        .learning_rate = 0.01,
        .window_size = 100,
        .batch_size = 32,
        .max_iterations = 1000,
        .convergence_threshold = 0.01,
        .enable_online_learning = true,
        .enable_incremental_learning = false
    };
    
    rers_learning_engine_t* engine = rers_learning_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    bug_prediction_t bugs[3] = {
        {
            .priority = BUG_PRIORITY_P2_MEDIUM_MEDIUM,
            .confidence = 0.7,
            .risk_level = RISK_LEVEL_MEDIUM
        },
        {
            .priority = BUG_PRIORITY_P0_CRITICAL_CRITICAL,
            .confidence = 0.95,
            .risk_level = RISK_LEVEL_CRITICAL
        },
        {
            .priority = BUG_PRIORITY_P1_HIGH_HIGH,
            .confidence = 0.85,
            .risk_level = RISK_LEVEL_HIGH
        }
    };
    
    rers_bug_ranking_t rankings[3];
    crrss_status_t status = rers_rank_bugs(
        engine,
        bugs,
        3,
        RERS_RANK_BY_SEVERITY,
        rankings
    );
    
    ASSERT_SUCCESS(status, "Bug ranking should succeed");
    ASSERT(rankings[1].severity_score > rankings[0].severity_score,
           "Critical bug should have higher severity score");
    
    rers_learning_cleanup(engine);
    TEST_PASS();
}

// ==================== Test: Integration Engine ====================

TEST(integration_engine_init) {
    rers_integration_config_t config = {
        .mode = RERS_INTEGRATION_FULL,
        .enable_cross_correlation = true,
        .enable_profile_selection = true,
        .enable_unified_reporting = true,
        .correlation_threshold = 0.7
    };
    
    rers_integration_engine_t* engine = rers_integration_init(&config);
    ASSERT_NE(engine, NULL, "Integration engine should initialize");
    
    rers_integration_cleanup(engine);
    TEST_PASS();
}

TEST(profile_connection) {
    rers_integration_config_t config = {
        .mode = RERS_INTEGRATION_ACTIVE,
        .enable_cross_correlation = true,
        .enable_profile_selection = false,
        .enable_unified_reporting = false,
        .correlation_threshold = 0.7
    };
    
    rers_integration_engine_t* engine = rers_integration_init(&config);
    ASSERT_NE(engine, NULL, "Engine should initialize");
    
    // Create dummy context
    int dummy_msm = 1;
    crrss_status_t status = rers_connect_msm(engine, &dummy_msm);
    ASSERT_SUCCESS(status, "MSM connection should succeed");
    
    bool is_connected;
    status = rers_is_profile_connected(engine, RERS_PROFILE_MSM, &is_connected);
    ASSERT_SUCCESS(status, "Check connection should succeed");
    ASSERT(is_connected, "MSM should be connected");
    
    rers_integration_cleanup(engine);
    TEST_PASS();
}

// ==================== Test: Main RERS Interface ====================

TEST(rers_init) {
    rers_config_t config = {
        .mode = RERS_MODE_FULL,
        .enable_error_replay = true,
        .enable_error_injection = false,
        .enable_controlled_replay = true,
        .capture_full_context = true,
        .max_replay_iterations = 10,
        .enable_active_learning = true,
        .enable_pattern_extraction = true,
        .enable_hierarchical_learning = true,
        .enable_priority_ranking = true,
        .learning_window_size = 100,
        .learning_rate = 0.01,
        .enable_pattern_db = true,
        .enable_pattern_matching = true,
        .enable_pattern_similarity = true,
        .pattern_db_path = NULL,
        .max_patterns = 1000,
        .integrate_with_msm = false,
        .integrate_with_stp = false,
        .integrate_with_bpme = false,
        .integrate_with_tdt = false,
        .generate_reports = true,
        .verbose_logging = false,
        .report_output_dir = "/tmp",
        .log_output_dir = "/tmp"
    };
    
    rers_context_t* ctx = rers_initialize(&config);
    ASSERT_NE(ctx, NULL, "RERS should initialize");
    ASSERT(rers_is_initialized(ctx), "RERS should be initialized");
    
    rers_shutdown(ctx);
    TEST_PASS();
}

TEST(rers_error_workflow) {
    rers_config_t config = {
        .mode = RERS_MODE_REPLAY,
        .enable_error_replay = true,
        .enable_error_injection = false,
        .enable_controlled_replay = false,
        .capture_full_context = false,
        .max_replay_iterations = 1,
        .enable_active_learning = false,
        .enable_pattern_extraction = false,
        .enable_hierarchical_learning = false,
        .enable_priority_ranking = false,
        .learning_window_size = 10,
        .learning_rate = 0.01,
        .enable_pattern_db = false,
        .enable_pattern_matching = false,
        .enable_pattern_similarity = false,
        .pattern_db_path = NULL,
        .max_patterns = 100,
        .integrate_with_msm = false,
        .integrate_with_stp = false,
        .integrate_with_bpme = false,
        .integrate_with_tdt = false,
        .generate_reports = false,
        .verbose_logging = false,
        .report_output_dir = NULL,
        .log_output_dir = NULL
    };
    
    rers_context_t* ctx = rers_initialize(&config);
    ASSERT_NE(ctx, NULL, "RERS should initialize");
    
    // Capture error
    uint64_t error_id;
    crrss_status_t status = rers_capture_error(
        ctx,
        "segfault",
        NULL,
        &error_id
    );
    ASSERT_SUCCESS(status, "Error capture should succeed");
    
    // Replay error (stub)
    status = rers_replay_error(ctx, error_id, NULL);
    ASSERT_SUCCESS(status, "Error replay should succeed");
    
    // Get statistics
    rers_statistics_t stats;
    status = rers_get_statistics(ctx, &stats);
    ASSERT_SUCCESS(status, "Get statistics should succeed");
    ASSERT(stats.total_errors_captured > 0, "Should have captured errors");
    
    rers_shutdown(ctx);
    TEST_PASS();
}

TEST(rers_pattern_workflow) {
    rers_config_t config = {
        .mode = RERS_MODE_LEARNING,
        .enable_error_replay = false,
        .enable_error_injection = false,
        .enable_controlled_replay = false,
        .capture_full_context = false,
        .max_replay_iterations = 1,
        .enable_active_learning = true,
        .enable_pattern_extraction = true,
        .enable_hierarchical_learning = false,
        .enable_priority_ranking = false,
        .learning_window_size = 50,
        .learning_rate = 0.01,
        .enable_pattern_db = true,
        .enable_pattern_matching = true,
        .enable_pattern_similarity = false,
        .pattern_db_path = NULL,
        .max_patterns = 500,
        .integrate_with_msm = false,
        .integrate_with_stp = false,
        .integrate_with_bpme = false,
        .integrate_with_tdt = false,
        .generate_reports = false,
        .verbose_logging = false,
        .report_output_dir = NULL,
        .log_output_dir = NULL
    };
    
    rers_context_t* ctx = rers_initialize(&config);
    ASSERT_NE(ctx, NULL, "RERS should initialize");
    
    // Store pattern
    bug_pattern_info_t pattern = {
        .pattern_type = PATTERN_MEMORY_LEAK,
        .pattern_name = "Memory Leak Pattern",
        .description = "Common memory leak pattern",
        .typical_priority = BUG_PRIORITY_P1_HIGH_HIGH,
        .occurrence_count = 10,
        .risk_score = 0.75
    };
    
    crrss_status_t status = rers_store_pattern(ctx, &pattern);
    ASSERT_SUCCESS(status, "Store pattern should succeed");
    
    // Retrieve pattern
    bug_pattern_info_t retrieved;
    status = rers_retrieve_pattern(ctx, 0, &retrieved);
    ASSERT_SUCCESS(status, "Retrieve pattern should succeed");
    
    rers_shutdown(ctx);
    TEST_PASS();
}

TEST(rers_version_info) {
    const char* version = rers_get_version();
    ASSERT_NE(version, NULL, "Version should not be NULL");
    ASSERT(strlen(version) > 0, "Version string should not be empty");
    
    TEST_PASS();
}

// ==================== Main Test Runner ====================

int main(int argc, char** argv) {
    (void)argc;  // Unused parameter
    (void)argv;  // Unused parameter
    
    printf("=== RERS Module Unit Tests ===\n\n");
    
    // Replay Engine Tests
    printf("--- Replay Engine Tests ---\n");
    RUN_TEST(replay_engine_init);
    RUN_TEST(error_capture);
    RUN_TEST(error_replay);
    RUN_TEST(error_injection);
    
    // Pattern Database Tests
    printf("\n--- Pattern Database Tests ---\n");
    RUN_TEST(pattern_db_init);
    RUN_TEST(pattern_insert_and_retrieve);
    RUN_TEST(pattern_matching);
    RUN_TEST(pattern_similarity);
    
    // Learning Engine Tests
    printf("\n--- Learning Engine Tests ---\n");
    RUN_TEST(learning_engine_init);
    RUN_TEST(learn_from_bug);
    RUN_TEST(pattern_extraction);
    RUN_TEST(bug_ranking);
    
    // Integration Engine Tests
    printf("\n--- Integration Engine Tests ---\n");
    RUN_TEST(integration_engine_init);
    RUN_TEST(profile_connection);
    
    // Main RERS Interface Tests
    printf("\n--- Main RERS Interface Tests ---\n");
    RUN_TEST(rers_init);
    RUN_TEST(rers_error_workflow);
    RUN_TEST(rers_pattern_workflow);
    RUN_TEST(rers_version_info);
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed\n");
        return 1;
    }
}
