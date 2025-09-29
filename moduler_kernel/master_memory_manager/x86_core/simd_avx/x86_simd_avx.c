
/**
 * @file x86_simd_avx.c
 * @brief x86 SIMD/AVX Instruction Support and Optimization Implementation
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete SIMD/AVX instruction support (SSE/AVX/AVX-512) with optimization
 */

#include "x86_simd_avx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Global SIMD state
static simd_capabilities_t simd_caps;
static bool simd_initialized = false;
static simd_error_t last_error = SIMD_ERROR_NONE;

// Feature names for debugging
static const char* feature_names[] = {
    "MMX", "SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2",
    "AVX", "AVX2", "AVX-512F", "AVX-512BW", "AVX-512CD", "AVX-512DQ",
    "AVX-512VL", "AVX-512VNNI", "AVX-512BF16", "AVX-512FP16"
};

// Error strings
static const char* error_strings[] = {
    "No error",
    "Unsupported feature",
    "Invalid parameter",
    "Alignment error",
    "Buffer too small",
    "Division by zero",
    "Overflow",
    "Underflow"
};

/**
 * @brief Detect SIMD capabilities using CPUID
 */
static void detect_simd_capabilities(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Clear capabilities
    memset(&simd_caps, 0, sizeof(simd_caps));
    
    // Check basic CPUID support
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    if (eax < 1) return;
    
    // Get feature flags (CPUID.01H)
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    // Check MMX
    if (edx & (1 << 23)) {
        simd_caps.features |= SIMD_FEATURE_MMX;
    }
    
    // Check SSE
    if (edx & (1 << 25)) {
        simd_caps.features |= SIMD_FEATURE_SSE;
        simd_caps.num_xmm_registers = 8; // 8 in 32-bit mode, 16 in 64-bit
    }
    
    // Check SSE2
    if (edx & (1 << 26)) {
        simd_caps.features |= SIMD_FEATURE_SSE2;
    }
    
    // Check SSE3
    if (ecx & (1 << 0)) {
        simd_caps.features |= SIMD_FEATURE_SSE3;
    }
    
    // Check SSSE3
    if (ecx & (1 << 9)) {
        simd_caps.features |= SIMD_FEATURE_SSSE3;
    }
    
    // Check SSE4.1
    if (ecx & (1 << 19)) {
        simd_caps.features |= SIMD_FEATURE_SSE4_1;
    }
    
    // Check SSE4.2
    if (ecx & (1 << 20)) {
        simd_caps.features |= SIMD_FEATURE_SSE4_2;
    }
    
    // Check AVX
    if (ecx & (1 << 28)) {
        // Check OS support for AVX (XGETBV)
        uint32_t xcr0_low, xcr0_high;
        __asm__ volatile("xgetbv" : "=a"(xcr0_low), "=d"(xcr0_high) : "c"(0));
        
        if ((xcr0_low & 0x6) == 0x6) { // XMM and YMM state enabled
            simd_caps.features |= SIMD_FEATURE_AVX;
            simd_caps.num_ymm_registers = 16;
            simd_caps.supports_unaligned = true;
        }
    }
    
    // Check extended features (CPUID.07H)
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    // Check AVX2
    if (ebx & (1 << 5)) {
        simd_caps.features |= SIMD_FEATURE_AVX2;
        simd_caps.supports_gather_scatter = true;
    }
    
    // Check AVX-512F
    if (ebx & (1 << 16)) {
        // Check OS support for AVX-512
        uint32_t xcr0_low, xcr0_high;
        __asm__ volatile("xgetbv" : "=a"(xcr0_low), "=d"(xcr0_high) : "c"(0));
        
        if ((xcr0_low & 0xE6) == 0xE6) { // ZMM, Hi16_ZMM, and opmask state enabled
            simd_caps.features |= SIMD_FEATURE_AVX512F;
            simd_caps.num_zmm_registers = 32;
            simd_caps.num_mask_registers = 8;
            simd_caps.supports_masking = true;
            simd_caps.supports_broadcast = true;
        }
    }
    
    // Check other AVX-512 extensions
    if (simd_caps.features & SIMD_FEATURE_AVX512F) {
        if (ebx & (1 << 17)) simd_caps.features |= SIMD_FEATURE_AVX512DQ;
        if (ebx & (1 << 28)) simd_caps.features |= SIMD_FEATURE_AVX512CD;
        if (ebx & (1 << 30)) simd_caps.features |= SIMD_FEATURE_AVX512BW;
        if (ebx & (1 << 31)) simd_caps.features |= SIMD_FEATURE_AVX512VL;
        if (ecx & (1 << 11)) simd_caps.features |= SIMD_FEATURE_AVX512VNNI;
    }
    
    // Determine maximum vector size
    if (simd_caps.features & SIMD_FEATURE_AVX512F) {
        simd_caps.max_vector_size = SIMD_VECTOR_SIZE_512;
    } else if (simd_caps.features & SIMD_FEATURE_AVX) {
        simd_caps.max_vector_size = SIMD_VECTOR_SIZE_256;
    } else if (simd_caps.features & SIMD_FEATURE_SSE) {
        simd_caps.max_vector_size = SIMD_VECTOR_SIZE_128;
    } else if (simd_caps.features & SIMD_FEATURE_MMX) {
        simd_caps.max_vector_size = SIMD_VECTOR_SIZE_64;
    }
    
    // Set number of XMM registers based on architecture
#ifdef __x86_64__
    if (simd_caps.features & SIMD_FEATURE_SSE) {
        simd_caps.num_xmm_registers = 16;
    }
#endif
}

