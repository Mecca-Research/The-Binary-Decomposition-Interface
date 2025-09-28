
// ===================================================================
// x86 μABI Implementation - AVX2/AVX-512 Optimized Operations
// Assembly islands pattern for maximum performance
// ===================================================================

#include "../../uabi/uops.h"
#include "../../capgraph/capability.h"
#include <immintrin.h>
#include <string.h>
#include <cpuid.h>

// ===================================================================
// Scalar Fallback Implementations
// ===================================================================

static void memcpy_scalar(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    
    // Handle unaligned start
    while (n > 0 && ((uintptr_t)d & 7)) {
        *d++ = *s++;
        n--;
    }
    
    // 8-byte aligned copy
    while (n >= 8) {
        *(uint64_t*)d = *(const uint64_t*)s;
        d += 8;
        s += 8;
        n -= 8;
    }
    
    // Handle remaining bytes
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
}

static void memset_scalar(void* dst, int c, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    uint8_t val = (uint8_t)c;
    
    // Create 8-byte pattern
    uint64_t pattern = 0x0101010101010101ULL * val;
    
    // Handle unaligned start
    while (n > 0 && ((uintptr_t)d & 7)) {
        *d++ = val;
        n--;
    }
    
    // 8-byte aligned set
    while (n >= 8) {
        *(uint64_t*)d = pattern;
        d += 8;
        n -= 8;
    }
    
    // Handle remaining bytes
    while (n > 0) {
        *d++ = val;
        n--;
    }
}

// ===================================================================
// AVX2 Optimized Implementations
// ===================================================================

static void memcpy_avx2(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    
    // Small copies use scalar
    if (n < 32) {
        memcpy_scalar(dst, src, n);
        return;
    }
    
    // Handle unaligned start (up to 32 bytes)
    size_t align_offset = 32 - ((uintptr_t)d & 31);
    if (align_offset < 32) {
        memcpy_scalar(d, s, align_offset);
        d += align_offset;
        s += align_offset;
        n -= align_offset;
    }
    
    // 32-byte aligned AVX2 copy with prefetching
    while (n >= 128) {
        // Prefetch next cache lines
        _mm_prefetch((const char*)(s + 64), _MM_HINT_T0);
        _mm_prefetch((const char*)(s + 128), _MM_HINT_T0);
        
        // Load 4x 32-byte chunks
        __m256i v0 = _mm256_load_si256((const __m256i*)(s + 0));
        __m256i v1 = _mm256_load_si256((const __m256i*)(s + 32));
        __m256i v2 = _mm256_load_si256((const __m256i*)(s + 64));
        __m256i v3 = _mm256_load_si256((const __m256i*)(s + 96));
        
        // Store 4x 32-byte chunks
        _mm256_store_si256((__m256i*)(d + 0), v0);
        _mm256_store_si256((__m256i*)(d + 32), v1);
        _mm256_store_si256((__m256i*)(d + 64), v2);
        _mm256_store_si256((__m256i*)(d + 96), v3);
        
        d += 128;
        s += 128;
        n -= 128;
    }
    
    // Handle remaining 32-byte chunks
    while (n >= 32) {
        __m256i v = _mm256_load_si256((const __m256i*)s);
        _mm256_store_si256((__m256i*)d, v);
        d += 32;
        s += 32;
        n -= 32;
    }
    
    // Handle remaining bytes with scalar
    if (n > 0) {
        memcpy_scalar(d, s, n);
    }
}

static void memset_avx2(void* dst, int c, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    
    if (n < 32) {
        memset_scalar(dst, c, n);
        return;
    }
    
    // Create 32-byte pattern
    __m256i pattern = _mm256_set1_epi8((char)c);
    
    // Handle unaligned start
    size_t align_offset = 32 - ((uintptr_t)d & 31);
    if (align_offset < 32) {
        memset_scalar(d, c, align_offset);
        d += align_offset;
        n -= align_offset;
    }
    
    // 32-byte aligned AVX2 set
    while (n >= 128) {
        _mm256_store_si256((__m256i*)(d + 0), pattern);
        _mm256_store_si256((__m256i*)(d + 32), pattern);
        _mm256_store_si256((__m256i*)(d + 64), pattern);
        _mm256_store_si256((__m256i*)(d + 96), pattern);
        d += 128;
        n -= 128;
    }
    
    while (n >= 32) {
        _mm256_store_si256((__m256i*)d, pattern);
        d += 32;
        n -= 32;
    }
    
    if (n > 0) {
        memset_scalar(d, c, n);
    }
}

// ===================================================================
// Big Integer Operations (Comba Multiplication with AVX2)
// ===================================================================

