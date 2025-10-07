
#ifndef BDI_HARDWARE_DETECTOR_H
#define BDI_HARDWARE_DETECTOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// CPU vendor
typedef enum {
    CPU_VENDOR_UNKNOWN,
    CPU_VENDOR_INTEL,
    CPU_VENDOR_AMD,
    CPU_VENDOR_ARM,
    CPU_VENDOR_RISCV
} CPUVendor;

// CPU features
typedef struct {
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_ssse3;
    bool has_sse4_1;
    bool has_sse4_2;
    bool has_avx;
    bool has_avx2;
    bool has_avx512;
    bool has_fma;
    bool has_aes;
    bool has_popcnt;
} CPUFeatures;

// Cache information
typedef struct {
    size_t l1_data_cache_size;
    size_t l1_instruction_cache_size;
    size_t l2_cache_size;
    size_t l3_cache_size;
    size_t cache_line_size;
} CacheInfo;

// NUMA information
typedef struct {
    int node_count;
    int *node_cpus;
    size_t *node_memory;
} NUMAInfo;

// Hardware capabilities
typedef struct {
    CPUVendor vendor;
    char cpu_model[128];
    int core_count;
    int thread_count;
    CPUFeatures features;
    CacheInfo cache;
    NUMAInfo numa;
} HardwareCapabilities;

// Detect hardware capabilities
HardwareCapabilities* hardware_detector_detect(void);

// Free hardware capabilities
void hardware_detector_free(HardwareCapabilities *caps);

// Print hardware information
void hardware_detector_print(const HardwareCapabilities *caps);

// Check if specific feature is available
bool hardware_detector_has_feature(const HardwareCapabilities *caps, const char *feature);

// Get optimal SIMD width
int hardware_detector_get_simd_width(const HardwareCapabilities *caps);

#endif // BDI_HARDWARE_DETECTOR_H
