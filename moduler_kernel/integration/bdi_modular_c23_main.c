
// ===================================================================
// BDI Modular Kernel C23 Integration Main
// Enhanced integration demo with C23 features and AI trainer
// ===================================================================

#include "../orchestrator/orchestrator_c23.h"
#include "../ai_trainer/ai_trainer.h"
#include "../attention_mm/attention_mm_c23.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

// ===================================================================
// Global State and Configuration
// ===================================================================

static bdi_orchestrator_c23_t* global_orchestrator = NULL;
static volatile bool shutdown_requested = false;

// C23 constexpr configuration
constexpr size_t DEMO_FEATURE_COUNT = 20;
constexpr size_t DEMO_SAMPLE_COUNT = 5000;
constexpr bdi_learning_rate_t DEMO_LEARNING_RATE = 0.005dd;

// ===================================================================
// Signal Handling
// ===================================================================

static void signal_handler(int sig) {
    printf("\nReceived signal %d, initiating graceful shutdown...\n", sig);
    shutdown_requested = true;
    
    if (global_orchestrator) {
        bdi_orchestrator_c23_destroy(global_orchestrator);
        global_orchestrator = NULL;
    }
}

// ===================================================================
// Demo Data Generation
// ===================================================================

// C23 auto for automatic type deduction
static auto generate_demo_dataset(size_t sample_count, size_t feature_count) -> bdi_training_sample_t** {
    auto samples = (bdi_training_sample_t**)malloc(sample_count * sizeof(bdi_training_sample_t*));
    if (!samples) return NULL;
    
    printf("Generating %zu demo samples with %zu features each...\n", sample_count, feature_count);
    
    for (size_t i = 0; i < sample_count; ++i) {
        // Generate features with some pattern
        auto features = (float*)malloc(feature_count * sizeof(float));
        if (!features) {
            // Cleanup on failure
            for (size_t j = 0; j < i; ++j) {
                free(samples[j]);
            }
            free(samples);
            return NULL;
        }
        
        // Create a pattern: positive samples have more positive features
        float feature_sum = 0.0f;
        for (size_t j = 0; j < feature_count; ++j) {
            features[j] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
            
            // Add bias for pattern creation
            if (i % 3 == 0) { // Make every 3rd sample more likely to be positive
                features[j] += 0.3f;
            } else if (i % 3 == 1) { // Make every other 3rd sample more likely to be negative
                features[j] -= 0.3f;
            }
            
            feature_sum += features[j];
        }
        
        // Determine label based on feature sum with some noise
        bdi_label_t label;
        if (feature_sum > 0.5f) {
            label = 1; // Liked
        } else if (feature_sum < -0.5f) {
            label = -1; // Disliked
        } else {
            label = (rand() % 2 == 0) ? 1 : -1; // Random for ambiguous cases
        }
        
        samples[i] = bdi_create_training_sample(features, feature_count, label);
        free(features);
        
        if (!samples[i]) {
            // Cleanup on failure
            for (size_t j = 0; j < i; ++j) {
                free(samples[j]);
            }
            free(samples);
            return NULL;
        }
        
        // Progress indicator
        if ((i + 1) % 1000 == 0) {
            printf("Generated %zu/%zu samples\n", i + 1, sample_count);
        }
    }
    
    return samples;
}

// ===================================================================
// C23 Enhanced Orchestrator Setup
// ===================================================================

static bool setup_c23_orchestrator(void) {
    printf("Setting up C23-enhanced orchestrator...\n");
    
    // Create orchestrator with AI training profile
    global_orchestrator = bdi_orchestrator_c23_create(&bdi_profile_ai_train);
    if (!global_orchestrator) {
        printf("ERROR: Failed to create C23 orchestrator\n");
        return false;
    }
    
    // Detect and apply C23 capabilities
    bdi_c23_capabilities_t c23_caps;
    if (bdi_detect_c23_capabilities(&c23_caps)) {
        printf("C23 capabilities detected:\n");
        bdi_print_c23_capabilities(&c23_caps);
        
        if (!bdi_orchestrator_apply_c23_features(global_orchestrator)) {
            printf("WARNING: Failed to apply C23 features\n");
        } else {
            printf("C23 features applied successfully\n");
        }
    }
    
    // Optimize modules with C23 features
    if (bdi_orchestrator_optimize_modules_c23(global_orchestrator)) {
        printf("C23 module optimizations applied\n");
    } else {
        printf("WARNING: C23 module optimizations failed\n");
    }
    
    return true;
}

