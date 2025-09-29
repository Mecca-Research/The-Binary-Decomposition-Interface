
// ===================================================================
// BDI AI Trainer Test Suite - C23 Enhanced
// Comprehensive tests for the AI trainer system
// ===================================================================

#include "../ai_trainer/ai_trainer.h"
#include "../orchestrator/orchestrator_c23.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>

// ===================================================================
// Test Configuration and Utilities
// ===================================================================

// C23 constexpr test parameters
constexpr size_t TEST_FEATURE_COUNT = 10;
constexpr size_t TEST_SAMPLE_COUNT = 1000;
constexpr size_t TEST_BATCH_SIZE = 32;
constexpr bdi_learning_rate_t TEST_LEARNING_RATE = 0.01dd;

// Test result tracking
typedef struct {
    size_t tests_run;
    size_t tests_passed;
    size_t tests_failed;
    char last_error[256];
} test_results_t;

static test_results_t test_results = {0};

// C23 typeof for test assertion macros
#define TEST_ASSERT(condition, message) \
    do { \
        test_results.tests_run++; \
        if (!(condition)) { \
            test_results.tests_failed++; \
            snprintf(test_results.last_error, sizeof(test_results.last_error), \
                    "FAIL: %s (line %d): %s", __func__, __LINE__, message); \
            printf("%s\n", test_results.last_error); \
            return false; \
        } else { \
            test_results.tests_passed++; \
            printf("PASS: %s: %s\n", __func__, message); \
        } \
    } while(0)

// C23 auto for test data generation
static auto generate_test_features(size_t count) -> float* {
    auto features = (float*)malloc(count * sizeof(float));
    if (!features) return NULL;
    
    for (size_t i = 0; i < count; ++i) {
        features[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
    }
    
    return features;
}

// ===================================================================
// Basic AI Trainer Tests
// ===================================================================

static bool test_ai_trainer_creation(void) {
    // Create AI trainer configuration
    bdi_ai_trainer_config_t config = {
        .num_layers = 3,
        .layer_sizes = (size_t[]){64, 32, 1},
        .base_learning_rate = TEST_LEARNING_RATE,
        .learning_rate_decay = 0.99dd,
        .momentum = 0.9dd,
        .weight_decay = 0.001dd,
        .dropout_rate = 0.1dd,
        .batch_size = TEST_BATCH_SIZE,
        .max_epochs = 100,
        .validation_frequency = 10,
        .use_attention_mm = false,
        .enable_simd = true,
        .enable_gpu_acceleration = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "AI trainer creation");
    
    TEST_ASSERT(trainer->config.num_layers == 3, "Layer count configuration");
    TEST_ASSERT(trainer->config.batch_size == TEST_BATCH_SIZE, "Batch size configuration");
    TEST_ASSERT(trainer->current_learning_rate == TEST_LEARNING_RATE, "Learning rate initialization");
    
    bdi_ai_trainer_destroy(trainer);
    return true;
}

static bool test_training_sample_creation(void) {
    auto features = generate_test_features(TEST_FEATURE_COUNT);
    TEST_ASSERT(features != NULL, "Test feature generation");
    
    // Test positive sample creation
    auto positive_sample = bdi_create_training_sample(features, TEST_FEATURE_COUNT, 1);
    TEST_ASSERT(positive_sample != NULL, "Positive sample creation");
    TEST_ASSERT(positive_sample->label == 1, "Positive label assignment");
    TEST_ASSERT(positive_sample->sample_id > 0, "Sample ID generation");
    
    // Test negative sample creation
    auto negative_sample = bdi_create_training_sample(features, TEST_FEATURE_COUNT, -1);
    TEST_ASSERT(negative_sample != NULL, "Negative sample creation");
    TEST_ASSERT(negative_sample->label == -1, "Negative label assignment");
    
    // Verify feature copying
    bool features_match = true;
    for (size_t i = 0; i < TEST_FEATURE_COUNT; ++i) {
        if (fabsf(positive_sample->features[i] - features[i]) > 1e-6f) {
            features_match = false;
            break;
        }
    }
    TEST_ASSERT(features_match, "Feature copying accuracy");
    
    free(features);
    free(positive_sample);
    free(negative_sample);
    return true;
}

static bool test_sample_addition(void) {
    bdi_ai_trainer_config_t config = {
        .num_layers = 2,
        .layer_sizes = (size_t[]){32, 1},
        .base_learning_rate = TEST_LEARNING_RATE,
        .batch_size = TEST_BATCH_SIZE,
        .use_attention_mm = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "Trainer creation for sample addition");
    
    // Add positive samples
    for (size_t i = 0; i < 50; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        bool result = bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, 1);
        TEST_ASSERT(result, "Positive sample addition");
        free(features);
    }
    
    // Add negative samples
    for (size_t i = 0; i < 50; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        bool result = bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, -1);
        TEST_ASSERT(result, "Negative sample addition");
        free(features);
    }
    
    TEST_ASSERT(trainer->num_samples == 100, "Total sample count");
    
    bdi_ai_trainer_destroy(trainer);
    return true;
}