static void bign_mul_scalar(uint64_t* c, const uint64_t* a, const uint64_t* b, size_t limbs) {
    // Clear result
    memset(c, 0, 2 * limbs * sizeof(uint64_t));
    
    // Comba multiplication
    for (size_t i = 0; i < limbs; i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < limbs; j++) {
            // Multiply a[i] * b[j]
            __uint128_t prod = (__uint128_t)a[i] * b[j];
            
            // Add to result with carry
            __uint128_t sum = (__uint128_t)c[i + j] + prod + carry;
            c[i + j] = (uint64_t)sum;
            carry = (uint64_t)(sum >> 64);
        }
        c[i + limbs] = carry;
    }
}

static void bign_add_scalar(uint64_t* c, const uint64_t* a, const uint64_t* b, size_t limbs) {
    uint64_t carry = 0;
    for (size_t i = 0; i < limbs; i++) {
        __uint128_t sum = (__uint128_t)a[i] + b[i] + carry;
        c[i] = (uint64_t)sum;
        carry = (uint64_t)(sum >> 64);
    }
}

// ===================================================================
// Tensor Operations (Matrix Multiplication)
// ===================================================================

static void tile_gemm_f32_scalar(float* C, const float* A, const float* B,
                                 int M, int N, int K, int lda, int ldb, int ldc) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * lda + k] * B[k * ldb + j];
            }
            C[i * ldc + j] = sum;
        }
    }
}

static void tile_gemm_f32_avx2(float* C, const float* A, const float* B,
                               int M, int N, int K, int lda, int ldb, int ldc) {
    // AVX2 optimized GEMM with 8-wide float vectors
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j += 8) {
            __m256 sum = _mm256_setzero_ps();
            
            for (int k = 0; k < K; k++) {
                __m256 a_vec = _mm256_broadcast_ss(&A[i * lda + k]);
                __m256 b_vec = _mm256_loadu_ps(&B[k * ldb + j]);
                sum = _mm256_fmadd_ps(a_vec, b_vec, sum);
            }
            
            _mm256_storeu_ps(&C[i * ldc + j], sum);
        }
    }
}

// ===================================================================
// Gather/Scatter Operations
// ===================================================================

static void gather_u32_avx2(uint32_t* dst, const uint32_t* base, const uint32_t* idx, size_t n) {
    size_t i = 0;
    
    // AVX2 gather (8 elements at a time)
    for (; i + 8 <= n; i += 8) {
        __m256i indices = _mm256_loadu_si256((const __m256i*)&idx[i]);
        __m256i gathered = _mm256_i32gather_epi32((const int*)base, indices, 4);
        _mm256_storeu_si256((__m256i*)&dst[i], gathered);
    }
    
    // Handle remaining elements
    for (; i < n; i++) {
        dst[i] = base[idx[i]];
    }
}

static void scatter_u32_scalar(uint32_t* base, const uint32_t* idx, const uint32_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        base[idx[i]] = src[i];
    }
}

// ===================================================================
// Vector Operations
// ===================================================================

static void vec_add_f32_avx2(float* c, const float* a, const float* b, size_t n) {
    size_t i = 0;
    
    // AVX2 vectorized addition (8 floats at a time)
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&c[i], vc);
    }
    
    // Handle remaining elements
    for (; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

static void vec_dot_f32_avx2(float* result, const float* a, const float* b, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    
    // AVX2 vectorized dot product
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        sum = _mm256_fmadd_ps(va, vb, sum);
    }
    
    // Horizontal sum of AVX2 register
    __m128 sum_high = _mm256_extractf128_ps(sum, 1);
    __m128 sum_low = _mm256_castps256_ps128(sum);
    __m128 sum128 = _mm_add_ps(sum_low, sum_high);
    
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    float partial_sum = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements
    for (; i < n; i++) {
        partial_sum += a[i] * b[i];
    }
    
    *result = partial_sum;
}

// ===================================================================
// Timing and Synchronization
// ===================================================================

static uint64_t ticks_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void lfence_x86(void) {
    _mm_lfence();
}

static void sfence_x86(void) {
    _mm_sfence();
}

static void mfence_x86(void) {
    _mm_mfence();
}

// ===================================================================
// Hardware Random Number Generation
// ===================================================================

static uint64_t rng64_rdrand(void) {
    uint64_t result;
    int success = 0;
    
    // Try up to 10 times
    for (int i = 0; i < 10; i++) {
        __asm__ volatile (
            "rdrand %0\n\t"
            "setc %b1"
            : "=r"(result), "=r"(success)
            :
            : "cc"
        );
        if (success) return result;
    }
    
    // Fallback to simple PRNG if RDRAND fails
    static uint64_t seed = 1;
    seed = seed * 1103515245 + 12345;
    return seed;
}

