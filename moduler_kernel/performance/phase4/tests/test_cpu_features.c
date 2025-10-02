#include "../intrinsics/cpu_features.h"
#include <stdio.h>

int main(void) {
    printf("Testing CPU feature detection...\n");
    
    uint32_t features = cpu_detect_features();
    printf("Detected features: 0x%08x\n", features);
    
    printf("SSE2: %s\n", cpu_has_feature(CPU_FEATURE_SSE2) ? "YES" : "NO");
    printf("SSE4.2: %s\n", cpu_has_feature(CPU_FEATURE_SSE4_2) ? "YES" : "NO");
    printf("AVX: %s\n", cpu_has_feature(CPU_FEATURE_AVX) ? "YES" : "NO");
    printf("AVX2: %s\n", cpu_has_feature(CPU_FEATURE_AVX2) ? "YES" : "NO");
    printf("AVX-512: %s\n", cpu_has_feature(CPU_FEATURE_AVX512F) ? "YES" : "NO");
    
    char vendor[13], brand[49];
    cpu_get_vendor(vendor);
    cpu_get_brand(brand);
    printf("CPU Vendor: %s\n", vendor);
    printf("CPU Brand: %s\n", brand);
    
    printf("CPU feature test: PASS\n");
    return 0;
}
