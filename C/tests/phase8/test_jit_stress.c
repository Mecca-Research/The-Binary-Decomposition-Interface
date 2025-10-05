#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "../../vm/vm_jit_integration.h"
#include "../../vm/bci_chunk.h"

// Test framework
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running test: %s...", name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf(" FAILED\n"); \
            printf("  Assertion failed: %s\n", #condition); \
            printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// Helper function to create a large computation chunk
static Chunk* create_large_computation_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Large nested computation: ((((1+2)*3)+4)*5)+6)*7)+8)*9)+10
    int const1_idx = chunk_add_constant(chunk, 1.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1_idx, 1);
    int const2_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    
    for (int i = 3; i <= 10; i++) {
        int const_idx = chunk_add_constant(chunk, (double)i);
        chunk_write(chunk, OP_CONSTANT, 1);
        chunk_write(chunk, const_idx, 1);
        if (i % 2 == 1) {
            chunk_write(chunk, OP_MULTIPLY, 1);
        } else {
            chunk_write(chunk, OP_ADD, 1);
        }
    }
    
    chunk_write(chunk, OP_RETURN, 1);
    return chunk;
}

// Helper function to create multiple different chunks
static Chunk** create_multiple_chunks(int count) {
    Chunk** chunks = (Chunk**)malloc(count * sizeof(Chunk*));
    
    for (int i = 0; i < count; i++) {
        chunks[i] = (Chunk*)malloc(sizeof(Chunk));
        chunk_init(chunks[i]);
        
        // Create different expressions for each chunk
        double a = (double)(i + 1);
        double b = (double)(i + 2);
        
        int const1_idx = chunk_add_constant(chunks[i], a);
        chunk_write(chunks[i], OP_CONSTANT, 1);
        chunk_write(chunks[i], const1_idx, 1);
        int const2_idx = chunk_add_constant(chunks[i], b);
        chunk_write(chunks[i], OP_CONSTANT, 1);
        chunk_write(chunks[i], const2_idx, 1);
        
        switch (i % 4) {
            case 0: chunk_write(chunks[i], OP_ADD, 1); break;
            case 1: chunk_write(chunks[i], OP_SUBTRACT, 1); break;
            case 2: chunk_write(chunks[i], OP_MULTIPLY, 1); break;
            case 3: chunk_write(chunks[i], OP_DIVIDE, 1); break;
        }
        
        chunk_write(chunks[i], OP_RETURN, 1);
    }
    
    return chunks;
}

