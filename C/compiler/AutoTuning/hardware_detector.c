
#include "hardware_detector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __x86_64__
#include <cpuid.h>
#endif

static CPUVendor detect_cpu_vendor(void) {
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    char vendor[13];
    
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
        memcpy(vendor, &ebx, 4);
        memcpy(vendor + 4, &edx, 4);
        memcpy(vendor + 8, &ecx, 4);
        vendor[12] = '\0';
        
        if (strcmp(vendor, "GenuineIntel") == 0) {
            return CPU_VENDOR_INTEL;
        } else if (strcmp(vendor, "AuthenticAMD") == 0) {
            return CPU_VENDOR_AMD;
        }
    }
#elif defined(__aarch64__)
    return CPU_VENDOR_ARM;
#elif defined(__riscv)
    return CPU_VENDOR_RISCV;
#endif
    
    return CPU_VENDOR_UNKNOWN;
}

static void detect_cpu_features(CPUFeatures *features) {
    memset(features, 0, sizeof(CPUFeatures));
    
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        features->has_sse = (edx & (1 << 25)) != 0;
        features->has_sse2 = (edx & (1 << 26)) != 0;
        features->has_sse3 = (ecx & (1 << 0)) != 0;
        features->has_ssse3 = (ecx & (1 << 9)) != 0;
        features->has_sse4_1 = (ecx & (1 << 19)) != 0;
        features->has_sse4_2 = (ecx & (1 << 20)) != 0;
        features->has_avx = (ecx & (1 << 28)) != 0;
        features->has_fma = (ecx & (1 << 12)) != 0;
        features->has_aes = (ecx & (1 << 25)) != 0;
        features->has_popcnt = (ecx & (1 << 23)) != 0;
    }
    
    if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
        features->has_avx2 = (ebx & (1 << 5)) != 0;
        features->has_avx512 = (ebx & (1 << 16)) != 0;
    }
#endif
}

static void detect_cache_info(CacheInfo *cache) {
    memset(cache, 0, sizeof(CacheInfo));
    
    // Default values
    cache->l1_data_cache_size = 32 * 1024;      // 32 KB
    cache->l1_instruction_cache_size = 32 * 1024;
    cache->l2_cache_size = 256 * 1024;          // 256 KB
    cache->l3_cache_size = 8 * 1024 * 1024;     // 8 MB
    cache->cache_line_size = 64;
    
#ifdef __linux__
    FILE *fp;
    char line[256];
    
    // Try to read from sysfs
    fp = fopen("/sys/devices/system/cpu/cpu0/cache/index0/size", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            cache->l1_data_cache_size = atoi(line) * 1024;
        }
        fclose(fp);
    }
    
    fp = fopen("/sys/devices/system/cpu/cpu0/cache/index2/size", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            cache->l2_cache_size = atoi(line) * 1024;
        }
        fclose(fp);
    }
    
    fp = fopen("/sys/devices/system/cpu/cpu0/cache/index3/size", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            cache->l3_cache_size = atoi(line) * 1024;
        }
        fclose(fp);
    }
#endif
}

HardwareCapabilities* hardware_detector_detect(void) {
    HardwareCapabilities *caps = calloc(1, sizeof(HardwareCapabilities));
    if (!caps) {
        return NULL;
    }

    // Detect CPU vendor
    caps->vendor = detect_cpu_vendor();

    // Get CPU model
    strncpy(caps->cpu_model, "Unknown CPU", sizeof(caps->cpu_model) - 1);

    // Get core and thread count
    caps->core_count = sysconf(_SC_NPROCESSORS_ONLN);
    caps->thread_count = caps->core_count;  // Simplified

    // Detect CPU features
    detect_cpu_features(&caps->features);

    // Detect cache information
    detect_cache_info(&caps->cache);

    // NUMA detection (simplified)
    caps->numa.node_count = 1;
    caps->numa.node_cpus = NULL;
    caps->numa.node_memory = NULL;

    return caps;
}

void hardware_detector_free(HardwareCapabilities *caps) {
    if (!caps) return;
    free(caps->numa.node_cpus);
    free(caps->numa.node_memory);
    free(caps);
}

