
// ===================================================================
// BDI AI Trainer System Implementation - C23 Enhanced
// Advanced AI training system leveraging C23 features
// ===================================================================

#include "ai_trainer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <threads.h>
#include <assert.h>

// ===================================================================
// C23 Thread-Local Storage
// ===================================================================

thread_local bdi_ai_trainer_error_t bdi_ai_trainer_last_error = BDI_AI_TRAINER_SUCCESS;
thread_local char bdi_ai_trainer_error_message[256] = {0};
thread_local bool thread_initialized = false;

// ===================================================================
// Internal Helper Functions with C23 Features
// ===================================================================

// C23 constexpr helper for validation
constexpr bool bdi_is_valid_learning_rate(bdi_learning_rate_t rate) {
    return rate > 0.0dd && rate < 1.0dd;
}

// C23 typeof for generic initialization
#define BDI_INIT_ARRAY(arr, size, init_val) \
    do { \
        for (size_t i = 0; i < (size); ++i) { \
            (arr)[i] = (typeof((arr)[0]))(init_val); \
        } \
    } while(0)

// Thread initialization with C23 thread_local
static void bdi_init_thread_local_storage(void) {
    if (!thread_initialized) {
        bdi_ai_trainer_last_error = BDI_AI_TRAINER_SUCCESS;
        memset(bdi_ai_trainer_error_message, 0, sizeof(bdi_ai_trainer_error_message));
        thread_initialized = true;
    }
}

// ===================================================================
// Neural Layer Implementation with C23 Features
// ===================================================================

static bdi_neural_layer_t* bdi_create_neural_layer(size_t input_size, 
                                                   size_t output_size,
                                                   const char* activation_fn) {
    bdi_init_thread_local_storage();
    
    auto layer = (bdi_neural_layer_t*)aligned_alloc(64, sizeof(bdi_neural_layer_t));
    if (!layer) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION, 
                                "Failed to allocate neural layer");
        return NULL;
    }
    
    layer->input_size = input_size;
    layer->output_size = output_size;
    
    // Allocate aligned memory for SIMD operations
    size_t weight_count = input_size * output_size;
    layer->weights = (float*)aligned_alloc(64, weight_count * sizeof(float));
    layer->biases = (float*)aligned_alloc(64, output_size * sizeof(float));
    layer->weight_gradients = (float*)aligned_alloc(64, weight_count * sizeof(float));
    layer->bias_gradients = (float*)aligned_alloc(64, output_size * sizeof(float));
    
    if (!layer->weights || !layer->biases || 
        !layer->weight_gradients || !layer->bias_gradients) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
                                "Failed to allocate layer arrays");
        free(layer);
        return NULL;
    }
    
    // Initialize weights with Xavier initialization
    float xavier_std = sqrtf(2.0f / (input_size + output_size));
    for (size_t i = 0; i < weight_count; ++i) {
        layer->weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * xavier_std;
    }
    
    // Initialize biases to zero
    BDI_INIT_ARRAY(layer->biases, output_size, 0.0f);
    BDI_INIT_ARRAY(layer->weight_gradients, weight_count, 0.0f);
    BDI_INIT_ARRAY(layer->bias_gradients, output_size, 0.0f);
    
    // Set activation function
    strncpy(layer->activation_function, activation_fn, sizeof(layer->activation_function) - 1);
    layer->learning_rate = BDI_DEFAULT_LEARNING_RATE;
    
    // Thread-local storage allocation
    layer->activations = (float*)aligned_alloc(64, output_size * sizeof(float));
    layer->deltas = (float*)aligned_alloc(64, output_size * sizeof(float));
    
    return layer;
}

static void bdi_destroy_neural_layer(bdi_neural_layer_t* layer) {
    if (!layer) return;
    
    free(layer->weights);
    free(layer->biases);
    free(layer->weight_gradients);
    free(layer->bias_gradients);
    free(layer->activations);
    free(layer->deltas);
    free(layer);
}

// ===================================================================
// Forward Propagation with C23 SIMD Optimization
// ===================================================================

