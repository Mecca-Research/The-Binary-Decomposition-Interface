
// ===================================================================
// BDI AI Trainer System - C23 Enhanced
// Advanced AI training system for 'liked' and 'unliked' pairs
// Leverages C23 features: typeof, auto, _BitInt, constexpr, _Decimal, thread_local
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>
#include "../attention_mm/attention_mm.h"
#include "../capgraph/capability.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// C23 Enhanced Type System
// ===================================================================

// Use C23 _BitInt for precise bit-width integers
typedef _BitInt(128) bdi_feature_hash_t;    // 128-bit feature hash
typedef _BitInt(64) bdi_timestamp_t;        // 64-bit timestamp
typedef _BitInt(32) bdi_sample_id_t;        // 32-bit sample ID
typedef _BitInt(16) bdi_label_t;            // 16-bit label (-1 for dislike, +1 for like)

// Use C23 _Decimal for precise decimal arithmetic in learning rates
typedef _Decimal128 bdi_learning_rate_t;    // High-precision learning rate
typedef _Decimal64 bdi_reward_t;            // Precise reward values
typedef _Decimal32 bdi_confidence_t;        // Confidence scores

// C23 constexpr for compile-time constants
constexpr size_t BDI_MAX_FEATURES = 1024;
constexpr size_t BDI_MAX_SAMPLES = 1000000;
constexpr bdi_learning_rate_t BDI_DEFAULT_LEARNING_RATE = 0.001dd;
constexpr bdi_reward_t BDI_POSITIVE_REWARD = 1.0dd;
constexpr bdi_reward_t BDI_NEGATIVE_REWARD = -1.0dd;

// ===================================================================
// Training Sample Structure with C23 Features
// ===================================================================

typedef struct {
    bdi_sample_id_t sample_id;              // Unique sample identifier
    bdi_feature_hash_t feature_hash;        // 128-bit feature hash
    bdi_timestamp_t timestamp;              // When sample was created
    bdi_label_t label;                      // Like (+1) or dislike (-1)
    
    // Feature vector (using C23 flexible array member enhancements)
    alignas(64) float features[];           // C23 alignas for cache alignment
} bdi_training_sample_t;

// C23 typeof for type-safe macros
#define BDI_SAMPLE_FEATURE(sample, idx) \
    (typeof((sample)->features[0]))((sample)->features[idx])

// ===================================================================
// Neural Network Layer with C23 Enhancements
// ===================================================================

typedef struct {
    size_t input_size;
    size_t output_size;
    
    // Weight matrices with C23 alignment
    alignas(64) float* weights;             // Aligned for SIMD operations
    alignas(64) float* biases;
    alignas(64) float* weight_gradients;    // For backpropagation
    alignas(64) float* bias_gradients;
    
    // C23 thread_local for per-thread activation caches
    thread_local float* activations;
    thread_local float* deltas;
    
    // Layer metadata
    char activation_function[32];           // "relu", "sigmoid", "tanh"
    bdi_learning_rate_t learning_rate;      // Per-layer learning rate
    
} bdi_neural_layer_t;

// ===================================================================
// AI Trainer Configuration with C23 Features
// ===================================================================

typedef struct {
    // Network architecture
    size_t num_layers;
    size_t* layer_sizes;                    // Array of layer sizes
    
    // Training hyperparameters (using C23 _Decimal for precision)
    bdi_learning_rate_t base_learning_rate;
    bdi_learning_rate_t learning_rate_decay;
    _Decimal64 momentum;
    _Decimal64 weight_decay;
    _Decimal64 dropout_rate;
    
    // Training configuration
    size_t batch_size;
    size_t max_epochs;
    size_t validation_frequency;
    
    // C23 constexpr validation
    constexpr size_t min_batch_size = 32;
    constexpr size_t max_batch_size = 1024;
    
    // Memory management
    bdi_attention_config_t attention_config;
    bool use_attention_mm;
    
    // Performance optimization
    bool enable_simd;                       // Enable SIMD optimizations
    bool enable_gpu_acceleration;           // Enable GPU acceleration
    size_t num_threads;                     // Number of training threads
    
} bdi_ai_trainer_config_t;

// ===================================================================
// AI Trainer State with C23 Thread Safety
// ===================================================================

