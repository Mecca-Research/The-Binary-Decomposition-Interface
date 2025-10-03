
/**
 * BDI Kernel Math Subsystem - C23 Compatibility Header
 * Phase 7: Math Subsystem Modernization
 * 
 * Provides C23 features and compatibility layer for math operations
 */

#ifndef BDI_C23_MATH_H
#define BDI_C23_MATH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ============================================================================
 * C23 Feature Detection and Compatibility
 * ============================================================================ */

/* nullptr support */
#ifndef nullptr
    #define nullptr ((void*)0)
#endif

/* [[nodiscard]] attribute */
#if defined(__GNUC__) || defined(__clang__)
    #define NODISCARD __attribute__((warn_unused_result))
#else
    #define NODISCARD
#endif

/* constexpr support (C23) */
#if __STDC_VERSION__ >= 202311L
    #define MATH_CONSTEXPR constexpr
#else
    #define MATH_CONSTEXPR static const
#endif

/* _Static_assert for structure validation */
#define MATH_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

/* ============================================================================
 * Mathematical Constants (constexpr)
 * ============================================================================ */

MATH_CONSTEXPR double MATH_PI = 3.14159265358979323846;
MATH_CONSTEXPR double MATH_E = 2.71828182845904523536;
MATH_CONSTEXPR double MATH_SQRT2 = 1.41421356237309504880;
MATH_CONSTEXPR double MATH_LN2 = 0.69314718055994530942;
MATH_CONSTEXPR double MATH_LN10 = 2.30258509299404568402;
MATH_CONSTEXPR double MATH_EPSILON = 1e-15;
MATH_CONSTEXPR double MATH_INFINITY = __builtin_inf();
MATH_CONSTEXPR double MATH_NAN = __builtin_nan("");

/* Precision thresholds */
MATH_CONSTEXPR uint32_t MATH_PRECISION_LOW = 8;
MATH_CONSTEXPR uint32_t MATH_PRECISION_MEDIUM = 16;
MATH_CONSTEXPR uint32_t MATH_PRECISION_HIGH = 32;
MATH_CONSTEXPR uint32_t MATH_PRECISION_ULTRA = 64;
MATH_CONSTEXPR uint32_t MATH_PRECISION_MAX = 2048;

/* SIMD alignment requirements */
MATH_CONSTEXPR size_t MATH_SIMD_ALIGN_SSE = 16;
MATH_CONSTEXPR size_t MATH_SIMD_ALIGN_AVX = 32;
MATH_CONSTEXPR size_t MATH_SIMD_ALIGN_AVX512 = 64;

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef enum {
    MATH_SUCCESS = 0,
    MATH_ERROR_NULL_POINTER = -1,
    MATH_ERROR_INVALID_ARGUMENT = -2,
    MATH_ERROR_OUT_OF_MEMORY = -3,
    MATH_ERROR_OVERFLOW = -4,
    MATH_ERROR_UNDERFLOW = -5,
    MATH_ERROR_DIVISION_BY_ZERO = -6,
    MATH_ERROR_INVALID_PRECISION = -7,
    MATH_ERROR_INVALID_BASE = -8,
    MATH_ERROR_BUFFER_TOO_SMALL = -9,
    MATH_ERROR_PARSE_FAILED = -10,
    MATH_ERROR_NOT_IMPLEMENTED = -11,
    MATH_ERROR_SIMD_NOT_AVAILABLE = -12
} math_error_t;

/* ============================================================================
 * Memory Pool Configuration
 * ============================================================================ */

#define MATH_POOL_SMALL_SIZE 64
#define MATH_POOL_MEDIUM_SIZE 256
#define MATH_POOL_LARGE_SIZE 1024
#define MATH_POOL_SMALL_COUNT 128
#define MATH_POOL_MEDIUM_COUNT 64
#define MATH_POOL_LARGE_COUNT 32

/* ============================================================================
 * SIMD Feature Detection
 * ============================================================================ */

typedef enum {
    MATH_SIMD_NONE = 0,
    MATH_SIMD_SSE2 = 1 << 0,
    MATH_SIMD_SSE4_2 = 1 << 1,
    MATH_SIMD_AVX = 1 << 2,
    MATH_SIMD_AVX2 = 1 << 3,
    MATH_SIMD_AVX512 = 1 << 4,
    MATH_SIMD_FMA = 1 << 5
} math_simd_features_t;

/* Detect available SIMD features at runtime */
NODISCARD math_simd_features_t math_detect_simd_features(void);

/* ============================================================================
 * Utility Macros
 * ============================================================================ */

#define MATH_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MATH_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MATH_ABS(x) ((x) < 0 ? -(x) : (x))
#define MATH_CLAMP(x, min, max) MATH_MIN(MATH_MAX(x, min), max)

/* Alignment helpers */
#define MATH_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define MATH_IS_ALIGNED(ptr, align) (((uintptr_t)(ptr) & ((align) - 1)) == 0)

/* Safe overflow detection */
#define MATH_ADD_OVERFLOW(a, b, result) __builtin_add_overflow(a, b, result)
#define MATH_SUB_OVERFLOW(a, b, result) __builtin_sub_overflow(a, b, result)
#define MATH_MUL_OVERFLOW(a, b, result) __builtin_mul_overflow(a, b, result)

#endif /* BDI_C23_MATH_H */
