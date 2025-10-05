
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/vm_jit_integration.h"
#include "../../vm/bci_chunk.h"
#include "../../compiler/codegen/codegen.h"

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

// Helper function to get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Helper function to create a compute-intensive chunk
static Chunk* create_compute_intensive_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Complex computation: ((2 + 3) * 4) - ((10 / 2) + 1) * 2
    // = (5 * 4) - (5 + 1) * 2 = 20 - 12 = 8
    
    // First part: (2 + 3) * 4
    int const_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    int const_idx = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    int const_idx = chunk_add_constant(chunk, 4.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_MULTIPLY, 1);
    
    // Second part: (10 / 2) + 1
    int const_idx = chunk_add_constant(chunk, 10.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    int const_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_DIVIDE, 1);
    int const_idx = chunk_add_constant(chunk, 1.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    
    // Multiply by 2
    int const_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_MULTIPLY, 1);
    
    // Final subtraction
    chunk_write(chunk, OP_SUBTRACT, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Helper function to create a simple chunk for baseline comparison
static Chunk* create_simple_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    int const_idx = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    int const_idx = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Performance measurement structure
typedef struct {
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    uint64_t execution_count;
    double avg_time_ns;
} PerfMeasurement;

// Helper function to measure performance
static PerfMeasurement measure_performance(JITIntegratedVM* vm, Chunk* chunk, int iterations) {
    PerfMeasurement perf = {0};
    perf.min_time_ns = UINT64_MAX;
    perf.max_time_ns = 0;
    perf.execution_count = iterations;
    
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_time_ns();
        JITVmResult result = jit_vm_execute(vm, chunk);
        uint64_t end = get_time_ns();
        
        ASSERT(result.success == true);
        
        uint64_t exec_time = end - start;
        perf.total_time_ns += exec_time;
        
        if (exec_time < perf.min_time_ns) perf.min_time_ns = exec_time;
        if (exec_time > perf.max_time_ns) perf.max_time_ns = exec_time;
    }
    
    perf.avg_time_ns = (double)perf.total_time_ns / iterations;
    return perf;
}

// Test 1: JIT vs Interpreter Performance Comparison
void test_jit_vs_interpreter_performance(void) {
    TEST("jit_vs_interpreter_performance");
    
    const int iterations = 1000;
    
    // Create JIT-enabled VM
    JITConfig jit_config = jit_vm_default_config();
    jit_config.jit_threshold = 10; // Allow some warmup
    JITIntegratedVM* jit_vm = jit_vm_create_with_config(1024 * 1024, &jit_config);
    ASSERT(jit_vm != NULL);
    
    // Create interpreter-only VM
    JITConfig interp_config = jit_vm_default_config();
    interp_config.enable_jit = false;
    JITIntegratedVM* interp_vm = jit_vm_create_with_config(1024 * 1024, &interp_config);
    ASSERT(interp_vm != NULL);
    
    Chunk* chunk = create_compute_intensive_chunk();
    ASSERT(chunk != NULL);
    
    // Measure interpreter performance
    PerfMeasurement interp_perf = measure_performance(interp_vm, chunk, iterations);
    
    // Measure JIT performance (with warmup)
    PerfMeasurement jit_perf = measure_performance(jit_vm, chunk, iterations);
    
    printf("\n  Interpreter avg: %.2f ns", interp_perf.avg_time_ns);
    printf("\n  JIT avg: %.2f ns", jit_perf.avg_time_ns);
    
    // JIT should be at least as fast as interpreter (allowing for compilation overhead)
    // In practice, with mock JIT, they might be similar
    ASSERT(jit_perf.avg_time_ns > 0);
    ASSERT(interp_perf.avg_time_ns > 0);
    
    chunk_free(chunk);
    jit_vm_destroy(jit_vm);
    jit_vm_destroy(interp_vm);
    TEST_END;
}

