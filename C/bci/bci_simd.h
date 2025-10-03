
// BCI SIMD Operations - AVX2 vectorized binary operations
#ifndef BCI_SIMD_H
#define BCI_SIMD_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Feature detection for AVX2
#if defined(__AVX2__)
#define BCI_HAS_AVX2 1
#include <immintrin.h>
#else
#define BCI_HAS_AVX2 0
#endif

// Check if AVX2 is available at runtime
bool bci_has_avx2_support(void);

#if BCI_HAS_AVX2
// AVX2 vectorized binary addition (8 x 32-bit integers)
void binary_add_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);

// AVX2 vectorized binary XOR (8 x 32-bit integers)
void binary_xor_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);

// AVX2 vectorized binary AND
void binary_and_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);

// AVX2 vectorized binary OR
void binary_or_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);

// AVX2 vectorized popcount (approximate, returns sum of set bits)
uint32_t popcount_vec_avx2(const uint32_t *data, size_t count);

// AVX2 vectorized byte operations (32 x 8-bit integers)
void binary_add_vec_avx2_u8(const uint8_t *a, const uint8_t *b, uint8_t *result, size_t count);
void binary_xor_vec_avx2_u8(const uint8_t *a, const uint8_t *b, uint8_t *result, size_t count);

// AVX2 vectorized 64-bit operations (4 x 64-bit integers)
void binary_add_vec_avx2_u64(const uint64_t *a, const uint64_t *b, uint64_t *result, size_t count);
void binary_xor_vec_avx2_u64(const uint64_t *a, const uint64_t *b, uint64_t *result, size_t count);
#endif

// Scalar fallback implementations (always available)
void binary_add_vec_scalar(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);
void binary_xor_vec_scalar(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count);

#endif // BCI_SIMD_H
