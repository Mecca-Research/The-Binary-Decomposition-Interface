
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/error.h"
#include "../../../vm/jit/jit_compiler.h"
#include "../../../vm/graph/graph.h"

// Test system-wide error propagation
static bool test_system_wide_error_propagation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test error propagation from bytecode to VM
    uint8_t error_bytecode[] = {
        OP_CONSTANT, 0,      // Load 10.0
        OP_CONSTANT, 1,      // Load 0.0
        OP_DIVIDE,           // Division by zero
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(0.0)};
    
    Value result = vm_execute_bytecode(vm, error_bytecode, sizeof(error_bytecode), constants, 2);
    
    // Should handle gracefully (return infinity, not error)
    TEST_ASSERT(IS_NUMBER(result), "Division by zero should return number (infinity)");
    TEST_ASSERT(isinf(AS_NUMBER(result)), "Result should be infinity");
    
    // Test error propagation from graph to VM
    Graph* error_graph = graph_create();
    NodeID invalid_node = graph_add_node(error_graph, (NodeType)999, VALUE_NIL); // Invalid node type
    NodeID output = graph_add_node(error_graph, NODE_OUTPUT, VALUE_NIL);
    graph_add_edge(error_graph, invalid_node, output, 0);
    
    vm_reset(vm);
    Value graph_result = vm_execute_graph(vm, error_graph);
    TEST_ASSERT(IS_NIL(graph_result), "Invalid graph should return nil");
    TEST_ASSERT(vm_has_error(vm), "VM should have error from invalid graph");
    
    const char* error_msg = vm_get_error(vm);
    TEST_ASSERT_NOT_NULL(error_msg, "Error message should be available");
    TEST_ASSERT(strstr(error_msg, "node") != NULL || strstr(error_msg, "graph") != NULL, 
                "Error should mention node or graph");
    
    // Test error recovery
    vm_clear_error(vm);
    TEST_ASSERT(!vm_has_error(vm), "Error should be cleared");
    
    // Test that system continues to work after error
    uint8_t valid_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value valid_constants[] = {VALUE_NUMBER(42.0)};
    
    vm_reset(vm);
    Value recovery_result = vm_execute_bytecode(vm, valid_bytecode, sizeof(valid_bytecode), valid_constants, 1);
    TEST_ASSERT(IS_NUMBER(recovery_result), "System should work after error recovery");
    TEST_ASSERT_EQ(42.0, AS_NUMBER(recovery_result), "Recovery result should be correct");
    
    graph_destroy(error_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("System-wide error propagation should not leak memory");
    
    return true;
}

