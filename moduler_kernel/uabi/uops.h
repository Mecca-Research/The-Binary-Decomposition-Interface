
// ===================================================================
// μABI v0.1 - Universal Binary Application Interface
// The metal verbs that kernel and VM call for maximum performance
// ===================================================================

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// μABI Operation Table - Hardware-Optimized Primitives
// ===================================================================

typedef struct {
    // === Memory Operations ===
    void (*memcpy_fast)(void* dst, const void* src, size_t n);
    void (*memset_fast)(void* dst, int c, size_t n);
    void (*memmove_fast)(void* dst, const void* src, size_t n);
    
    // === Cryptographic Operations ===
    void (*sha256)(uint8_t* out32, const void* data, size_t n);
    void (*aes_encrypt)(uint8_t* out, const uint8_t* in, const uint8_t* key);
    void (*aes_decrypt)(uint8_t* out, const uint8_t* in, const uint8_t* key);
    
    // === Big Integer Operations (BDI Smart Numbers) ===
    void (*bign_mul)(uint64_t* c, const uint64_t* a, const uint64_t* b, size_t limbs);
    void (*bign_add)(uint64_t* c, const uint64_t* a, const uint64_t* b, size_t limbs);
    void (*bign_sub)(uint64_t* c, const uint64_t* a, const uint64_t* b, size_t limbs);
    void (*bign_mod)(uint64_t* c, const uint64_t* a, const uint64_t* m, size_t limbs);
    
    // === Tensor Operations ===
    void (*tile_gemm_f32)(float* C, const float* A, const float* B,
                          int M, int N, int K, int lda, int ldb, int ldc);
    void (*tile_gemm_f16)(uint16_t* C, const uint16_t* A, const uint16_t* B,
                          int M, int N, int K, int lda, int ldb, int ldc);
    void (*conv2d_f32)(float* out, const float* in, const float* kernel,
                       int batch, int in_h, int in_w, int in_c,
                       int out_h, int out_w, int out_c,
                       int kernel_h, int kernel_w, int stride, int pad);
    
    // === Gather/Scatter (Graph VM Operations) ===
    void (*gather_u32)(uint32_t* dst, const uint32_t* base, const uint32_t* idx, size_t n);
    void (*scatter_u32)(uint32_t* base, const uint32_t* idx, const uint32_t* src, size_t n);
    void (*gather_f32)(float* dst, const float* base, const uint32_t* idx, size_t n);
    void (*scatter_f32)(float* base, const uint32_t* idx, const float* src, size_t n);
    
    // === Memory Fences & Synchronization ===
    void (*lfence)(void);      // Load fence
    void (*sfence)(void);      // Store fence  
    void (*mfence)(void);      // Memory fence
    void (*serialize)(void);   // Full serialization
    
    // === Timing & Random Number Generation ===
    uint64_t (*ticks)(void);   // High-resolution timestamp
    uint64_t (*rng64)(void);   // Hardware RNG if available
    uint32_t (*rng32)(void);   // 32-bit hardware RNG
    
    // === Bit Manipulation ===
    int (*popcount64)(uint64_t x);     // Population count
    int (*clz64)(uint64_t x);          // Count leading zeros
    int (*ctz64)(uint64_t x);          // Count trailing zeros
    uint64_t (*bswap64)(uint64_t x);   // Byte swap
    
    // === Vector Operations ===
    void (*vec_add_f32)(float* c, const float* a, const float* b, size_t n);
    void (*vec_mul_f32)(float* c, const float* a, const float* b, size_t n);
    void (*vec_dot_f32)(float* result, const float* a, const float* b, size_t n);
    void (*vec_norm_f32)(float* result, const float* a, size_t n);
    
} bdi_uops_t;

// Global μABI operation table - initialized by orchestrator
extern bdi_uops_t bdi_uops;

// ===================================================================
// μABI Initialization & Management
// ===================================================================

// Forward declaration of capability structure
struct bdi_caps;

// Initialize μABI with detected capabilities
void bdi_init_uops(const struct bdi_caps* caps);

// Verify μABI operations are working correctly
bool bdi_verify_uops(void);

// Get μABI version information
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* arch;
    const char* features;
} bdi_uabi_version_t;

void bdi_get_uabi_version(bdi_uabi_version_t* version);

// ===================================================================
// Performance Hints & Optimization
// ===================================================================

// Hint for upcoming memory access patterns
typedef enum {
    BDI_HINT_TEMPORAL,      // Data will be reused soon
    BDI_HINT_NON_TEMPORAL,  // Data won't be reused
    BDI_HINT_STREAMING,     // Sequential access pattern
    BDI_HINT_RANDOM         // Random access pattern
} bdi_access_hint_t;

void bdi_prefetch(const void* addr, bdi_access_hint_t hint);

// Cache management
void bdi_cache_flush(void* addr, size_t size);
void bdi_cache_invalidate(void* addr, size_t size);

// ===================================================================
// Error Handling & Diagnostics
// ===================================================================

typedef enum {
    BDI_UOPS_OK = 0,
    BDI_UOPS_ERROR_INVALID_CAPS,
    BDI_UOPS_ERROR_NO_IMPL,
    BDI_UOPS_ERROR_SELF_TEST_FAILED,
    BDI_UOPS_ERROR_UNSUPPORTED_ARCH
} bdi_uops_error_t;

const char* bdi_uops_error_string(bdi_uops_error_t error);

// Get last error from μABI operations
bdi_uops_error_t bdi_get_last_uops_error(void);

#ifdef __cplusplus
}
#endif
