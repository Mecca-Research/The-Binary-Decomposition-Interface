
/**
 * BDI Kernel - Smart Number Library Header
 * Phase 7: Math Subsystem Modernization
 * 
 * M→B→H arithmetic with precision management
 * Modernized with C23 features, atomic reference counting, and SIMD support
 */

#ifndef AEON_SMART_NUMBER_H
#define AEON_SMART_NUMBER_H

#include "c23_math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

/* ============================================================================
 * Smart Number Representation Types
 * ============================================================================ */

typedef enum {
    SMART_NUM_MACHINE = 0,      /* Machine representation (binary) */
    SMART_NUM_BINARY = 1,       /* Binary decomposition */
    SMART_NUM_HUMAN = 2         /* Human-readable representation */
} smart_num_repr_t;

/* ============================================================================
 * Precision Levels
 * ============================================================================ */

typedef enum {
    PRECISION_LOW = 8,          /* 8-bit precision */
    PRECISION_MEDIUM = 16,      /* 16-bit precision */
    PRECISION_HIGH = 32,        /* 32-bit precision */
    PRECISION_ULTRA = 64,       /* 64-bit precision */
    PRECISION_ARBITRARY = 0     /* Arbitrary precision */
} precision_level_t;

/* ============================================================================
 * Smart Number Types
 * ============================================================================ */

typedef enum {
    SMART_TYPE_INT8,
    SMART_TYPE_INT16,
    SMART_TYPE_INT32,
    SMART_TYPE_INT64,
    SMART_TYPE_UINT8,
    SMART_TYPE_UINT16,
    SMART_TYPE_UINT32,
    SMART_TYPE_UINT64,
    SMART_TYPE_FLOAT,
    SMART_TYPE_DOUBLE,
    SMART_TYPE_DECIMAL,
    SMART_TYPE_FRACTION,
    SMART_TYPE_COMPLEX,
    SMART_TYPE_BIGINT
} smart_number_type_t;

/* ============================================================================
 * Smart Number Flags
 * ============================================================================ */

#define SMART_FLAG_EXACT        0x01    /* Exact representation */
#define SMART_FLAG_APPROXIMATE  0x02    /* Approximate representation */
#define SMART_FLAG_OVERFLOW     0x04    /* Overflow occurred */
#define SMART_FLAG_UNDERFLOW    0x08    /* Underflow occurred */
#define SMART_FLAG_INFINITE     0x10    /* Infinite value */
#define SMART_FLAG_NAN          0x20    /* Not a number */
#define SMART_FLAG_POOLED       0x40    /* Allocated from memory pool */

/* ============================================================================
 * Smart Number Structures
 * ============================================================================ */

/* Decimal representation for high precision */
typedef struct smart_decimal {
    uint8_t *digits;            /* Digit array */
    uint32_t integer_part;      /* Number of integer digits */
    uint32_t decimal_part;      /* Number of decimal digits */
    int8_t sign;                /* Sign */
    _Atomic uint32_t refcount;  /* Reference count */
} smart_decimal_t;

/* Fraction representation */
typedef struct smart_fraction {
    int64_t numerator;
    int64_t denominator;
} smart_fraction_t;

/* Forward declaration for complex numbers */
typedef struct smart_number smart_number_t;

/* Complex number representation */
typedef struct smart_complex {
    smart_number_t *real;
    smart_number_t *imaginary;
} smart_complex_t;

/* Main smart number structure */
struct smart_number {
    smart_number_type_t type;
    union {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float f;
        double d;
        smart_decimal_t decimal;
        smart_fraction_t fraction;
        smart_complex_t complex;
        void *bigint;           /* Pointer to arbitrary precision integer */
    } value;
    uint32_t precision;         /* Precision hint */
    uint32_t flags;             /* Special flags */
    _Atomic uint32_t refcount;  /* Atomic reference count */
};

/* Structure size validation */
MATH_STATIC_ASSERT(sizeof(smart_number_t) <= 128, "smart_number_t size exceeds expected bounds");

/* ============================================================================
 * Arithmetic Operations Enum
 * ============================================================================ */

typedef enum {
    SMART_OP_ADD = 0,
    SMART_OP_SUB = 1,
    SMART_OP_MUL = 2,
    SMART_OP_DIV = 3,
    SMART_OP_MOD = 4,
    SMART_OP_POW = 5,
    SMART_OP_SQRT = 6,
    SMART_OP_LOG = 7,
    SMART_OP_EXP = 8,
    SMART_OP_SIN = 9,
    SMART_OP_COS = 10,
    SMART_OP_TAN = 11
} smart_operation_t;

/* ============================================================================
 * Initialization and Cleanup
 * ============================================================================ */

NODISCARD math_error_t smart_init(smart_number_t *num);
NODISCARD math_error_t smart_init_pooled(smart_number_t **num);
void smart_cleanup(smart_number_t *num);
void smart_retain(smart_number_t *num);
void smart_release(smart_number_t *num);

/* ============================================================================
 * Conversion Functions
 * ============================================================================ */

NODISCARD math_error_t smart_from_int(smart_number_t *num, int64_t value);
NODISCARD math_error_t smart_from_uint(smart_number_t *num, uint64_t value);
NODISCARD math_error_t smart_from_double(smart_number_t *num, double value);
NODISCARD math_error_t smart_from_string(smart_number_t *num, const char *str);
NODISCARD math_error_t smart_from_fraction(smart_number_t *num, int64_t numerator, int64_t denominator);

NODISCARD char *smart_to_string(const smart_number_t *num);
NODISCARD double smart_to_double(const smart_number_t *num);
NODISCARD int64_t smart_to_int(const smart_number_t *num);

/* ============================================================================
 * Arithmetic Operations
 * ============================================================================ */

NODISCARD math_error_t smart_add(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_subtract(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_multiply(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_divide(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_modulo(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_power(smart_number_t *result, const smart_number_t *base, const smart_number_t *exponent);

/* Fast path operations */
NODISCARD math_error_t smart_add_fast(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_mul_fast(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_div_fast(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);

/* ============================================================================
 * Comparison and Utility
 * ============================================================================ */

NODISCARD int smart_compare(const smart_number_t *a, const smart_number_t *b);
NODISCARD math_error_t smart_copy(smart_number_t *dest, const smart_number_t *src);
NODISCARD bool smart_is_zero(const smart_number_t *num);
NODISCARD bool smart_is_negative(const smart_number_t *num);
NODISCARD bool smart_is_integer(const smart_number_t *num);
NODISCARD bool smart_validate(const smart_number_t *num);

/* ============================================================================
 * Memory Pool Management
 * ============================================================================ */

NODISCARD math_error_t smart_pool_init(void);
void smart_pool_cleanup(void);
NODISCARD size_t smart_pool_stats(void);

#endif /* AEON_SMART_NUMBER_H */
