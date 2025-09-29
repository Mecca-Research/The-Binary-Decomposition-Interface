
// ===================================================================
// BDI Attention Memory Manager C23 Enhancements
// Enhanced memory manager with C23 features
// ===================================================================

#pragma once

#include "attention_mm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// C23 Enhanced Memory Metadata
// ===================================================================

typedef struct {
    // Base metadata
    bdi_page_meta_t base_meta;
    
    // C23 enhanced fields with precise types
    _BitInt(64) extended_flags;         // Extended flags using C23 _BitInt
    _Decimal64 precise_attention;       // High-precision attention score
    _Decimal32 confidence_score;        // Confidence in attention prediction
    
    // C23 typeof for flexible metadata
    union {
        float float_metadata[4];        // Generic float metadata
        int32_t int_metadata[4];        // Generic integer metadata
        void* ptr_metadata[2];          // Generic pointer metadata
    } flexible_data;
    
    // Thread-local access tracking
    thread_local uint64_t thread_access_count;
    thread_local _Decimal64 thread_attention_contribution;
    
} bdi_page_meta_c23_t;

// ===================================================================
// C23 Enhanced Memory Pool Configuration
// ===================================================================

typedef struct {
    // Base pool configuration
    bdi_memory_pool_info_t base_info;
    
    // C23 enhanced configuration
    _Decimal128 precise_latency_ns;     // Ultra-precise latency measurement
    _Decimal64 bandwidth_efficiency;    // Bandwidth utilization efficiency
    
    // C23 constexpr validation
    constexpr size_t min_pool_size = 1024 * 1024;  // 1MB minimum
    constexpr size_t max_pool_size = 1024ULL * 1024 * 1024 * 1024; // 1TB maximum
    
    // Advanced pool characteristics
    _BitInt(32) pool_features;          // Feature flags using C23 _BitInt
    bool supports_encryption;
    bool supports_compression;
    bool supports_deduplication;
    
} bdi_memory_pool_c23_t;

// ===================================================================
// C23 Enhanced Attention Configuration
// ===================================================================

typedef struct {
    // Base attention configuration
    bdi_attention_config_t base_config;
    
    // C23 enhanced learning parameters with _Decimal precision
    _Decimal128 ultra_precise_learning_rate;    // Ultra-high precision learning
    _Decimal64 adaptive_decay_rate;             // Adaptive decay based on usage
    _Decimal64 confidence_threshold;            // Confidence threshold for decisions
    
    // C23 constexpr optimization parameters
    constexpr _Decimal64 min_learning_rate = 0.0001dd;
    constexpr _Decimal64 max_learning_rate = 0.1dd;
    constexpr size_t max_adaptation_history = 10000;
    
    // Advanced attention mechanisms
    bool enable_meta_learning;          // Learn how to learn attention
    bool enable_transfer_learning;      // Transfer attention patterns
    bool enable_ensemble_attention;     // Use multiple attention models
    
    // C23 thread_local optimization settings
    thread_local bool thread_optimization_enabled;
    thread_local _Decimal64 thread_learning_rate_multiplier;
    
} bdi_attention_config_c23_t;

// ===================================================================
// C23 Enhanced Memory Manager Interface
// ===================================================================

typedef struct bdi_attention_mm_c23 bdi_attention_mm_c23_t;

// Enhanced memory manager creation with C23 features
bdi_attention_mm_c23_t* bdi_attention_mm_c23_create(const bdi_attention_config_c23_t* config);
void bdi_attention_mm_c23_destroy(bdi_attention_mm_c23_t* mm);

// C23 auto return type deduction for allocation
auto bdi_attention_alloc_c23(bdi_attention_mm_c23_t* mm, size_t size, 
                            uint32_t flags) -> void*;

// C23 typeof for type-safe allocation
#define BDI_ALLOC_TYPED(mm, type, count) \
    ({ \
        size_t _size = sizeof(type) * (count); \
        typeof(type)* _ptr = (typeof(type)*)bdi_attention_alloc_c23(mm, _size, 0); \
        _ptr; \
    })

// Enhanced attention score management with C23 _Decimal precision
bool bdi_set_precise_attention_score(bdi_attention_mm_c23_t* mm, void* ptr, 
                                    _Decimal64 attention);
_Decimal64 bdi_get_precise_attention_score(bdi_attention_mm_c23_t* mm, void* ptr);

