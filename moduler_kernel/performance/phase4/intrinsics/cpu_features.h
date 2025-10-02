/**
 * @file cpu_features.h
 * @brief CPU feature detection for ISA dispatch
 */

#ifndef PHASE4_CPU_FEATURES_H
#define PHASE4_CPU_FEATURES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// CPU feature flags
typedef enum {
    CPU_FEATURE_SSE2     = (1 << 0),
    CPU_FEATURE_SSE3     = (1 << 1),
    CPU_FEATURE_SSSE3    = (1 << 2),
    CPU_FEATURE_SSE4_1   = (1 << 3),
    CPU_FEATURE_SSE4_2   = (1 << 4),
    CPU_FEATURE_AVX      = (1 << 5),
    CPU_FEATURE_AVX2     = (1 << 6),
    CPU_FEATURE_AVX512F  = (1 << 7),
    CPU_FEATURE_AVX512BW = (1 << 8),
    CPU_FEATURE_AVX512VL = (1 << 9),
    CPU_FEATURE_NEON     = (1 << 10),
} cpu_feature_t;

/**
 * @brief Detect CPU features
 * @return Bitmask of supported features
 */
uint32_t cpu_detect_features(void);

/**
 * @brief Check if feature is supported
 * @param feature Feature to check
 * @return true if supported, false otherwise
 */
bool cpu_has_feature(cpu_feature_t feature);

/**
 * @brief Get CPU vendor string
 * @param vendor Output buffer (min 13 bytes)
 */
void cpu_get_vendor(char* vendor);

/**
 * @brief Get CPU brand string
 * @param brand Output buffer (min 49 bytes)
 */
void cpu_get_brand(char* brand);

#ifdef __cplusplus
}
#endif

#endif
