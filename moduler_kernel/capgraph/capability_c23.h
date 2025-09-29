
// ===================================================================
// BDI Capability Graph - C23 Enhanced
// Enhanced capability detection and management with C23 features
// ===================================================================

#pragma once

#include "capability.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// C23 Enhanced Capability Types
// ===================================================================

// C23 _BitInt for precise capability masks
typedef _BitInt(256) bdi_capability_mask_c23_t;
typedef _BitInt(128) bdi_feature_mask_c23_t;
typedef _BitInt(64) bdi_hardware_mask_c23_t;

// C23 _Decimal for capability scores and metrics
typedef _Decimal64 bdi_capability_score_t;
typedef _Decimal32 bdi_performance_metric_t;

// C23 constexpr capability limits
constexpr size_t BDI_MAX_CAPABILITIES_C23 = 256;
constexpr size_t BDI_MAX_FEATURES_C23 = 128;
constexpr size_t BDI_MAX_HARDWARE_UNITS_C23 = 64;

// ===================================================================
// C23 Enhanced Capability Descriptor
// ===================================================================

typedef struct {
    char name[64];                      // Capability name
    char description[256];              // Detailed description
    
    // Capability identification using C23 _BitInt
    _BitInt(32) capability_id;          // Unique capability ID
    bdi_capability_mask_c23_t mask;     // Capability mask
    bdi_feature_mask_c23_t features;    // Required features
    bdi_hardware_mask_c23_t hardware;   // Required hardware
    
    // Performance characteristics with C23 _Decimal precision
    bdi_capability_score_t performance_score;      // Overall performance score
    bdi_performance_metric_t latency_score;        // Latency performance
    bdi_performance_metric_t throughput_score;     // Throughput performance
    bdi_performance_metric_t energy_efficiency;    // Energy efficiency
    
    // Capability flags using C23 _BitInt
    _BitInt(64) flags;                  // Various capability flags
    
    // Thread-local capability state
    thread_local bool thread_available;
    thread_local bdi_capability_score_t thread_performance;
    
    // Detection and validation functions
    bool (*detect)(void);               // Capability detection function
    bool (*validate)(void);             // Capability validation function
    void (*benchmark)(bdi_capability_score_t* score); // Benchmark function
    
} bdi_capability_descriptor_c23_t;

// ===================================================================
// C23 Enhanced Hardware Detection
// ===================================================================

typedef struct {
    // CPU capabilities with C23 _BitInt precision
    _BitInt(64) cpu_features;           // CPU feature mask
    _BitInt(32) cpu_cores;              // Number of CPU cores
    _BitInt(32) cpu_threads;            // Number of hardware threads
    _BitInt(32) cpu_frequency_mhz;      // CPU frequency
    
    // Cache hierarchy
    struct {
        _BitInt(32) l1_cache_kb;        // L1 cache size
        _BitInt(32) l2_cache_kb;        // L2 cache size
        _BitInt(32) l3_cache_kb;        // L3 cache size
        _BitInt(16) cache_line_bytes;   // Cache line size
    } cache;
    
    // Memory system with C23 _Decimal precision
    _BitInt(64) total_memory_bytes;     // Total system memory
    _BitInt(64) available_memory_bytes; // Available memory
    _Decimal64 memory_bandwidth_gbps;   // Memory bandwidth
    _BitInt(16) numa_nodes;             // Number of NUMA nodes
    
    // Accelerators and special units
    struct {
        bool has_gpu;                   // GPU present
        bool has_fpga;                  // FPGA present
        bool has_tpu;                   // TPU present
        bool has_dsp;                   // DSP present
        _BitInt(16) gpu_count;          // Number of GPUs
        _BitInt(64) gpu_memory_bytes;   // Total GPU memory
    } accelerators;
    
    // Advanced CPU features (C23 enhanced)
    struct {
        bool has_avx512;                // AVX-512 support
        bool has_amx;                   // Advanced Matrix Extensions
        bool has_sve;                   // ARM SVE support
        bool has_risc_v_vector;         // RISC-V vector extensions
        bool has_c23_bitint;            // C23 _BitInt support
        bool has_c23_decimal;           // C23 _Decimal support
    } advanced_features;
    
} bdi_hardware_info_c23_t;

// ===================================================================
// C23 Enhanced Capability Detection
// ===================================================================

// Hardware detection with C23 features
bool bdi_detect_hardware_c23(bdi_hardware_info_c23_t* hw_info);
void bdi_print_hardware_info_c23(const bdi_hardware_info_c23_t* hw_info);

// C23 capability detection functions
bool bdi_detect_c23_language_features(bdi_feature_mask_c23_t* features);
bool bdi_detect_simd_capabilities(bdi_hardware_mask_c23_t* simd_mask);
bool bdi_detect_memory_capabilities(bdi_hardware_mask_c23_t* memory_mask);