// ===================================================================
// AI Trainer Setup and Training
// ===================================================================

static bool setup_and_train_ai_models(void) {
    printf("\nSetting up AI trainer system...\n");
    
    // Create AI trainer configuration
    bdi_ai_trainer_config_t trainer_config = {
        .num_layers = 4,
        .layer_sizes = (size_t[]){64, 32, 16, 1},
        .base_learning_rate = DEMO_LEARNING_RATE,
        .learning_rate_decay = 0.995dd,
        .momentum = 0.9dd,
        .weight_decay = 0.001dd,
        .dropout_rate = 0.1dd,
        .batch_size = 64,
        .max_epochs = 50,
        .validation_frequency = 5,
        .use_attention_mm = true,
        .enable_simd = true,
        .enable_gpu_acceleration = false,
        .num_threads = 1
    };
    
    // Initialize attention memory manager configuration
    trainer_config.attention_config = (bdi_attention_config_t){
        .attention_learning_rate = 0.1f,
        .recency_decay_rate = 0.99f,
        .hotness_learning_rate = 0.3f,
        .regularization_factor = 0.001f,
        .weight_attention = 0.5f,
        .weight_recency = 0.1f,
        .weight_hotness = 0.4f,
        .weight_numa_locality = 0.1f,
        .weight_criticality = 0.3f,
        .eviction_threshold = 0.1f,
        .promotion_threshold = 0.9f,
        .demotion_threshold = 0.05f,
        .update_frequency = 100,
        .gc_frequency = 1000,
        .enable_prefetching = true,
        .enable_numa_balancing = false
    };
    
    // Create AI trainer
    auto trainer = bdi_ai_trainer_create(&trainer_config);
    if (!trainer) {
        printf("ERROR: Failed to create AI trainer\n");
        return false;
    }
    
    // Add trainer to orchestrator
    if (!bdi_orchestrator_add_ai_trainer(global_orchestrator, trainer)) {
        printf("ERROR: Failed to integrate AI trainer with orchestrator\n");
        bdi_ai_trainer_destroy(trainer);
        return false;
    }
    
    printf("AI trainer created and integrated successfully\n");
    
    // Generate training dataset
    auto samples = generate_demo_dataset(DEMO_SAMPLE_COUNT, DEMO_FEATURE_COUNT);
    if (!samples) {
        printf("ERROR: Failed to generate demo dataset\n");
        return false;
    }
    
    // Add samples to trainer
    printf("Adding samples to AI trainer...\n");
    size_t positive_samples = 0, negative_samples = 0;
    
    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) {
        if (!bdi_ai_trainer_add_sample(trainer, samples[i]->features, 
                                      DEMO_FEATURE_COUNT, samples[i]->label)) {
            printf("WARNING: Failed to add sample %zu\n", i);
            continue;
        }
        
        if (samples[i]->label > 0) {
            positive_samples++;
        } else {
            negative_samples++;
        }
        
        if (shutdown_requested) {
            printf("Shutdown requested during sample addition\n");
            break;
        }
    }
    
    printf("Added %zu positive and %zu negative samples\n", positive_samples, negative_samples);
    
    // Train the AI model
    printf("\nStarting AI training...\n");
    struct timespec training_start, training_end;
    clock_gettime(CLOCK_MONOTONIC, &training_start);
    
    for (size_t epoch = 0; epoch < trainer_config.max_epochs && !shutdown_requested; ++epoch) {
        if (!bdi_ai_trainer_train_epoch(trainer)) {
            printf("ERROR: Training failed at epoch %zu\n", epoch);
            break;
        }
        
        // Print progress every few epochs
        if ((epoch + 1) % 5 == 0) {
            printf("Epoch %zu/%zu - Loss: %.6f, Accuracy: %.4f\n", 
                   epoch + 1, trainer_config.max_epochs,
                   (double)trainer->training_loss, (double)trainer->training_accuracy);
        }
        
        // Update C23 performance metrics
        bdi_orchestrator_update_c23_metrics(global_orchestrator);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &training_end);
    uint64_t training_time_us = (training_end.tv_sec - training_start.tv_sec) * 1000000ULL +
                               (training_end.tv_nsec - training_start.tv_nsec) / 1000ULL;
    
    printf("Training completed in %.2f seconds\n", (double)training_time_us / 1000000.0);
    printf("Final training loss: %.6f\n", (double)trainer->training_loss);
    printf("Final training accuracy: %.4f\n", (double)trainer->training_accuracy);
    
    // Test inference on some samples
    printf("\nTesting inference...\n");
    for (size_t i = 0; i < 10 && i < DEMO_SAMPLE_COUNT; ++i) {
        auto confidence = bdi_ai_trainer_predict(trainer, samples[i]->features, DEMO_FEATURE_COUNT);
        printf("Sample %zu: Label=%d, Confidence=%.4f, Prediction=%s\n", 
               i, samples[i]->label, (double)confidence,
               (confidence > 0.5df) ? "Liked" : "Disliked");
    }
    
    // Cleanup samples
    for (size_t i = 0; i < DEMO_SAMPLE_COUNT; ++i) {
        free(samples[i]);
    }
    free(samples);
    
    return true;
}