// C23 constexpr validation for attention scores
constexpr bool bdi_is_valid_attention_score(_Decimal64 score) {
    return score >= 0.0dd && score <= 1.0dd;
}

// ===================================================================
// C23 Generic Memory Operations
// ===================================================================

// C23 _Generic for type-safe memory operations
#define bdi_attention_track_access(mm, ptr, access_type) _Generic((access_type), \
    bool: bdi_attention_track_read_write, \
    int: bdi_attention_track_operation_type, \
    char*: bdi_attention_track_named_access, \
    default: bdi_attention_track_generic_access \
)(mm, ptr, access_type)

// Type-specific access tracking functions
void bdi_attention_track_read_write(bdi_attention_mm_c23_t* mm, void* ptr, bool is_write);
void bdi_attention_track_operation_type(bdi_attention_mm_c23_t* mm, void* ptr, int op_type);
void bdi_attention_track_named_access(bdi_attention_mm_c23_t* mm, void* ptr, const char* access_name);
void bdi_attention_track_generic_access(bdi_attention_mm_c23_t* mm, void* ptr, const void* access_info);

// ===================================================================
// C23 BitInt Operations for Memory Flags
// ===================================================================

// Extended memory flags using C23 _BitInt(64)
typedef enum {
    BDI_MEM_C23_PRECISE_TRACKING    = (_BitInt(64))1 << 32,  // Enable precise tracking
    BDI_MEM_C23_META_LEARNING       = (_BitInt(64))1 << 33,  // Enable meta-learning
    BDI_MEM_C23_TRANSFER_LEARNING   = (_BitInt(64))1 << 34,  // Enable transfer learning
    BDI_MEM_C23_ENSEMBLE_ATTENTION  = (_BitInt(64))1 << 35,  // Use ensemble attention
    BDI_MEM_C23_ULTRA_PRECISION     = (_BitInt(64))1 << 36,  // Ultra-precise calculations
    BDI_MEM_C23_ADAPTIVE_DECAY      = (_BitInt(64))1 << 37,  // Adaptive decay rates
    BDI_MEM_C23_CONFIDENCE_TRACKING = (_BitInt(64))1 << 38,  // Track confidence scores
    BDI_MEM_C23_THREAD_OPTIMIZATION = (_BitInt(64))1 << 39,  // Thread-local optimization
} bdi_memory_flags_c23_t;

// Flag manipulation functions
static inline void bdi_set_memory_flag_c23(bdi_page_meta_c23_t* meta, 
                                          bdi_memory_flags_c23_t flag) {
    meta->extended_flags |= (_BitInt(64))flag;
}

static inline bool bdi_has_memory_flag_c23(const bdi_page_meta_c23_t* meta, 
                                          bdi_memory_flags_c23_t flag) {
    return (meta->extended_flags & (_BitInt(64))flag) != 0;
}

// ===================================================================
// C23 Constexpr Memory Validation
// ===================================================================

// Compile-time memory validation using C23 constexpr
constexpr bool bdi_is_valid_memory_size(size_t size) {
    return size > 0 && size <= (1ULL << 48); // Maximum 256TB
}

constexpr bool bdi_is_aligned_size(size_t size, size_t alignment) {
    return (size & (alignment - 1)) == 0;
}

constexpr size_t bdi_align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Validation macros using C23 constexpr
#define BDI_VALIDATE_MEMORY_SIZE(size) \
    static_assert(bdi_is_valid_memory_size(size), "Invalid memory size")

#define BDI_VALIDATE_ALIGNMENT(size, align) \
    static_assert(bdi_is_aligned_size(size, align), "Size not aligned")

// ===================================================================
// C23 Thread-Local Memory Statistics
// ===================================================================

typedef struct {
    // Thread-local allocation statistics
    thread_local size_t thread_allocations;
    thread_local size_t thread_deallocations;
    thread_local size_t thread_bytes_allocated;
    thread_local size_t thread_bytes_freed;
    
    // Thread-local attention statistics
    thread_local _Decimal64 thread_avg_attention;
    thread_local _Decimal64 thread_attention_variance;
    thread_local size_t thread_attention_updates;
    
    // Thread-local performance metrics
    thread_local uint64_t thread_cache_hits;
    thread_local uint64_t thread_cache_misses;
    thread_local _Decimal64 thread_hit_rate;
    
} bdi_memory_stats_c23_t;

