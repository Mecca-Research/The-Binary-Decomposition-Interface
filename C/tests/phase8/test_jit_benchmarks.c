#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/vm_jit_integration.h"
#include "../../vm/bci_chunk.h"

// Test framework
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running benchmark: %s...", name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" COMPLETED\n"); \
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

// Benchmark result structure
typedef struct {
    const char* name;
    uint64_t interpreter_time_ns;
    uint64_t jit_time_ns;
    double speedup_ratio;
    uint64_t iterations;
    bool jit_faster;
} BenchmarkResult;

// Helper function to get time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Helper function to create arithmetic benchmark chunk
static Chunk* create_arithmetic_benchmark_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Complex arithmetic: ((a + b) * c) - ((d / e) + f)
    // Where a=10, b=20, c=3, d=100, e=4, f=5
    // Result: ((10+20)*3) - ((100/4)+5) = (30*3) - (25+5) = 90 - 30 = 60
    
    int const1_idx = chunk_add_constant(chunk, 10.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1_idx, 1);
    int const2_idx = chunk_add_constant(chunk, 20.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    
    int const3_idx = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const3_idx, 1);
    chunk_write(chunk, OP_MULTIPLY, 1);
    
    int const4_idx = chunk_add_constant(chunk, 100.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const4_idx, 1);
    int const5_idx = chunk_add_constant(chunk, 4.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const5_idx, 1);
    chunk_write(chunk, OP_DIVIDE, 1);
    
    int const6_idx = chunk_add_constant(chunk, 5.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const6_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    
    chunk_write(chunk, OP_SUBTRACT, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Helper function to create nested computation benchmark
static Chunk* create_nested_benchmark_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Deeply nested computation
    for (int i = 1; i <= 8; i++) {
        int const_idx = chunk_add_constant(chunk, (double)i);
        chunk_write(chunk, OP_CONSTANT, 1);
        chunk_write(chunk, const_idx, 1);
        if (i > 1) {
            if (i % 2 == 0) {
                chunk_write(chunk, OP_ADD, 1);
            } else {
                chunk_write(chunk, OP_MULTIPLY, 1);
            }
        }
    }
    
    chunk_write(chunk, OP_RETURN, 1);
    return chunk;
}

// Helper function to run benchmark comparison
static BenchmarkResult run_benchmark_comparison(const char* name, Chunk* chunk, int iterations) {
    BenchmarkResult result = {0};
    result.name = name;
    result.iterations = iterations;
    
    // Create interpreter-only VM
    JITConfig interp_config = jit_vm_default_config();
    interp_config.enable_jit = false;
    JITIntegratedVM* interp_vm = jit_vm_create_with_config(2 * 1024 * 1024, &interp_config);
    ASSERT(interp_vm != NULL);
    
    // Create JIT-enabled VM
    JITConfig jit_config = jit_vm_default_config();
    jit_config.jit_threshold = iterations / 10; // Compile after 10% of iterations
    JITIntegratedVM* jit_vm = jit_vm_create_with_config(2 * 1024 * 1024, &jit_config);
    ASSERT(jit_vm != NULL);
    
    // Benchmark interpreter
    uint64_t interp_start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        JITVmResult res = jit_vm_execute(interp_vm, chunk);
        ASSERT(res.success == true);
    }
    uint64_t interp_end = get_time_ns();
    result.interpreter_time_ns = interp_end - interp_start;
    
    // Benchmark JIT (with warmup)
    uint64_t jit_start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        JITVmResult res = jit_vm_execute(jit_vm, chunk);
        ASSERT(res.success == true);
    }
    uint64_t jit_end = get_time_ns();
    result.jit_time_ns = jit_end - jit_start;
    
    // Calculate speedup
    if (result.jit_time_ns > 0) {
        result.speedup_ratio = (double)result.interpreter_time_ns / (double)result.jit_time_ns;
        result.jit_faster = result.speedup_ratio > 1.0;
    }
    
    jit_vm_destroy(interp_vm);
    jit_vm_destroy(jit_vm);
    
    return result;
}