/**
 * @brief Initialize SIMD subsystem
 */
int x86_simd_init(void) {
    if (simd_initialized) {
        return 0;
    }
    
    // Detect capabilities
    detect_simd_capabilities();
    
    // Initialize FPU/SSE control registers
    if (simd_caps.features & SIMD_FEATURE_SSE) {
        // Set MXCSR to default state
        uint32_t mxcsr = 0x1F80; // Mask all exceptions, round to nearest
        __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
    }
    
    simd_initialized = true;
    return 0;
}

/**
 * @brief Get SIMD capabilities
 */
simd_capabilities_t* x86_simd_get_capabilities(void) {
    if (!simd_initialized) {
        x86_simd_init();
    }
    return &simd_caps;
}

/**
 * @brief Check if a SIMD feature is supported
 */
bool x86_simd_is_feature_supported(uint32_t feature) {
    return (simd_caps.features & feature) != 0;
}

/**
 * @brief Get feature name string
 */
const char* x86_simd_get_feature_name(uint32_t feature) {
    for (int i = 0; i < 32; i++) {
        if (feature & (1 << i)) {
            if (i < sizeof(feature_names) / sizeof(feature_names[0])) {
                return feature_names[i];
            }
        }
    }
    return "Unknown";
}

/**
 * @brief Print SIMD capabilities
 */
void x86_simd_print_capabilities(void) {
    printf("SIMD Capabilities:\n");
    printf("  Maximum Vector Size: %d bytes\n", simd_caps.max_vector_size);
    printf("  XMM Registers: %d\n", simd_caps.num_xmm_registers);
    printf("  YMM Registers: %d\n", simd_caps.num_ymm_registers);
    printf("  ZMM Registers: %d\n", simd_caps.num_zmm_registers);
    printf("  Mask Registers: %d\n", simd_caps.num_mask_registers);
    printf("  Unaligned Access: %s\n", simd_caps.supports_unaligned ? "Yes" : "No");
    printf("  Gather/Scatter: %s\n", simd_caps.supports_gather_scatter ? "Yes" : "No");
    printf("  Masking: %s\n", simd_caps.supports_masking ? "Yes" : "No");
    printf("  Broadcast: %s\n", simd_caps.supports_broadcast ? "Yes" : "No");
    
    printf("  Supported Features:\n");
    for (int i = 0; i < 32; i++) {
        if (simd_caps.features & (1 << i)) {
            if (i < sizeof(feature_names) / sizeof(feature_names[0])) {
                printf("    %s\n", feature_names[i]);
            }
        }
    }
}

/**
 * @brief Save SIMD context
 */
int x86_simd_save_context(simd_context_t* context) {
    if (!context) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    // Save MXCSR
    __asm__ volatile("stmxcsr %0" : "=m"(context->mxcsr));
    
    // Save XMM registers
    if (simd_caps.features & SIMD_FEATURE_SSE) {
        for (int i = 0; i < simd_caps.num_xmm_registers; i++) {
            __asm__ volatile("movaps %%xmm%c1, %0" : "=m"(context->xmm[i]) : "i"(i));
        }
        context->sse_state_valid = true;
    }
    
    // Save YMM registers (upper 128 bits)
    if (simd_caps.features & SIMD_FEATURE_AVX) {
        for (int i = 0; i < simd_caps.num_ymm_registers; i++) {
            __asm__ volatile("vextractf128 $1, %%ymm%c1, %0" : "=m"(context->ymm_high[i]) : "i"(i));
        }
        context->avx_state_valid = true;
    }
    
    // Save ZMM registers (upper 256 bits) and mask registers
    if (simd_caps.features & SIMD_FEATURE_AVX512F) {
        for (int i = 0; i < simd_caps.num_zmm_registers; i++) {
            __asm__ volatile("vextractf64x4 $1, %%zmm%c1, %0" : "=m"(context->zmm_high[i]) : "i"(i));
        }
        
        for (int i = 0; i < simd_caps.num_mask_registers; i++) {
            __asm__ volatile("kmovq %%k%c1, %0" : "=m"(context->k_registers[i]) : "i"(i));
        }
        context->avx512_state_valid = true;
    }
    
    return 0;
}

