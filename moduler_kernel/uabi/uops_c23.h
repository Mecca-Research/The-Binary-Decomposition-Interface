
// ===================================================================
// BDI μABI Operations - C23 Enhanced
// Enhanced micro-operations with C23 features
// ===================================================================

#pragma once

#include "uops.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// C23 Enhanced μABI Types
// ===================================================================

// C23 _BitInt for precise operation identifiers
typedef _BitInt(32) bdi_uop_id_t;
typedef _BitInt(64) bdi_uop_flags_t;

// C23 _Decimal for precise timing measurements
typedef _Decimal64 bdi_uop_latency_t;
typedef _Decimal32 bdi_uop_throughput_t;

// C23 constexpr operation limits
constexpr size_t BDI_MAX_UOP_PARAMS = 16;
constexpr size_t BDI_MAX_UOP_RESULTS = 8;
constexpr bdi_uop_latency_t BDI_MAX_LATENCY_NS = 1000000.0dd; // 1ms max

// ===================================================================
// C23 Enhanced Operation Descriptor
// ===================================================================

typedef struct {
    bdi_uop_id_t operation_id;          // Unique operation ID
    char name[64];                      // Operation name
    bdi_uop_flags_t flags;              // Operation flags using _BitInt
    
    // Performance characteristics with C23 _Decimal precision
    bdi_uop_latency_t avg_latency_ns;   // Average latency
    bdi_uop_latency_t min_latency_ns;   // Minimum latency
    bdi_uop_latency_t max_latency_ns;   // Maximum latency
    bdi_uop_throughput_t throughput_ops_per_us; // Throughput
    
    // C23 typeof for flexible parameter handling
    union {
        void* generic_params[BDI_MAX_UOP_PARAMS];
        size_t size_params[BDI_MAX_UOP_PARAMS];
        float float_params[BDI_MAX_UOP_PARAMS];
        _BitInt(64) int_params[BDI_MAX_UOP_PARAMS];
    } parameters;
    
    // Thread-local execution context
    thread_local uint64_t thread_call_count;
    thread_local bdi_uop_latency_t thread_avg_latency;
    
} bdi_uop_descriptor_c23_t;

// ===================================================================
// C23 Enhanced Memory Operations
// ===================================================================

// C23 auto return type for memory operations
auto bdi_memcpy_c23(void* restrict dst, const void* restrict src, size_t n) -> void*;
auto bdi_memset_c23(void* dst, int c, size_t n) -> void*;
auto bdi_memmove_c23(void* dst, const void* src, size_t n) -> void*;

// C23 typeof for type-safe memory operations
#define BDI_MEMCPY_TYPED(dst, src, count) \
    ({ \
        typeof(dst) _dst = (dst); \
        typeof(src) _src = (src); \
        static_assert(sizeof(*_dst) == sizeof(*_src), "Type size mismatch"); \
        bdi_memcpy_c23(_dst, _src, (count) * sizeof(*_dst)); \
    })

// C23 constexpr validation for memory operations
constexpr bool bdi_is_valid_memory_size_c23(size_t size) {
    return size > 0 && size <= (1ULL << 48); // Max 256TB
}

constexpr bool bdi_is_power_of_two(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Memory operations with C23 alignment hints
void bdi_memcpy_aligned_c23(void* restrict dst, const void* restrict src, size_t n)
    __attribute__((assume_aligned(64, 1), assume_aligned(64, 2)));

void bdi_memset_aligned_c23(void* dst, int c, size_t n)
    __attribute__((assume_aligned(64, 1)));

// ===================================================================
// C23 Enhanced Arithmetic Operations
// ===================================================================

// High-precision arithmetic using C23 _Decimal
_Decimal128 bdi_add_decimal128(_Decimal128 a, _Decimal128 b);
_Decimal128 bdi_mul_decimal128(_Decimal128 a, _Decimal128 b);
_Decimal128 bdi_div_decimal128(_Decimal128 a, _Decimal128 b);

// C23 _BitInt arithmetic operations
_BitInt(128) bdi_add_bitint128(_BitInt(128) a, _BitInt(128) b);
_BitInt(128) bdi_mul_bitint128(_BitInt(128) a, _BitInt(128) b);
_BitInt(128) bdi_div_bitint128(_BitInt(128) a, _BitInt(128) b);

// C23 _Generic for type-safe arithmetic
#define bdi_add_generic(a, b) _Generic((a), \
    int: bdi_add_int, \
    float: bdi_add_float, \
    double: bdi_add_double, \
    _Decimal64: bdi_add_decimal64, \
    _Decimal128: bdi_add_decimal128, \
    _BitInt(64): bdi_add_bitint64, \
    _BitInt(128): bdi_add_bitint128, \
    default: bdi_add_generic_fallback \
)(a, b)

