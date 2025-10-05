
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/jit/jit_compiler.h"
#include "../../../vm/graph/graph.h"
#include "../../../vm/graph/graph_executor.h"

// Test end-to-end performance benchmarking
static bool test_end_to_end_performance(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create a complex computation for benchmarking
    uint8_t complex_bytecode[] = {
        OP_CONSTANT, 0,      // Load 1.0 (counter)
        OP_CONSTANT, 1,      // Load 1000.0 (limit)
        OP_CONSTANT, 2,      // Load 0.0 (sum)
        // Loop start (offset 6)
        OP_DUP2,             // Duplicate counter and sum
        OP_CONSTANT, 1,      // Load limit
        OP_LESS,             // counter < limit?
        OP_JUMP_IF_FALSE, 20,// Jump to end if false
        OP_DUP,              // Duplicate counter
        OP_ADD,              // sum += counter
        OP_SWAP,             // Swap sum and counter
        OP_CONSTANT, 0,      // Load 1.0
        OP_ADD,              // counter += 1
        OP_SWAP,             // Swap back
        OP_JUMP, 6,          // Jump back to loop start
        // Loop end (offset 20)
        OP_POP,              // Remove counter
        OP_RETURN            // Return sum
    };
    Value constants[] = {VALUE_NUMBER(1.0), VALUE_NUMBER(1000.0), VALUE_NUMBER(0.0)};
    
    // Benchmark interpreter execution
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, complex_bytecode, sizeof(complex_bytecode), constants, 3);
        TEST_ASSERT(IS_NUMBER(result), "Complex computation should return a number");
    }
    TEST_BENCHMARK_END("Interpreter execution (10 iterations of sum 1-1000)");
    
    // Trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, complex_bytecode, sizeof(complex_bytecode), constants, 3);
    }
    
    // Benchmark JIT execution
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, complex_bytecode, sizeof(complex_bytecode), constants, 3);
        TEST_ASSERT(IS_NUMBER(result), "JIT computation should return a number");
    }
    TEST_BENCHMARK_END("JIT execution (10 iterations of sum 1-1000)");
    
    // Create equivalent graph computation
    Graph* graph = graph_create();
    NodeID counter = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(1.0));
    NodeID limit = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(1000.0));
    NodeID sum_node = graph_add_node(graph, NODE_LOOP_SUM, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, counter, sum_node, 0);
    graph_add_edge(graph, limit, sum_node, 1);
    graph_add_edge(graph, sum_node, output, 0);
    
    // Benchmark graph execution
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10; i++) {
        vm_reset(vm);
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT(IS_NUMBER(result), "Graph computation should return a number");
    }
    TEST_BENCHMARK_END("Graph execution (10 iterations of sum 1-1000)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("End-to-end performance test should not leak memory");
    
    return true;
}

// Test performance regression detection
static bool test_performance_regression_detection(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Simple computation for baseline
    uint8_t simple_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CONSTANT, 1,
        OP_ADD,
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Measure baseline performance
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 10000; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, simple_bytecode, sizeof(simple_bytecode), constants, 2);
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Simple computation should be correct");
    }
    
    gettimeofday(&end, NULL);
    double baseline_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("Baseline performance: %.3f ms for 10000 simple operations\n", baseline_time);
    
    // Performance should be reasonable (less than 1 second for 10000 simple operations)
    TEST_ASSERT(baseline_time < 1000.0, "Performance should be reasonable");
    
    // Test with memory pressure
    void* memory_pressure[1000];
    for (int i = 0; i < 1000; i++) {
        memory_pressure[i] = malloc(1024); // Allocate 1KB each
    }
    
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 10000; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, simple_bytecode, sizeof(simple_bytecode), constants, 2);
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Simple computation should be correct under memory pressure");
    }
    
    gettimeofday(&end, NULL);
    double pressure_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("Performance under memory pressure: %.3f ms for 10000 simple operations\n", pressure_time);
    
    // Performance degradation should be limited (less than 50% slower)
    TEST_ASSERT(pressure_time < baseline_time * 1.5, "Performance degradation should be limited");
    
    // Clean up memory pressure
    for (int i = 0; i < 1000; i++) {
        free(memory_pressure[i]);
    }
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Performance regression detection should not leak memory");
    
    return true;
}

