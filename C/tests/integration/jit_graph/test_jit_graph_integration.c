
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/jit/jit_compiler.h"
#include "../../../vm/graph/graph.h"
#include "../../../vm/graph/graph_executor.h"

// Test JIT compilation of graph nodes
static bool test_jit_graph_node_compilation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create a graph with hot execution paths
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(10.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID mult = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    graph_add_edge(graph, add, mult, 0);
    graph_add_edge(graph, const2, mult, 1);
    graph_add_edge(graph, mult, output, 0);
    
    // Execute graph multiple times to trigger JIT compilation of hot nodes
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT(IS_NUMBER(result), "Graph execution should return a number");
        TEST_ASSERT_EQ(75.0, AS_NUMBER(result), "Result should be (10+5)*5 = 75");
    }
    
    // Verify that hot graph nodes were JIT compiled
    TEST_ASSERT(graph_node_is_jit_compiled(graph, add), "Hot ADD node should be JIT compiled");
    TEST_ASSERT(graph_node_is_jit_compiled(graph, mult), "Hot MULTIPLY node should be JIT compiled");
    
    // Test performance improvement with JIT-compiled nodes
    TEST_BENCHMARK_START();
    for (int i = 0; i < 100; i++) {
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT_EQ(75.0, AS_NUMBER(result), "JIT-compiled graph should produce correct results");
    }
    TEST_BENCHMARK_END("JIT-compiled graph execution (100 iterations)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT graph node compilation should not leak memory");
    
    return true;
}

// Test hot graph path optimization
static bool test_hot_graph_path_optimization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create a graph with conditional execution paths
    Graph* graph = graph_create();
    NodeID input = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(0)); // Input index 0
    NodeID zero = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(0.0));
    NodeID condition = graph_add_node(graph, NODE_GREATER, VALUE_NIL);
    NodeID true_branch = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(100.0));
    NodeID false_branch = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(-100.0));
    NodeID select = graph_add_node(graph, NODE_SELECT, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, input, condition, 0);
    graph_add_edge(graph, zero, condition, 1);
    graph_add_edge(graph, condition, select, 0);
    graph_add_edge(graph, true_branch, select, 1);
    graph_add_edge(graph, false_branch, select, 2);
    graph_add_edge(graph, select, output, 0);
    
    // Execute with positive inputs (hot path) multiple times
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value inputs[] = {VALUE_NUMBER(5.0)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 1);
        TEST_ASSERT(IS_NUMBER(result), "Graph execution should return a number");
        TEST_ASSERT_EQ(100.0, AS_NUMBER(result), "Positive input should take true branch");
    }
    
    // Verify hot path optimization
    TEST_ASSERT(graph_path_is_optimized(graph, true_branch), "True branch should be optimized as hot path");
    
    // Test that cold path still works correctly
    Value cold_inputs[] = {VALUE_NUMBER(-3.0)};
    Value cold_result = vm_execute_graph_with_inputs(vm, graph, cold_inputs, 1);
    TEST_ASSERT(IS_NUMBER(cold_result), "Cold path should still work");
    TEST_ASSERT_EQ(-100.0, AS_NUMBER(cold_result), "Negative input should take false branch");
    
    // Benchmark hot path performance
    TEST_BENCHMARK_START();
    for (int i = 0; i < 1000; i++) {
        Value inputs[] = {VALUE_NUMBER(i + 1.0)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 1);
        TEST_ASSERT_EQ(100.0, AS_NUMBER(result), "Hot path should be fast and correct");
    }
    TEST_BENCHMARK_END("Hot path execution (1000 iterations)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Hot graph path optimization should not leak memory");
    
    return true;
}

// Test native code generation for graphs
static bool test_graph_native_code_generation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create a computationally intensive graph
    Graph* graph = graph_create();
    NodeID x = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID y = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(1));
    
    // Compute polynomial: 3*x^2 + 2*x*y + y^2
    NodeID x_squared = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID y_squared = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID xy = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID three = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(3.0));
    NodeID two = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(2.0));
    NodeID term1 = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID term2 = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID sum1 = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID sum2 = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    // Build computation graph
    graph_add_edge(graph, x, x_squared, 0);
    graph_add_edge(graph, x, x_squared, 1);
    graph_add_edge(graph, y, y_squared, 0);
    graph_add_edge(graph, y, y_squared, 1);
    graph_add_edge(graph, x, xy, 0);
    graph_add_edge(graph, y, xy, 1);
    graph_add_edge(graph, three, term1, 0);
    graph_add_edge(graph, x_squared, term1, 1);
    graph_add_edge(graph, two, term2, 0);
    graph_add_edge(graph, xy, term2, 1);
    graph_add_edge(graph, term1, sum1, 0);
    graph_add_edge(graph, term2, sum1, 1);
    graph_add_edge(graph, sum1, sum2, 0);
    graph_add_edge(graph, y_squared, sum2, 1);
    graph_add_edge(graph, sum2, output, 0);
    
    // Execute multiple times to trigger native code generation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value inputs[] = {VALUE_NUMBER(2.0), VALUE_NUMBER(3.0)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 2);
        TEST_ASSERT(IS_NUMBER(result), "Complex graph should return a number");
        // Expected: 3*4 + 2*6 + 9 = 12 + 12 + 9 = 33
        TEST_ASSERT_EQ(33.0, AS_NUMBER(result), "Complex computation should be correct");
    }
    
    // Verify native code was generated
    TEST_ASSERT(graph_has_native_code(graph), "Complex graph should have native code generated");
    
    // Test native code execution performance
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10000; i++) {
        Value inputs[] = {VALUE_NUMBER(i % 10), VALUE_NUMBER((i + 1) % 10)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 2);
        TEST_ASSERT(IS_NUMBER(result), "Native code execution should return a number");
    }
    TEST_BENCHMARK_END("Native code execution (10000 iterations)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graph native code generation should not leak memory");
    
    return true;
}