// ===================================================================
// C23 Enhanced Vector Operations
// ===================================================================

// Vector operations with C23 alignment and SIMD hints
typedef struct {
    alignas(64) float data[16];         // 64-byte aligned for AVX-512
    size_t length;
    bdi_uop_flags_t flags;
} bdi_vector_f32_c23_t;

typedef struct {
    alignas(64) double data[8];         // 64-byte aligned for AVX-512
    size_t length;
    bdi_uop_flags_t flags;
} bdi_vector_f64_c23_t;

// C23 auto vector operations
auto bdi_vector_add_f32_c23(const bdi_vector_f32_c23_t* a, 
                           const bdi_vector_f32_c23_t* b) -> bdi_vector_f32_c23_t*;

auto bdi_vector_mul_f32_c23(const bdi_vector_f32_c23_t* a, 
                           const bdi_vector_f32_c23_t* b) -> bdi_vector_f32_c23_t*;

auto bdi_vector_dot_f32_c23(const bdi_vector_f32_c23_t* a, 
                           const bdi_vector_f32_c23_t* b) -> float;

// C23 constexpr vector validation
constexpr bool bdi_is_valid_vector_length(size_t length) {
    return length > 0 && length <= 1024 && bdi_is_power_of_two(length);
}

// Vector operations with C23 SIMD optimization hints
void bdi_vector_add_simd_c23(const float* restrict a, const float* restrict b, 
                            float* restrict result, size_t length)
    __attribute__((assume_aligned(64, 1), assume_aligned(64, 2), assume_aligned(64, 3)));

// ===================================================================
// C23 Enhanced String Operations
// ===================================================================

// String operations with C23 features
auto bdi_strlen_c23(const char* str) -> size_t;
auto bdi_strcpy_c23(char* restrict dst, const char* restrict src) -> char*;
auto bdi_strncpy_c23(char* restrict dst, const char* restrict src, size_t n) -> char*;

// C23 typeof for safe string operations
#define BDI_STRCPY_SAFE(dst, src) \
    ({ \
        typeof(dst) _dst = (dst); \
        typeof(src) _src = (src); \
        static_assert(sizeof(_dst) >= sizeof(_src), "Destination too small"); \
        bdi_strcpy_c23(_dst, _src); \
    })

// String hash using C23 _BitInt
_BitInt(64) bdi_string_hash_c23(const char* str);
_BitInt(128) bdi_string_hash_128_c23(const char* str);

// ===================================================================
// C23 Enhanced Bit Operations
// ===================================================================

// Bit manipulation using C23 _BitInt
_BitInt(64) bdi_bit_set_c23(_BitInt(64) value, unsigned int bit);
_BitInt(64) bdi_bit_clear_c23(_BitInt(64) value, unsigned int bit);
_BitInt(64) bdi_bit_toggle_c23(_BitInt(64) value, unsigned int bit);
bool bdi_bit_test_c23(_BitInt(64) value, unsigned int bit);

// Population count for C23 _BitInt
unsigned int bdi_popcount_bitint64(_BitInt(64) value);
unsigned int bdi_popcount_bitint128(_BitInt(128) value);

// Leading/trailing zero count
unsigned int bdi_clz_bitint64(_BitInt(64) value);
unsigned int bdi_ctz_bitint64(_BitInt(64) value);

// ===================================================================
// C23 Enhanced Atomic Operations
// ===================================================================

// Atomic operations with C23 _BitInt
_Atomic _BitInt(64) bdi_atomic_bitint64_t;
_Atomic _BitInt(128) bdi_atomic_bitint128_t;

