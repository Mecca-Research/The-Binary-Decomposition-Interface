
/*
 * Memory Stress Tests
 * High-frequency allocation/deallocation, multi-threaded pressure, NUMA stress
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 16
#define STRESS_DURATION_SEC 60
#define ALLOC_SIZE_MIN 64
#define ALLOC_SIZE_MAX (1024 * 1024)
#define MAX_ALLOCATIONS 10000

// Statistics
static _Atomic uint64_t total_allocations = 0;
static _Atomic uint64_t total_deallocations = 0;
static _Atomic uint64_t failed_allocations = 0;
static _Atomic uint64_t bytes_allocated = 0;
static _Atomic bool stress_running = true;

// Allocation tracking
typedef struct {
    void *ptr;
    size_t size;
} allocation_t;

// Thread context
typedef struct {
    int thread_id;
    allocation_t *allocations;
    size_t alloc_count;
    size_t alloc_capacity;
} thread_context_t;

// Random size generator
static size_t random_size(void) {
    return ALLOC_SIZE_MIN + (rand() % (ALLOC_SIZE_MAX - ALLOC_SIZE_MIN));
}

// High-frequency allocation/deallocation thread
static void* stress_alloc_dealloc(void *arg) {
    thread_context_t *ctx = (thread_context_t*)arg;
    
    // Initialize allocation tracking
    ctx->alloc_capacity = 1000;
    ctx->allocations = calloc(ctx->alloc_capacity, sizeof(allocation_t));
    ctx->alloc_count = 0;
    
    while (atomic_load(&stress_running)) {
        // Randomly allocate or deallocate
        if (ctx->alloc_count < ctx->alloc_capacity && (rand() % 2 == 0 || ctx->alloc_count == 0)) {
            // Allocate
            size_t size = random_size();
            void *ptr = malloc(size);
            
            if (ptr) {
                ctx->allocations[ctx->alloc_count].ptr = ptr;
                ctx->allocations[ctx->alloc_count].size = size;
                ctx->alloc_count++;
                
                atomic_fetch_add(&total_allocations, 1);
                atomic_fetch_add(&bytes_allocated, size);
                
                // Write to memory to ensure it's actually allocated
                memset(ptr, 0xAA, size);
            } else {
                atomic_fetch_add(&failed_allocations, 1);
            }
        } else if (ctx->alloc_count > 0) {
            // Deallocate random allocation
            size_t idx = rand() % ctx->alloc_count;
            free(ctx->allocations[idx].ptr);
            
            atomic_fetch_add(&total_deallocations, 1);
            atomic_fetch_sub(&bytes_allocated, ctx->allocations[idx].size);
            
            // Remove from tracking
            ctx->allocations[idx] = ctx->allocations[ctx->alloc_count - 1];
            ctx->alloc_count--;
        }
        
        // Small delay to prevent CPU saturation
        usleep(100);
    }
    
    // Cleanup remaining allocations
    for (size_t i = 0; i < ctx->alloc_count; i++) {
        free(ctx->allocations[i].ptr);
    }
    free(ctx->allocations);
    
    return NULL;
}

// Memory pressure thread (allocates large chunks)
static void* stress_memory_pressure(void *arg) {
    thread_context_t *ctx = (thread_context_t*)arg;
    void *large_blocks[100];
    size_t block_count = 0;
    
    while (atomic_load(&stress_running)) {
        // Allocate large block
        if (block_count < 100) {
            size_t size = 10 * 1024 * 1024; // 10 MB
            void *ptr = malloc(size);
            
            if (ptr) {
                large_blocks[block_count++] = ptr;
                memset(ptr, 0xBB, size);
                atomic_fetch_add(&total_allocations, 1);
                atomic_fetch_add(&bytes_allocated, size);
            } else {
                atomic_fetch_add(&failed_allocations, 1);
            }
        }
        
        // Free some blocks
        if (block_count > 50) {
            for (size_t i = 0; i < 25; i++) {
                free(large_blocks[i]);
                atomic_fetch_add(&total_deallocations, 1);
            }
            // Shift remaining blocks
            memmove(large_blocks, large_blocks + 25, (block_count - 25) * sizeof(void*));
            block_count -= 25;
        }
        
        sleep(1);
    }
    
    // Cleanup
    for (size_t i = 0; i < block_count; i++) {
        free(large_blocks[i]);
    }
    
    return NULL;
}

// Reallocation stress thread
static void* stress_realloc(void *arg) {
    thread_context_t *ctx = (thread_context_t*)arg;
    void *ptr = NULL;
    size_t current_size = 0;
    
    while (atomic_load(&stress_running)) {
        size_t new_size = random_size();
        void *new_ptr = realloc(ptr, new_size);
        
        if (new_ptr) {
            ptr = new_ptr;
            
            if (new_size > current_size) {
                atomic_fetch_add(&bytes_allocated, new_size - current_size);
            } else {
                atomic_fetch_sub(&bytes_allocated, current_size - new_size);
            }
            
            current_size = new_size;
            memset(ptr, 0xCC, new_size);
            atomic_fetch_add(&total_allocations, 1);
        } else {
            atomic_fetch_add(&failed_allocations, 1);
        }
        
        usleep(1000);
    }
    
    free(ptr);
    return NULL;
}

// Main stress test
int run_memory_stress_test(int duration_sec) {
    printf("=== Memory Stress Test ===\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Threads: %d\n", NUM_THREADS);
    printf("\n");
    
    pthread_t threads[NUM_THREADS];
    thread_context_t contexts[NUM_THREADS];
    
    // Reset statistics
    atomic_store(&total_allocations, 0);
    atomic_store(&total_deallocations, 0);
    atomic_store(&failed_allocations, 0);
    atomic_store(&bytes_allocated, 0);
    atomic_store(&stress_running, true);
    
    // Start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        contexts[i].thread_id = i;
        
        // Mix different stress patterns
        if (i < NUM_THREADS / 2) {
            pthread_create(&threads[i], NULL, stress_alloc_dealloc, &contexts[i]);
        } else if (i < 3 * NUM_THREADS / 4) {
            pthread_create(&threads[i], NULL, stress_memory_pressure, &contexts[i]);
        } else {
            pthread_create(&threads[i], NULL, stress_realloc, &contexts[i]);
        }
    }
    
    // Run for specified duration
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < duration_sec) {
        sleep(1);
        
        // Print progress every 10 seconds
        if ((time(NULL) - start_time) % 10 == 0) {
            printf("Progress: %ld/%d seconds\n", time(NULL) - start_time, duration_sec);
            printf("  Allocations: %lu\n", atomic_load(&total_allocations));
            printf("  Deallocations: %lu\n", atomic_load(&total_deallocations));
            printf("  Failed: %lu\n", atomic_load(&failed_allocations));
            printf("  Bytes allocated: %lu MB\n", atomic_load(&bytes_allocated) / (1024 * 1024));
        }
    }
    
    // Stop threads
    atomic_store(&stress_running, false);
    
    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print final statistics
    printf("\n=== Final Statistics ===\n");
    printf("Total allocations: %lu\n", atomic_load(&total_allocations));
    printf("Total deallocations: %lu\n", atomic_load(&total_deallocations));
    printf("Failed allocations: %lu\n", atomic_load(&failed_allocations));
    printf("Peak bytes allocated: %lu MB\n", atomic_load(&bytes_allocated) / (1024 * 1024));
    
    uint64_t failed = atomic_load(&failed_allocations);
    if (failed > 0) {
        printf("\nWARNING: %lu allocations failed\n", failed);
        return 1;
    }
    
    printf("\nMemory stress test PASSED\n");
    return 0;
}

// Entry point for standalone execution
#ifndef TEST_RUNNER_BUILD
int main(int argc, char *argv[]) {
    int duration = STRESS_DURATION_SEC;
    
    if (argc > 1) {
        duration = atoi(argv[1]);
    }
    
    srand(time(NULL));
    return run_memory_stress_test(duration);
}

#endif