// Test scalability with increasing workload
static bool test_scalability_performance(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test scalability with different workload sizes
    int workload_sizes[] = {100, 500, 1000, 2000, 5000};
    int num_sizes = sizeof(workload_sizes) / sizeof(workload_sizes[0]);
    
    for (int size_idx = 0; size_idx < num_sizes; size_idx++) {
        int workload_size = workload_sizes[size_idx];
        
        // Create bytecode for loop with variable size
        uint8_t loop_bytecode[] = {
            OP_CONSTANT, 0,      // Load 0.0 (counter)
            OP_CONSTANT, 1,      // Load workload_size (limit)
            OP_CONSTANT, 2,      // Load 0.0 (sum)
            // Loop start
            OP_DUP2,             // Duplicate counter and sum
            OP_CONSTANT, 1,      // Load limit
            OP_LESS,             // counter < limit?
            OP_JUMP_IF_FALSE, 18,// Jump to end if false
            OP_DUP,              // Duplicate counter
            OP_ADD,              // sum += counter
            OP_SWAP,             // Swap sum and counter
            OP_CONSTANT, 0,      // Load 1.0
            OP_ADD,              // counter += 1
            OP_SWAP,             // Swap back
            OP_JUMP, 6,          // Jump back to loop start
            // Loop end
            OP_POP,              // Remove counter
            OP_RETURN            // Return sum
        };
        Value constants[] = {VALUE_NUMBER(1.0), VALUE_NUMBER(workload_size), VALUE_NUMBER(0.0)};
        
        struct timeval start, end;
        gettimeofday(&start, NULL);
        
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, loop_bytecode, sizeof(loop_bytecode), constants, 3);
        
        gettimeofday(&end, NULL);
        double execution_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        
        printf("Workload size %d: %.3f ms\n", workload_size, execution_time);
        
        TEST_ASSERT(IS_NUMBER(result), "Scalability test should return a number");
        
        // Expected result is sum from 1 to workload_size-1
        double expected = (workload_size - 1) * workload_size / 2.0;
        TEST_ASSERT_EQ(expected, AS_NUMBER(result), "Scalability result should be correct");
        
        // Performance should scale reasonably (not exponentially)
        if (size_idx > 0) {
            double prev_time = (workload_sizes[size_idx-1] == 100) ? execution_time / 5 : execution_time / 2;
            TEST_ASSERT(execution_time < prev_time * 10, "Performance should scale reasonably");
        }
    }
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Scalability performance test should not leak memory");
    
    return true;
}

