
/**
 * BDI Kernel Optimization Header - Phase 6
 * ISA-specific intrinsics, optimization macros, and compiler hints
 */

#ifndef BDI_OPTIMIZATION_H
#define BDI_OPTIMIZATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Compiler Detection and Version Checks
 * ============================================================================ */

#if defined(__GNUC__)
    #define COMPILER_GCC 1
    #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(__clang__)
    #define COMPILER_CLANG 1
    #define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

/* ============================================================================
 * Compiler Hints and Attributes
 * ============================================================================ */

/* Branch prediction hints */
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

/* Function attributes */
#define ALWAYS_INLINE   __attribute__((always_inline)) inline
#define NEVER_INLINE    __attribute__((noinline))
#define FLATTEN         __attribute__((flatten))
#define HOT             __attribute__((hot))
#define COLD            __attribute__((cold))
#define PURE            __attribute__((pure))
#define CONST           __attribute__((const))

/* Memory alignment */
#define ALIGNED(x)      __attribute__((aligned(x)))
#define CACHE_ALIGNED   ALIGNED(64)
#define PAGE_ALIGNED    ALIGNED(4096)

/* Section placement */
#define HOT_CODE        __attribute__((section(".text.hot")))
#define COLD_CODE       __attribute__((section(".text.cold")))
#define HOT_DATA        __attribute__((section(".data.hot")))
#define COLD_DATA       __attribute__((section(".data.cold")))

/* Optimization hints */
#define RESTRICT        __restrict__
#define NORETURN        __attribute__((noreturn))
#define MALLOC_LIKE     __attribute__((malloc))
#define WARN_UNUSED     __attribute__((warn_unused_result))

/* Prefetch hints */
#define PREFETCH_READ(addr)     __builtin_prefetch((addr), 0, 3)
#define PREFETCH_WRITE(addr)    __builtin_prefetch((addr), 1, 3)
#define PREFETCH_TEMPORAL(addr) __builtin_prefetch((addr), 0, 3)
#define PREFETCH_NTA(addr)      __builtin_prefetch((addr), 0, 0)

/* Assume hints (GCC 13+) */
#if GCC_VERSION >= 130000
    #define ASSUME(x)   __attribute__((assume(x)))
#else
    #define ASSUME(x)   do { if (!(x)) __builtin_unreachable(); } while(0)
#endif

/* ============================================================================
 * ISA Feature Detection
 * ============================================================================ */

/* SSE/SSE2 (baseline for x86-64) */
#if defined(__SSE2__)
    #define HAS_SSE2 1
    #include <emmintrin.h>
#endif

/* SSE4.2 */
#if defined(__SSE4_2__)
    #define HAS_SSE4_2 1
    #include <nmmintrin.h>
#endif

/* AVX */
#if defined(__AVX__)
    #define HAS_AVX 1
    #include <immintrin.h>
#endif

/* AVX2 */
#if defined(__AVX2__)
    #define HAS_AVX2 1
    #include <immintrin.h>
#endif

/* FMA */
#if defined(__FMA__)
    #define HAS_FMA 1
    #include <immintrin.h>
#endif

/* AVX-512 */
#if defined(__AVX512F__)
    #define HAS_AVX512F 1
    #include <immintrin.h>
#endif

/* BMI/BMI2 */
#if defined(__BMI2__)
    #define HAS_BMI2 1
    #include <immintrin.h>
#endif

/* POPCNT */
#if defined(__POPCNT__)
    #define HAS_POPCNT 1
    #include <immintrin.h>
#endif

/* AES-NI */
#if defined(__AES__)
    #define HAS_AES 1
    #include <wmmintrin.h>
#endif

/* ============================================================================
 * Optimized Memory Operations
 * ============================================================================ */

/**
 * Fast memcpy using SIMD instructions
 */
ALWAYS_INLINE HOT_CODE
void* opt_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t n) {
    void *orig_dst = dst;  /* Preserve original pointer for return */
    
#if defined(HAS_AVX512F)
    /* AVX-512 path: 64 bytes per iteration */
    if (n >= 64 && ((uintptr_t)dst & 63) == 0 && ((uintptr_t)src & 63) == 0) {
        __m512i* d = (__m512i*)dst;
        const __m512i* s = (const __m512i*)src;
        size_t chunks = n / 64;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm512_store_si512(&d[i], _mm512_load_si512(&s[i]));
        }
        
        n -= chunks * 64;
        dst = (void*)(&d[chunks]);
        src = (const void*)(&s[chunks]);
    }
#elif defined(HAS_AVX2)
    /* AVX2 path: 32 bytes per iteration */
    if (n >= 32 && ((uintptr_t)dst & 31) == 0 && ((uintptr_t)src & 31) == 0) {
        __m256i* d = (__m256i*)dst;
        const __m256i* s = (const __m256i*)src;
        size_t chunks = n / 32;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm256_store_si256(&d[i], _mm256_load_si256(&s[i]));
        }
        
        n -= chunks * 32;
        dst = (void*)(&d[chunks]);
        src = (const void*)(&s[chunks]);
    }
#elif defined(HAS_SSE2)
    /* SSE2 path: 16 bytes per iteration */
    if (n >= 16 && ((uintptr_t)dst & 15) == 0 && ((uintptr_t)src & 15) == 0) {
        __m128i* d = (__m128i*)dst;
        const __m128i* s = (const __m128i*)src;
        size_t chunks = n / 16;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm_store_si128(&d[i], _mm_load_si128(&s[i]));
        }
        
        n -= chunks * 16;
        dst = (void*)(&d[chunks]);
        src = (const void*)(&s[chunks]);
    }
