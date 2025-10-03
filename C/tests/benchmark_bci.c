// BCI Performance Benchmarks
#include "../bci/bci_arithmetic.h"
#include "../bci/bci_bitops.h"
#include "../bci/bci_simd.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define ITERATIONS 1000000

double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void benchmark_arithmetic(void) {
    printf("\n=== Arithmetic Benchmarks ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        binary_add_u64(i, i * 2, false);
    }
    double end = get_time_ms();
    printf("binary_add_u64: %.2f ms (%.2f Mops/s)\n", 
           end - start, ITERATIONS / (end - start) / 1000.0);
    
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        binary_mul_u64(i, i * 2);
    }
    end = get_time_ms();
    printf("binary_mul_u64: %.2f ms (%.2f Mops/s)\n", 
           end - start, ITERATIONS / (end - start) / 1000.0);
}

void benchmark_bitops(void) {
    printf("\n=== Bit Operations Benchmarks ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        popcount_u64(i);
    }
    double end = get_time_ms();
    printf("popcount_u64: %.2f ms (%.2f Mops/s)\n", 
           end - start, ITERATIONS / (end - start) / 1000.0);
    
    start = get_time_ms();
    for (int i = 1; i < ITERATIONS; i++) {
        clz_u64(i);
    }
    end = get_time_ms();
    printf("clz_u64: %.2f ms (%.2f Mops/s)\n", 
           end - start, ITERATIONS / (end - start) / 1000.0);
    
    start = get_time_ms();
    for (int i = 1; i < ITERATIONS; i++) {
        ctz_u64(i);
    }
    end = get_time_ms();
    printf("ctz_u64: %.2f ms (%.2f Mops/s)\n", 
           end - start, ITERATIONS / (end - start) / 1000.0);
}

void benchmark_simd(void) {
    printf("\n=== SIMD Benchmarks ===\n");
    
    const size_t SIZE = 1024;
    uint32_t *a = malloc(SIZE * sizeof(uint32_t));
    uint32_t *b = malloc(SIZE * sizeof(uint32_t));
    uint32_t *result = malloc(SIZE * sizeof(uint32_t));
    
    for (size_t i = 0; i < SIZE; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 100; i++) {
        binary_add_vec_scalar(a, b, result, SIZE);
    }
    double end = get_time_ms();
    printf("scalar_add: %.2f ms\n", end - start);
    
#if BCI_HAS_AVX2
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 100; i++) {
        binary_add_vec_avx2(a, b, result, SIZE);
    }
    end = get_time_ms();
    printf("avx2_add: %.2f ms (%.2fx speedup)\n", 
           end - start, (end - start) > 0 ? (end - start) / (end - start) : 0);
#endif
    
    free(a);
    free(b);
    free(result);
}

int main(void) {
    printf("=== BCI Performance Benchmarks ===\n");
    printf("Iterations: %d\n", ITERATIONS);
    
    benchmark_arithmetic();
    benchmark_bitops();
    benchmark_simd();
    
    return 0;
}