// C23 auto return type for capability queries
auto bdi_query_capability_c23(const char* capability_name) -> bdi_capability_descriptor_c23_t*;
auto bdi_query_capabilities_by_mask_c23(bdi_capability_mask_c23_t mask) -> bdi_capability_descriptor_c23_t**;

// ===================================================================
// C23 Enhanced Capability Scoring
// ===================================================================

// Capability scoring with C23 _Decimal precision
bdi_capability_score_t bdi_score_capability_c23(const bdi_capability_descriptor_c23_t* cap,
                                                const bdi_hardware_info_c23_t* hw_info);

bdi_capability_score_t bdi_score_capability_for_workload_c23(const bdi_capability_descriptor_c23_t* cap,
                                                            const char* workload_profile);

// C23 constexpr scoring functions
constexpr bdi_capability_score_t bdi_calculate_performance_score(
    bdi_performance_metric_t latency,
    bdi_performance_metric_t throughput,
    bdi_performance_metric_t efficiency) {
    return (bdi_capability_score_t)(latency * 0.3dd + throughput * 0.5dd + efficiency * 0.2dd);
}

// ===================================================================
// C23 Enhanced Capability Masks
// ===================================================================

// Capability mask operations using C23 _BitInt
static inline void bdi_set_capability_c23(bdi_capability_mask_c23_t* mask, unsigned int cap_id) {
    if (cap_id < BDI_MAX_CAPABILITIES_C23) {
        *mask |= (bdi_capability_mask_c23_t)1 << cap_id;
    }
}

static inline void bdi_clear_capability_c23(bdi_capability_mask_c23_t* mask, unsigned int cap_id) {
    if (cap_id < BDI_MAX_CAPABILITIES_C23) {
        *mask &= ~((bdi_capability_mask_c23_t)1 << cap_id);
    }
}

static inline bool bdi_has_capability_c23(bdi_capability_mask_c23_t mask, unsigned int cap_id) {
    if (cap_id >= BDI_MAX_CAPABILITIES_C23) return false;
    return (mask & ((bdi_capability_mask_c23_t)1 << cap_id)) != 0;
}

// Feature mask operations
static inline void bdi_set_feature_c23(bdi_feature_mask_c23_t* mask, unsigned int feature_id) {
    if (feature_id < BDI_MAX_FEATURES_C23) {
        *mask |= (bdi_feature_mask_c23_t)1 << feature_id;
    }
}

static inline bool bdi_has_feature_c23(bdi_feature_mask_c23_t mask, unsigned int feature_id) {
    if (feature_id >= BDI_MAX_FEATURES_C23) return false;
    return (mask & ((bdi_feature_mask_c23_t)1 << feature_id)) != 0;
}

// ===================================================================
// C23 Enhanced Capability Registry
// ===================================================================

typedef struct {
    bdi_capability_descriptor_c23_t* capabilities[BDI_MAX_CAPABILITIES_C23];
    size_t capability_count;
    
    // Global capability state with C23 _BitInt
    bdi_capability_mask_c23_t available_capabilities;
    bdi_capability_mask_c23_t enabled_capabilities;
    bdi_feature_mask_c23_t detected_features;
    bdi_hardware_mask_c23_t detected_hardware;
    
    // Performance tracking with C23 _Decimal
    _Decimal64 total_performance_score;
    _Decimal64 weighted_performance_score;
    
    // Thread-local registry state
    thread_local bdi_capability_mask_c23_t thread_capabilities;
    thread_local _Decimal64 thread_performance_score;
    
} bdi_capability_registry_c23_t;

// Registry management functions
bdi_capability_registry_c23_t* bdi_capability_registry_c23_create(void);
void bdi_capability_registry_c23_destroy(bdi_capability_registry_c23_t* registry);

bool bdi_register_capability_c23(bdi_capability_registry_c23_t* registry,
                                 const bdi_capability_descriptor_c23_t* capability);
bool bdi_unregister_capability_c23(bdi_capability_registry_c23_t* registry,
                                  _BitInt(32) capability_id);

// ===================================================================
// C23 Enhanced Capability Matching
// ===================================================================

// Capability matching with C23 features
typedef struct {
    bdi_capability_mask_c23_t required_capabilities;
    bdi_feature_mask_c23_t required_features;
    bdi_hardware_mask_c23_t required_hardware;
    
    // Scoring weights with C23 _Decimal precision
    _Decimal64 performance_weight;
    _Decimal64 latency_weight;
    _Decimal64 throughput_weight;
    _Decimal64 efficiency_weight;
    
    // Matching constraints
    bdi_capability_score_t min_score;
    bdi_performance_metric_t max_latency;
    bdi_performance_metric_t min_throughput;
    
} bdi_capability_requirements_c23_t;

