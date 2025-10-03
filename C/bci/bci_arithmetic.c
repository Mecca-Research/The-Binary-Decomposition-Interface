
// BCI Arithmetic Library Implementation
#include "bci_arithmetic.h"
#include <limits.h>

// 8-bit addition with carry
BinaryAddResult binary_add_u8(uint8_t a, uint8_t b, bool carry_in) {
    BinaryAddResult result;
    uint16_t sum = (uint16_t)a + (uint16_t)b + (carry_in ? 1 : 0);
    result.sum = (uint8_t)sum;
    result.carry = (sum > UINT8_MAX);
    return result;
}

// 8-bit multiplication
BinaryMulResult binary_mul_u8(uint8_t a, uint8_t b) {
    BinaryMulResult result;
    uint16_t product = (uint16_t)a * (uint16_t)b;
    result.product_low = (uint8_t)product;
    result.product_high = (uint8_t)(product >> 8);
    result.overflow = (result.product_high != 0);
    return result;
}

// 16-bit addition with carry
BinaryAddResult binary_add_u16(uint16_t a, uint16_t b, bool carry_in) {
    BinaryAddResult result;
    uint32_t sum = (uint32_t)a + (uint32_t)b + (carry_in ? 1 : 0);
    result.sum = (uint16_t)sum;
    result.carry = (sum > UINT16_MAX);
    return result;
}

// 16-bit multiplication
BinaryMulResult binary_mul_u16(uint16_t a, uint16_t b) {
    BinaryMulResult result;
    uint32_t product = (uint32_t)a * (uint32_t)b;
    result.product_low = (uint16_t)product;
    result.product_high = (uint16_t)(product >> 16);
    result.overflow = (result.product_high != 0);
    return result;
}

// 32-bit addition with carry
BinaryAddResult binary_add_u32(uint32_t a, uint32_t b, bool carry_in) {
    BinaryAddResult result;
    uint64_t sum = (uint64_t)a + (uint64_t)b + (carry_in ? 1 : 0);
    result.sum = (uint32_t)sum;
    result.carry = (sum > UINT32_MAX);
    return result;
}

// 32-bit multiplication
BinaryMulResult binary_mul_u32(uint32_t a, uint32_t b) {
    BinaryMulResult result;
    uint64_t product = (uint64_t)a * (uint64_t)b;
    result.product_low = (uint32_t)product;
    result.product_high = (uint32_t)(product >> 32);
    result.overflow = (result.product_high != 0);
    return result;
}

// 64-bit addition with carry
BinaryAddResult binary_add_u64(uint64_t a, uint64_t b, bool carry_in) {
    BinaryAddResult result;
    result.sum = a + b + (carry_in ? 1 : 0);
    // Check for overflow: if sum < a, overflow occurred
    result.carry = (result.sum < a) || (carry_in && result.sum == a);
    return result;
}

// 64-bit multiplication
BinaryMulResult binary_mul_u64(uint64_t a, uint64_t b) {
    BinaryMulResult result;
    
#ifdef __SIZEOF_INT128__
    // Use 128-bit arithmetic if available
    __uint128_t product = (__uint128_t)a * (__uint128_t)b;
    result.product_low = (uint64_t)product;
    result.product_high = (uint64_t)(product >> 64);
#else
    // Fallback: split into 32-bit parts
    uint64_t a_lo = a & 0xFFFFFFFFULL;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = b & 0xFFFFFFFFULL;
    uint64_t b_hi = b >> 32;
    
    uint64_t p0 = a_lo * b_lo;
    uint64_t p1 = a_lo * b_hi;
    uint64_t p2 = a_hi * b_lo;
    uint64_t p3 = a_hi * b_hi;
    
    uint64_t carry = ((p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL)) >> 32;
    
    result.product_low = p0 + ((p1 + p2) << 32);
    result.product_high = p3 + (p1 >> 32) + (p2 >> 32) + carry;
#endif
    
    result.overflow = (result.product_high != 0);
    return result;
}

// 128-bit operations (if supported)
#ifdef __SIZEOF_INT128__
BinaryAddResult128 binary_add_u128(uint128_t a, uint128_t b, bool carry_in) {
    BinaryAddResult128 result;
    result.sum = a + b + (carry_in ? 1 : 0);
    result.carry = (result.sum < a) || (carry_in && result.sum == a);
    return result;
}

BinaryMulResult128 binary_mul_u128(uint128_t a, uint128_t b) {
    BinaryMulResult128 result;
    // Split into 64-bit parts for multiplication
    uint64_t a_lo = (uint64_t)a;
    uint64_t a_hi = (uint64_t)(a >> 64);
    uint64_t b_lo = (uint64_t)b;
    uint64_t b_hi = (uint64_t)(b >> 64);
    
    uint128_t p0 = (uint128_t)a_lo * b_lo;
    uint128_t p1 = (uint128_t)a_lo * b_hi;
    uint128_t p2 = (uint128_t)a_hi * b_lo;
    uint128_t p3 = (uint128_t)a_hi * b_hi;
    
    uint128_t carry = ((p0 >> 64) + (p1 & 0xFFFFFFFFFFFFFFFFULL) + (p2 & 0xFFFFFFFFFFFFFFFFULL)) >> 64;
    
    result.product_low = p0 + ((p1 + p2) << 64);
    result.product_high = p3 + (p1 >> 64) + (p2 >> 64) + carry;
    result.overflow = (result.product_high != 0);
    return result;
}
#endif
