
/**
 * @file fuzzing_utils.h
 * @brief Fuzzing Utilities for Red-Team Testing
 * @details Provides random data generation, size/alignment fuzzing,
 *          and corpus management for comprehensive testing.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#ifndef FUZZING_UTILS_H
#define FUZZING_UTILS_H

#include "../../../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// Fuzzing Configuration
// ============================================================================

#define FUZZ_MAX_SIZE           (1024 * 1024 * 1024)  // 1 GB
#define FUZZ_MIN_SIZE           1
#define FUZZ_MAX_ALIGNMENT      4096
#define FUZZ_CORPUS_SIZE        10000

// ============================================================================
// Fuzzing Strategies
// ============================================================================

typedef enum {
    FUZZ_STRATEGY_RANDOM,       // Completely random
    FUZZ_STRATEGY_BOUNDARY,     // Boundary values
    FUZZ_STRATEGY_POWER_OF_TWO, // Powers of 2
    FUZZ_STRATEGY_ALIGNED,      // Aligned values
    FUZZ_STRATEGY_UNALIGNED,    // Unaligned values
    FUZZ_STRATEGY_EXTREME,      // Extreme values
    FUZZ_STRATEGY_SEQUENTIAL,   // Sequential values
    FUZZ_STRATEGY_MAX
} fuzz_strategy_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize fuzzing utilities
 * @param seed Random seed for reproducibility
 */
void fuzz_init(uint32_t seed);

/**
 * @brief Generate random size
 * @param min Minimum size
 * @param max Maximum size
 * @return Random size
 */
size_t fuzz_random_size(size_t min, size_t max);

/**
 * @brief Generate random alignment
 * @param max_alignment Maximum alignment
 * @return Random alignment (power of 2)
 */
size_t fuzz_random_alignment(size_t max_alignment);

/**
 * @brief Generate random flags
 * @param max_flags Maximum flag value
 * @return Random flags
 */
uint32_t fuzz_random_flags(uint32_t max_flags);

/**
 * @brief Generate random pointer offset
 * @param max_offset Maximum offset
 * @return Random offset
 */
ptrdiff_t fuzz_random_offset(ptrdiff_t max_offset);

/**
 * @brief Generate boundary size value
 * @param boundary Boundary to test around
 * @return Size near boundary
 */
size_t fuzz_boundary_size(size_t boundary);

/**
 * @brief Generate power-of-two size
 * @param min_power Minimum power
 * @param max_power Maximum power
 * @return Power of 2 size
 */
size_t fuzz_power_of_two_size(uint32_t min_power, uint32_t max_power);

/**
 * @brief Generate aligned size
 * @param alignment Alignment requirement
 * @param max_size Maximum size
 * @return Aligned size
 */
size_t fuzz_aligned_size(size_t alignment, size_t max_size);

/**
 * @brief Generate unaligned size
 * @param alignment Alignment to avoid
 * @param max_size Maximum size
 * @return Unaligned size
 */
size_t fuzz_unaligned_size(size_t alignment, size_t max_size);

/**
 * @brief Generate extreme size value
 * @return Extreme size (very small or very large)
 */
size_t fuzz_extreme_size(void);

/**
 * @brief Generate size using strategy
 * @param strategy Fuzzing strategy
 * @param min Minimum size
 * @param max Maximum size
 * @return Generated size
 */
size_t fuzz_size_with_strategy(fuzz_strategy_t strategy, size_t min, size_t max);

/**
 * @brief Fill buffer with random data
 * @param buffer Buffer to fill
 * @param size Buffer size
 */
void fuzz_fill_random(void *buffer, size_t size);

/**
 * @brief Fill buffer with pattern
 * @param buffer Buffer to fill
 * @param size Buffer size
 * @param pattern Pattern byte
 */
void fuzz_fill_pattern(void *buffer, size_t size, uint8_t pattern);

/**
 * @brief Generate random boolean
 * @return Random boolean
 */
bool fuzz_random_bool(void);

/**
 * @brief Generate random value in range
 * @param min Minimum value
 * @param max Maximum value
 * @return Random value
 */
uint64_t fuzz_random_range(uint64_t min, uint64_t max);

/**
 * @brief Generate random double in range
 * @param min Minimum value
 * @param max Maximum value
 * @return Random double
 */
double fuzz_random_double(double min, double max);

/**
 * @brief Get interesting size values for testing
 * @param values Output array
 * @param max_count Maximum values to return
 * @return Number of values returned
 */
uint32_t fuzz_get_interesting_sizes(size_t *values, uint32_t max_count);

/**
 * @brief Get interesting alignment values
 * @param values Output array
 * @param max_count Maximum values to return
 * @return Number of values returned
 */
uint32_t fuzz_get_interesting_alignments(size_t *values, uint32_t max_count);

#endif // FUZZING_UTILS_H