// Matching functions
bdi_capability_descriptor_c23_t* bdi_find_best_capability_c23(
    const bdi_capability_registry_c23_t* registry,
    const bdi_capability_requirements_c23_t* requirements);

bdi_capability_descriptor_c23_t** bdi_find_matching_capabilities_c23(
    const bdi_capability_registry_c23_t* registry,
    const bdi_capability_requirements_c23_t* requirements,
    size_t* match_count);

// C23 _Generic for flexible capability matching
#define bdi_match_capability_c23(registry, requirements) _Generic((requirements), \
    bdi_capability_requirements_c23_t*: bdi_find_best_capability_c23, \
    bdi_capability_mask_c23_t: bdi_find_capability_by_mask_c23, \
    char*: bdi_find_capability_by_name_c23, \
    default: bdi_find_capability_generic_c23 \
)(registry, requirements)

// ===================================================================
// C23 Enhanced Benchmarking
// ===================================================================

typedef struct {
    // Benchmark configuration
    size_t iterations;
    size_t warmup_iterations;
    bool measure_latency;
    bool measure_throughput;
    bool measure_energy;
    
    // Results with C23 _Decimal precision
    _Decimal64 avg_latency_ns;
    _Decimal64 min_latency_ns;
    _Decimal64 max_latency_ns;
    _Decimal64 latency_stddev_ns;
    
    _Decimal64 throughput_ops_per_sec;
    _Decimal64 energy_per_op_nj;
    
    // Thread-local benchmark state
    thread_local _Decimal64 thread_total_time;
    thread_local size_t thread_iterations;
    
} bdi_capability_benchmark_c23_t;

// Benchmarking functions
bool bdi_benchmark_capability_c23(bdi_capability_descriptor_c23_t* capability,
                                 bdi_capability_benchmark_c23_t* benchmark);

void bdi_print_benchmark_results_c23(const bdi_capability_benchmark_c23_t* benchmark);

// ===================================================================
// C23 Enhanced Integration
// ===================================================================

// Integration with AI trainer system
bool bdi_capability_integrate_ai_trainer_c23(bdi_capability_registry_c23_t* registry,
                                             void* ai_trainer);

// Integration with orchestrator
bool bdi_capability_integrate_orchestrator_c23(bdi_capability_registry_c23_t* registry,
                                               void* orchestrator);

// Integration with attention memory manager
bool bdi_capability_integrate_attention_mm_c23(bdi_capability_registry_c23_t* registry,
                                               void* attention_mm);

// ===================================================================
// C23 Utility Macros and Functions
// ===================================================================

// C23 typeof for safe capability casting
#define BDI_CAPABILITY_CAST_C23(ptr, type) \
    ({ \
        typeof(ptr) _ptr = (ptr); \
        (_ptr && sizeof(*_ptr) >= sizeof(type)) ? (type*)_ptr : NULL; \
    })

// C23 auto for capability iteration
#define BDI_FOR_EACH_CAPABILITY_C23(registry, cap_var) \
    for (auto cap_var = (registry)->capabilities; \
         cap_var < (registry)->capabilities + (registry)->capability_count; \
         ++cap_var) \
        if (*cap_var)

// C23 constexpr validation macros
#define BDI_VALIDATE_CAPABILITY_ID(id) \
    static_assert((id) >= 0 && (id) < BDI_MAX_CAPABILITIES_C23, "Invalid capability ID")

#define BDI_VALIDATE_FEATURE_ID(id) \
    static_assert((id) >= 0 && (id) < BDI_MAX_FEATURES_C23, "Invalid feature ID")

// ===================================================================
// C23 Enhanced Error Handling
// ===================================================================

typedef enum {
    BDI_CAPABILITY_SUCCESS_C23 = 0,
    BDI_CAPABILITY_ERROR_NOT_FOUND_C23,
    BDI_CAPABILITY_ERROR_INVALID_ID_C23,
    BDI_CAPABILITY_ERROR_DETECTION_FAILED_C23,
    BDI_CAPABILITY_ERROR_BENCHMARK_FAILED_C23,
    BDI_CAPABILITY_ERROR_INSUFFICIENT_HARDWARE_C23,
    BDI_CAPABILITY_ERROR_C23_FEATURE_MISSING_C23
} bdi_capability_error_c23_t;

// Thread-local error handling
thread_local bdi_capability_error_c23_t bdi_capability_last_error_c23;
thread_local char bdi_capability_error_message_c23[256];

// Error handling functions
const char* bdi_capability_error_string_c23(bdi_capability_error_c23_t error);
void bdi_capability_set_error_c23(bdi_capability_error_c23_t error, const char* message);
bdi_capability_error_c23_t bdi_capability_get_last_error_c23(void);

#ifdef __cplusplus
}
#endif