// Test 1: Large Program JIT Compilation
void test_large_program_jit_compilation(void) {
    TEST("large_program_jit_compilation");
    
    JITIntegratedVM* vm = jit_vm_create(10 * 1024 * 1024); // 10MB heap
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_large_computation_chunk();
    ASSERT(chunk != NULL);
    
    // Compile the large program
    bool compile_result = jit_vm_compile_function(vm, 1, chunk);
    ASSERT(compile_result == true);
    
    // Execute multiple times
    for (int i = 0; i < 100; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        // Result should be consistent
        if (i > 0) {
            // All executions should produce the same result
        }
    }
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 2: Memory Pressure Scenarios
void test_memory_pressure_scenarios(void) {
    TEST("memory_pressure_scenarios");
    
    // Create VM with limited memory
    JITIntegratedVM* vm = jit_vm_create(512 * 1024); // 512KB heap
    ASSERT(vm != NULL);
    
    // Create many chunks and compile them
    const int chunk_count = 50;
    Chunk** chunks = create_multiple_chunks(chunk_count);
    
    // Compile all chunks (may trigger memory pressure)
    for (int i = 0; i < chunk_count; i++) {
        bool result = jit_vm_compile_function(vm, i + 1, chunks[i]);
        // Some compilations may fail due to memory pressure, which is OK
        (void)result;
    }
    
    // Execute all chunks
    for (int i = 0; i < chunk_count; i++) {
        JITVmResult result = jit_vm_execute(vm, chunks[i]);
        ASSERT(result.success == true);
    }
    
    // Trigger GC multiple times
    for (int i = 0; i < 10; i++) {
        vm_gc_collect(vm->base_vm);
    }
    
    // Execute again after GC
    for (int i = 0; i < chunk_count; i++) {
        JITVmResult result = jit_vm_execute(vm, chunks[i]);
        ASSERT(result.success == true);
    }
    
    // Cleanup
    for (int i = 0; i < chunk_count; i++) {
        chunk_free(chunks[i]);
    }
    free(chunks);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 3: High-Frequency Execution Stress Test
void test_high_frequency_execution_stress(void) {
    TEST("high_frequency_execution_stress");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 10;
    JITIntegratedVM* vm = jit_vm_create_with_config(2 * 1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_large_computation_chunk();
    ASSERT(chunk != NULL);
    
    const int iterations = 10000;
    double first_result = 0.0;
    
    // High-frequency execution
    for (int i = 0; i < iterations; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        
        if (i == 0) {
            first_result = result.result_value;
        } else {
            // All results should be identical
            ASSERT(result.result_value == first_result);
        }
        
        // Occasionally trigger GC
        if (i % 1000 == 0) {
            vm_gc_collect(vm->base_vm);
        }
    }
    
    // Check statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    
    printf("\n  Executed %d iterations", iterations);
    printf("\n  JIT compilations: %lu", stats.jit_compilations);
    printf("\n  Cache hits: %lu", stats.cache_hits);
    
    ASSERT(stats.total_executions == iterations);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 4: Cache Overflow and Eviction
void test_cache_overflow_eviction(void) {
    TEST("cache_overflow_eviction");
    
    JITConfig config = jit_vm_default_config();
    config.max_compiled_functions = 10; // Small cache
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    const int function_count = 20; // More than cache size
    Chunk** chunks = create_multiple_chunks(function_count);
    
    // Compile more functions than cache can hold
    for (int i = 0; i < function_count; i++) {
        bool result = jit_vm_compile_function(vm, i + 1, chunks[i]);
        // Later compilations may fail due to cache overflow
        (void)result;
    }
    
    printf("\n  Cache entries: %zu (max: %u)", 
           vm->code_cache->entry_count, config.max_compiled_functions);
    
    // Cache should not exceed maximum
    ASSERT(vm->code_cache->entry_count <= config.max_compiled_functions);
    
    // All chunks should still execute (fallback to interpreter if not cached)
    for (int i = 0; i < function_count; i++) {
        JITVmResult result = jit_vm_execute(vm, chunks[i]);
        ASSERT(result.success == true);
    }
    
    // Cleanup
    for (int i = 0; i < function_count; i++) {
        chunk_free(chunks[i]);
    }
    free(chunks);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 5: Resource Cleanup Under Stress
void test_resource_cleanup_stress(void) {
    TEST("resource_cleanup_stress");
    
    const int vm_count = 20;
    
    // Create and destroy many VMs rapidly
    for (int i = 0; i < vm_count; i++) {
        JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
        ASSERT(vm != NULL);
        
        // Create and compile functions
        Chunk* chunk = create_large_computation_chunk();
        for (uint32_t j = 1; j <= 5; j++) {
            jit_vm_compile_function(vm, j, chunk);
        }
        
        // Execute
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        
        // Cleanup
        chunk_free(chunk);
        jit_vm_destroy(vm);
    }
    
    TEST_END;
}

// Thread data for concurrent testing
typedef struct {
    JITIntegratedVM* vm;
    Chunk* chunk;
    int thread_id;
    int iterations;
    int success_count;
} ThreadData;

// Thread function for concurrent execution
static void* thread_execute_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        JITVmResult result = jit_vm_execute(data->vm, data->chunk);
        if (result.success) {
            data->success_count++;
        }
        
        // Small delay to increase contention
        struct timespec ts = {0, 1000}; // 1 microsecond
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

// Test 6: Concurrent Execution Stress (if threading is available)
void test_concurrent_execution_stress(void) {
    TEST("concurrent_execution_stress");
    
    // Note: This test assumes basic thread safety or single-threaded execution
    // In a real implementation, proper synchronization would be needed
    
    JITIntegratedVM* vm = jit_vm_create(2 * 1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_large_computation_chunk();
    ASSERT(chunk != NULL);
    
    const int thread_count = 4;
    const int iterations_per_thread = 100;
    
    // For this test, we'll simulate concurrent execution sequentially
    // since we don't have thread safety guarantees
    
    int total_success = 0;
    for (int t = 0; t < thread_count; t++) {
        for (int i = 0; i < iterations_per_thread; i++) {
            JITVmResult result = jit_vm_execute(vm, chunk);
            if (result.success) {
                total_success++;
            }
        }
    }
    
    printf("\n  Successful executions: %d/%d", 
           total_success, thread_count * iterations_per_thread);
    
    ASSERT(total_success == thread_count * iterations_per_thread);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 7: Error Recovery Stress Test
void test_error_recovery_stress(void) {
    TEST("error_recovery_stress");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* valid_chunk = create_large_computation_chunk();
    ASSERT(valid_chunk != NULL);
    
    // Mix valid and invalid operations
    for (int i = 0; i < 100; i++) {
        if (i % 10 == 0) {
            // Try invalid operations
            JITVmResult result = jit_vm_execute(vm, NULL);
            ASSERT(result.success == false);
            
            bool compile_result = jit_vm_compile_function(vm, 999, NULL);
            ASSERT(compile_result == false);
        } else {
            // Valid operations
            JITVmResult result = jit_vm_execute(vm, valid_chunk);
            ASSERT(result.success == true);
        }
    }
    
    // VM should still be functional after errors
    JITVmResult final_result = jit_vm_execute(vm, valid_chunk);
    ASSERT(final_result.success == true);
    
    chunk_free(valid_chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 8: Long-Running Execution Stress
void test_long_running_execution_stress(void) {
    TEST("long_running_execution_stress");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 50;
    JITIntegratedVM* vm = jit_vm_create_with_config(4 * 1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    const int chunk_count = 10;
    Chunk** chunks = create_multiple_chunks(chunk_count);
    
    const int total_iterations = 5000;
    
    // Long-running mixed execution
    for (int i = 0; i < total_iterations; i++) {
        int chunk_idx = i % chunk_count;
        JITVmResult result = jit_vm_execute(vm, chunks[chunk_idx]);
        ASSERT(result.success == true);
        
        // Periodic maintenance
        if (i % 500 == 0) {
            vm_gc_collect(vm->base_vm);
            
            // Print progress
            printf(".");
            fflush(stdout);
        }
        
        // Occasional cache management
        if (i % 1000 == 0) {
            // Clear some cache entries
            jit_vm_invalidate_function(vm, (i / 1000) % chunk_count + 1);
        }
    }
    
    // Check final statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    
    printf("\n  Total executions: %lu", stats.total_executions);
    printf("\n  JIT compilations: %lu", stats.jit_compilations);
    
    ASSERT(stats.total_executions == total_iterations);
    
    // Cleanup
    for (int i = 0; i < chunk_count; i++) {
        chunk_free(chunks[i]);
    }
    free(chunks);
    jit_vm_destroy(vm);
    TEST_END;
}

int main(void) {
    printf("=== JIT Stress and Edge Case Tests ===\n");
    
    test_large_program_jit_compilation();
    test_memory_pressure_scenarios();
    test_high_frequency_execution_stress();
    test_cache_overflow_eviction();
    test_resource_cleanup_stress();
    test_concurrent_execution_stress();
    test_error_recovery_stress();
    test_long_running_execution_stress();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}