// Test JIT-compiled graph memory management
static bool test_jit_graph_memory_management(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Create and JIT-compile many graphs
    for (int graph_id = 0; graph_id < 50; graph_id++) {
        Graph* graph = graph_create();
        NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(graph_id));
        NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(graph_id + 1));
        NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
        NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
        
        graph_add_edge(graph, const1, add, 0);
        graph_add_edge(graph, const2, add, 1);
        graph_add_edge(graph, add, output, 0);
        
        // Execute enough times to trigger JIT compilation
        for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
            Value result = vm_execute_graph(vm, graph);
            TEST_ASSERT(IS_NUMBER(result), "Graph execution should return a number");
            TEST_ASSERT_EQ(2 * graph_id + 1, AS_NUMBER(result), "Graph result should be correct");
        }
        
        // Verify JIT compilation occurred
        TEST_ASSERT(graph_has_native_code(graph), "Graph should have native code");
        
        graph_destroy(graph);
        
        // Trigger garbage collection periodically
        if (graph_id % 10 == 0) {
            vm_collect_garbage(vm);
            jit_cleanup_unused_code(jit);
        }
    }
    
    // Final cleanup
    vm_collect_garbage(vm);
    jit_cleanup_unused_code(jit);
    
    size_t final_memory = vm_get_memory_usage(vm);
    
    // Memory usage should be controlled despite many JIT compilations
    TEST_ASSERT(final_memory < initial_memory + 10 * 1024 * 1024, "JIT memory usage should be controlled");
    
    // Verify JIT cache management
    TEST_ASSERT(jit_get_compiled_function_count(jit) <= JIT_CACHE_MAX_SIZE, "JIT cache should respect size limits");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT graph memory management should not leak memory");
    
    return true;
}

