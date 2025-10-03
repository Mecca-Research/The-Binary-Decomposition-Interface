
// BCI Arithmetic Library - Binary arithmetic operations with carry
#ifndef BCI_ARITHMETIC_H
#define BCI_ARITHMETIC_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>

// Binary addition result with carry
typedef struct {
    uint64_t sum;
    bool carry;
} BinaryAddResult;

// Binary multiplication result with overflow detection
typedef struct {
    uint64_t product_low;
    uint64_t product_high;
    bool overflow;
} BinaryMulResult;

// 8-bit operations
NODISCARD BinaryAddResult binary_add_u8(uint8_t a, uint8_t b, bool carry_in);
NODISCARD BinaryMulResult binary_mul_u8(uint8_t a, uint8_t b);

// 16-bit operations
NODISCARD BinaryAddResult binary_add_u16(uint16_t a, uint16_t b, bool carry_in);
NODISCARD BinaryMulResult binary_mul_u16(uint16_t a, uint16_t b);

// 32-bit operations
NODISCARD BinaryAddResult binary_add_u32(uint32_t a, uint32_t b, bool carry_in);
NODISCARD BinaryMulResult binary_mul_u32(uint32_t a, uint32_t b);

// 64-bit operations
NODISCARD BinaryAddResult binary_add_u64(uint64_t a, uint64_t b, bool carry_in);
NODISCARD BinaryMulResult binary_mul_u64(uint64_t a, uint64_t b);

// 128-bit operations (using __uint128_t if available)
#ifdef __SIZEOF_INT128__
typedef __uint128_t uint128_t;
typedef struct {
    uint128_t sum;
    bool carry;
} BinaryAddResult128;

typedef struct {
    uint128_t product_low;
    uint128_t product_high;
    bool overflow;
} BinaryMulResult128;

NODISCARD BinaryAddResult128 binary_add_u128(uint128_t a, uint128_t b, bool carry_in);
NODISCARD BinaryMulResult128 binary_mul_u128(uint128_t a, uint128_t b);
#endif

// C23 _BitInt support (feature detection)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#if defined(__BITINT_MAXWIDTH__) && __BITINT_MAXWIDTH__ >= 128
#define BCI_HAS_BITINT 1

// Generic _BitInt addition
#define binary_add_bitint(N, a, b, carry_in) \
    _Generic((a), \
        _BitInt(N): binary_add_bitint_impl_##N, \
        unsigned _BitInt(N): binary_add_bitint_impl_u##N \
    )(a, b, carry_in)
#endif
#endif

#endif // BCI_ARITHMETIC_H
