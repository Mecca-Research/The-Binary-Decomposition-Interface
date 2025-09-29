
/**
 * @file x86_simd_avx.h
 * @brief x86 SIMD/AVX Instruction Support and Optimization
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete SIMD/AVX instruction support (SSE/AVX/AVX-512) with optimization
 */

#ifndef X86_SIMD_AVX_H
#define X86_SIMD_AVX_H

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// SIMD Feature Detection Flags
#define SIMD_FEATURE_MMX        (1 << 0)
#define SIMD_FEATURE_SSE        (1 << 1)
#define SIMD_FEATURE_SSE2       (1 << 2)
#define SIMD_FEATURE_SSE3       (1 << 3)
#define SIMD_FEATURE_SSSE3      (1 << 4)
#define SIMD_FEATURE_SSE4_1     (1 << 5)
#define SIMD_FEATURE_SSE4_2     (1 << 6)
#define SIMD_FEATURE_AVX        (1 << 7)
#define SIMD_FEATURE_AVX2       (1 << 8)
#define SIMD_FEATURE_AVX512F    (1 << 9)
#define SIMD_FEATURE_AVX512BW   (1 << 10)
#define SIMD_FEATURE_AVX512CD   (1 << 11)
#define SIMD_FEATURE_AVX512DQ   (1 << 12)
#define SIMD_FEATURE_AVX512VL   (1 << 13)
#define SIMD_FEATURE_AVX512VNNI (1 << 14)
#define SIMD_FEATURE_AVX512BF16 (1 << 15)
#define SIMD_FEATURE_AVX512FP16 (1 << 16)

// Vector Sizes
#define SIMD_VECTOR_SIZE_64     8    // MMX
#define SIMD_VECTOR_SIZE_128    16   // SSE
#define SIMD_VECTOR_SIZE_256    32   // AVX
#define SIMD_VECTOR_SIZE_512    64   // AVX-512

// Alignment Requirements
#define SIMD_ALIGN_16   __attribute__((aligned(16)))
#define SIMD_ALIGN_32   __attribute__((aligned(32)))
#define SIMD_ALIGN_64   __attribute__((aligned(64)))

/**
 * @brief SIMD Capability Structure
 */
typedef struct {
    uint32_t features;              // Feature flags
    uint8_t max_vector_size;        // Maximum vector size in bytes
    bool supports_unaligned;        // Supports unaligned memory access
    bool supports_gather_scatter;   // Supports gather/scatter operations
    bool supports_masking;          // Supports masked operations
    bool supports_broadcast;        // Supports broadcast operations
    uint8_t num_xmm_registers;      // Number of XMM registers
    uint8_t num_ymm_registers;      // Number of YMM registers
    uint8_t num_zmm_registers;      // Number of ZMM registers
    uint8_t num_mask_registers;     // Number of mask registers (AVX-512)
} simd_capabilities_t;

/**
 * @brief SIMD Context Structure (for context switching)
 */
typedef struct {
    // XMM registers (128-bit)
    __m128 xmm[16];
    
    // YMM registers (256-bit) - upper 128 bits
    __m128 ymm_high[16];
    
    // ZMM registers (512-bit) - upper 256 bits
    __m256 zmm_high[32];
    
    // AVX-512 mask registers
    uint64_t k_registers[8];
    
    // Control registers
    uint32_t mxcsr;         // SSE control and status register
    uint16_t fpu_control;   // FPU control word
    uint16_t fpu_status;    // FPU status word
    uint16_t fpu_tag;       // FPU tag word
    
    // State flags
    bool fpu_state_valid;
    bool sse_state_valid;
    bool avx_state_valid;
    bool avx512_state_valid;
    
} simd_context_t;

/**
 * @brief SIMD Memory Operation Parameters
 */
typedef struct {
    void* src;              // Source pointer
    void* dst;              // Destination pointer
    size_t count;           // Number of elements
    size_t element_size;    // Size of each element
    bool aligned;           // Memory is aligned
    bool temporal;          // Use temporal hints
} simd_mem_params_t;

/**
 * @brief SIMD Math Operation Parameters
 */
typedef struct {
    void* operand1;         // First operand
    void* operand2;         // Second operand (if applicable)
    void* result;           // Result buffer
    size_t count;           // Number of elements
    uint8_t data_type;      // Data type (float, double, int32, etc.)
    bool saturate;          // Use saturation arithmetic
} simd_math_params_t;

// Core SIMD Detection and Initialization
int x86_simd_init(void);
simd_capabilities_t* x86_simd_get_capabilities(void);
bool x86_simd_is_feature_supported(uint32_t feature);
const char* x86_simd_get_feature_name(uint32_t feature);
void x86_simd_print_capabilities(void);

