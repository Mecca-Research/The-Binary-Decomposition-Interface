
// BCI Bit Manipulation Utilities
#ifndef BCI_BITOPS_H
#define BCI_BITOPS_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>

// Bit operations: set, clear, toggle, test
NODISCARD static inline uint64_t bit_set(uint64_t value, unsigned int bit) {
    return value | (1ULL << bit);
}

NODISCARD static inline uint64_t bit_clear(uint64_t value, unsigned int bit) {
    return value & ~(1ULL << bit);
}

NODISCARD static inline uint64_t bit_toggle(uint64_t value, unsigned int bit) {
    return value ^ (1ULL << bit);
}

NODISCARD static inline bool bit_test(uint64_t value, unsigned int bit) {
    return (value & (1ULL << bit)) != 0;
}

// Bit counting operations using compiler intrinsics
// popcount: count number of set bits
NODISCARD static inline int popcount_u32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(value);
#else
    // Software fallback
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
#endif
}

NODISCARD static inline int popcount_u64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(value);
#else
    // Software fallback
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
#endif
}

// clz: count leading zeros (undefined for value == 0)
NODISCARD static inline int clz_u32(uint32_t value) {
    if (value == 0) return 32;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clz(value);
#else
    // Software fallback
    int count = 0;
    for (int i = 31; i >= 0; i--) {
        if (value & (1U << i)) break;
        count++;
    }
    return count;
#endif
}

NODISCARD static inline int clz_u64(uint64_t value) {
    if (value == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(value);
#else
    // Software fallback
    int count = 0;
    for (int i = 63; i >= 0; i--) {
        if (value & (1ULL << i)) break;
        count++;
    }
    return count;
#endif
}

// ctz: count trailing zeros (undefined for value == 0)
NODISCARD static inline int ctz_u32(uint32_t value) {
    if (value == 0) return 32;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(value);
#else
    // Software fallback
    int count = 0;
    for (int i = 0; i < 32; i++) {
        if (value & (1U << i)) break;
        count++;
    }
    return count;
#endif
}

NODISCARD static inline int ctz_u64(uint64_t value) {
    if (value == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(value);
#else
    // Software fallback
    int count = 0;
    for (int i = 0; i < 64; i++) {
        if (value & (1ULL << i)) break;
        count++;
    }
    return count;
#endif
}

// Find first set bit (1-indexed, 0 if no bits set)
NODISCARD static inline int ffs_u32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ffs((int)value);
#else
    if (value == 0) return 0;
    return ctz_u32(value) + 1;
#endif
}

NODISCARD static inline int ffs_u64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ffsll((long long)value);
#else
    if (value == 0) return 0;
    return ctz_u64(value) + 1;
#endif
}

// Parity: returns 1 if odd number of bits set, 0 if even
NODISCARD static inline int parity_u32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_parity(value);
#else
    return popcount_u32(value) & 1;
#endif
}

NODISCARD static inline int parity_u64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_parityll(value);
#else
    return popcount_u64(value) & 1;
#endif
}

// Bit reversal
NODISCARD uint32_t bit_reverse_u32(uint32_t value);
NODISCARD uint64_t bit_reverse_u64(uint64_t value);

// Byte swap (endianness conversion)
NODISCARD static inline uint16_t bswap_u16(uint16_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#else
    return (value >> 8) | (value << 8);
#endif
}

NODISCARD static inline uint32_t bswap_u32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    return ((value >> 24) & 0xFF) |
           ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) |
           ((value << 24) & 0xFF000000);
#endif
}

NODISCARD static inline uint64_t bswap_u64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);
#else
    return ((value >> 56) & 0xFFULL) |
           ((value >> 40) & 0xFF00ULL) |
           ((value >> 24) & 0xFF0000ULL) |
           ((value >> 8) & 0xFF000000ULL) |
           ((value << 8) & 0xFF00000000ULL) |
           ((value << 24) & 0xFF0000000000ULL) |
           ((value << 40) & 0xFF000000000000ULL) |
           ((value << 56) & 0xFF00000000000000ULL);
#endif
}

#endif // BCI_BITOPS_H
