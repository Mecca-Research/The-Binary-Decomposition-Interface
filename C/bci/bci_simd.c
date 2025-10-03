
// BCI SIMD Operations Implementation
#include "bci_simd.h"

// Runtime CPU feature detection
bool bci_has_avx2_support(void) {
#if BCI_HAS_AVX2
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_cpu_supports("avx2");
#else
    // Conservative: assume not available if we can't detect
    return false;
#endif
#else
    return false;
#endif
}

#if BCI_HAS_AVX2
// AVX2 vectorized binary addition (32-bit)
void binary_add_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    size_t i = 0;
    
    // Process 8 elements at a time (256 bits / 32 bits = 8)
    for (; i + 8 <= count; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_add_epi32(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

// AVX2 vectorized binary XOR (32-bit)
void binary_xor_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    size_t i = 0;
    
    // Process 8 elements at a time
    for (; i + 8 <= count; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_xor_si256(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] ^ b[i];
    }
}

// AVX2 vectorized binary AND
void binary_and_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    size_t i = 0;
    
    for (; i + 8 <= count; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_and_si256(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] & b[i];
    }
}

// AVX2 vectorized binary OR
void binary_or_vec_avx2(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    size_t i = 0;
    
    for (; i + 8 <= count; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_or_si256(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] | b[i];
    }
}

// AVX2 vectorized popcount (sum of set bits)
uint32_t popcount_vec_avx2(const uint32_t *data, size_t count) {
    uint32_t total = 0;
    
    // Use scalar popcount for simplicity (true vectorized popcount is complex)
    for (size_t i = 0; i < count; i++) {
#if defined(__GNUC__) || defined(__clang__)
        total += __builtin_popcount(data[i]);
#else
        uint32_t v = data[i];
        while (v) {
            total += v & 1;
            v >>= 1;
        }
#endif
    }
    
    return total;
}

// AVX2 vectorized byte operations (8-bit)
void binary_add_vec_avx2_u8(const uint8_t *a, const uint8_t *b, uint8_t *result, size_t count) {
    size_t i = 0;
    
    // Process 32 bytes at a time
    for (; i + 32 <= count; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_add_epi8(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

void binary_xor_vec_avx2_u8(const uint8_t *a, const uint8_t *b, uint8_t *result, size_t count) {
    size_t i = 0;
    
    for (; i + 32 <= count; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_xor_si256(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] ^ b[i];
    }
}

// AVX2 vectorized 64-bit operations
void binary_add_vec_avx2_u64(const uint64_t *a, const uint64_t *b, uint64_t *result, size_t count) {
    size_t i = 0;
    
    // Process 4 elements at a time (256 bits / 64 bits = 4)
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_add_epi64(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

void binary_xor_vec_avx2_u64(const uint64_t *a, const uint64_t *b, uint64_t *result, size_t count) {
    size_t i = 0;
    
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(b + i));
        __m256i vresult = _mm256_xor_si256(va, vb);
        _mm256_storeu_si256((__m256i*)(result + i), vresult);
    }
    
    for (; i < count; i++) {
        result[i] = a[i] ^ b[i];
    }
}
#endif // BCI_HAS_AVX2

// Scalar fallback implementations
void binary_add_vec_scalar(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    for (size_t i = 0; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

void binary_xor_vec_scalar(const uint32_t *a, const uint32_t *b, uint32_t *result, size_t count) {
    for (size_t i = 0; i < count; i++) {
        result[i] = a[i] ^ b[i];
    }
}
