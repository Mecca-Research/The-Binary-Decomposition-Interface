
/**
 * @file bench_rings.c
 * @brief Benchmarks for ring buffers
 */

#include "../rings/spsc_ring.h"
#include "../rings/mpsc_ring.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define ITERATIONS 1000000

// Get timestamp in nanoseconds
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Benchmark SPSC ring
void bench_spsc_ring(void) {
    printf("Benchmarking SPSC ring buffer...\n");
    
    spsc_ring_t* ring = spsc_ring_create(1024);
    
    // Benchmark enqueue
    uint64_t start = get_timestamp_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        while (spsc_ring_enqueue(ring, data) != RING_SUCCESS) {
            void* dummy;
            spsc_ring_dequeue(ring, &dummy);
        }
    }
    uint64_t end = get_timestamp_ns();
    
    double enqueue_ns = (double)(end - start) / ITERATIONS;
    printf("  SPSC enqueue: %.2f ns/op\n", enqueue_ns);
    
    // Benchmark dequeue
    start = get_timestamp_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        void* data = NULL;
        while (spsc_ring_dequeue(ring, &data) != RING_SUCCESS) {
            void* dummy = (void*)(uintptr_t)(i + 1);
            spsc_ring_enqueue(ring, dummy);
        }
    }
    end = get_timestamp_ns();
    
    double dequeue_ns = (double)(end - start) / ITERATIONS;
    printf("  SPSC dequeue: %.2f ns/op\n", dequeue_ns);
    
    spsc_ring_destroy(ring);
}

// Benchmark MPSC ring
void bench_mpsc_ring(void) {
    printf("Benchmarking MPSC ring buffer...\n");
    
    mpsc_ring_t* ring = mpsc_ring_create(1024);
    
    // Benchmark enqueue (single producer)
    uint64_t start = get_timestamp_ns();
    for (int i = 0; i < ITERATIONS; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        while (mpsc_ring_enqueue(ring, data) != RING_SUCCESS) {
            void* dummy;
            mpsc_ring_dequeue(ring, &dummy);
        }
    }
    uint64_t end = get_timestamp_ns();
    
    double enqueue_ns = (double)(end - start) / ITERATIONS;
    printf("  MPSC enqueue: %.2f ns/op\n", enqueue_ns);
    
    // Get CAS failures
    uint64_t cas_failures = mpsc_ring_get_cas_failures(ring);
    printf("  MPSC CAS failures: %lu\n", cas_failures);
    
    mpsc_ring_destroy(ring);
}

// Compare with syscall overhead
void bench_syscall_comparison(void) {
    printf("\nSyscall overhead comparison:\n");
    
    // Benchmark getpid() syscall
    uint64_t start = get_timestamp_ns();
    for (int i = 0; i < 10000; i++) {
        getpid();
    }
    uint64_t end = get_timestamp_ns();
    
    double syscall_ns = (double)(end - start) / 10000;
    printf("  getpid() syscall: %.2f ns/op\n", syscall_ns);
    
    // Compare with SPSC ring
    spsc_ring_t* ring = spsc_ring_create(1024);
    start = get_timestamp_ns();
    for (int i = 0; i < 10000; i++) {
        void* data = (void*)(uintptr_t)(i + 1);
        spsc_ring_enqueue(ring, data);
        spsc_ring_dequeue(ring, &data);
    }
    end = get_timestamp_ns();
    
    double ring_ns = (double)(end - start) / 10000;
    printf("  SPSC ring (enqueue+dequeue): %.2f ns/op\n", ring_ns);
    printf("  Speedup: %.2fx faster than syscall\n", syscall_ns / ring_ns);
    
    spsc_ring_destroy(ring);
}

int main(void) {
    printf("=== Ring Buffer Benchmarks ===\n\n");
    
    bench_spsc_ring();
    printf("\n");
    bench_mpsc_ring();
    printf("\n");
    bench_syscall_comparison();
    
    printf("\n=== Benchmarks complete ===\n");
    return 0;
}