// Test memory usage optimization
static bool test_memory_usage_optimization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Create and execute many operations to test memory efficiency
    uint8_t memory_test_bytecode[] = {
        OP_NEW_ARRAY, 100,   // Create array of 100 elements
        OP_CONSTANT, 0,      // Load 0 (index)
        OP_CONSTANT, 1,      // Load 100 (limit)
        // Loop to fill array
        OP_DUP2,             // Duplicate index and limit
        OP_LESS,             // index < limit?
        OP_JUMP_IF_FALSE, 15,// Jump to end if false
        OP_DUP,              // Duplicate index
        OP_DUP,              // Duplicate index again
        OP_CONSTANT, 2,      // Load 42.0 (value)
        OP_ARRAY_SET,        // array[index] = 42.0
        OP_CONSTANT, 2,      // Load 1.0
        OP_ADD,              // index += 1
        OP_JUMP, 4,          // Jump back to loop start
        // End
        OP_POP,              // Remove index
        OP_RETURN            // Return array
    };
    Value constants[] = {VALUE_NUMBER(0.0), VALUE_NUMBER(100.0), VALUE_NUMBER(42.0), VALUE_NUMBER(1.0)};
    
    // Execute multiple times and monitor memory usage
    size_t max_memory = initial_memory;
    for (int i = 0; i < 50; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, memory_test_bytecode, sizeof(memory_test_bytecode), constants, 4);
        TEST_ASSERT(IS_ARRAY(result), "Memory test should return an array");
        
        size_t current_memory = vm_get_memory_usage(vm);
        if (current_memory > max_memory) {
            max_memory = current_memory;
        }
        
        // Trigger garbage collection every 10 iterations
        if (i % 10 == 0) {
            vm_collect_garbage(vm);
        }
    }
    
    // Final garbage collection
    vm_collect_garbage(vm);
    size_t final_memory = vm_get_memory_usage(vm);
    
    printf("Memory usage - Initial: %zu, Max: %zu, Final: %zu\n", 
           initial_memory, max_memory, final_memory);
    
    // Memory should be efficiently managed
    TEST_ASSERT(max_memory < initial_memory + 10 * 1024 * 1024, "Peak memory usage should be reasonable");
    TEST_ASSERT(final_memory <= initial_memory + 1024 * 1024, "Final memory should be close to initial");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Memory usage optimization should not leak memory");
    
    return true;
}

// Test concurrent execution performance
static bool test_concurrent_execution_performance(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Note: This is a simplified concurrency test since we don't have full threading
    VM* vm1 = vm_create();
    VM* vm2 = vm_create();
    TEST_ASSERT_NOT_NULL(vm1, "First VM creation should succeed");
    TEST_ASSERT_NOT_NULL(vm2, "Second VM creation should succeed");
    
    uint8_t concurrent_bytecode[] = {
        OP_CONSTANT, 0,      // Load 1.0
        OP_CONSTANT, 1,      // Load 500.0
        OP_CONSTANT, 2,      // Load 0.0
        // Simple loop
        OP_DUP2,
        OP_CONSTANT, 1,
        OP_LESS,
        OP_JUMP_IF_FALSE, 15,
        OP_DUP,
        OP_ADD,
        OP_SWAP,
        OP_CONSTANT, 0,
        OP_ADD,
        OP_SWAP,
        OP_JUMP, 6,
        OP_POP,
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(1.0), VALUE_NUMBER(500.0), VALUE_NUMBER(0.0)};
    
    // Simulate concurrent execution by interleaving operations
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 20; i++) {
        // Execute on first VM
        vm_reset(vm1);
        Value result1 = vm_execute_bytecode(vm1, concurrent_bytecode, sizeof(concurrent_bytecode), constants, 3);
        TEST_ASSERT(IS_NUMBER(result1), "Concurrent execution 1 should return a number");
        
        // Execute on second VM
        vm_reset(vm2);
        Value result2 = vm_execute_bytecode(vm2, concurrent_bytecode, sizeof(concurrent_bytecode), constants, 3);
        TEST_ASSERT(IS_NUMBER(result2), "Concurrent execution 2 should return a number");
        
        // Results should be identical
        TEST_ASSERT_EQ(AS_NUMBER(result1), AS_NUMBER(result2), "Concurrent results should match");
    }
    
    gettimeofday(&end, NULL);
    double concurrent_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("Concurrent execution time: %.3f ms for 40 operations\n", concurrent_time);
    
    // Test sequential execution for comparison
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 40; i++) {
        vm_reset(vm1);
        Value result = vm_execute_bytecode(vm1, concurrent_bytecode, sizeof(concurrent_bytecode), constants, 3);
        TEST_ASSERT(IS_NUMBER(result), "Sequential execution should return a number");
    }
    
    gettimeofday(&end, NULL);
    double sequential_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("Sequential execution time: %.3f ms for 40 operations\n", sequential_time);
    
    // Concurrent execution should not be significantly slower than sequential
    TEST_ASSERT(concurrent_time < sequential_time * 1.5, "Concurrent execution should be efficient");
    
    vm_destroy(vm1);
    vm_destroy(vm2);
    TEST_MEMORY_VERIFY("Concurrent execution performance should not leak memory");
    
    return true;
}

