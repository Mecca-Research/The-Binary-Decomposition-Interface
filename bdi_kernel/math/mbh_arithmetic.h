
/**
 * BDI Kernel - Multi-Base Hybrid (MBH) Arithmetic Header
 * Phase 7: Math Subsystem Modernization
 * 
 * Provides arbitrary precision arithmetic with multiple base support
 * Modernized with C23 features and SIMD optimizations
 */

#ifndef BDI_MBH_ARITHMETIC_H
#define BDI_MBH_ARITHMETIC_H

#include "c23_math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ============================================================================
 * MBH Constants
 * ============================================================================ */

#define MBH_MAX_DIGITS 1024
#define MBH_DEFAULT_BASE 10
#define MBH_MAX_BASE 36
#define MBH_MIN_BASE 2

/* ============================================================================
 * MBH Number Structure
 * ============================================================================ */

typedef struct mbh_number {
    uint8_t digits[MBH_MAX_DIGITS];     /* Digit array (least significant first) */
    uint32_t length;                    /* Number of significant digits */
    uint32_t base;                      /* Number base (2-36) */
    int8_t sign;                        /* Sign: 1 for positive, -1 for negative */
    uint32_t decimal_point;             /* Position of decimal point (0 = integer) */
    _Atomic uint32_t refcount;          /* Reference count for memory management */
} mbh_number_t;

/* Structure size validation */
MATH_STATIC_ASSERT(sizeof(mbh_number_t) <= 1056, "mbh_number_t size exceeds expected bounds");

/* ============================================================================
 * Initialization and Cleanup
 * ============================================================================ */

NODISCARD math_error_t mbh_init(mbh_number_t *num, uint32_t base);
NODISCARD math_error_t mbh_init_pooled(mbh_number_t **num, uint32_t base);
void mbh_cleanup(mbh_number_t *num);
void mbh_retain(mbh_number_t *num);
void mbh_release(mbh_number_t *num);

/* ============================================================================
 * Conversion Functions
 * ============================================================================ */

NODISCARD math_error_t mbh_from_string(mbh_number_t *num, const char *str, uint32_t base);
NODISCARD math_error_t mbh_from_int(mbh_number_t *num, int64_t value, uint32_t base);
NODISCARD math_error_t mbh_from_uint(mbh_number_t *num, uint64_t value, uint32_t base);
NODISCARD char *mbh_to_string(const mbh_number_t *num);
NODISCARD int64_t mbh_to_int(const mbh_number_t *num);
NODISCARD uint64_t mbh_to_uint(const mbh_number_t *num);

/* ============================================================================
 * Arithmetic Operations
 * ============================================================================ */

NODISCARD math_error_t mbh_add(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_subtract(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_multiply(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_divide(mbh_number_t *quotient, mbh_number_t *remainder,
                                  const mbh_number_t *dividend, const mbh_number_t *divisor);
NODISCARD math_error_t mbh_power(mbh_number_t *result, const mbh_number_t *base, const mbh_number_t *exponent);
NODISCARD math_error_t mbh_modulo(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);

/* Fast path operations (optimized for common cases) */
NODISCARD math_error_t mbh_add_fast(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_mul_fast(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_div_fast(mbh_number_t *quotient, const mbh_number_t *dividend, const mbh_number_t *divisor);

/* ============================================================================
 * Comparison and Utility
 * ============================================================================ */

NODISCARD int mbh_compare(const mbh_number_t *a, const mbh_number_t *b);
NODISCARD math_error_t mbh_copy(mbh_number_t *dest, const mbh_number_t *src);
NODISCARD math_error_t mbh_convert_base(mbh_number_t *result, const mbh_number_t *num, uint32_t new_base);
void mbh_normalize(mbh_number_t *num);
NODISCARD bool mbh_is_zero(const mbh_number_t *num);
NODISCARD bool mbh_is_negative(const mbh_number_t *num);

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

NODISCARD uint8_t mbh_char_to_digit(char c);
NODISCARD char mbh_digit_to_char(uint8_t digit);
NODISCARD bool mbh_validate(const mbh_number_t *num);

#endif /* BDI_MBH_ARITHMETIC_H */