// Helper function to print benchmark results
static void print_benchmark_result(const BenchmarkResult* result) {
    printf("\n  === %s Benchmark Results ===", result->name);
    printf("\n  Iterations: %lu", result->iterations);
    printf("\n  Interpreter time: %.2f ms", result->interpreter_time_ns / 1000000.0);
    printf("\n  JIT time: %.2f ms", result->jit_time_ns / 1000000.0);
    printf("\n  Speedup ratio: %.2fx", result->speedup_ratio);
    printf("\n  JIT faster: %s", result->jit_faster ? "YES" : "NO");
    printf("\n  Interpreter avg: %.2f ns/op", (double)result->interpreter_time_ns / result->iterations);
    printf("\n  JIT avg: %.2f ns/op", (double)result->jit_time_ns / result->iterations);
    printf("\n");
}

// Benchmark 1: Simple Arithmetic Operations
void benchmark_simple_arithmetic(void) {
    TEST("simple_arithmetic");
    
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    int const1_idx = chunk_add_constant(chunk, 42.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1_idx, 1);
    int const2_idx = chunk_add_constant(chunk, 13.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    BenchmarkResult result = run_benchmark_comparison("Simple Arithmetic", chunk, 10000);
    print_benchmark_result(&result);
    
    // Verify correctness
    ASSERT(result.interpreter_time_ns > 0);
    ASSERT(result.jit_time_ns > 0);
    
    chunk_free(chunk);
    TEST_END;
}

// Benchmark 2: Complex Arithmetic Operations
void benchmark_complex_arithmetic(void) {
    TEST("complex_arithmetic");
    
    Chunk* chunk = create_arithmetic_benchmark_chunk();
    BenchmarkResult result = run_benchmark_comparison("Complex Arithmetic", chunk, 5000);
    print_benchmark_result(&result);
    
    ASSERT(result.interpreter_time_ns > 0);
    ASSERT(result.jit_time_ns > 0);
    
    chunk_free(chunk);
    TEST_END;
}

// Benchmark 3: Nested Computations
void benchmark_nested_computations(void) {
    TEST("nested_computations");
    
    Chunk* chunk = create_nested_benchmark_chunk();
    BenchmarkResult result = run_benchmark_comparison("Nested Computations", chunk, 3000);
    print_benchmark_result(&result);
    
    ASSERT(result.interpreter_time_ns > 0);
    ASSERT(result.jit_time_ns > 0);
    
    chunk_free(chunk);
    TEST_END;
}

// Benchmark 4: Compilation Overhead Analysis
void benchmark_compilation_overhead(void) {
    TEST("compilation_overhead");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 1; // Compile immediately
    JITIntegratedVM* vm = jit_vm_create_with_config(2 * 1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_arithmetic_benchmark_chunk();
    
    // Measure first execution (includes compilation)
    uint64_t first_start = get_time_ns();
    JITVmResult first_result = jit_vm_execute(vm, chunk);
    uint64_t first_end = get_time_ns();
    ASSERT(first_result.success == true);
    
    uint64_t first_time = first_end - first_start;
    
    // Measure subsequent executions (no compilation)
    const int subsequent_runs = 100;
    uint64_t subsequent_start = get_time_ns();
    for (int i = 0; i < subsequent_runs; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    uint64_t subsequent_end = get_time_ns();
    
    uint64_t subsequent_total = subsequent_end - subsequent_start;
    double subsequent_avg = (double)subsequent_total / subsequent_runs;
    
    printf("\n  === Compilation Overhead Analysis ===");
    printf("\n  First execution (with compilation): %.2f ms", first_time / 1000000.0);
    printf("\n  Subsequent executions average: %.2f ms", subsequent_avg / 1000000.0);
    printf("\n  Compilation overhead: %.2f ms", (first_time - subsequent_avg) / 1000000.0);
    printf("\n  Overhead ratio: %.2fx", (double)first_time / subsequent_avg);
    printf("\n");
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Benchmark 5: Throughput Comparison
void benchmark_throughput_comparison(void) {
    TEST("throughput_comparison");
    
    const int iterations = 50000;
    
    // Interpreter throughput
    JITConfig interp_config = jit_vm_default_config();
    interp_config.enable_jit = false;
    JITIntegratedVM* interp_vm = jit_vm_create_with_config(2 * 1024 * 1024, &interp_config);
    
    Chunk* chunk = create_arithmetic_benchmark_chunk();
    
    uint64_t interp_start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        JITVmResult result = jit_vm_execute(interp_vm, chunk);
        ASSERT(result.success == true);
    }
    uint64_t interp_end = get_time_ns();
    
    double interp_time_sec = (double)(interp_end - interp_start) / 1000000000.0;
    double interp_throughput = iterations / interp_time_sec;
    
    // JIT throughput
    JITConfig jit_config = jit_vm_default_config();
    jit_config.jit_threshold = iterations / 20; // Compile after 5%
    JITIntegratedVM* jit_vm = jit_vm_create_with_config(2 * 1024 * 1024, &jit_config);
    
    uint64_t jit_start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        JITVmResult result = jit_vm_execute(jit_vm, chunk);
        ASSERT(result.success == true);
    }
    uint64_t jit_end = get_time_ns();
    
    double jit_time_sec = (double)(jit_end - jit_start) / 1000000000.0;
    double jit_throughput = iterations / jit_time_sec;
    
    printf("\n  === Throughput Comparison ===");
    printf("\n  Iterations: %d", iterations);
    printf("\n  Interpreter throughput: %.2f ops/sec", interp_throughput);
    printf("\n  JIT throughput: %.2f ops/sec", jit_throughput);
    printf("\n  Throughput improvement: %.2fx", jit_throughput / interp_throughput);
    printf("\n");
    
    chunk_free(chunk);
    jit_vm_destroy(interp_vm);
    jit_vm_destroy(jit_vm);
    TEST_END;
}

// Benchmark 6: Memory Usage Comparison
void benchmark_memory_usage(void) {
    TEST("memory_usage");
    
    // This is a simplified memory usage test
    // In a real implementation, we would measure actual memory consumption
    
    JITIntegratedVM* vm = jit_vm_create(4 * 1024 * 1024);
    ASSERT(vm != NULL);
    
    const int function_count = 50;
    Chunk** chunks = (Chunk**)malloc(function_count * sizeof(Chunk*));
    
    // Create different chunks
    for (int i = 0; i < function_count; i++) {
        chunks[i] = create_arithmetic_benchmark_chunk();
    }
    
    // Compile all functions
    uint64_t compile_start = get_time_ns();
    for (int i = 0; i < function_count; i++) {
        jit_vm_compile_function(vm, i + 1, chunks[i]);
    }
    uint64_t compile_end = get_time_ns();
    
    // Execute all functions
    uint64_t execute_start = get_time_ns();
    for (int i = 0; i < function_count; i++) {
        JITVmResult result = jit_vm_execute(vm, chunks[i]);
        ASSERT(result.success == true);
    }
    uint64_t execute_end = get_time_ns();
    
    printf("\n  === Memory Usage Analysis ===");
    printf("\n  Functions compiled: %d", function_count);
    printf("\n  Cache entries: %zu", vm->code_cache->entry_count);
    printf("\n  Total compilation time: %.2f ms", (compile_end - compile_start) / 1000000.0);
    printf("\n  Total execution time: %.2f ms", (execute_end - execute_start) / 1000000.0);
    printf("\n  Average compilation time: %.2f ms", 
           (compile_end - compile_start) / 1000000.0 / function_count);
    printf("\n");
    
    // Cleanup
    for (int i = 0; i < function_count; i++) {
        chunk_free(chunks[i]);
    }
    free(chunks);
    jit_vm_destroy(vm);
    TEST_END;
}

// Benchmark 7: Hot Path Detection Effectiveness
void benchmark_hot_path_detection(void) {
    TEST("hot_path_detection");
    
    JITConfig config = jit_vm_default_config();
    config.enable_hotspot_detection = true;
    config.jit_threshold = 100;
    JITIntegratedVM* vm = jit_vm_create_with_config(2 * 1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* hot_chunk = create_arithmetic_benchmark_chunk();
    Chunk* cold_chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(cold_chunk);
    int const_idx = chunk_add_constant(cold_chunk, 1.0);
    chunk_write(cold_chunk, OP_CONSTANT, 1);
    chunk_write(cold_chunk, const_idx, 1);
    chunk_write(cold_chunk, OP_RETURN, 1);
    
    const int hot_executions = 1000;
    const int cold_executions = 10;
    
    // Execute hot path many times
    uint64_t hot_start = get_time_ns();
    for (int i = 0; i < hot_executions; i++) {
        JITVmResult result = jit_vm_execute(vm, hot_chunk);
        ASSERT(result.success == true);
    }
    uint64_t hot_end = get_time_ns();
    
    // Execute cold path few times
    uint64_t cold_start = get_time_ns();
    for (int i = 0; i < cold_executions; i++) {
        JITVmResult result = jit_vm_execute(vm, cold_chunk);
        ASSERT(result.success == true);
    }
    uint64_t cold_end = get_time_ns();
    
    // Get hot path statistics
    uint64_t total_executions, hot_hits, cold_hits;
    hot_path_detector_get_stats(vm->hot_path_detector, &total_executions, &hot_hits, &cold_hits);
    
    printf("\n  === Hot Path Detection Analysis ===");
    printf("\n  Hot path executions: %d", hot_executions);
    printf("\n  Cold path executions: %d", cold_executions);
    printf("\n  Total detector executions: %lu", total_executions);
    printf("\n  Hot path hits: %lu", hot_hits);
    printf("\n  Cold path hits: %lu", cold_hits);
    printf("\n  Hot path avg time: %.2f ns", (double)(hot_end - hot_start) / hot_executions);
    printf("\n  Cold path avg time: %.2f ns", (double)(cold_end - cold_start) / cold_executions);
    printf("\n");
    
    chunk_free(hot_chunk);
    chunk_free(cold_chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Benchmark 8: Scalability Analysis
void benchmark_scalability_analysis(void) {
    TEST("scalability_analysis");
    
    const int scale_factors[] = {100, 500, 1000, 2000, 5000};
    const int num_scales = sizeof(scale_factors) / sizeof(scale_factors[0]);
    
    printf("\n  === Scalability Analysis ===");
    
    for (int s = 0; s < num_scales; s++) {
        int iterations = scale_factors[s];
        
        JITConfig config = jit_vm_default_config();
        config.jit_threshold = iterations / 10;
        JITIntegratedVM* vm = jit_vm_create_with_config(4 * 1024 * 1024, &config);
        
        Chunk* chunk = create_arithmetic_benchmark_chunk();
        
        uint64_t start = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            JITVmResult result = jit_vm_execute(vm, chunk);
            ASSERT(result.success == true);
        }
        uint64_t end = get_time_ns();
        
        double total_time_ms = (end - start) / 1000000.0;
        double avg_time_ns = (double)(end - start) / iterations;
        double throughput = iterations / (total_time_ms / 1000.0);
        
        printf("\n  Scale %d: %.2f ms total, %.2f ns/op, %.2f ops/sec", 
               iterations, total_time_ms, avg_time_ns, throughput);
        
        chunk_free(chunk);
        jit_vm_destroy(vm);
    }
    
    printf("\n");
    TEST_END;
}

int main(void) {
    printf("=== JIT Performance Benchmarks ===\n");
    printf("Note: These benchmarks measure relative performance characteristics.\n");
    printf("Actual speedups depend on the underlying JIT implementation.\n\n");
    
    benchmark_simple_arithmetic();
    benchmark_complex_arithmetic();
    benchmark_nested_computations();
    benchmark_compilation_overhead();
    benchmark_throughput_comparison();
    benchmark_memory_usage();
    benchmark_hot_path_detection();
    benchmark_scalability_analysis();
    
    printf("\n=== Benchmark Results Summary ===\n");
    printf("Benchmarks run: %d\n", tests_run);
    printf("Benchmarks completed: %d\n", tests_passed);
    printf("Benchmarks failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("All benchmarks COMPLETED successfully!\n");
        return 0;
    } else {
        printf("Some benchmarks FAILED!\n");
        return 1;
    }
}