// SIMD Context Management
int x86_simd_save_context(simd_context_t* context);
int x86_simd_restore_context(const simd_context_t* context);
int x86_simd_init_context(simd_context_t* context);
void x86_simd_clear_context(simd_context_t* context);

// SSE Operations
int x86_sse_copy_memory(void* dst, const void* src, size_t size);
int x86_sse_set_memory(void* dst, uint8_t value, size_t size);
int x86_sse_compare_memory(const void* ptr1, const void* ptr2, size_t size);
int x86_sse_add_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_sse_multiply_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_sse_dot_product_float(const float* a, const float* b, size_t count, float* result);

// AVX Operations
int x86_avx_copy_memory(void* dst, const void* src, size_t size);
int x86_avx_set_memory(void* dst, uint8_t value, size_t size);
int x86_avx_add_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_avx_multiply_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_avx_fma_float_arrays(float* result, const float* a, const float* b, const float* c, size_t count);
int x86_avx_matrix_multiply_float(const float* a, const float* b, float* c, 
                                  size_t rows_a, size_t cols_a, size_t cols_b);

// AVX-512 Operations
int x86_avx512_copy_memory(void* dst, const void* src, size_t size);
int x86_avx512_set_memory(void* dst, uint8_t value, size_t size);
int x86_avx512_add_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_avx512_multiply_float_arrays(float* result, const float* a, const float* b, size_t count);
int x86_avx512_fma_float_arrays(float* result, const float* a, const float* b, const float* c, size_t count);
int x86_avx512_gather_float(float* result, const float* base, const int32_t* indices, size_t count);
int x86_avx512_scatter_float(float* base, const int32_t* indices, const float* values, size_t count);

// Masked Operations (AVX-512)
int x86_avx512_masked_add_float(float* result, const float* a, const float* b, 
                                const uint8_t* mask, size_t count);
int x86_avx512_masked_multiply_float(float* result, const float* a, const float* b, 
                                    const uint8_t* mask, size_t count);
int x86_avx512_compress_float(float* result, const float* src, const uint8_t* mask, 
                             size_t count, size_t* result_count);
int x86_avx512_expand_float(float* result, const float* src, const uint8_t* mask, size_t count);

// Specialized Math Functions
int x86_simd_sin_float(float* result, const float* input, size_t count);
int x86_simd_cos_float(float* result, const float* input, size_t count);
int x86_simd_exp_float(float* result, const float* input, size_t count);
int x86_simd_log_float(float* result, const float* input, size_t count);
int x86_simd_sqrt_float(float* result, const float* input, size_t count);
int x86_simd_rsqrt_float(float* result, const float* input, size_t count);

// String and Memory Operations
int x86_simd_strlen(const char* str, size_t* length);
int x86_simd_strcmp(const char* str1, const char* str2, int* result);
int x86_simd_strcpy(char* dst, const char* src, size_t max_len);
int x86_simd_memchr(const void* ptr, int value, size_t size, void** result);
int x86_simd_memcmp(const void* ptr1, const void* ptr2, size_t size, int* result);

// Cryptographic Operations
int x86_simd_aes_encrypt_block(const uint8_t* plaintext, const uint8_t* key, 
                              uint8_t* ciphertext);
int x86_simd_aes_decrypt_block(const uint8_t* ciphertext, const uint8_t* key, 
                              uint8_t* plaintext);
int x86_simd_sha256_hash(const uint8_t* data, size_t length, uint8_t* hash);
int x86_simd_crc32_compute(const uint8_t* data, size_t length, uint32_t* crc);

// Performance Optimization
int x86_simd_prefetch_data(const void* addr, int locality);
int x86_simd_stream_store(void* dst, const void* src, size_t size);
int x86_simd_non_temporal_copy(void* dst, const void* src, size_t size);
void x86_simd_memory_fence(void);
void x86_simd_load_fence(void);
void x86_simd_store_fence(void);

// Data Type Conversions
int x86_simd_convert_float_to_int32(int32_t* result, const float* input, size_t count);
int x86_simd_convert_int32_to_float(float* result, const int32_t* input, size_t count);
int x86_simd_convert_double_to_float(float* result, const double* input, size_t count);
int x86_simd_convert_float_to_double(double* result, const float* input, size_t count);
int x86_simd_pack_int32_to_int16(int16_t* result, const int32_t* input, size_t count);
int x86_simd_unpack_int16_to_int32(int32_t* result, const int16_t* input, size_t count);