// ===================================================================
// Training Process Tests
// ===================================================================

static bool test_training_epoch(void) {
    bdi_ai_trainer_config_t config = {
        .num_layers = 2,
        .layer_sizes = (size_t[]){16, 1},
        .base_learning_rate = 0.1dd,
        .batch_size = 10,
        .use_attention_mm = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "Trainer creation for epoch test");
    
    // Add training samples with clear patterns
    for (size_t i = 0; i < 100; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        
        // Create a simple pattern: positive if sum > 0, negative otherwise
        float sum = 0.0f;
        for (size_t j = 0; j < TEST_FEATURE_COUNT; ++j) {
            sum += features[j];
        }
        bdi_label_t label = (sum > 0.0f) ? 1 : -1;
        
        bool result = bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, label);
        TEST_ASSERT(result, "Pattern sample addition");
        free(features);
    }
    
    // Train for one epoch
    bool training_result = bdi_ai_trainer_train_epoch(trainer);
    TEST_ASSERT(training_result, "Training epoch execution");
    
    TEST_ASSERT(trainer->current_epoch == 1, "Epoch counter increment");
    TEST_ASSERT(trainer->training_loss >= 0.0dd, "Training loss validity");
    TEST_ASSERT(trainer->training_accuracy >= 0.0dd && trainer->training_accuracy <= 1.0dd, 
               "Training accuracy range");
    
    bdi_ai_trainer_destroy(trainer);
    return true;
}

static bool test_inference(void) {
    bdi_ai_trainer_config_t config = {
        .num_layers = 2,
        .layer_sizes = (size_t[]){8, 1},
        .base_learning_rate = 0.1dd,
        .batch_size = 10,
        .use_attention_mm = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "Trainer creation for inference test");
    
    // Add some training samples
    for (size_t i = 0; i < 50; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        bdi_label_t label = (i % 2 == 0) ? 1 : -1;
        
        bool result = bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, label);
        TEST_ASSERT(result, "Sample addition for inference test");
        free(features);
    }
    
    // Train briefly
    bdi_ai_trainer_train_epoch(trainer);
    
    // Test inference
    auto test_features = generate_test_features(TEST_FEATURE_COUNT);
    bdi_confidence_t confidence = bdi_ai_trainer_predict(trainer, test_features, TEST_FEATURE_COUNT);
    
    TEST_ASSERT(confidence >= 0.0df && confidence <= 1.0df, "Confidence score range");
    
    free(test_features);
    bdi_ai_trainer_destroy(trainer);
    return true;
}

// ===================================================================
// C23 Feature Tests
// ===================================================================

static bool test_c23_bitint_features(void) {
    #ifdef HAVE_C23_BITINT
    // Test _BitInt usage in feature hashing
    auto features = generate_test_features(TEST_FEATURE_COUNT);
    auto sample = bdi_create_training_sample(features, TEST_FEATURE_COUNT, 1);
    
    TEST_ASSERT(sample != NULL, "Sample creation with _BitInt hash");
    TEST_ASSERT(sample->feature_hash != 0, "_BitInt feature hash generation");
    
    // Test that different features produce different hashes
    auto different_features = generate_test_features(TEST_FEATURE_COUNT);
    auto different_sample = bdi_create_training_sample(different_features, TEST_FEATURE_COUNT, 1);
    
    TEST_ASSERT(sample->feature_hash != different_sample->feature_hash, 
               "_BitInt hash uniqueness");
    
    free(features);
    free(different_features);
    free(sample);
    free(different_sample);
    #else
    printf("SKIP: C23 _BitInt not available\n");
    #endif
    
    return true;
}