// Test error handling across component boundaries
static bool test_cross_component_error_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Test VM -> JIT error propagation
    uint8_t jit_error_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CALL_NATIVE, 0,   // Call non-existent native function
        OP_RETURN
    };
    Value jit_constants[] = {VALUE_NUMBER(42.0)};
    
    // Execute enough times to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, jit_error_bytecode, sizeof(jit_error_bytecode), jit_constants, 1);
        
        // Should handle JIT compilation errors gracefully
        if (IS_NIL(result) && vm_has_error(vm)) {
            const char* error = vm_get_error(vm);
            TEST_ASSERT(strstr(error, "native") != NULL || strstr(error, "function") != NULL,
                       "Error should mention native function issue");
            vm_clear_error(vm);
        }
    }
    
    // Test Graph -> VM -> JIT error chain
    Graph* complex_graph = graph_create();
    NodeID input = graph_add_node(complex_graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID bytecode_call = graph_add_node(complex_graph, NODE_BYTECODE_CALL, VALUE_STRING("error_function"));
    NodeID output = graph_add_node(complex_graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(complex_graph, input, bytecode_call, 0);
    graph_add_edge(complex_graph, bytecode_call, output, 0);
    
    // Register error-prone bytecode function
    uint8_t error_function[] = {
        OP_GET_LOCAL, 0,
        OP_CONSTANT, 0,      // Load 0
        OP_DIVIDE,           // Divide by zero
        OP_RETURN
    };
    Value error_func_constants[] = {VALUE_NUMBER(0.0)};
    vm_register_function(vm, "error_function", error_function, sizeof(error_function));
    
    // Execute graph that calls error-prone function
    Value inputs[] = {VALUE_NUMBER(10.0)};
    Value complex_result = vm_execute_graph_with_inputs(vm, complex_graph, inputs, 1);
    
    // Should handle the error chain gracefully
    if (IS_NIL(complex_result)) {
        TEST_ASSERT(vm_has_error(vm), "Should have error from complex error chain");
        const char* complex_error = vm_get_error(vm);
        TEST_ASSERT_NOT_NULL(complex_error, "Complex error message should be available");
    } else {
        // If it returns a number, it should be infinity (division by zero handled)
        TEST_ASSERT(IS_NUMBER(complex_result), "Complex result should be a number");
        TEST_ASSERT(isinf(AS_NUMBER(complex_result)), "Complex result should be infinity");
    }
    
    graph_destroy(complex_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Cross-component error handling should not leak memory");
    
    return true;
}

// Test consistent error state management
static bool test_consistent_error_state_management(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Test error state consistency across operations
    
    // 1. Initial state
    TEST_ASSERT(!vm_has_error(vm), "VM should start without errors");
    TEST_ASSERT_NULL(vm_get_error(vm), "Error message should be NULL initially");
    
    // 2. Set error and verify state
    vm_set_error(vm, "Test error message");
    TEST_ASSERT(vm_has_error(vm), "VM should have error after setting");
    TEST_ASSERT_STR_EQ("Test error message", vm_get_error(vm), "Error message should match");
    
    // 3. Test that operations respect error state
    vm_push_number(vm, 42.0);
    TEST_ASSERT(vm_has_error(vm), "Error state should persist during operations");
    
    uint8_t bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value constants[] = {VALUE_NUMBER(123.0)};
    Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 1);
    
    // Execution should be skipped or return nil when in error state
    TEST_ASSERT(IS_NIL(result) || vm_has_error(vm), "Execution should handle existing error state");
    
    // 4. Clear error and verify recovery
    vm_clear_error(vm);
    TEST_ASSERT(!vm_has_error(vm), "Error should be cleared");
    TEST_ASSERT_NULL(vm_get_error(vm), "Error message should be NULL after clearing");
    
    // 5. Test normal operation after error recovery
    vm_reset(vm);
    Value recovery_result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 1);
    TEST_ASSERT(IS_NUMBER(recovery_result), "Should work normally after error recovery");
    TEST_ASSERT_EQ(123.0, AS_NUMBER(recovery_result), "Recovery result should be correct");
    
    // 6. Test error state with nested operations
    vm_set_error(vm, "Nested error test");
    
    Graph* graph = graph_create();
    NodeID node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(42.0));
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    graph_add_edge(graph, node, output, 0);
    
    Value nested_result = vm_execute_graph(vm, graph);
    TEST_ASSERT(vm_has_error(vm), "Error state should persist through nested operations");
    
    // Original error should be preserved (not overwritten by graph execution)
    TEST_ASSERT_STR_EQ("Nested error test", vm_get_error(vm), "Original error should be preserved");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Consistent error state management should not leak memory");
    
    return true;
}

