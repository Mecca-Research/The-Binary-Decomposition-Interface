
// BCI Bit Manipulation Utilities Implementation
#include "bci_bitops.h"

// Bit reversal for 32-bit values
uint32_t bit_reverse_u32(uint32_t value) {
    // Swap bits in pairs
    value = ((value & 0x55555555) << 1) | ((value & 0xAAAAAAAA) >> 1);
    // Swap 2-bit groups
    value = ((value & 0x33333333) << 2) | ((value & 0xCCCCCCCC) >> 2);
    // Swap 4-bit groups
    value = ((value & 0x0F0F0F0F) << 4) | ((value & 0xF0F0F0F0) >> 4);
    // Swap bytes
    value = ((value & 0x00FF00FF) << 8) | ((value & 0xFF00FF00) >> 8);
    // Swap 16-bit halves
    value = (value << 16) | (value >> 16);
    return value;
}

// Bit reversal for 64-bit values
uint64_t bit_reverse_u64(uint64_t value) {
    // Swap bits in pairs
    value = ((value & 0x5555555555555555ULL) << 1) | ((value & 0xAAAAAAAAAAAAAAAAULL) >> 1);
    // Swap 2-bit groups
    value = ((value & 0x3333333333333333ULL) << 2) | ((value & 0xCCCCCCCCCCCCCCCCULL) >> 2);
    // Swap 4-bit groups
    value = ((value & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((value & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
    // Swap bytes
    value = ((value & 0x00FF00FF00FF00FFULL) << 8) | ((value & 0xFF00FF00FF00FF00ULL) >> 8);
    // Swap 16-bit groups
    value = ((value & 0x0000FFFF0000FFFFULL) << 16) | ((value & 0xFFFF0000FFFF0000ULL) >> 16);
    // Swap 32-bit halves
    value = (value << 32) | (value >> 32);
    return value;
}