// Thread-local statistics functions
void bdi_init_thread_memory_stats(void);
void bdi_update_thread_memory_stats(size_t bytes_allocated, _Decimal64 attention_score);
void bdi_get_thread_memory_stats(bdi_memory_stats_c23_t* stats);
void bdi_reset_thread_memory_stats(void);

// ===================================================================
// C23 Advanced Attention Learning
// ===================================================================

// Meta-learning structure for attention patterns
typedef struct {
    // Learning history with C23 _Decimal precision
    _Decimal64 learning_history[1000];  // Recent learning rates
    size_t history_index;
    
    // Pattern recognition
    _BitInt(128) access_pattern_hash;    // Hash of recent access patterns
    _Decimal64 pattern_confidence;       // Confidence in pattern recognition
    
    // Transfer learning
    struct {
        void* source_memory;             // Source memory for transfer
        _Decimal64 transfer_weight;      // Weight for transfer learning
        _Decimal64 similarity_score;     // Similarity to source
    } transfer_info;
    
    // Ensemble attention models
    struct {
        _Decimal64 model_weights[8];     // Weights for ensemble models
        _Decimal64 model_predictions[8]; // Predictions from each model
        size_t num_models;
    } ensemble;
    
} bdi_attention_learning_c23_t;

// Advanced learning functions
bool bdi_enable_meta_learning(bdi_attention_mm_c23_t* mm, void* ptr);
bool bdi_setup_transfer_learning(bdi_attention_mm_c23_t* mm, void* target_ptr, 
                                void* source_ptr, _Decimal64 transfer_weight);
bool bdi_configure_ensemble_attention(bdi_attention_mm_c23_t* mm, void* ptr, 
                                     size_t num_models);

// ===================================================================
// C23 Auto Type Deduction for Memory Operations
// ===================================================================

// C23 auto for automatic type deduction in memory operations
#define BDI_AUTO_ALLOC(mm, size) \
    ({ \
        auto _ptr = bdi_attention_alloc_c23(mm, size, 0); \
        _ptr; \
    })

#define BDI_AUTO_TRACK_ACCESS(mm, ptr, is_write) \
    ({ \
        auto _result = bdi_attention_track_access(mm, ptr, is_write); \
        _result; \
    })

// ===================================================================
// C23 Decimal Arithmetic for Precise Memory Calculations
// ===================================================================

// High-precision memory calculations using C23 _Decimal
static inline _Decimal128 bdi_calculate_memory_efficiency(
    size_t bytes_allocated,
    size_t bytes_used,
    _Decimal64 access_frequency) {
    
    if (bytes_allocated == 0) return 0.0dl;
    
    _Decimal128 utilization = (_Decimal128)bytes_used / (_Decimal128)bytes_allocated;
    _Decimal128 efficiency = utilization * (_Decimal128)access_frequency;
    
    return efficiency;
}

static inline _Decimal64 bdi_calculate_attention_decay(
    _Decimal64 current_attention,
    _Decimal64 decay_rate,
    uint64_t time_delta_us) {
    
    // Exponential decay with high precision
    _Decimal64 time_factor = (_Decimal64)time_delta_us / 1000000.0dd; // Convert to seconds
    _Decimal64 decay_factor = 1.0dd - (decay_rate * time_factor);
    
    return current_attention * decay_factor;
}

// ===================================================================
// Integration with AI Trainer System
// ===================================================================

// Integration functions for AI trainer
bool bdi_attention_mm_c23_integrate_ai_trainer(bdi_attention_mm_c23_t* mm,
                                               void* ai_trainer);
bool bdi_attention_mm_c23_provide_training_data(bdi_attention_mm_c23_t* mm,
                                                void* ai_trainer);

// Memory access pattern extraction for AI training
typedef struct {
    _BitInt(128) pattern_hash;           // Pattern identifier
    float features[32];                  // Feature vector for AI training
    _Decimal64 attention_score;          // Target attention score
    int32_t access_type;                 // Type of memory access
} bdi_memory_training_sample_t;

// Extract training samples from memory access patterns
size_t bdi_extract_memory_training_samples(bdi_attention_mm_c23_t* mm,
                                          bdi_memory_training_sample_t* samples,
                                          size_t max_samples);

#ifdef __cplusplus
}
#endif