void bdi_neural_forward_pass(bdi_neural_layer_t* layer,
                            const float* restrict input,
                            float* restrict output) {
    // Matrix multiplication: output = weights * input + biases
    for (size_t i = 0; i < layer->output_size; ++i) {
        float sum = layer->biases[i];
        
        // Vectorized dot product (compiler will optimize with SIMD)
        #pragma GCC ivdep
        for (size_t j = 0; j < layer->input_size; ++j) {
            sum += layer->weights[i * layer->input_size + j] * input[j];
        }
        
        // Apply activation function using C23 constexpr
        if (strcmp(layer->activation_function, "relu") == 0) {
            output[i] = bdi_activation_relu(sum);
        } else if (strcmp(layer->activation_function, "sigmoid") == 0) {
            output[i] = bdi_activation_sigmoid(sum);
        } else if (strcmp(layer->activation_function, "tanh") == 0) {
            output[i] = bdi_activation_tanh(sum);
        } else {
            output[i] = sum; // Linear activation
        }
        
        // Store in thread-local cache
        layer->activations[i] = output[i];
    }
}

// ===================================================================
// Training Sample Management with C23 Features
// ===================================================================

auto bdi_create_training_sample(const float* features, size_t feature_count, 
                               bdi_label_t label) -> bdi_training_sample_t* {
    bdi_init_thread_local_storage();
    
    size_t sample_size = sizeof(bdi_training_sample_t) + feature_count * sizeof(float);
    auto sample = (bdi_training_sample_t*)aligned_alloc(64, sample_size);
    
    if (!sample) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
                                "Failed to allocate training sample");
        return NULL;
    }
    
    // Generate unique sample ID using C23 _BitInt
    static _Atomic bdi_sample_id_t next_sample_id = 1;
    sample->sample_id = atomic_fetch_add(&next_sample_id, 1);
    
    // Generate feature hash using C23 128-bit integers
    bdi_feature_hash_t hash = 0;
    for (size_t i = 0; i < feature_count; ++i) {
        // Simple hash combining features
        hash ^= (bdi_feature_hash_t)(features[i] * 1000000) << (i % 128);
    }
    sample->feature_hash = hash;
    
    // Set timestamp using C23 64-bit integer
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sample->timestamp = (bdi_timestamp_t)(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    
    sample->label = label;
    
    // Copy features
    memcpy(sample->features, features, feature_count * sizeof(float));
    
    return sample;
}

// ===================================================================
// AI Trainer Creation with C23 Enhanced Configuration
// ===================================================================

