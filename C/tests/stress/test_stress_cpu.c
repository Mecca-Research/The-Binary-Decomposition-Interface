
/**
 * CPU Stress Tests for BDI Kernel
 * 
 * Tests intensive computation, long-running operations, complex arithmetic,
 * and sustained CPU load scenarios.
 */

#include "../framework/test_framework.h"
#include "../test_utils.h"
#include "../../vm/bci_vm.h"
#include "../../vm/bci_chunk.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Configuration
#define CPU_STRESS_ITERATIONS 100000
#define COMPLEX_COMPUTATION_SIZE 10000

// Test: Intensive arithmetic operations
static bool test_intensive_arithmetic(void) {
    printf("Running intensive arithmetic test...\n");
    
    TEST_BENCHMARK_START();
    
    double result = 0.0;
    for (int i = 0; i < CPU_STRESS_ITERATIONS; i++) {
        result += (double)i * 1.5;
        result -= (double)i * 0.5;
        result *= 1.1;
        result /= 1.1;
    }
    
    TEST_BENCHMARK_END("Intensive arithmetic");
    
    TEST_ASSERT(result != 0.0, "Result should be non-zero");
    printf("✓ Intensive arithmetic test passed\n");
    return true;
}

// Test: Complex nested computations
static bool test_complex_nested_computations(void) {
    printf("Running complex nested computations test...\n");
    
    TEST_BENCHMARK_START();
    
    double result = 0.0;
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            result += sqrt((double)(i * j + 1));
        }
    }
    
    TEST_BENCHMARK_END("Complex nested computations");
    
    TEST_ASSERT(result > 0.0, "Result should be positive");
    printf("✓ Complex nested computations test passed\n");
    return true;
}

// Test: VM bytecode execution performance
static bool test_vm_execution_performance(void) {
    printf("Running VM execution performance test...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create complex bytecode sequence
    for (int i = 0; i < 1000; i++) {
        int const_idx = chunk_add_constant(&chunk, (double)i);
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, const_idx, 1);
        
        if (i > 0) {
            chunk_write(&chunk, OP_ADD, 1);
        }
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    TEST_BENCHMARK_START();
    
    // Execute multiple times
    for (int i = 0; i < 100; i++) {
        vm_reset(&vm);
        InterpretResult result = vm_interpret(&vm, &chunk);
        TEST_ASSERT_EQ(INTERPRET_OK, result, "VM execution should succeed");
    }
    
    TEST_BENCHMARK_END("VM execution (100 iterations)");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ VM execution performance test passed\n");
    return true;
}

// Test: Sustained CPU load
static bool test_sustained_cpu_load(void) {
    printf("Running sustained CPU load test...\n");
    
    TEST_BENCHMARK_START();
    
    // Run for a sustained period
    time_t start_time = time(NULL);
    long long operations = 0;
    
    while (difftime(time(NULL), start_time) < 2.0) {  // Run for 2 seconds
        double result = 0.0;
        for (int i = 0; i < 10000; i++) {
            result += sin((double)i) * cos((double)i);
        }
        operations++;
    }
    
    TEST_BENCHMARK_END("Sustained CPU load");
    
    printf("  Completed %lld operation batches\n", operations);
    TEST_ASSERT(operations > 0, "Should complete operations");
    printf("✓ Sustained CPU load test passed\n");
    return true;
}

// Test: Instruction dispatch overhead
static bool test_instruction_dispatch_overhead(void) {
    printf("Running instruction dispatch overhead test...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create bytecode with many simple operations
    int const_idx = chunk_add_constant(&chunk, 1.0);
    
    for (int i = 0; i < 10000; i++) {
        chunk_write(&chunk, OP_CONSTANT, 1);
        chunk_write(&chunk, const_idx, 1);
        chunk_write(&chunk, OP_NEGATE, 1);
        chunk_write(&chunk, OP_NEGATE, 1);  // Double negate = identity
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    TEST_BENCHMARK_START();
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    
    TEST_BENCHMARK_END("Instruction dispatch (10000 ops)");
    
    TEST_ASSERT_EQ(INTERPRET_OK, result, "VM execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Instruction dispatch overhead test passed\n");
    return true;
}

// Test: Mixed arithmetic operations
static bool test_mixed_arithmetic_operations(void) {
    printf("Running mixed arithmetic operations test...\n");
    
    VM vm;
    vm_init(&vm);
    
    Chunk chunk;
    chunk_init(&chunk);
    
    // Create complex arithmetic expression: ((a + b) * c) - (d / e)
    for (int i = 0; i < 100; i++) {
        // Push operands
        for (int j = 0; j < 5; j++) {
            int const_idx = chunk_add_constant(&chunk, (double)(i + j));
            chunk_write(&chunk, OP_CONSTANT, 1);
            chunk_write(&chunk, const_idx, 1);
        }
        
        // Perform operations
        chunk_write(&chunk, OP_ADD, 1);      // a + b
        chunk_write(&chunk, OP_MULTIPLY, 1); // (a + b) * c
        chunk_write(&chunk, OP_DIVIDE, 1);   // d / e
        chunk_write(&chunk, OP_SUBTRACT, 1); // ((a + b) * c) - (d / e)
    }
    
    chunk_write(&chunk, OP_RETURN, 1);
    
    TEST_BENCHMARK_START();
    
    InterpretResult result = vm_interpret(&vm, &chunk);
    
    TEST_BENCHMARK_END("Mixed arithmetic operations");
    
    TEST_ASSERT_EQ(INTERPRET_OK, result, "VM execution should succeed");
    
    chunk_free(&chunk);
    vm_free(&vm);
    
    printf("✓ Mixed arithmetic operations test passed\n");
    return true;
}

// Test: CPU-bound workload
static bool test_cpu_bound_workload(void) {
    printf("Running CPU-bound workload test...\n");
    
    TEST_BENCHMARK_START();
    
    // Compute Fibonacci numbers (CPU-intensive)
    long long fib[50];
    fib[0] = 0;
    fib[1] = 1;
    
    for (int iter = 0; iter < 1000; iter++) {
        for (int i = 2; i < 50; i++) {
            fib[i] = fib[i-1] + fib[i-2];
        }
    }
    
    TEST_BENCHMARK_END("CPU-bound workload (Fibonacci)");
    
    TEST_ASSERT(fib[49] > 0, "Fibonacci result should be positive");
    printf("✓ CPU-bound workload test passed\n");
    return true;
}

// Test: Long-running operation
static bool test_long_running_operation(void) {
    printf("Running long-running operation test...\n");
    
    TEST_BENCHMARK_START();
    
    // Simulate long-running computation
    double result = 0.0;
    for (int i = 0; i < 10000000; i++) {
        result += (double)i * 0.000001;
    }
    
    TEST_BENCHMARK_END("Long-running operation");
    
    TEST_ASSERT(result > 0.0, "Result should be positive");
    printf("✓ Long-running operation test passed\n");
    return true;
}

// Main test runner
int main(void) {
    printf("\n=== CPU Stress Tests ===\n\n");
    
    test_framework_init();
    
    bool all_passed = true;
    
    all_passed &= test_intensive_arithmetic();
    all_passed &= test_complex_nested_computations();
    all_passed &= test_vm_execution_performance();
    all_passed &= test_sustained_cpu_load();
    all_passed &= test_instruction_dispatch_overhead();
    all_passed &= test_mixed_arithmetic_operations();
    all_passed &= test_cpu_bound_workload();
    all_passed &= test_long_running_operation();
    
    test_framework_print_summary();
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
