// BCI SIMD Tests
#include "../bci/bci_simd.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

static int tests_passed = 0;

#define TEST(name) printf("Testing %s... ", name); fflush(stdout);
#define PASS() printf("PASS\n"); tests_passed++;

void test_avx2_detection(void) {
    TEST("AVX2 detection");
    bool has_avx2 = bci_has_avx2_support();
    printf("AVX2 %s... ", has_avx2 ? "available" : "not available");
    PASS();
}

void test_scalar_operations(void) {
    TEST("scalar fallback operations");
    
    uint32_t a[16], b[16], result[16];
    for (int i = 0; i < 16; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    binary_add_vec_scalar(a, b, result, 16);
    for (int i = 0; i < 16; i++) {
        assert(result[i] == i + i * 2);
    }
    
    binary_xor_vec_scalar(a, b, result, 16);
    for (int i = 0; i < 16; i++) {
        assert(result[i] == (i ^ (i * 2)));
    }
    
    PASS();
}

#if BCI_HAS_AVX2
void test_avx2_add(void) {
    TEST("AVX2 vectorized addition");
    
    uint32_t a[32], b[32], result[32];
    for (int i = 0; i < 32; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    binary_add_vec_avx2(a, b, result, 32);
    for (int i = 0; i < 32; i++) {
        assert(result[i] == i + i * 2);
    }
    
    PASS();
}

void test_avx2_xor(void) {
    TEST("AVX2 vectorized XOR");
    
    uint32_t a[32], b[32], result[32];
    for (int i = 0; i < 32; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    binary_xor_vec_avx2(a, b, result, 32);
    for (int i = 0; i < 32; i++) {
        assert(result[i] == (i ^ (i * 2)));
    }
    
    PASS();
}
#endif

int main(void) {
    printf("=== BCI SIMD Tests ===\n\n");
    
    test_avx2_detection();
    test_scalar_operations();
    
#if BCI_HAS_AVX2
    test_avx2_add();
    test_avx2_xor();
#endif
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    return 0;
}