bdi_ai_trainer_t* bdi_ai_trainer_create(const bdi_ai_trainer_config_t* config) {
    bdi_init_thread_local_storage();
    
    if (!config) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_CONFIG,
                                "Configuration cannot be NULL");
        return NULL;
    }
    
    // Validate configuration using C23 constexpr
    if (!bdi_is_valid_learning_rate(config->base_learning_rate)) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_CONFIG,
                                "Invalid learning rate");
        return NULL;
    }
    
    auto trainer = (bdi_ai_trainer_t*)aligned_alloc(64, sizeof(bdi_ai_trainer_t));
    if (!trainer) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
                                "Failed to allocate trainer");
        return NULL;
    }
    
    // Copy configuration
    trainer->config = *config;
    
    // Create neural network layers
    trainer->num_layers = config->num_layers;
    trainer->layers = (bdi_neural_layer_t*)calloc(config->num_layers, 
                                                  sizeof(bdi_neural_layer_t));
    
    if (!trainer->layers) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
                                "Failed to allocate layers");
        free(trainer);
        return NULL;
    }
    
    // Initialize layers
    for (size_t i = 0; i < config->num_layers; ++i) {
        size_t input_size = (i == 0) ? BDI_MAX_FEATURES : config->layer_sizes[i-1];
        size_t output_size = config->layer_sizes[i];
        
        auto layer = bdi_create_neural_layer(input_size, output_size, 
                                           (i == config->num_layers - 1) ? "sigmoid" : "relu");
        if (!layer) {
            // Cleanup on failure
            for (size_t j = 0; j < i; ++j) {
                bdi_destroy_neural_layer(&trainer->layers[j]);
            }
            free(trainer->layers);
            free(trainer);
            return NULL;
        }
        trainer->layers[i] = *layer;
        free(layer); // We copied the contents
    }
    
    // Initialize training data storage
    trainer->sample_capacity = BDI_MAX_SAMPLES;
    trainer->samples = (bdi_training_sample_t**)calloc(trainer->sample_capacity,
                                                       sizeof(bdi_training_sample_t*));
    trainer->num_samples = 0;
    
    // Initialize training state
    trainer->current_epoch = 0;
    trainer->current_batch = 0;
    trainer->current_learning_rate = config->base_learning_rate;
    
    // Initialize performance metrics with C23 _Decimal precision
    trainer->training_loss = 0.0dd;
    trainer->validation_loss = 0.0dd;
    trainer->training_accuracy = 0.0dd;
    trainer->validation_accuracy = 0.0dd;
    
    // Initialize thread synchronization
    if (mtx_init(&trainer->training_mutex, mtx_plain) != thrd_success) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_THREAD_CREATION,
                                "Failed to initialize mutex");
        bdi_ai_trainer_destroy(trainer);
        return NULL;
    }
    
    if (cnd_init(&trainer->training_condition) != thrd_success) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_THREAD_CREATION,
                                "Failed to initialize condition variable");
        bdi_ai_trainer_destroy(trainer);
        return NULL;
    }
    
    // Initialize memory manager if requested
    if (config->use_attention_mm) {
        trainer->memory_manager = bdi_attention_mm_create(&config->attention_config);
        if (!trainer->memory_manager) {
            bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION,
                                    "Failed to create attention memory manager");
            bdi_ai_trainer_destroy(trainer);
            return NULL;
        }
    }
    
    // Initialize statistics
    trainer->total_samples_processed = 0;
    trainer->total_training_time_us = 0;
    trainer->total_inference_time_us = 0;
    
    return trainer;
}

// ===================================================================
// Training Sample Addition with C23 Features
// ===================================================================

bool bdi_ai_trainer_add_sample(bdi_ai_trainer_t* trainer, 
                              const void* features, size_t feature_count,
                              bdi_label_t label) {
    if (!trainer || !features || feature_count == 0) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_SAMPLE,
                                "Invalid sample parameters");
        return false;
    }
    
    if (trainer->num_samples >= trainer->sample_capacity) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_SAMPLE,
                                "Sample capacity exceeded");
        return false;
    }
    
    // Create training sample using C23 auto
    auto sample = bdi_create_training_sample((const float*)features, 
                                           feature_count, label);
    if (!sample) {
        return false; // Error already set
    }
    
    // Thread-safe addition
    mtx_lock(&trainer->training_mutex);
    trainer->samples[trainer->num_samples++] = sample;
    mtx_unlock(&trainer->training_mutex);
    
    // Update attention score if using attention memory manager
    if (trainer->memory_manager) {
        float attention_score = (label > 0) ? 0.8f : 0.2f; // Higher attention for liked samples
        bdi_set_attention_score(trainer->memory_manager, sample, attention_score);
    }
    
    return true;
}

// ===================================================================
// Training Epoch Implementation with C23 Precision
// ===================================================================

