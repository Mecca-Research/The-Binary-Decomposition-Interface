
/**
 * @file fuzzing_utils.c
 * @brief Fuzzing Utilities Implementation
 */

#include "fuzzing_utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ============================================================================
// Implementation
// ============================================================================

void fuzz_init(uint32_t seed) {
    srand(seed);
}

size_t fuzz_random_size(size_t min, size_t max) {
    if (min >= max) {
        return min;
    }
    return min + (rand() % (max - min + 1));
}

size_t fuzz_random_alignment(size_t max_alignment) {
    // Generate power of 2 alignment
    uint32_t power = rand() % 13; // 2^0 to 2^12 (1 to 4096)
    size_t alignment = 1UL << power;
    return alignment <= max_alignment ? alignment : max_alignment;
}

uint32_t fuzz_random_flags(uint32_t max_flags) {
    return rand() % (max_flags + 1);
}

ptrdiff_t fuzz_random_offset(ptrdiff_t max_offset) {
    return (rand() % (2 * max_offset + 1)) - max_offset;
}

size_t fuzz_boundary_size(size_t boundary) {
    // Generate size near boundary: boundary ± random offset
    int offset = (rand() % 17) - 8; // -8 to +8
    size_t size = boundary + offset;
    return size > 0 ? size : 1;
}

size_t fuzz_power_of_two_size(uint32_t min_power, uint32_t max_power) {
    uint32_t power = min_power + (rand() % (max_power - min_power + 1));
    return 1UL << power;
}

size_t fuzz_aligned_size(size_t alignment, size_t max_size) {
    size_t size = fuzz_random_size(alignment, max_size);
    return (size / alignment) * alignment;
}

size_t fuzz_unaligned_size(size_t alignment, size_t max_size) {
    size_t size = fuzz_random_size(1, max_size);
    if (size % alignment == 0) {
        size += 1; // Make it unaligned
    }
    return size;
}

size_t fuzz_extreme_size(void) {
    int choice = rand() % 4;
    switch (choice) {
        case 0: return 0;
        case 1: return 1;
        case 2: return SIZE_MAX;
        case 3: return SIZE_MAX / 2;
        default: return 1024;
    }
}

size_t fuzz_size_with_strategy(fuzz_strategy_t strategy, size_t min, size_t max) {
    switch (strategy) {
        case FUZZ_STRATEGY_RANDOM:
            return fuzz_random_size(min, max);
        
        case FUZZ_STRATEGY_BOUNDARY:
            return fuzz_boundary_size((min + max) / 2);
        
        case FUZZ_STRATEGY_POWER_OF_TWO:
            return fuzz_power_of_two_size(0, 30);
        
        case FUZZ_STRATEGY_ALIGNED:
            return fuzz_aligned_size(4096, max);
        
        case FUZZ_STRATEGY_UNALIGNED:
            return fuzz_unaligned_size(4096, max);
        
        case FUZZ_STRATEGY_EXTREME:
            return fuzz_extreme_size();
        
        case FUZZ_STRATEGY_SEQUENTIAL:
            return min + (rand() % 1000);
        
        default:
            return fuzz_random_size(min, max);
    }
}

void fuzz_fill_random(void *buffer, size_t size) {
    uint8_t *bytes = (uint8_t *)buffer;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = rand() & 0xFF;
    }
}

void fuzz_fill_pattern(void *buffer, size_t size, uint8_t pattern) {
    memset(buffer, pattern, size);
}

bool fuzz_random_bool(void) {
    return (rand() % 2) == 1;
}

uint64_t fuzz_random_range(uint64_t min, uint64_t max) {
    if (min >= max) {
        return min;
    }
    return min + (rand() % (max - min + 1));
}

double fuzz_random_double(double min, double max) {
    double random = (double)rand() / RAND_MAX;
    return min + random * (max - min);
}

uint32_t fuzz_get_interesting_sizes(size_t *values, uint32_t max_count) {
    size_t interesting[] = {
        0, 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64,
        127, 128, 255, 256, 511, 512, 1023, 1024,
        2047, 2048, 4095, 4096, 8191, 8192,
        16383, 16384, 32767, 32768, 65535, 65536,
        1024*1024 - 1, 1024*1024, 1024*1024 + 1,
        2*1024*1024 - 1, 2*1024*1024, 2*1024*1024 + 1,
        1024*1024*1024 - 1, 1024*1024*1024
    };
    
    uint32_t count = sizeof(interesting) / sizeof(interesting[0]);
    if (count > max_count) {
        count = max_count;
    }
    
    memcpy(values, interesting, count * sizeof(size_t));
    return count;
}

uint32_t fuzz_get_interesting_alignments(size_t *values, uint32_t max_count) {
    size_t interesting[] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
    };
    
    uint32_t count = sizeof(interesting) / sizeof(interesting[0]);
    if (count > max_count) {
        count = max_count;
    }
    
    memcpy(values, interesting, count * sizeof(size_t));
    return count;
}