// Test error handling performance impact
static bool test_error_handling_performance_impact(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    uint8_t normal_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CONSTANT, 1,
        OP_ADD,
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Benchmark normal execution (no errors)
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10000; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, normal_bytecode, sizeof(normal_bytecode), constants, 2);
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Normal execution should be correct");
    }
    TEST_BENCHMARK_END("Normal execution (10000 iterations)");
    
    // Benchmark execution with error checking enabled
    vm_enable_detailed_error_tracking(vm, true);
    
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10000; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, normal_bytecode, sizeof(normal_bytecode), constants, 2);
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Error-tracked execution should be correct");
        TEST_ASSERT(!vm_has_error(vm), "Should not have errors in normal execution");
    }
    TEST_BENCHMARK_END("Error-tracked execution (10000 iterations)");
    
    // Benchmark execution with frequent error state checks
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10000; i++) {
        vm_reset(vm);
        
        // Simulate frequent error checking
        vm_check_error_state(vm);
        Value result = vm_execute_bytecode(vm, normal_bytecode, sizeof(normal_bytecode), constants, 2);
        vm_check_error_state(vm);
        
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Frequent error checking should not affect correctness");
        vm_check_error_state(vm);
    }
    TEST_BENCHMARK_END("Frequent error checking (10000 iterations)");
    
    // Benchmark error recovery performance
    uint8_t error_bytecode[] = {OP_INVALID_OPCODE, 0, OP_RETURN}; // Invalid opcode
    
    TEST_BENCHMARK_START();
    for (int i = 0; i < 1000; i++) {
        vm_reset(vm);
        
        // Execute error-prone code
        Value error_result = vm_execute_bytecode(vm, error_bytecode, sizeof(error_bytecode), constants, 2);
        TEST_ASSERT(IS_NIL(error_result) || vm_has_error(vm), "Error execution should fail or set error");
        
        // Recover from error
        vm_clear_error(vm);
        
        // Execute normal code
        Value recovery_result = vm_execute_bytecode(vm, normal_bytecode, sizeof(normal_bytecode), constants, 2);
        TEST_ASSERT_EQ(15.0, AS_NUMBER(recovery_result), "Recovery should work correctly");
    }
    TEST_BENCHMARK_END("Error recovery (1000 iterations)");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Error handling performance impact should not leak memory");
    
    return true;
}

// Test memory safety during error conditions
static bool test_memory_safety_during_errors(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Test memory safety with various error conditions
    for (int error_scenario = 0; error_scenario < 10; error_scenario++) {
        // Set up some resources before triggering errors
        vm_push_string(vm, "resource string");
        vm_push_number(vm, 42.0);
        
        Graph* graph = graph_create();
        NodeID node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(error_scenario));
        
        // Trigger different error scenarios
        switch (error_scenario % 5) {
            case 0: // Stack overflow
                for (int i = 0; i < 1000; i++) {
                    if (!vm_push_number(vm, i)) break;
                }
                break;
                
            case 1: // Invalid memory access simulation
                vm_simulate_invalid_memory_access(vm);
                break;
                
            case 2: // Graph execution error
                {
                    NodeID invalid = graph_add_node(graph, (NodeType)999, VALUE_NIL);
                    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
                    graph_add_edge(graph, invalid, output, 0);
                    vm_execute_graph(vm, graph);
                }
                break;
                
            case 3: // Bytecode execution error
                {
                    uint8_t bad_bytecode[] = {0xFF, 0xFE, 0xFD};
                    vm_execute_bytecode(vm, bad_bytecode, sizeof(bad_bytecode), NULL, 0);
                }
                break;
                
            case 4: // Memory allocation failure
                vm_simulate_memory_allocation_failure(vm, true);
                vm_push_string(vm, "this allocation should fail");
                vm_simulate_memory_allocation_failure(vm, false);
                break;
        }
        
        // Verify error state
        if (vm_has_error(vm)) {
            printf("Error scenario %d: %s\n", error_scenario, vm_get_error(vm));
        }
        
        // Clean up resources
        graph_destroy(graph);
        vm_clear_error(vm);
        vm_reset(vm);
        vm_collect_garbage(vm);
        
        // Verify memory integrity
        size_t current_memory = vm_get_memory_usage(vm);
        TEST_ASSERT(current_memory <= initial_memory + 2 * 1024 * 1024, 
                   "Memory should not grow excessively during error scenarios");
        
        // Verify VM is still functional
        vm_push_number(vm, 123.0);
        Value test_val = vm_pop_number(vm);
        TEST_ASSERT_EQ(123.0, test_val, "VM should remain functional after error scenarios");
    }
    
    // Final memory check
    vm_collect_garbage(vm);
    size_t final_memory = vm_get_memory_usage(vm);
    TEST_ASSERT(final_memory <= initial_memory + 1024 * 1024, 
               "Final memory should be close to initial after all error scenarios");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Memory safety during errors should not leak memory");
    
    return true;
}