static bool test_c23_decimal_precision(void) {
    #ifdef HAVE_C23_DECIMAL
    bdi_ai_trainer_config_t config = {
        .num_layers = 2,
        .layer_sizes = (size_t[]){4, 1},
        .base_learning_rate = 0.001dd,  // C23 _Decimal literal
        .learning_rate_decay = 0.999dd,
        .momentum = 0.9dd,
        .batch_size = 5,
        .use_attention_mm = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "Trainer creation with _Decimal precision");
    
    // Verify high-precision learning rate
    TEST_ASSERT(trainer->current_learning_rate == 0.001dd, "_Decimal learning rate precision");
    
    // Add samples and train to test _Decimal arithmetic
    for (size_t i = 0; i < 20; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, (i % 2 == 0) ? 1 : -1);
        free(features);
    }
    
    bdi_ai_trainer_train_epoch(trainer);
    
    // Verify _Decimal precision in loss calculation
    TEST_ASSERT(trainer->training_loss >= 0.0dd, "_Decimal loss calculation");
    
    bdi_ai_trainer_destroy(trainer);
    #else
    printf("SKIP: C23 _Decimal not available\n");
    #endif
    
    return true;
}

static bool test_c23_thread_local_features(void) {
    #ifdef HAVE_C23_BASIC_FEATURES
    // Test thread_local error handling
    bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_CONFIG, "Test error message");
    
    auto error = bdi_ai_trainer_get_last_error();
    TEST_ASSERT(error == BDI_AI_TRAINER_ERROR_INVALID_CONFIG, "Thread-local error storage");
    
    const char* error_msg = bdi_ai_trainer_error_string(error);
    TEST_ASSERT(error_msg != NULL, "Thread-local error message retrieval");
    
    #else
    printf("SKIP: C23 thread_local not available\n");
    #endif
    
    return true;
}

// ===================================================================
// Integration Tests
// ===================================================================

static bool test_orchestrator_integration(void) {
    // Create orchestrator with AI training profile
    bdi_optimization_profile_t ai_profile = bdi_profile_ai_train;
    auto orchestrator = bdi_orchestrator_c23_create(&ai_profile);
    TEST_ASSERT(orchestrator != NULL, "C23 orchestrator creation");
    
    // Create AI trainer
    bdi_ai_trainer_config_t trainer_config = {
        .num_layers = 2,
        .layer_sizes = (size_t[]){16, 1},
        .base_learning_rate = 0.01dd,
        .batch_size = 16,
        .use_attention_mm = false,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&trainer_config);
    TEST_ASSERT(trainer != NULL, "AI trainer creation for integration");
    
    // Integrate trainer with orchestrator
    bool integration_result = bdi_orchestrator_add_ai_trainer(orchestrator, trainer);
    TEST_ASSERT(integration_result, "AI trainer integration with orchestrator");
    
    TEST_ASSERT(orchestrator->num_ai_trainers == 1, "Trainer count in orchestrator");
    
    bdi_orchestrator_c23_destroy(orchestrator);
    return true;
}

static bool test_feature_extraction(void) {
    // Test C23 _Generic feature extraction
    int int_value = 42;
    float float_value = 3.14f;
    double double_value = 2.718;
    const char* string_value = "test";
    
    float int_feature = bdi_extract_feature(int_value);
    float float_feature = bdi_extract_feature(float_value);
    float double_feature = bdi_extract_feature(double_value);
    float string_feature = bdi_extract_feature(string_value);
    
    TEST_ASSERT(int_feature != 0.0f, "Integer feature extraction");
    TEST_ASSERT(float_feature == float_value, "Float feature extraction");
    TEST_ASSERT(fabsf(double_feature - (float)double_value) < 1e-6f, "Double feature extraction");
    TEST_ASSERT(string_feature != 0.0f, "String feature extraction");
    
    return true;
}

// ===================================================================
// Performance Tests
// ===================================================================