typedef struct {
    // Configuration
    bdi_ai_trainer_config_t config;
    
    // Neural network layers
    bdi_neural_layer_t* layers;
    size_t num_layers;
    
    // Training data management
    bdi_training_sample_t** samples;        // Array of sample pointers
    size_t num_samples;
    size_t sample_capacity;
    
    // Training state
    size_t current_epoch;
    size_t current_batch;
    bdi_learning_rate_t current_learning_rate;
    
    // Performance metrics (using C23 _Decimal for precision)
    _Decimal64 training_loss;
    _Decimal64 validation_loss;
    _Decimal64 training_accuracy;
    _Decimal64 validation_accuracy;
    
    // Thread synchronization (C23 thread support)
    mtx_t training_mutex;
    cnd_t training_condition;
    thread_local bool thread_initialized;
    
    // Memory management
    bdi_attention_mm_t* memory_manager;
    
    // Statistics
    uint64_t total_samples_processed;
    uint64_t total_training_time_us;
    uint64_t total_inference_time_us;
    
} bdi_ai_trainer_t;

// ===================================================================
// C23 Enhanced Function Declarations
// ===================================================================

// Trainer lifecycle with C23 features
bdi_ai_trainer_t* bdi_ai_trainer_create(const bdi_ai_trainer_config_t* config);
void bdi_ai_trainer_destroy(bdi_ai_trainer_t* trainer);

// Training sample management with C23 typeof
bool bdi_ai_trainer_add_sample(bdi_ai_trainer_t* trainer, 
                              const void* features, size_t feature_count,
                              bdi_label_t label);

// C23 auto return type deduction for sample creation
auto bdi_create_training_sample(const float* features, size_t feature_count, 
                               bdi_label_t label) -> bdi_training_sample_t*;

// Training functions with C23 _Decimal precision
bool bdi_ai_trainer_train_epoch(bdi_ai_trainer_t* trainer);
_Decimal64 bdi_ai_trainer_compute_loss(bdi_ai_trainer_t* trainer, 
                                      const bdi_training_sample_t* samples,
                                      size_t num_samples);

// Inference with C23 constexpr validation
bdi_confidence_t bdi_ai_trainer_predict(bdi_ai_trainer_t* trainer,
                                       const float* features,
                                       size_t feature_count);

// C23 typeof for generic feature extraction
#define BDI_EXTRACT_FEATURES(data, extractor) \
    ({ \
        typeof(data) _data = (data); \
        auto _features = (extractor)(_data); \
        _features; \
    })

// ===================================================================
// Neural Network Operations with C23 SIMD Hints
// ===================================================================

// Forward propagation with C23 alignment hints
void bdi_neural_forward_pass(bdi_neural_layer_t* layer,
                            const float* restrict input,
                            float* restrict output)
    __attribute__((assume_aligned(64)));

// Backward propagation with C23 thread_local optimization
void bdi_neural_backward_pass(bdi_neural_layer_t* layer,
                             const float* restrict delta_output,
                             float* restrict delta_input)
    __attribute__((assume_aligned(64)));

// Weight update with C23 _Decimal precision
void bdi_neural_update_weights(bdi_neural_layer_t* layer,
                              bdi_learning_rate_t learning_rate);

// ===================================================================
// Activation Functions with C23 constexpr
// ===================================================================

// C23 constexpr activation functions for compile-time optimization
constexpr float bdi_activation_relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

constexpr float bdi_activation_sigmoid(float x) {
    return 1.0f / (1.0f + __builtin_expf(-x));
}

constexpr float bdi_activation_tanh(float x) {
    return __builtin_tanhf(x);
}

// Function pointer type for activation functions
typedef float (*bdi_activation_fn_t)(float);

// ===================================================================
// Batch Processing with C23 Thread Support
// ===================================================================

// Batch processing structure with C23 thread_local storage
typedef struct {
    bdi_training_sample_t** samples;
    size_t batch_size;
    size_t current_index;
    
    // Thread-local batch statistics
    thread_local _Decimal64 batch_loss;
    thread_local _Decimal64 batch_accuracy;
    thread_local size_t processed_samples;
    
} bdi_training_batch_t;

// Batch processing functions
bdi_training_batch_t* bdi_create_training_batch(bdi_ai_trainer_t* trainer,
                                               size_t batch_size);
bool bdi_process_training_batch(bdi_ai_trainer_t* trainer,
                               bdi_training_batch_t* batch);
void bdi_destroy_training_batch(bdi_training_batch_t* batch);

// ===================================================================
// Feature Engineering with C23 Generic Macros
// ===================================================================