#endif
    
    /* Fallback for remaining bytes */
    uint8_t* d8 = (uint8_t*)dst;
    const uint8_t* s8 = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d8[i] = s8[i];
    }
    
    return orig_dst;  /* Return original pointer, not modified one */
}

/**
 * Fast memset using SIMD instructions
 */
ALWAYS_INLINE HOT_CODE
void* opt_memset(void* dst, int c, size_t n) {
    void *orig_dst = dst;  /* Preserve original pointer for return */
    uint8_t val = (uint8_t)c;
    
#if defined(HAS_AVX512F)
    if (n >= 64 && ((uintptr_t)dst & 63) == 0) {
        __m512i pattern = _mm512_set1_epi8(val);
        __m512i* d = (__m512i*)dst;
        size_t chunks = n / 64;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm512_store_si512(&d[i], pattern);
        }
        
        n -= chunks * 64;
        dst = (void*)(&d[chunks]);
    }
#elif defined(HAS_AVX2)
    if (n >= 32 && ((uintptr_t)dst & 31) == 0) {
        __m256i pattern = _mm256_set1_epi8(val);
        __m256i* d = (__m256i*)dst;
        size_t chunks = n / 32;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm256_store_si256(&d[i], pattern);
        }
        
        n -= chunks * 32;
        dst = (void*)(&d[chunks]);
    }
#elif defined(HAS_SSE2)
    if (n >= 16 && ((uintptr_t)dst & 15) == 0) {
        __m128i pattern = _mm_set1_epi8(val);
        __m128i* d = (__m128i*)dst;
        size_t chunks = n / 16;
        
        for (size_t i = 0; i < chunks; i++) {
            _mm_store_si128(&d[i], pattern);
        }
        
        n -= chunks * 16;
        dst = (void*)(&d[chunks]);
    }
#endif
    
    /* Fallback */
    uint8_t* d8 = (uint8_t*)dst;
    for (size_t i = 0; i < n; i++) {
        d8[i] = val;
    }
    
    return orig_dst;  /* Return original pointer, not modified one */
}

/* ============================================================================
 * Optimized Hash Functions
 * ============================================================================ */

/**
 * Fast hash using CRC32C instruction (SSE4.2)
 */
ALWAYS_INLINE HOT_CODE
uint32_t opt_hash_crc32c(const void* data, size_t len) {
#if defined(HAS_SSE4_2)
    uint32_t hash = 0xFFFFFFFF;
    const uint8_t* p = (const uint8_t*)data;
    
    /* Process 8 bytes at a time */
    while (len >= 8) {
        hash = _mm_crc32_u64(hash, *(const uint64_t*)p);
        p += 8;
        len -= 8;
    }
    
    /* Process remaining bytes */
    while (len > 0) {
        hash = _mm_crc32_u8(hash, *p);
        p++;
        len--;
    }
    
    return ~hash;
#else
    /* Fallback: simple hash */
    uint32_t hash = 0;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        hash = hash * 31 + p[i];
    }
    return hash;
#endif
}

/* ============================================================================
 * Optimized Bit Operations
 * ============================================================================ */

/**
 * Count leading zeros
 */
ALWAYS_INLINE CONST
int opt_clz(uint64_t x) {
    return __builtin_clzll(x);
}

/**
 * Count trailing zeros
 */
ALWAYS_INLINE CONST
int opt_ctz(uint64_t x) {
    return __builtin_ctzll(x);
}

/**
 * Population count (number of set bits)
 */
ALWAYS_INLINE CONST
int opt_popcount(uint64_t x) {
#if defined(HAS_POPCNT)
    return _mm_popcnt_u64(x);
#else
    return __builtin_popcountll(x);
#endif
}

/**
 * Byte swap
 */
ALWAYS_INLINE CONST
uint64_t opt_bswap64(uint64_t x) {
    return __builtin_bswap64(x);
}

/* ============================================================================
 * Optimized Crypto Operations
 * ============================================================================ */

#if defined(HAS_AES)
/**
 * AES encryption round using AES-NI
 */
ALWAYS_INLINE HOT_CODE
__m128i opt_aes_encrypt_round(__m128i state, __m128i round_key) {
    return _mm_aesenc_si128(state, round_key);
}

/**
 * AES decryption round using AES-NI
 */
ALWAYS_INLINE HOT_CODE
__m128i opt_aes_decrypt_round(__m128i state, __m128i round_key) {
    return _mm_aesdec_si128(state, round_key);
}
#endif

/* ============================================================================
 * Optimization Validation
 * ============================================================================ */

/**
 * Check if optimizations are enabled at runtime
 */
static inline void opt_check_features(void) {
    #ifdef __OPTIMIZE__
        /* Optimizations enabled */
    #else
        #warning "Optimizations disabled - performance will be degraded"
    #endif
    
    #ifdef __NO_INLINE__
        #warning "Inlining disabled - performance will be degraded"
    #endif
}

/* ============================================================================
 * Performance Monitoring
 * ============================================================================ */

/**
 * Read CPU timestamp counter
 */
ALWAYS_INLINE
uint64_t opt_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/**
 * Serializing read of timestamp counter
 */
ALWAYS_INLINE
uint64_t opt_rdtscp(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return ((uint64_t)hi << 32) | lo;
}

/**
 * CPU pause instruction (for spin loops)
 */
ALWAYS_INLINE
void opt_cpu_pause(void) {
    __asm__ __volatile__ ("pause" ::: "memory");
}

/**
 * Memory fence
 */
ALWAYS_INLINE
void opt_mfence(void) {
    __asm__ __volatile__ ("mfence" ::: "memory");
}

#endif /* BDI_OPTIMIZATION_H */