/**
 * @brief Restore SIMD context
 */
int x86_simd_restore_context(const simd_context_t* context) {
    if (!context) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    // Restore MXCSR
    __asm__ volatile("ldmxcsr %0" : : "m"(context->mxcsr));
    
    // Restore XMM registers
    if (context->sse_state_valid && (simd_caps.features & SIMD_FEATURE_SSE)) {
        for (int i = 0; i < simd_caps.num_xmm_registers; i++) {
            __asm__ volatile("movaps %0, %%xmm%c1" : : "m"(context->xmm[i]), "i"(i));
        }
    }
    
    // Restore YMM registers (upper 128 bits)
    if (context->avx_state_valid && (simd_caps.features & SIMD_FEATURE_AVX)) {
        for (int i = 0; i < simd_caps.num_ymm_registers; i++) {
            __asm__ volatile("vinsertf128 $1, %0, %%ymm%c1, %%ymm%c1" : : "m"(context->ymm_high[i]), "i"(i));
        }
    }
    
    // Restore ZMM registers (upper 256 bits) and mask registers
    if (context->avx512_state_valid && (simd_caps.features & SIMD_FEATURE_AVX512F)) {
        for (int i = 0; i < simd_caps.num_zmm_registers; i++) {
            __asm__ volatile("vinsertf64x4 $1, %0, %%zmm%c1, %%zmm%c1" : : "m"(context->zmm_high[i]), "i"(i));
        }
        
        for (int i = 0; i < simd_caps.num_mask_registers; i++) {
            __asm__ volatile("kmovq %0, %%k%c1" : : "m"(context->k_registers[i]), "i"(i));
        }
    }
    
    return 0;
}

/**
 * @brief SSE memory copy
 */
int x86_sse_copy_memory(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (!(simd_caps.features & SIMD_FEATURE_SSE)) {
        last_error = SIMD_ERROR_UNSUPPORTED_FEATURE;
        return -1;
    }
    
    const uint8_t* src_ptr = (const uint8_t*)src;
    uint8_t* dst_ptr = (uint8_t*)dst;
    size_t remaining = size;
    
    // Check alignment
    bool src_aligned = ((uintptr_t)src_ptr & 15) == 0;
    bool dst_aligned = ((uintptr_t)dst_ptr & 15) == 0;
    
    // Process 16-byte chunks
    while (remaining >= 16) {
        __m128 data;
        
        if (src_aligned) {
            data = _mm_load_ps((const float*)src_ptr);
        } else {
            data = _mm_loadu_ps((const float*)src_ptr);
        }
        
        if (dst_aligned) {
            _mm_store_ps((float*)dst_ptr, data);
        } else {
            _mm_storeu_ps((float*)dst_ptr, data);
        }
        
        src_ptr += 16;
        dst_ptr += 16;
        remaining -= 16;
    }
    
    // Handle remaining bytes
    while (remaining > 0) {
        *dst_ptr++ = *src_ptr++;
        remaining--;
    }
    
    return 0;
}

/**
 * @brief SSE float array addition
 */