// Vector Reduction Operations
int x86_simd_sum_float(const float* input, size_t count, float* result);
int x86_simd_min_float(const float* input, size_t count, float* result);
int x86_simd_max_float(const float* input, size_t count, float* result);
int x86_simd_sum_int32(const int32_t* input, size_t count, int64_t* result);
int x86_simd_min_int32(const int32_t* input, size_t count, int32_t* result);
int x86_simd_max_int32(const int32_t* input, size_t count, int32_t* result);

// Bit Manipulation
int x86_simd_popcount(const uint64_t* input, size_t count, uint32_t* result);
int x86_simd_leading_zeros(const uint32_t* input, size_t count, uint32_t* result);
int x86_simd_trailing_zeros(const uint32_t* input, size_t count, uint32_t* result);
int x86_simd_bit_reverse(const uint32_t* input, size_t count, uint32_t* result);

// Neural Network Primitives
int x86_simd_relu_float(float* result, const float* input, size_t count);
int x86_simd_sigmoid_float(float* result, const float* input, size_t count);
int x86_simd_tanh_float(float* result, const float* input, size_t count);
int x86_simd_softmax_float(float* result, const float* input, size_t count);
int x86_simd_convolution_2d(const float* input, const float* kernel, float* output,
                           size_t input_height, size_t input_width, size_t input_channels,
                           size_t kernel_height, size_t kernel_width, size_t output_channels);

// Memory Alignment Utilities
void* x86_simd_aligned_alloc(size_t size, size_t alignment);
void x86_simd_aligned_free(void* ptr);
bool x86_simd_is_aligned(const void* ptr, size_t alignment);
size_t x86_simd_get_alignment_offset(const void* ptr, size_t alignment);

// Performance Measurement
typedef struct {
    uint64_t cycles_start;
    uint64_t cycles_end;
    uint64_t instructions_start;
    uint64_t instructions_end;
    double throughput_gbps;
    double latency_ns;
} simd_perf_metrics_t;

int x86_simd_perf_start(simd_perf_metrics_t* metrics);
int x86_simd_perf_end(simd_perf_metrics_t* metrics, size_t bytes_processed);
void x86_simd_perf_print(const simd_perf_metrics_t* metrics);

// Debugging and Diagnostics
void x86_simd_dump_xmm_registers(void);
void x86_simd_dump_ymm_registers(void);
void x86_simd_dump_zmm_registers(void);
void x86_simd_dump_mask_registers(void);
void x86_simd_print_vector_float(__m128 vec, const char* name);
void x86_simd_print_vector_int32(__m128i vec, const char* name);

// Error Handling
typedef enum {
    SIMD_ERROR_NONE = 0,
    SIMD_ERROR_UNSUPPORTED_FEATURE,
    SIMD_ERROR_INVALID_PARAMETER,
    SIMD_ERROR_ALIGNMENT_ERROR,
    SIMD_ERROR_BUFFER_TOO_SMALL,
    SIMD_ERROR_DIVISION_BY_ZERO,
    SIMD_ERROR_OVERFLOW,
    SIMD_ERROR_UNDERFLOW
} simd_error_t;

const char* x86_simd_get_error_string(simd_error_t error);
simd_error_t x86_simd_get_last_error(void);
void x86_simd_clear_error(void);

// Compiler Intrinsic Wrappers (for portability)
#ifdef __AVX512F__
#define SIMD_LOAD_512(ptr)          _mm512_load_ps(ptr)
#define SIMD_STORE_512(ptr, vec)    _mm512_store_ps(ptr, vec)
#define SIMD_ADD_512(a, b)          _mm512_add_ps(a, b)
#define SIMD_MUL_512(a, b)          _mm512_mul_ps(a, b)
#define SIMD_FMA_512(a, b, c)       _mm512_fmadd_ps(a, b, c)
#endif

#ifdef __AVX2__
#define SIMD_LOAD_256(ptr)          _mm256_load_ps(ptr)
#define SIMD_STORE_256(ptr, vec)    _mm256_store_ps(ptr, vec)
#define SIMD_ADD_256(a, b)          _mm256_add_ps(a, b)
#define SIMD_MUL_256(a, b)          _mm256_mul_ps(a, b)
#define SIMD_FMA_256(a, b, c)       _mm256_fmadd_ps(a, b, c)
#endif

#ifdef __SSE__
#define SIMD_LOAD_128(ptr)          _mm_load_ps(ptr)
#define SIMD_STORE_128(ptr, vec)    _mm_store_ps(ptr, vec)
#define SIMD_ADD_128(a, b)          _mm_add_ps(a, b)
#define SIMD_MUL_128(a, b)          _mm_mul_ps(a, b)
#endif

#ifdef __cplusplus
}
#endif

#endif // X86_SIMD_AVX_H