bool bdi_ai_trainer_train_epoch(bdi_ai_trainer_t* trainer) {
    if (!trainer || trainer->num_samples == 0) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_CONFIG,
                                "No training samples available");
        return false;
    }
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Shuffle samples for this epoch
    for (size_t i = trainer->num_samples - 1; i > 0; --i) {
        size_t j = rand() % (i + 1);
        auto temp = trainer->samples[i];
        trainer->samples[i] = trainer->samples[j];
        trainer->samples[j] = temp;
    }
    
    _Decimal64 epoch_loss = 0.0dd;
    size_t correct_predictions = 0;
    
    // Process samples in batches
    for (size_t batch_start = 0; batch_start < trainer->num_samples; 
         batch_start += trainer->config.batch_size) {
        
        size_t batch_end = (batch_start + trainer->config.batch_size < trainer->num_samples) ?
                          batch_start + trainer->config.batch_size : trainer->num_samples;
        
        _Decimal64 batch_loss = 0.0dd;
        
        // Forward pass for batch
        for (size_t i = batch_start; i < batch_end; ++i) {
            auto sample = trainer->samples[i];
            
            // Forward propagation through all layers
            const float* layer_input = sample->features;
            float layer_output[trainer->layers[trainer->num_layers - 1].output_size];
            
            for (size_t layer_idx = 0; layer_idx < trainer->num_layers; ++layer_idx) {
                bdi_neural_forward_pass(&trainer->layers[layer_idx], 
                                       layer_input, layer_output);
                layer_input = layer_output;
            }
            
            // Compute loss (binary cross-entropy)
            float prediction = layer_output[0];
            float target = (sample->label > 0) ? 1.0f : 0.0f;
            
            // Prevent log(0) with small epsilon
            constexpr float epsilon = 1e-7f;
            prediction = fmaxf(epsilon, fminf(1.0f - epsilon, prediction));
            
            _Decimal64 sample_loss = -(((_Decimal64)target * log(prediction)) + 
                                     ((1.0dd - (_Decimal64)target) * log(1.0dd - (_Decimal64)prediction)));
            batch_loss += sample_loss;
            
            // Count correct predictions
            if ((prediction > 0.5f && sample->label > 0) || 
                (prediction <= 0.5f && sample->label <= 0)) {
                correct_predictions++;
            }
        }
        
        epoch_loss += batch_loss;
        trainer->current_batch++;
        
        // Update attention scores for processed samples
        if (trainer->memory_manager) {
            for (size_t i = batch_start; i < batch_end; ++i) {
                bdi_track_memory_access(trainer->memory_manager, 
                                       trainer->samples[i], false);
            }
        }
    }
    
    // Update trainer state
    trainer->training_loss = epoch_loss / (_Decimal64)trainer->num_samples;
    trainer->training_accuracy = (_Decimal64)correct_predictions / (_Decimal64)trainer->num_samples;
    trainer->current_epoch++;
    
    // Apply learning rate decay
    trainer->current_learning_rate *= trainer->config.learning_rate_decay;
    
    // Update timing statistics
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t epoch_time_us = (end_time.tv_sec - start_time.tv_sec) * 1000000ULL +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1000ULL;
    trainer->total_training_time_us += epoch_time_us;
    trainer->total_samples_processed += trainer->num_samples;
    
    return true;
}

// ===================================================================
// Inference with C23 Precision
// ===================================================================

bdi_confidence_t bdi_ai_trainer_predict(bdi_ai_trainer_t* trainer,
                                       const float* features,
                                       size_t feature_count) {
    if (!trainer || !features || feature_count == 0) {
        bdi_ai_trainer_set_error(BDI_AI_TRAINER_ERROR_INVALID_SAMPLE,
                                "Invalid prediction parameters");
        return 0.0df;
    }
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Forward propagation through all layers
    const float* layer_input = features;
    float layer_output[trainer->layers[trainer->num_layers - 1].output_size];
    
    for (size_t layer_idx = 0; layer_idx < trainer->num_layers; ++layer_idx) {
        bdi_neural_forward_pass(&trainer->layers[layer_idx], 
                               layer_input, layer_output);
        layer_input = layer_output;
    }
    
    // Update timing statistics
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t inference_time_us = (end_time.tv_sec - start_time.tv_sec) * 1000000ULL +
                                (end_time.tv_nsec - start_time.tv_nsec) / 1000ULL;
    trainer->total_inference_time_us += inference_time_us;
    
    return (bdi_confidence_t)layer_output[0];
}

// ===================================================================
// Feature Extraction Functions
// ===================================================================

float bdi_extract_int_feature(int value) {
    return (float)value / 1000.0f; // Normalize
}

float bdi_extract_float_feature(float value) {
    return value;
}

float bdi_extract_double_feature(double value) {
    return (float)value;
}