// ===================================================================
// Performance Monitoring and Statistics
// ===================================================================

static void print_performance_statistics(void) {
    if (!global_orchestrator) return;
    
    printf("\n=== Performance Statistics ===\n");
    
    // C23 performance metrics
    _Decimal64 c23_performance = bdi_orchestrator_measure_c23_performance(global_orchestrator);
    printf("C23 Performance Score: %.6f ops/μs\n", (double)c23_performance);
    printf("C23 Performance Boost: %.2f%%\n", (double)global_orchestrator->c23_performance_boost);
    printf("AI Training Efficiency: %.2f\n", (double)global_orchestrator->ai_training_efficiency);
    
    // AI trainer statistics
    if (global_orchestrator->num_ai_trainers > 0) {
        auto trainer = global_orchestrator->ai_trainers[0];
        if (trainer) {
            bdi_ai_trainer_stats_t stats;
            bdi_ai_trainer_get_stats(trainer, &stats);
            
            printf("\nAI Trainer Statistics:\n");
            printf("  Samples per second: %lu\n", stats.samples_per_second);
            printf("  Average loss: %.6f\n", (double)stats.average_loss);
            printf("  Total memory allocated: %zu bytes\n", stats.total_memory_allocated);
            printf("  Peak memory usage: %zu bytes\n", stats.peak_memory_usage);
            printf("  Thread samples processed: %lu\n", stats.thread_samples_processed);
            printf("  Thread training time: %lu μs\n", stats.thread_training_time_us);
        }
    }
    
    // Base orchestrator statistics
    if (global_orchestrator->base_orchestrator) {
        bdi_print_orchestrator_stats(global_orchestrator->base_orchestrator);
    }
}

// ===================================================================
// Interactive Demo Mode
// ===================================================================