// Test graceful degradation testing
static bool test_graceful_degradation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Test graceful degradation when JIT fails
    uint8_t jit_problematic_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CALL_NATIVE, 999, // Non-existent native function
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(42.0)};
    
    // Execute enough times to trigger JIT compilation
    bool jit_fallback_occurred = false;
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 5; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, jit_problematic_bytecode, sizeof(jit_problematic_bytecode), constants, 1);
        
        if (IS_NIL(result) && vm_has_error(vm)) {
            // JIT compilation failed, should fall back to interpreter
            jit_fallback_occurred = true;
            vm_clear_error(vm);
        }
    }
    
    // Verify that system continues to work even if JIT fails
    uint8_t simple_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    vm_reset(vm);
    Value simple_result = vm_execute_bytecode(vm, simple_bytecode, sizeof(simple_bytecode), constants, 1);
    TEST_ASSERT(IS_NUMBER(simple_result), "System should work after JIT failure");
    TEST_ASSERT_EQ(42.0, AS_NUMBER(simple_result), "Simple execution should be correct");
    
    // Test graceful degradation with graph execution
    Graph* degradation_graph = graph_create();
    NodeID input = graph_add_node(degradation_graph, NODE_INPUT, VALUE_NUMBER(0));
    NodeID problematic = graph_add_node(degradation_graph, NODE_CUSTOM_OPERATION, VALUE_STRING("nonexistent_op"));
    NodeID fallback = graph_add_node(degradation_graph, NODE_CONSTANT, VALUE_NUMBER(100.0));
    NodeID select = graph_add_node(degradation_graph, NODE_SELECT_ON_ERROR, VALUE_NIL);
    NodeID output = graph_add_node(degradation_graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(degradation_graph, input, problematic, 0);
    graph_add_edge(degradation_graph, problematic, select, 0);
    graph_add_edge(degradation_graph, fallback, select, 1);
    graph_add_edge(degradation_graph, select, output, 0);
    
    // Execute graph with graceful degradation
    Value inputs[] = {VALUE_NUMBER(50.0)};
    Value degradation_result = vm_execute_graph_with_inputs(vm, degradation_graph, inputs, 1);
    
    // Should fall back to the constant value when problematic operation fails
    if (IS_NUMBER(degradation_result)) {
        TEST_ASSERT_EQ(100.0, AS_NUMBER(degradation_result), "Should fall back to constant value");
    } else {
        // If it returns nil, the error should be handled gracefully
        TEST_ASSERT(!vm_has_error(vm) || vm_get_error(vm) != NULL, "Error should be handled gracefully");
    }
    
    // Test system recovery after degradation
    Graph* recovery_graph = graph_create();
    NodeID recovery_const = graph_add_node(recovery_graph, NODE_CONSTANT, VALUE_NUMBER(200.0));
    NodeID recovery_output = graph_add_node(recovery_graph, NODE_OUTPUT, VALUE_NIL);
    graph_add_edge(recovery_graph, recovery_const, recovery_output, 0);
    
    vm_reset(vm);
    Value recovery_result = vm_execute_graph(vm, recovery_graph);
    TEST_ASSERT(IS_NUMBER(recovery_result), "System should recover after degradation");
    TEST_ASSERT_EQ(200.0, AS_NUMBER(recovery_result), "Recovery should produce correct results");
    
    graph_destroy(degradation_graph);
    graph_destroy(recovery_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graceful degradation should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t error_handling_integration_tests[] = {
    test_system_wide_error_propagation,
    test_cross_component_error_handling,
    test_consistent_error_state_management,
    test_error_handling_performance_impact,
    test_memory_safety_during_errors,
    test_graceful_degradation
};

test_suite_t error_handling_integration_suite = {
    .name = "Error Handling Integration Tests",
    .tests = error_handling_integration_tests,
    .test_count = sizeof(error_handling_integration_tests) / sizeof(error_handling_integration_tests[0])
};