float bdi_extract_string_feature(const char* str) {
    if (!str) return 0.0f;
    
    // Simple string hash as feature
    uint32_t hash = 0;
    for (const char* p = str; *p; ++p) {
        hash = hash * 31 + (uint32_t)*p;
    }
    return (float)(hash % 10000) / 10000.0f;
}

float bdi_extract_generic_feature(const void* data) {
    if (!data) return 0.0f;
    
    // Use pointer value as feature (not recommended for production)
    uintptr_t ptr_val = (uintptr_t)data;
    return (float)(ptr_val % 10000) / 10000.0f;
}

// ===================================================================
// Statistics and Monitoring
// ===================================================================

void bdi_ai_trainer_get_stats(bdi_ai_trainer_t* trainer, 
                             bdi_ai_trainer_stats_t* stats) {
    if (!trainer || !stats) return;
    
    memset(stats, 0, sizeof(bdi_ai_trainer_stats_t));
    
    // Training performance metrics
    if (trainer->total_training_time_us > 0) {
        stats->samples_per_second = (trainer->total_samples_processed * 1000000ULL) / 
                                   trainer->total_training_time_us;
    }
    
    stats->average_loss = trainer->training_loss;
    
    // Memory usage
    stats->total_memory_allocated = trainer->num_samples * sizeof(bdi_training_sample_t*);
    for (size_t i = 0; i < trainer->num_samples; ++i) {
        stats->total_memory_allocated += sizeof(bdi_training_sample_t) + 
                                        BDI_MAX_FEATURES * sizeof(float);
    }
    
    // Thread-local statistics
    stats->thread_samples_processed = trainer->total_samples_processed;
    stats->thread_training_time_us = trainer->total_training_time_us;
}

// ===================================================================
// Error Handling Implementation
// ===================================================================

const char* bdi_ai_trainer_error_string(bdi_ai_trainer_error_t error) {
    switch (error) {
        case BDI_AI_TRAINER_SUCCESS:
            return "Success";
        case BDI_AI_TRAINER_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case BDI_AI_TRAINER_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case BDI_AI_TRAINER_ERROR_INVALID_SAMPLE:
            return "Invalid training sample";
        case BDI_AI_TRAINER_ERROR_TRAINING_FAILED:
            return "Training failed";
        case BDI_AI_TRAINER_ERROR_THREAD_CREATION:
            return "Thread creation failed";
        case BDI_AI_TRAINER_ERROR_CAPABILITY_MISSING:
            return "Required capability missing";
        default:
            return "Unknown error";
    }
}

void bdi_ai_trainer_set_error(bdi_ai_trainer_error_t error, const char* message) {
    bdi_ai_trainer_last_error = error;
    if (message) {
        strncpy(bdi_ai_trainer_error_message, message, 
                sizeof(bdi_ai_trainer_error_message) - 1);
        bdi_ai_trainer_error_message[sizeof(bdi_ai_trainer_error_message) - 1] = '\0';
    }
}

bdi_ai_trainer_error_t bdi_ai_trainer_get_last_error(void) {
    return bdi_ai_trainer_last_error;
}

// ===================================================================
// Cleanup and Destruction
// ===================================================================

void bdi_ai_trainer_destroy(bdi_ai_trainer_t* trainer) {
    if (!trainer) return;
    
    // Destroy neural layers
    if (trainer->layers) {
        for (size_t i = 0; i < trainer->num_layers; ++i) {
            bdi_destroy_neural_layer(&trainer->layers[i]);
        }
        free(trainer->layers);
    }
    
    // Free training samples
    if (trainer->samples) {
        for (size_t i = 0; i < trainer->num_samples; ++i) {
            free(trainer->samples[i]);
        }
        free(trainer->samples);
    }
    
    // Destroy memory manager
    if (trainer->memory_manager) {
        bdi_attention_mm_destroy(trainer->memory_manager);
    }
    
    // Destroy synchronization objects
    mtx_destroy(&trainer->training_mutex);
    cnd_destroy(&trainer->training_condition);
    
    free(trainer);
}