// ===================================================================
// x86 Capability Detection
// ===================================================================

void bdi_probe_x86_caps(bdi_caps_t* caps) {
    uint32_t eax, ebx, ecx, edx;
    
    // Get vendor string
    __cpuid(0, eax, ebx, ecx, edx);
    memcpy(caps->vendor_string, &ebx, 4);
    memcpy(caps->vendor_string + 4, &edx, 4);
    memcpy(caps->vendor_string + 8, &ecx, 4);
    caps->vendor_string[12] = '\0';
    
    // Get basic CPU info
    __cpuid(1, eax, ebx, ecx, edx);
    caps->family = (eax >> 8) & 0xF;
    caps->model = (eax >> 4) & 0xF;
    caps->stepping = eax & 0xF;
    
    // Extended family/model for newer CPUs
    if (caps->family == 0xF) {
        caps->family += (eax >> 20) & 0xFF;
    }
    if (caps->family == 0xF || caps->family == 0x6) {
        caps->model += ((eax >> 16) & 0xF) << 4;
    }
    
    // Feature flags from CPUID.1
    caps->cpu.sse = (edx >> 25) & 1;
    caps->cpu.sse2 = (edx >> 26) & 1;
    caps->cpu.sse3 = ecx & 1;
    caps->cpu.ssse3 = (ecx >> 9) & 1;
    caps->cpu.sse4_1 = (ecx >> 19) & 1;
    caps->cpu.sse4_2 = (ecx >> 20) & 1;
    caps->cpu.avx = (ecx >> 28) & 1;
    caps->cpu.fma = (ecx >> 12) & 1;
    caps->cpu.aes_ni = (ecx >> 25) & 1;
    caps->cpu.pclmulqdq = (ecx >> 1) & 1;
    caps->cpu.rdrand = (ecx >> 30) & 1;
    caps->cpu.popcnt = (ecx >> 23) & 1;
    
    // Extended features (CPUID.7)
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    caps->cpu.avx2 = (ebx >> 5) & 1;
    caps->cpu.bmi1 = (ebx >> 3) & 1;
    caps->cpu.bmi2 = (ebx >> 8) & 1;
    caps->cpu.avx512f = (ebx >> 16) & 1;
    caps->cpu.avx512dq = (ebx >> 17) & 1;
    caps->cpu.rdseed = (ebx >> 18) & 1;
    caps->cpu.avx512cd = (ebx >> 28) & 1;
    caps->cpu.avx512bw = (ebx >> 30) & 1;
    caps->cpu.avx512vl = (ebx >> 31) & 1;
    caps->cpu.sha_ni = (ebx >> 29) & 1;
    
    // Security features
    caps->security.intel_sgx = (ebx >> 2) & 1;
    caps->security.intel_cet = (ecx >> 7) & 1;
    caps->security.intel_ibt = (edx >> 20) & 1;
    caps->security.intel_shstk = (ecx >> 7) & 1;
    
    // Set architecture
    caps->architecture = BDI_ARCH_X86_64;
}

// ===================================================================
// x86 μABI Initialization
// ===================================================================

void bdi_init_x86_uops(const bdi_caps_t* caps) {
    // Memory operations
    if (caps->cpu.avx2) {
        bdi_uops.memcpy_fast = memcpy_avx2;
        bdi_uops.memset_fast = memset_avx2;
    } else {
        bdi_uops.memcpy_fast = memcpy_scalar;
        bdi_uops.memset_fast = memset_scalar;
    }
    
    // Big integer operations
    bdi_uops.bign_mul = bign_mul_scalar;
    bdi_uops.bign_add = bign_add_scalar;
    
    // Tensor operations
    if (caps->cpu.avx2 && caps->cpu.fma) {
        bdi_uops.tile_gemm_f32 = tile_gemm_f32_avx2;
    } else {
        bdi_uops.tile_gemm_f32 = tile_gemm_f32_scalar;
    }
    
    // Gather/scatter
    if (caps->cpu.avx2) {
        bdi_uops.gather_u32 = gather_u32_avx2;
    }
    bdi_uops.scatter_u32 = scatter_u32_scalar;
    
    // Vector operations
    if (caps->cpu.avx2) {
        bdi_uops.vec_add_f32 = vec_add_f32_avx2;
        bdi_uops.vec_dot_f32 = vec_dot_f32_avx2;
    }
    
    // Synchronization
    bdi_uops.lfence = lfence_x86;
    bdi_uops.sfence = sfence_x86;
    bdi_uops.mfence = mfence_x86;
    
    // Timing
    bdi_uops.ticks = ticks_rdtsc;
    
    // Random number generation
    if (caps->cpu.rdrand) {
        bdi_uops.rng64 = rng64_rdrand;
    }
}