int x86_sse_add_float_arrays(float* result, const float* a, const float* b, size_t count) {
    if (!result || !a || !b || count == 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (!(simd_caps.features & SIMD_FEATURE_SSE)) {
        last_error = SIMD_ERROR_UNSUPPORTED_FEATURE;
        return -1;
    }
    
    size_t simd_count = count & ~3; // Process 4 floats at a time
    size_t i;
    
    // SIMD processing
    for (i = 0; i < simd_count; i += 4) {
        __m128 va = _mm_loadu_ps(&a[i]);
        __m128 vb = _mm_loadu_ps(&b[i]);
        __m128 vresult = _mm_add_ps(va, vb);
        _mm_storeu_ps(&result[i], vresult);
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
    
    return 0;
}

/**
 * @brief AVX float array addition
 */
int x86_avx_add_float_arrays(float* result, const float* a, const float* b, size_t count) {
    if (!result || !a || !b || count == 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (!(simd_caps.features & SIMD_FEATURE_AVX)) {
        last_error = SIMD_ERROR_UNSUPPORTED_FEATURE;
        return -1;
    }
    
    size_t simd_count = count & ~7; // Process 8 floats at a time
    size_t i;
    
    // SIMD processing
    for (i = 0; i < simd_count; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vresult = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&result[i], vresult);
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
    
    return 0;
}

/**
 * @brief AVX-512 float array addition
 */
int x86_avx512_add_float_arrays(float* result, const float* a, const float* b, size_t count) {
    if (!result || !a || !b || count == 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (!(simd_caps.features & SIMD_FEATURE_AVX512F)) {
        last_error = SIMD_ERROR_UNSUPPORTED_FEATURE;
        return -1;
    }
    
#ifdef __AVX512F__
    size_t simd_count = count & ~15; // Process 16 floats at a time
    size_t i;
    
    // SIMD processing
    for (i = 0; i < simd_count; i += 16) {
        __m512 va = _mm512_loadu_ps(&a[i]);
        __m512 vb = _mm512_loadu_ps(&b[i]);
        __m512 vresult = _mm512_add_ps(va, vb);
        _mm512_storeu_ps(&result[i], vresult);
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
    
    return 0;
#else
    // Fallback to AVX if AVX-512 not available at compile time
    return x86_avx_add_float_arrays(result, a, b, count);
#endif
}

/**
 * @brief SIMD square root
 */
int x86_simd_sqrt_float(float* result, const float* input, size_t count) {
    if (!result || !input || count == 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (simd_caps.features & SIMD_FEATURE_AVX512F) {
#ifdef __AVX512F__
        size_t simd_count = count & ~15;
        size_t i;
        
        for (i = 0; i < simd_count; i += 16) {
            __m512 vinput = _mm512_loadu_ps(&input[i]);
            __m512 vresult = _mm512_sqrt_ps(vinput);
            _mm512_storeu_ps(&result[i], vresult);
        }
        
        for (; i < count; i++) {
            result[i] = sqrtf(input[i]);
        }
#endif
    } else if (simd_caps.features & SIMD_FEATURE_AVX) {
        size_t simd_count = count & ~7;
        size_t i;
        
        for (i = 0; i < simd_count; i += 8) {
            __m256 vinput = _mm256_loadu_ps(&input[i]);
            __m256 vresult = _mm256_sqrt_ps(vinput);
            _mm256_storeu_ps(&result[i], vresult);
        }
        
        for (; i < count; i++) {
            result[i] = sqrtf(input[i]);
        }
    } else if (simd_caps.features & SIMD_FEATURE_SSE) {
        size_t simd_count = count & ~3;
        size_t i;
        
        for (i = 0; i < simd_count; i += 4) {
            __m128 vinput = _mm_loadu_ps(&input[i]);
            __m128 vresult = _mm_sqrt_ps(vinput);
            _mm_storeu_ps(&result[i], vresult);
        }
        
        for (; i < count; i++) {
            result[i] = sqrtf(input[i]);
        }
    } else {
        // Scalar fallback
        for (size_t i = 0; i < count; i++) {
            result[i] = sqrtf(input[i]);
        }
    }
    
    return 0;
}

/**
 * @brief Aligned memory allocation
 */
void* x86_simd_aligned_alloc(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        last_error = SIMD_ERROR_INVALID_PARAMETER;
        return NULL;
    }
    
    void* ptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief Free aligned memory
 */
void x86_simd_aligned_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/**
 * @brief Check memory alignment
 */
bool x86_simd_is_aligned(const void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

/**
 * @brief Get error string
 */
const char* x86_simd_get_error_string(simd_error_t error) {
    if (error < sizeof(error_strings) / sizeof(error_strings[0])) {
        return error_strings[error];
    }
    return "Unknown error";
}

/**
 * @brief Get last error
 */
simd_error_t x86_simd_get_last_error(void) {
    return last_error;
}

/**
 * @brief Clear error state
 */
void x86_simd_clear_error(void) {
    last_error = SIMD_ERROR_NONE;
}

/**
 * @brief Print XMM registers for debugging
 */
void x86_simd_dump_xmm_registers(void) {
    if (!(simd_caps.features & SIMD_FEATURE_SSE)) {
        printf("SSE not supported\n");
        return;
    }
    
    printf("XMM Registers:\n");
    for (int i = 0; i < simd_caps.num_xmm_registers; i++) {
        float values[4];
        __asm__ volatile("movups %%xmm%c1, %0" : "=m"(values) : "i"(i));
        printf("  XMM%d: [%f, %f, %f, %f]\n", i, values[0], values[1], values[2], values[3]);
    }
}

/**
 * @brief Print vector for debugging
 */
void x86_simd_print_vector_float(__m128 vec, const char* name) {
    float values[4];
    _mm_storeu_ps(values, vec);
    printf("%s: [%f, %f, %f, %f]\n", name, values[0], values[1], values[2], values[3]);
}