// C23 _Generic for type-safe feature extraction
#define bdi_extract_feature(data) _Generic((data), \
    int: bdi_extract_int_feature, \
    float: bdi_extract_float_feature, \
    double: bdi_extract_double_feature, \
    char*: bdi_extract_string_feature, \
    default: bdi_extract_generic_feature \
)(data)

// Feature extraction functions
float bdi_extract_int_feature(int value);
float bdi_extract_float_feature(float value);
float bdi_extract_double_feature(double value);
float bdi_extract_string_feature(const char* str);
float bdi_extract_generic_feature(const void* data);

// ===================================================================
// Performance Monitoring with C23 Features
// ===================================================================

typedef struct {
    // Training performance metrics
    uint64_t samples_per_second;
    uint64_t batches_per_second;
    _Decimal64 average_loss;
    _Decimal64 loss_variance;
    
    // Memory usage with attention tracking
    size_t total_memory_allocated;
    size_t peak_memory_usage;
    float average_attention_score;
    
    // Thread performance (C23 thread_local)
    thread_local uint64_t thread_samples_processed;
    thread_local uint64_t thread_training_time_us;
    
    // Hardware utilization
    float cpu_utilization;
    float memory_bandwidth_utilization;
    float cache_hit_rate;
    
} bdi_ai_trainer_stats_t;

// Statistics functions
void bdi_ai_trainer_get_stats(bdi_ai_trainer_t* trainer, 
                             bdi_ai_trainer_stats_t* stats);
void bdi_ai_trainer_reset_stats(bdi_ai_trainer_t* trainer);
void bdi_ai_trainer_print_stats(const bdi_ai_trainer_stats_t* stats);

// ===================================================================
// Integration with BDI Modular Kernel
// ===================================================================

// Integration with orchestrator
bool bdi_ai_trainer_integrate_orchestrator(bdi_ai_trainer_t* trainer,
                                          void* orchestrator);

// Integration with attention memory manager
bool bdi_ai_trainer_integrate_attention_mm(bdi_ai_trainer_t* trainer,
                                          bdi_attention_mm_t* mm);

// Integration with capability system
bool bdi_ai_trainer_check_capabilities(const bdi_caps_t* caps);
void bdi_ai_trainer_optimize_for_capabilities(bdi_ai_trainer_t* trainer,
                                             const bdi_caps_t* caps);

// ===================================================================
// C23 Utility Macros and Functions
// ===================================================================

// C23 typeof for safe type casting
#define BDI_SAFE_CAST(ptr, type) \
    ({ \
        typeof(ptr) _ptr = (ptr); \
        (_ptr && sizeof(*_ptr) >= sizeof(type)) ? (type*)_ptr : NULL; \
    })

// C23 auto for automatic type deduction in loops
#define BDI_FOR_EACH_SAMPLE(trainer, sample_var) \
    for (auto sample_var = (trainer)->samples; \
         sample_var < (trainer)->samples + (trainer)->num_samples; \
         ++sample_var)

// C23 constexpr validation macros
#define BDI_VALIDATE_CONFIG(config) \
    static_assert((config)->batch_size >= BDI_MIN_BATCH_SIZE && \
                  (config)->batch_size <= BDI_MAX_BATCH_SIZE, \
                  "Invalid batch size")

// ===================================================================
// Error Handling with C23 Features
// ===================================================================

typedef enum {
    BDI_AI_TRAINER_SUCCESS = 0,
    BDI_AI_TRAINER_ERROR_INVALID_CONFIG,
    BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
    BDI_AI_TRAINER_ERROR_INVALID_SAMPLE,
    BDI_AI_TRAINER_ERROR_TRAINING_FAILED,
    BDI_AI_TRAINER_ERROR_THREAD_CREATION,
    BDI_AI_TRAINER_ERROR_CAPABILITY_MISSING
} bdi_ai_trainer_error_t;

// Error handling with C23 thread_local storage
thread_local bdi_ai_trainer_error_t bdi_ai_trainer_last_error;
thread_local char bdi_ai_trainer_error_message[256];

// Error reporting functions
const char* bdi_ai_trainer_error_string(bdi_ai_trainer_error_t error);
void bdi_ai_trainer_set_error(bdi_ai_trainer_error_t error, const char* message);
bdi_ai_trainer_error_t bdi_ai_trainer_get_last_error(void);

#ifdef __cplusplus
}
#endif