void hardware_detector_print(const HardwareCapabilities *caps) {
    if (!caps) return;

    printf("\n=== Hardware Capabilities ===\n");
    
    const char *vendor_str = "Unknown";
    switch (caps->vendor) {
        case CPU_VENDOR_INTEL: vendor_str = "Intel"; break;
        case CPU_VENDOR_AMD: vendor_str = "AMD"; break;
        case CPU_VENDOR_ARM: vendor_str = "ARM"; break;
        case CPU_VENDOR_RISCV: vendor_str = "RISC-V"; break;
        default: break;
    }
    
    printf("CPU Vendor: %s\n", vendor_str);
    printf("CPU Model: %s\n", caps->cpu_model);
    printf("Cores: %d\n", caps->core_count);
    printf("Threads: %d\n", caps->thread_count);
    
    printf("\n--- CPU Features ---\n");
    printf("SSE: %s\n", caps->features.has_sse ? "Yes" : "No");
    printf("SSE2: %s\n", caps->features.has_sse2 ? "Yes" : "No");
    printf("SSE3: %s\n", caps->features.has_sse3 ? "Yes" : "No");
    printf("SSSE3: %s\n", caps->features.has_ssse3 ? "Yes" : "No");
    printf("SSE4.1: %s\n", caps->features.has_sse4_1 ? "Yes" : "No");
    printf("SSE4.2: %s\n", caps->features.has_sse4_2 ? "Yes" : "No");
    printf("AVX: %s\n", caps->features.has_avx ? "Yes" : "No");
    printf("AVX2: %s\n", caps->features.has_avx2 ? "Yes" : "No");
    printf("AVX-512: %s\n", caps->features.has_avx512 ? "Yes" : "No");
    printf("FMA: %s\n", caps->features.has_fma ? "Yes" : "No");
    printf("AES: %s\n", caps->features.has_aes ? "Yes" : "No");
    printf("POPCNT: %s\n", caps->features.has_popcnt ? "Yes" : "No");
    
    printf("\n--- Cache Information ---\n");
    printf("L1 Data Cache: %zu KB\n", caps->cache.l1_data_cache_size / 1024);
    printf("L1 Instruction Cache: %zu KB\n", caps->cache.l1_instruction_cache_size / 1024);
    printf("L2 Cache: %zu KB\n", caps->cache.l2_cache_size / 1024);
    printf("L3 Cache: %zu KB\n", caps->cache.l3_cache_size / 1024);
    printf("Cache Line Size: %zu bytes\n", caps->cache.cache_line_size);
    
    printf("\n--- NUMA Information ---\n");
    printf("NUMA Nodes: %d\n", caps->numa.node_count);
}

bool hardware_detector_has_feature(const HardwareCapabilities *caps, const char *feature) {
    if (!caps || !feature) return false;

    if (strcmp(feature, "sse") == 0) return caps->features.has_sse;
    if (strcmp(feature, "sse2") == 0) return caps->features.has_sse2;
    if (strcmp(feature, "sse3") == 0) return caps->features.has_sse3;
    if (strcmp(feature, "ssse3") == 0) return caps->features.has_ssse3;
    if (strcmp(feature, "sse4.1") == 0) return caps->features.has_sse4_1;
    if (strcmp(feature, "sse4.2") == 0) return caps->features.has_sse4_2;
    if (strcmp(feature, "avx") == 0) return caps->features.has_avx;
    if (strcmp(feature, "avx2") == 0) return caps->features.has_avx2;
    if (strcmp(feature, "avx512") == 0) return caps->features.has_avx512;
    if (strcmp(feature, "fma") == 0) return caps->features.has_fma;
    if (strcmp(feature, "aes") == 0) return caps->features.has_aes;
    if (strcmp(feature, "popcnt") == 0) return caps->features.has_popcnt;

    return false;
}

int hardware_detector_get_simd_width(const HardwareCapabilities *caps) {
    if (!caps) return 1;

    if (caps->features.has_avx512) return 512;
    if (caps->features.has_avx2) return 256;
    if (caps->features.has_avx) return 256;
    if (caps->features.has_sse4_2) return 128;
    if (caps->features.has_sse2) return 128;

    return 1;  // Scalar
}