static bool test_training_performance(void) {
    bdi_ai_trainer_config_t config = {
        .num_layers = 3,
        .layer_sizes = (size_t[]){64, 32, 1},
        .base_learning_rate = 0.01dd,
        .batch_size = 32,
        .use_attention_mm = false,
        .enable_simd = true,
        .num_threads = 1
    };
    
    auto trainer = bdi_ai_trainer_create(&config);
    TEST_ASSERT(trainer != NULL, "Performance test trainer creation");
    
    // Add a substantial number of samples
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (size_t i = 0; i < TEST_SAMPLE_COUNT; ++i) {
        auto features = generate_test_features(TEST_FEATURE_COUNT);
        bdi_label_t label = (i % 2 == 0) ? 1 : -1;
        bdi_ai_trainer_add_sample(trainer, features, TEST_FEATURE_COUNT, label);
        free(features);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t sample_addition_time_us = (end_time.tv_sec - start_time.tv_sec) * 1000000ULL +
                                      (end_time.tv_nsec - start_time.tv_nsec) / 1000ULL;
    
    printf("Sample addition time: %lu μs for %zu samples\n", 
           sample_addition_time_us, TEST_SAMPLE_COUNT);
    
    // Test training performance
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    bool training_result = bdi_ai_trainer_train_epoch(trainer);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    uint64_t training_time_us = (end_time.tv_sec - start_time.tv_sec) * 1000000ULL +
                               (end_time.tv_nsec - start_time.tv_nsec) / 1000ULL;
    
    printf("Training epoch time: %lu μs for %zu samples\n", 
           training_time_us, TEST_SAMPLE_COUNT);
    
    TEST_ASSERT(training_result, "Performance training execution");
    TEST_ASSERT(training_time_us < 10000000, "Training time under 10 seconds"); // 10s limit
    
    // Test inference performance
    auto test_features = generate_test_features(TEST_FEATURE_COUNT);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (size_t i = 0; i < 1000; ++i) {
        bdi_ai_trainer_predict(trainer, test_features, TEST_FEATURE_COUNT);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t inference_time_us = (end_time.tv_sec - start_time.tv_sec) * 1000000ULL +
                                (end_time.tv_nsec - start_time.tv_nsec) / 1000ULL;
    
    printf("Inference time: %lu μs for 1000 predictions (%.2f μs per prediction)\n", 
           inference_time_us, (double)inference_time_us / 1000.0);
    
    TEST_ASSERT(inference_time_us < 1000000, "Inference time under 1 second"); // 1s limit
    
    free(test_features);
    bdi_ai_trainer_destroy(trainer);
    return true;
}

// ===================================================================
// Test Suite Runner
// ===================================================================

typedef struct {
    const char* name;
    bool (*test_func)(void);
} test_case_t;

static const test_case_t test_cases[] = {
    {"AI Trainer Creation", test_ai_trainer_creation},
    {"Training Sample Creation", test_training_sample_creation},
    {"Sample Addition", test_sample_addition},
    {"Training Epoch", test_training_epoch},
    {"Inference", test_inference},
    {"C23 BitInt Features", test_c23_bitint_features},
    {"C23 Decimal Precision", test_c23_decimal_precision},
    {"C23 Thread-Local Features", test_c23_thread_local_features},
    {"Orchestrator Integration", test_orchestrator_integration},
    {"Feature Extraction", test_feature_extraction},
    {"Training Performance", test_training_performance},
};

static const size_t num_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

int run_ai_trainer_tests(void) {
    printf("=== BDI AI Trainer Test Suite - C23 Enhanced ===\n\n");
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Reset test results
    memset(&test_results, 0, sizeof(test_results));
    
    // Run all test cases
    for (size_t i = 0; i < num_test_cases; ++i) {
        printf("Running test: %s\n", test_cases[i].name);
        
        if (!test_cases[i].test_func()) {
            printf("Test failed: %s\n", test_cases[i].name);
            if (strlen(test_results.last_error) > 0) {
                printf("Last error: %s\n", test_results.last_error);
            }
        }
        
        printf("\n");
    }
    
    // Print summary
    printf("=== Test Summary ===\n");
    printf("Tests run: %zu\n", test_results.tests_run);
    printf("Tests passed: %zu\n", test_results.tests_passed);
    printf("Tests failed: %zu\n", test_results.tests_failed);
    
    if (test_results.tests_failed == 0) {
        printf("All tests passed! ✓\n");
        return 0;
    } else {
        printf("Some tests failed! ✗\n");
        return 1;
    }
}

// Main function for standalone testing
#ifdef AI_TRAINER_TEST_STANDALONE
int main(int argc, char* argv[]) {
    return run_ai_trainer_tests();
}
#endif