static void run_interactive_demo(void) {
    if (!global_orchestrator || global_orchestrator->num_ai_trainers == 0) {
        printf("No AI trainer available for interactive demo\n");
        return;
    }
    
    auto trainer = global_orchestrator->ai_trainers[0];
    printf("\n=== Interactive AI Trainer Demo ===\n");
    printf("Enter feature values (space-separated) or 'quit' to exit:\n");
    
    char input_buffer[1024];
    while (!shutdown_requested) {
        printf("\nEnter %zu feature values: ", DEMO_FEATURE_COUNT);
        fflush(stdout);
        
        if (!fgets(input_buffer, sizeof(input_buffer), stdin)) {
            break;
        }
        
        // Check for quit command
        if (strncmp(input_buffer, "quit", 4) == 0) {
            break;
        }
        
        // Parse feature values
        float features[DEMO_FEATURE_COUNT];
        char* token = strtok(input_buffer, " \t\n");
        size_t feature_count = 0;
        
        while (token && feature_count < DEMO_FEATURE_COUNT) {
            features[feature_count] = atof(token);
            feature_count++;
            token = strtok(NULL, " \t\n");
        }
        
        if (feature_count != DEMO_FEATURE_COUNT) {
            printf("Please enter exactly %zu feature values\n", DEMO_FEATURE_COUNT);
            continue;
        }
        
        // Make prediction
        auto confidence = bdi_ai_trainer_predict(trainer, features, DEMO_FEATURE_COUNT);
        printf("Prediction: %s (confidence: %.4f)\n", 
               (confidence > 0.5df) ? "LIKED" : "DISLIKED", (double)confidence);
        
        // Ask for feedback to add as training sample
        printf("Was this prediction correct? (y/n/skip): ");
        fflush(stdout);
        
        char feedback[10];
        if (fgets(feedback, sizeof(feedback), stdin)) {
            if (feedback[0] == 'y' || feedback[0] == 'Y') {
                bdi_label_t label = (confidence > 0.5df) ? 1 : -1;
                bdi_ai_trainer_add_sample(trainer, features, DEMO_FEATURE_COUNT, label);
                printf("Added as training sample with label: %s\n", 
                       (label > 0) ? "LIKED" : "DISLIKED");
            } else if (feedback[0] == 'n' || feedback[0] == 'N') {
                bdi_label_t label = (confidence > 0.5df) ? -1 : 1;
                bdi_ai_trainer_add_sample(trainer, features, DEMO_FEATURE_COUNT, label);
                printf("Added as training sample with corrected label: %s\n", 
                       (label > 0) ? "LIKED" : "DISLIKED");
            }
        }
    }
    
    printf("Interactive demo ended\n");
}

// ===================================================================
// Main Function
// ===================================================================

int main(int argc, char* argv[]) {
    printf("=== BDI Modular Kernel C23 Integration Demo ===\n");
    printf("Enhanced with C23 features and AI trainer system\n\n");
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Parse command line arguments
    bool interactive_mode = false;
    bool benchmark_mode = false;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = true;
        } else if (strcmp(argv[i], "--benchmark") == 0) {
            benchmark_mode = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --interactive    Run in interactive mode\n");
            printf("  --benchmark      Run performance benchmarks\n");
            printf("  --help          Show this help message\n");
            return 0;
        }
    }
    
    // Setup C23 orchestrator
    if (!setup_c23_orchestrator()) {
        printf("ERROR: Failed to setup orchestrator\n");
        return 1;
    }
    
    // Setup and train AI models
    if (!setup_and_train_ai_models()) {
        printf("ERROR: Failed to setup AI training\n");
        bdi_orchestrator_c23_destroy(global_orchestrator);
        return 1;
    }
    
    // Print performance statistics
    print_performance_statistics();
    
    // Run interactive demo if requested
    if (interactive_mode && !shutdown_requested) {
        run_interactive_demo();
    }
    
    // Run benchmarks if requested
    if (benchmark_mode && !shutdown_requested) {
        printf("\n=== Running Benchmarks ===\n");
        // Additional benchmark code would go here
        printf("Benchmark mode not fully implemented yet\n");
    }
    
    // Graceful shutdown
    printf("\nShutting down BDI Modular Kernel C23 demo...\n");
    if (global_orchestrator) {
        bdi_orchestrator_c23_destroy(global_orchestrator);
        global_orchestrator = NULL;
    }
    
    printf("Demo completed successfully\n");
    return 0;
}