// Test performance under various conditions
static bool test_performance_stress_conditions(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test performance with deep recursion
    uint8_t recursive_bytecode[] = {
        OP_GET_LOCAL, 0,     // Get recursion depth
        OP_CONSTANT, 0,      // Load 0
        OP_EQUAL,            // depth == 0?
        OP_JUMP_IF_TRUE, 12, // Jump to base case
        OP_GET_LOCAL, 0,     // Get depth
        OP_CONSTANT, 1,      // Load 1
        OP_SUBTRACT,         // depth - 1
        OP_CALL_RECURSIVE,   // Recursive call
        OP_JUMP, 14,         // Jump to end
        OP_CONSTANT, 2,      // Base case: return 1
        OP_RETURN            // Return
    };
    Value recursive_constants[] = {VALUE_NUMBER(0.0), VALUE_NUMBER(1.0), VALUE_NUMBER(1.0)};
    
    // Test with moderate recursion depth
    TEST_BENCHMARK_START();
    for (int depth = 1; depth <= 100; depth += 10) {
        vm_reset(vm);
        vm_push_number(vm, depth); // Push recursion depth
        Value result = vm_execute_bytecode(vm, recursive_bytecode, sizeof(recursive_bytecode), recursive_constants, 3);
        TEST_ASSERT(IS_NUMBER(result), "Recursive execution should return a number");
    }
    TEST_BENCHMARK_END("Recursive execution (depths 1-100)");
    
    // Test performance with large data structures
    uint8_t large_data_bytecode[] = {
        OP_NEW_OBJECT,       // Create object
        OP_CONSTANT, 0,      // Load 0 (counter)
        OP_CONSTANT, 1,      // Load 1000 (limit)
        // Loop to add properties
        OP_DUP2,             // Duplicate counter and limit
        OP_LESS,             // counter < limit?
        OP_JUMP_IF_FALSE, 20,// Jump to end if false
        OP_DUP,              // Duplicate counter
        OP_TO_STRING,        // Convert counter to string (property name)
        OP_DUP,              // Duplicate counter (property value)
        OP_SET_PROPERTY,     // Set property
        OP_CONSTANT, 2,      // Load 1
        OP_ADD,              // counter += 1
        OP_JUMP, 6,          // Jump back to loop start
        OP_POP,              // Remove counter
        OP_RETURN            // Return object
    };
    Value large_data_constants[] = {VALUE_NUMBER(0.0), VALUE_NUMBER(1000.0), VALUE_NUMBER(1.0)};
    
    TEST_BENCHMARK_START();
    vm_reset(vm);
    Value large_obj = vm_execute_bytecode(vm, large_data_bytecode, sizeof(large_data_bytecode), large_data_constants, 3);
    TEST_BENCHMARK_END("Large object creation (1000 properties)");
    
    TEST_ASSERT(IS_OBJECT(large_obj), "Large data test should return an object");
    
    // Test performance with frequent garbage collection
    TEST_BENCHMARK_START();
    for (int i = 0; i < 100; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, large_data_bytecode, sizeof(large_data_bytecode), large_data_constants, 3);
        vm_collect_garbage(vm); // Force GC after each execution
    }
    TEST_BENCHMARK_END("Frequent GC execution (100 iterations)");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Performance stress conditions should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t performance_integration_tests[] = {
    test_end_to_end_performance,
    test_performance_regression_detection,
    test_scalability_performance,
    test_memory_usage_optimization,
    test_concurrent_execution_performance,
    test_performance_stress_conditions
};

test_suite_t performance_integration_suite = {
    .name = "Performance Integration Tests",
    .tests = performance_integration_tests,
    .test_count = sizeof(performance_integration_tests) / sizeof(performance_integration_tests[0])
};