// Atomic arithmetic on _BitInt
_BitInt(64) bdi_atomic_add_bitint64(_Atomic _BitInt(64)* ptr, _BitInt(64) value);
_BitInt(64) bdi_atomic_sub_bitint64(_Atomic _BitInt(64)* ptr, _BitInt(64) value);
_BitInt(64) bdi_atomic_and_bitint64(_Atomic _BitInt(64)* ptr, _BitInt(64) value);
_BitInt(64) bdi_atomic_or_bitint64(_Atomic _BitInt(64)* ptr, _BitInt(64) value);

// Compare-and-swap for _BitInt
bool bdi_atomic_cas_bitint64(_Atomic _BitInt(64)* ptr, _BitInt(64)* expected, _BitInt(64) desired);

// ===================================================================
// C23 Enhanced Performance Monitoring
// ===================================================================

typedef struct {
    // Operation statistics with C23 _Decimal precision
    _Decimal64 total_execution_time_us;
    _Decimal64 avg_latency_ns;
    _Decimal64 min_latency_ns;
    _Decimal64 max_latency_ns;
    _Decimal32 throughput_ops_per_us;
    
    // Call statistics using C23 _BitInt
    _BitInt(64) total_calls;
    _BitInt(64) successful_calls;
    _BitInt(64) failed_calls;
    
    // Thread-local performance data
    thread_local _Decimal64 thread_execution_time;
    thread_local _BitInt(32) thread_call_count;
    
    // Cache performance
    uint64_t cache_hits;
    uint64_t cache_misses;
    float cache_hit_rate;
    
} bdi_uop_performance_c23_t;

// Performance monitoring functions
void bdi_uop_perf_start_c23(bdi_uop_id_t op_id);
void bdi_uop_perf_end_c23(bdi_uop_id_t op_id);
void bdi_uop_perf_get_stats_c23(bdi_uop_id_t op_id, bdi_uop_performance_c23_t* stats);
void bdi_uop_perf_reset_c23(bdi_uop_id_t op_id);

// ===================================================================
// C23 Enhanced Operation Registration
// ===================================================================

// Operation registration with C23 features
typedef struct {
    bdi_uop_descriptor_c23_t descriptor;
    void* (*implementation)(void* params);
    bool (*validate_params)(const void* params);
    void (*optimize_for_hardware)(void);
    
    // C23 constexpr validation function
    bool (*constexpr_validate)(void);
    
} bdi_uop_registration_c23_t;

// Registration functions
bool bdi_register_uop_c23(const bdi_uop_registration_c23_t* registration);
bool bdi_unregister_uop_c23(bdi_uop_id_t op_id);
bdi_uop_descriptor_c23_t* bdi_find_uop_c23(bdi_uop_id_t op_id);

// ===================================================================
// C23 Enhanced Execution Context
// ===================================================================

typedef struct {
    // Execution state
    bdi_uop_id_t current_operation;
    bdi_uop_flags_t execution_flags;
    
    // Performance tracking with C23 precision
    _Decimal64 execution_start_time;
    _Decimal64 accumulated_time;
    
    // Thread-local context
    thread_local bool context_initialized;
    thread_local _BitInt(32) nested_call_depth;
    
    // Error handling
    int last_error_code;
    char last_error_message[256];
    
} bdi_uop_context_c23_t;

// Context management
bdi_uop_context_c23_t* bdi_uop_get_context_c23(void);
void bdi_uop_init_context_c23(bdi_uop_context_c23_t* ctx);
void bdi_uop_cleanup_context_c23(bdi_uop_context_c23_t* ctx);

// ===================================================================
// C23 Utility Macros
// ===================================================================

// C23 auto for operation execution
#define BDI_EXECUTE_UOP_C23(op_id, params) \
    ({ \
        auto _ctx = bdi_uop_get_context_c23(); \
        auto _op = bdi_find_uop_c23(op_id); \
        auto _result = _op ? _op->implementation(params) : NULL; \
        _result; \
    })

// C23 typeof for parameter validation
#define BDI_VALIDATE_UOP_PARAMS(op_id, params) \
    ({ \
        auto _op = bdi_find_uop_c23(op_id); \
        typeof(params) _params = (params); \
        _op && _op->validate_params(_params); \
    })

// C23 constexpr validation
#define BDI_STATIC_VALIDATE_UOP(op_id) \
    static_assert(op_id > 0 && op_id < (1 << 31), "Invalid operation ID")

#ifdef __cplusplus
}
#endif