// Test error handling in JIT-compiled graphs
static bool test_jit_graph_error_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create graph with potential runtime errors
    Graph* graph = graph_create();
    NodeID dividend = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID divisor = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(1));
    NodeID divide = graph_add_node(graph, NODE_DIVIDE, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, dividend, divide, 0);
    graph_add_edge(graph, divisor, divide, 1);
    graph_add_edge(graph, divide, output, 0);
    
    // Execute with valid inputs to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value inputs[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(2.0)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 2);
        TEST_ASSERT(IS_NUMBER(result), "Valid division should return a number");
        TEST_ASSERT_EQ(5.0, AS_NUMBER(result), "10 / 2 should equal 5");
    }
    
    // Verify JIT compilation occurred
    TEST_ASSERT(graph_has_native_code(graph), "Graph should be JIT compiled");
    
    // Test error handling in JIT-compiled code (division by zero)
    Value error_inputs[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(0.0)};
    Value error_result = vm_execute_graph_with_inputs(vm, graph, error_inputs, 2);
    
    // Should handle division by zero gracefully (return infinity)
    TEST_ASSERT(IS_NUMBER(error_result), "Division by zero should return a number");
    TEST_ASSERT(isinf(AS_NUMBER(error_result)), "Division by zero should return infinity");
    
    // VM should not be in error state for mathematical edge cases
    TEST_ASSERT(!vm_has_error(vm), "VM should handle mathematical edge cases gracefully");
    
    // Test with invalid node execution in JIT code
    Graph* invalid_graph = graph_create();
    NodeID invalid_input = graph_add_node(invalid_graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID invalid_op = graph_add_node(invalid_graph, (NodeType)999, VALUE_NIL); // Invalid operation
    NodeID invalid_output = graph_add_node(invalid_graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(invalid_graph, invalid_input, invalid_op, 0);
    graph_add_edge(invalid_graph, invalid_op, invalid_output, 0);
    
    // This should fail gracefully and fall back to interpreter
    Value inputs[] = {VALUE_NUMBER(42.0)};
    Value invalid_result = vm_execute_graph_with_inputs(vm, invalid_graph, inputs, 1);
    
    TEST_ASSERT(IS_NIL(invalid_result), "Invalid graph should return nil");
    TEST_ASSERT(vm_has_error(vm), "VM should have error for invalid graph");
    
    vm_clear_error(vm);
    
    graph_destroy(graph);
    graph_destroy(invalid_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT graph error handling should not leak memory");
    
    return true;
}

// Test debugging JIT-compiled graph code
static bool test_jit_graph_debugging(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Enable debugging
    vm_set_debug_mode(vm, true);
    jit_enable_debug_info(jit, true);
    
    // Create a graph for debugging
    Graph* graph = graph_create();
    NodeID a = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID b = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(1));
    NodeID c = graph_add_node(graph, NODE_INPUT, VALUE_NUMBER(2));
    NodeID ab = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID abc = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, a, ab, 0);
    graph_add_edge(graph, b, ab, 1);
    graph_add_edge(graph, ab, abc, 0);
    graph_add_edge(graph, c, abc, 1);
    graph_add_edge(graph, abc, output, 0);
    
    // Set breakpoints on nodes
    graph_set_breakpoint(graph, ab, true);
    graph_set_breakpoint(graph, abc, true);
    
    // Execute with debugging enabled
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value inputs[] = {VALUE_NUMBER(1.0), VALUE_NUMBER(2.0), VALUE_NUMBER(3.0)};
        Value result = vm_execute_graph_with_inputs(vm, graph, inputs, 3);
        TEST_ASSERT(IS_NUMBER(result), "Debugged graph should return a number");
        TEST_ASSERT_EQ(6.0, AS_NUMBER(result), "Result should be 1+2+3 = 6");
    }
    
    // Verify debugging information is preserved in JIT code
    TEST_ASSERT(graph_has_debug_info(graph), "JIT-compiled graph should preserve debug info");
    TEST_ASSERT(graph_breakpoint_hit_count(graph, ab) > 0, "Breakpoint should have been hit");
    TEST_ASSERT(graph_breakpoint_hit_count(graph, abc) > 0, "Breakpoint should have been hit");
    
    // Test step-by-step execution
    vm_set_single_step_mode(vm, true);
    Value inputs[] = {VALUE_NUMBER(4.0), VALUE_NUMBER(5.0), VALUE_NUMBER(6.0)};
    Value step_result = vm_execute_graph_with_inputs(vm, graph, inputs, 3);
    TEST_ASSERT(IS_NUMBER(step_result), "Single-step execution should work");
    TEST_ASSERT_EQ(15.0, AS_NUMBER(step_result), "Single-step result should be 4+5+6 = 15");
    
    // Verify execution trace
    ExecutionTrace* trace = vm_get_execution_trace(vm);
    TEST_ASSERT_NOT_NULL(trace, "Execution trace should be available");
    TEST_ASSERT(trace->step_count > 0, "Trace should contain execution steps");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT graph debugging should not leak memory");
    
    return true;
}

// Test JIT graph optimization passes
static bool test_jit_graph_optimization_passes(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Enable aggressive optimization
    jit_set_optimization_level(jit, JIT_OPT_AGGRESSIVE);
    
    // Create graph with optimization opportunities
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(3.0));
    NodeID const3 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(2.0));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID mult = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID identity = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL); // Multiply by 1 (can be optimized)
    NodeID one = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(1.0));
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    graph_add_edge(graph, add, mult, 0);
    graph_add_edge(graph, const3, mult, 1);
    graph_add_edge(graph, mult, identity, 0);
    graph_add_edge(graph, one, identity, 1);
    graph_add_edge(graph, identity, output, 0);
    
    // Execute without optimization
    jit_set_optimization_level(jit, JIT_OPT_NONE);
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT(IS_NUMBER(result), "Unoptimized graph should return a number");
        TEST_ASSERT_EQ(16.0, AS_NUMBER(result), "Result should be (5+3)*2*1 = 16");
    }
    
    size_t unoptimized_code_size = graph_get_native_code_size(graph);
    
    // Clear JIT cache and re-compile with optimization
    jit_clear_cache(jit);
    jit_set_optimization_level(jit, JIT_OPT_AGGRESSIVE);
    
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT(IS_NUMBER(result), "Optimized graph should return a number");
        TEST_ASSERT_EQ(16.0, AS_NUMBER(result), "Optimized result should be the same");
    }
    
    size_t optimized_code_size = graph_get_native_code_size(graph);
    
    // Optimized code should be smaller or equal (due to constant folding, dead code elimination)
    TEST_ASSERT(optimized_code_size <= unoptimized_code_size, "Optimized code should not be larger");
    
    // Test performance improvement
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10000; i++) {
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT_EQ(16.0, AS_NUMBER(result), "Optimized execution should be correct");
    }
    TEST_BENCHMARK_END("Optimized graph execution (10000 iterations)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT graph optimization passes should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t jit_graph_integration_tests[] = {
    test_jit_graph_node_compilation,
    test_hot_graph_path_optimization,
    test_graph_native_code_generation,
    test_jit_graph_memory_management,
    test_jit_graph_error_handling,
    test_jit_graph_debugging,
    test_jit_graph_optimization_passes
};

test_suite_t jit_graph_integration_suite = {
    .name = "JIT-Graph Integration Tests",
    .tests = jit_graph_integration_tests,
    .test_count = sizeof(jit_graph_integration_tests) / sizeof(jit_graph_integration_tests[0])
};
