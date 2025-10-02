#include "cpu_features.h"
#include <string.h>

#ifdef __x86_64__
#include <cpuid.h>

static uint32_t detected_features = 0;
static bool features_detected = false;

uint32_t cpu_detect_features(void) {
    if (features_detected) {
        return detected_features;
    }
    
    uint32_t eax, ebx, ecx, edx;
    
    // Check CPUID support
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return 0;
    }
    
    // SSE2 (always present on x86-64)
    detected_features |= CPU_FEATURE_SSE2;
    
    // SSE3
    if (ecx & bit_SSE3) {
        detected_features |= CPU_FEATURE_SSE3;
    }
    
    // SSSE3
    if (ecx & bit_SSSE3) {
        detected_features |= CPU_FEATURE_SSSE3;
    }
    
    // SSE4.1
    if (ecx & bit_SSE4_1) {
        detected_features |= CPU_FEATURE_SSE4_1;
    }
    
    // SSE4.2
    if (ecx & bit_SSE4_2) {
        detected_features |= CPU_FEATURE_SSE4_2;
    }
    
    // AVX
    if (ecx & bit_AVX) {
        detected_features |= CPU_FEATURE_AVX;
    }
    
    // Check extended features
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        // AVX2
        if (ebx & bit_AVX2) {
            detected_features |= CPU_FEATURE_AVX2;
        }
        
        // AVX-512
        if (ebx & bit_AVX512F) {
            detected_features |= CPU_FEATURE_AVX512F;
        }
        if (ebx & bit_AVX512BW) {
            detected_features |= CPU_FEATURE_AVX512BW;
        }
        if (ebx & bit_AVX512VL) {
            detected_features |= CPU_FEATURE_AVX512VL;
        }
    }
    
    features_detected = true;
    return detected_features;
}

bool cpu_has_feature(cpu_feature_t feature) {
    if (!features_detected) {
        cpu_detect_features();
    }
    return (detected_features & feature) != 0;
}

void cpu_get_vendor(char* vendor) {
    uint32_t ebx, ecx, edx;
    __get_cpuid(0, &ebx, &ebx, &ecx, &edx);
    memcpy(vendor, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';
}

void cpu_get_brand(char* brand) {
    uint32_t regs[12];
    for (int i = 0; i < 3; i++) {
        __get_cpuid(0x80000002 + i, &regs[i*4], &regs[i*4+1], 
                    &regs[i*4+2], &regs[i*4+3]);
    }
    memcpy(brand, regs, 48);
    brand[48] = '\0';
}

#else
// Non-x86 stub
uint32_t cpu_detect_features(void) { return 0; }
bool cpu_has_feature(cpu_feature_t feature) { (void)feature; return false; }
void cpu_get_vendor(char* vendor) { strcpy(vendor, "Unknown"); }
void cpu_get_brand(char* brand) { strcpy(brand, "Unknown"); }
#endif