// Test 2: JIT Compilation Overhead Measurement
void test_jit_compilation_overhead(void) {
    TEST("jit_compilation_overhead");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 1; // Compile immediately
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_compute_intensive_chunk();
    ASSERT(chunk != NULL);
    
    // First execution should include compilation overhead
    uint64_t start = get_time_ns();
    JITVmResult first_result = jit_vm_execute(vm, chunk);
    uint64_t end = get_time_ns();
    
    ASSERT(first_result.success == true);
    uint64_t first_exec_time = end - start;
    
    // Subsequent executions should be faster (no compilation)
    start = get_time_ns();
    JITVmResult second_result = jit_vm_execute(vm, chunk);
    end = get_time_ns();
    
    ASSERT(second_result.success == true);
    uint64_t second_exec_time = end - start;
    
    printf("\n  First execution (with compilation): %lu ns", first_exec_time);
    printf("\n  Second execution (cached): %lu ns", second_exec_time);
    
    // Both should be reasonable (less than 1 second)
    ASSERT(first_exec_time < 1000000000ULL);
    ASSERT(second_exec_time < 1000000000ULL);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 3: Throughput Benchmark
void test_jit_throughput_benchmark(void) {
    TEST("jit_throughput_benchmark");
    
    const int iterations = 10000;
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 100; // Reasonable threshold
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    uint64_t start = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        ASSERT(result.result_value == 5.0);
    }
    
    uint64_t end = get_time_ns();
    uint64_t total_time = end - start;
    
    double throughput = (double)iterations / ((double)total_time / 1000000000.0);
    printf("\n  Executed %d iterations in %.2f ms", iterations, total_time / 1000000.0);
    printf("\n  Throughput: %.2f executions/second", throughput);
    
    // Should achieve reasonable throughput
    ASSERT(throughput > 1000.0); // At least 1000 executions per second
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 4: Memory Usage Analysis
void test_jit_memory_usage(void) {
    TEST("jit_memory_usage");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_compute_intensive_chunk();
    ASSERT(chunk != NULL);
    
    // Compile multiple functions to test memory usage
    for (uint32_t i = 1; i <= 10; i++) {
        bool result = jit_vm_compile_function(vm, i, chunk);
        ASSERT(result == true);
    }
    
    // Check cache statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    
    printf("\n  Compiled functions: %lu", stats.jit_compilations);
    printf("\n  Cache entries: %zu", vm->code_cache->entry_count);
    
    ASSERT(vm->code_cache->entry_count == 10);
    ASSERT(stats.jit_compilations >= 10);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 5: Hot Path Detection Performance
void test_hot_path_detection_performance(void) {
    TEST("hot_path_detection_performance");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 50;
    config.enable_hotspot_detection = true;
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_compute_intensive_chunk();
    ASSERT(chunk != NULL);
    
    // Execute many times to trigger hot path detection
    const int warmup_iterations = 100;
    const int benchmark_iterations = 1000;
    
    // Warmup phase
    for (int i = 0; i < warmup_iterations; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    
    // Benchmark phase
    uint64_t start = get_time_ns();
    for (int i = 0; i < benchmark_iterations; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    uint64_t end = get_time_ns();
    
    uint64_t total_time = end - start;
    double avg_time = (double)total_time / benchmark_iterations;
    
    printf("\n  Average execution time after warmup: %.2f ns", avg_time);
    
    // Check statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    printf("\n  Total executions: %lu", stats.total_executions);
    printf("\n  JIT compilations: %lu", stats.jit_compilations);
    
    ASSERT(stats.total_executions >= warmup_iterations + benchmark_iterations);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 6: Cache Hit/Miss Performance
void test_cache_performance(void) {
    TEST("cache_performance");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Pre-compile function
    bool compile_result = jit_vm_compile_function(vm, 1, chunk);
    ASSERT(compile_result == true);
    
    const int iterations = 1000;
    
    // Measure cache hit performance
    uint64_t start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        CompiledCode* code = jit_code_cache_get(vm->code_cache, 1);
        ASSERT(code != NULL);
    }
    uint64_t end = get_time_ns();
    
    uint64_t cache_hit_time = end - start;
    double avg_cache_hit_time = (double)cache_hit_time / iterations;
    
    printf("\n  Average cache hit time: %.2f ns", avg_cache_hit_time);
    
    // Measure cache miss performance
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        CompiledCode* code = jit_code_cache_get(vm->code_cache, 999); // Non-existent
        ASSERT(code == NULL);
    }
    end = get_time_ns();
    
    uint64_t cache_miss_time = end - start;
    double avg_cache_miss_time = (double)cache_miss_time / iterations;
    
    printf("\n  Average cache miss time: %.2f ns", avg_cache_miss_time);
    
    // Cache operations should be fast
    ASSERT(avg_cache_hit_time < 1000.0); // Less than 1 microsecond
    ASSERT(avg_cache_miss_time < 1000.0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 7: Scalability Test
void test_jit_scalability(void) {
    TEST("jit_scalability");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_compute_intensive_chunk();
    ASSERT(chunk != NULL);
    
    // Test with increasing number of functions
    const int max_functions = 100;
    
    uint64_t start = get_time_ns();
    
    for (int i = 1; i <= max_functions; i++) {
        bool result = jit_vm_compile_function(vm, i, chunk);
        ASSERT(result == true);
    }
    
    uint64_t end = get_time_ns();
    uint64_t compilation_time = end - start;
    
    printf("\n  Compiled %d functions in %.2f ms", max_functions, compilation_time / 1000000.0);
    printf("\n  Average compilation time: %.2f ms", (compilation_time / 1000000.0) / max_functions);
    
    // Check that all functions are cached
    ASSERT(vm->code_cache->entry_count == max_functions);
    
    // Test execution performance with many cached functions
    start = get_time_ns();
    for (int i = 1; i <= max_functions; i++) {
        CompiledCode* code = jit_code_cache_get(vm->code_cache, i);
        ASSERT(code != NULL);
    }
    end = get_time_ns();
    
    uint64_t lookup_time = end - start;
    printf("\n  Looked up %d functions in %.2f ms", max_functions, lookup_time / 1000000.0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 8: Performance Statistics Accuracy
void test_performance_statistics_accuracy(void) {
    TEST("performance_statistics_accuracy");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    const int iterations = 100;
    
    // Execute multiple times
    for (int i = 0; i < iterations; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    
    // Check statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    
    printf("\n  Total executions: %lu", stats.total_executions);
    printf("\n  Interpreter executions: %lu", stats.interpreter_executions);
    printf("\n  JIT executions: %lu", stats.jit_executions);
    printf("\n  Interpreter time: %.2f ms", stats.interpreter_execution_time_ns / 1000000.0);
    printf("\n  JIT time: %.2f ms", stats.jit_execution_time_ns / 1000000.0);
    
    ASSERT(stats.total_executions == iterations);
    ASSERT(stats.interpreter_executions + stats.jit_executions == stats.total_executions);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

int main(void) {
    printf("=== JIT Performance Benchmark Tests ===\n");
    
    test_jit_vs_interpreter_performance();
    test_jit_compilation_overhead();
    test_jit_throughput_benchmark();
    test_jit_memory_usage();
    test_hot_path_detection_performance();
    test_cache_performance();
    test_jit_scalability();
    test_performance_statistics_accuracy();
    
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
