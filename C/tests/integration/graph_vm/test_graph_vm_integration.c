
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/graph/graph.h"
#include "../../../vm/graph/graph_executor.h"
#include "../../../vm/vm_graph_integration.h"

// Test graph execution within VM context
static bool test_graph_vm_execution(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create a simple computation graph
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(10.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    graph_add_edge(graph, add, output, 0);
    
    // Execute graph within VM context
    Value result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_NUMBER(result), "Graph execution should return a number");
    TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Graph result should be 10 + 5 = 15");
    
    // Verify VM state is preserved
    TEST_ASSERT(!vm_has_error(vm), "VM should not have errors after graph execution");
    TEST_ASSERT_EQ(0, vm_stack_size(vm), "VM stack should be clean after graph execution");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graph-VM execution should not leak memory");
    
    return true;
}

// Test bytecode to graph conversion
static bool test_bytecode_to_graph_conversion(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create bytecode for arithmetic expression
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load 7.0
        OP_CONSTANT, 1,  // Load 3.0
        OP_MULTIPLY,     // Multiply
        OP_CONSTANT, 2,  // Load 2.0
        OP_ADD,          // Add
        OP_RETURN        // Return result
    };
    Value constants[] = {VALUE_NUMBER(7.0), VALUE_NUMBER(3.0), VALUE_NUMBER(2.0)};
    
    // Convert bytecode to graph
    Graph* graph = vm_bytecode_to_graph(vm, bytecode, sizeof(bytecode), constants, 3);
    TEST_ASSERT_NOT_NULL(graph, "Bytecode to graph conversion should succeed");
    
    // Verify graph structure
    TEST_ASSERT(graph_node_count(graph) >= 5, "Graph should have at least 5 nodes (3 constants + multiply + add)");
    TEST_ASSERT(graph_validate(graph), "Converted graph should be valid");
    
    // Execute both bytecode and graph, compare results
    vm_reset(vm);
    Value bytecode_result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 3);
    
    vm_reset(vm);
    Value graph_result = vm_execute_graph(vm, graph);
    
    TEST_ASSERT(IS_NUMBER(bytecode_result) && IS_NUMBER(graph_result), "Both results should be numbers");
    TEST_ASSERT_EQ(AS_NUMBER(bytecode_result), AS_NUMBER(graph_result), "Bytecode and graph results should match");
    TEST_ASSERT_EQ(23.0, AS_NUMBER(graph_result), "Result should be 7*3+2 = 23");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Bytecode to graph conversion should not leak memory");
    
    return true;
}

// Test graph to bytecode conversion
static bool test_graph_to_bytecode_conversion(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create a computation graph
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(4.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(6.0));
    NodeID mult = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID const3 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(1.0));
    NodeID sub = graph_add_node(graph, NODE_SUBTRACT, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, mult, 0);
    graph_add_edge(graph, const2, mult, 1);
    graph_add_edge(graph, mult, sub, 0);
    graph_add_edge(graph, const3, sub, 1);
    graph_add_edge(graph, sub, output, 0);
    
    // Convert graph to bytecode
    size_t bytecode_size;
    size_t constants_count;
    uint8_t* bytecode = vm_graph_to_bytecode(vm, graph, &bytecode_size, &constants_count);
    Value* constants = vm_graph_get_constants(vm, graph, &constants_count);
    
    TEST_ASSERT_NOT_NULL(bytecode, "Graph to bytecode conversion should succeed");
    TEST_ASSERT_NOT_NULL(constants, "Constants extraction should succeed");
    TEST_ASSERT(bytecode_size > 0, "Bytecode should have non-zero size");
    TEST_ASSERT(constants_count > 0, "Should have constants");
    
    // Execute both graph and converted bytecode
    vm_reset(vm);
    Value graph_result = vm_execute_graph(vm, graph);
    
    vm_reset(vm);
    Value bytecode_result = vm_execute_bytecode(vm, bytecode, bytecode_size, constants, constants_count);
    
    TEST_ASSERT(IS_NUMBER(graph_result) && IS_NUMBER(bytecode_result), "Both results should be numbers");
    TEST_ASSERT_EQ(AS_NUMBER(graph_result), AS_NUMBER(bytecode_result), "Graph and bytecode results should match");
    TEST_ASSERT_EQ(23.0, AS_NUMBER(bytecode_result), "Result should be 4*6-1 = 23");
    
    free(bytecode);
    free(constants);
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graph to bytecode conversion should not leak memory");
    
    return true;
}

// Test memory sharing between graph and VM
static bool test_graph_vm_memory_sharing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Set up VM with some global variables
    vm_set_global_string(vm, "shared_var", "Hello from VM");
    vm_set_global_number(vm, "shared_num", 42.0);
    
    // Create graph that accesses VM globals
    Graph* graph = graph_create();
    NodeID global_var = graph_add_node(graph, NODE_GLOBAL_GET, VALUE_STRING("shared_var"));
    NodeID global_num = graph_add_node(graph, NODE_GLOBAL_GET, VALUE_STRING("shared_num"));
    NodeID concat = graph_add_node(graph, NODE_STRING_CONCAT, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, global_var, concat, 0);
    graph_add_edge(graph, global_num, concat, 1);
    graph_add_edge(graph, concat, output, 0);
    
    // Execute graph with VM context
    Value result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_STRING(result), "Result should be a string");
    
    const char* result_str = AS_CSTRING(result);
    TEST_ASSERT(strstr(result_str, "Hello from VM") != NULL, "Result should contain VM global string");
    TEST_ASSERT(strstr(result_str, "42") != NULL, "Result should contain VM global number");
    
    // Modify VM globals and re-execute graph
    vm_set_global_string(vm, "shared_var", "Modified");
    vm_set_global_number(vm, "shared_num", 99.0);
    
    Value result2 = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_STRING(result2), "Second result should be a string");
    
    const char* result2_str = AS_CSTRING(result2);
    TEST_ASSERT(strstr(result2_str, "Modified") != NULL, "Result should reflect updated VM globals");
    TEST_ASSERT(strstr(result2_str, "99") != NULL, "Result should reflect updated VM globals");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graph-VM memory sharing should not leak memory");
    
    return true;
}

// Test error propagation from graph to VM
static bool test_graph_vm_error_propagation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create graph with potential error (division by zero)
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(10.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(0.0));
    NodeID div = graph_add_node(graph, NODE_DIVIDE, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, div, 0);
    graph_add_edge(graph, const2, div, 1);
    graph_add_edge(graph, div, output, 0);
    
    // Execute graph and check error propagation
    Value result = vm_execute_graph(vm, graph);
    
    // Should handle division by zero gracefully
    TEST_ASSERT(IS_NUMBER(result), "Division by zero should return a number (infinity)");
    TEST_ASSERT(isinf(AS_NUMBER(result)), "Result should be infinity");
    
    // VM should not be in error state for mathematical edge cases
    TEST_ASSERT(!vm_has_error(vm), "VM should handle mathematical edge cases gracefully");
    
    // Test with actual error condition (invalid node type)
    Graph* error_graph = graph_create();
    NodeID invalid_node = graph_add_node(error_graph, (NodeType)999, VALUE_NIL); // Invalid node type
    NodeID error_output = graph_add_node(error_graph, NODE_OUTPUT, VALUE_NIL);
    graph_add_edge(error_graph, invalid_node, error_output, 0);
    
    Value error_result = vm_execute_graph(vm, error_graph);
    TEST_ASSERT(IS_NIL(error_result), "Invalid graph should return nil");
    TEST_ASSERT(vm_has_error(vm), "VM should have error after invalid graph execution");
    
    const char* error_msg = vm_get_error(vm);
    TEST_ASSERT_NOT_NULL(error_msg, "Error message should be available");
    TEST_ASSERT(strstr(error_msg, "graph") != NULL || strstr(error_msg, "node") != NULL, 
                "Error message should mention graph or node");
    
    graph_destroy(graph);
    graph_destroy(error_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Graph-VM error propagation should not leak memory");
    
    return true;
}

// Test performance comparison: graph vs bytecode
static bool test_graph_vs_bytecode_performance(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create equivalent computation in both graph and bytecode
    // Computation: (a * b) + (c * d) where a=2, b=3, c=4, d=5
    
    // Graph version
    Graph* graph = graph_create();
    NodeID a = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(2.0));
    NodeID b = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(3.0));
    NodeID c = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(4.0));
    NodeID d = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID mult1 = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID mult2 = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, a, mult1, 0);
    graph_add_edge(graph, b, mult1, 1);
    graph_add_edge(graph, c, mult2, 0);
    graph_add_edge(graph, d, mult2, 1);
    graph_add_edge(graph, mult1, add, 0);
    graph_add_edge(graph, mult2, add, 1);
    graph_add_edge(graph, add, output, 0);
    
    // Bytecode version
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load 2.0
        OP_CONSTANT, 1,  // Load 3.0
        OP_MULTIPLY,     // a * b
        OP_CONSTANT, 2,  // Load 4.0
        OP_CONSTANT, 3,  // Load 5.0
        OP_MULTIPLY,     // c * d
        OP_ADD,          // (a*b) + (c*d)
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(2.0), VALUE_NUMBER(3.0), VALUE_NUMBER(4.0), VALUE_NUMBER(5.0)};
    
    // Benchmark graph execution
    TEST_BENCHMARK_START();
    for (int i = 0; i < 1000; i++) {
        vm_reset(vm);
        Value graph_result = vm_execute_graph(vm, graph);
        TEST_ASSERT_EQ(26.0, AS_NUMBER(graph_result), "Graph result should be (2*3)+(4*5) = 26");
    }
    TEST_BENCHMARK_END("Graph execution (1000 iterations)");
    
    // Benchmark bytecode execution
    TEST_BENCHMARK_START();
    for (int i = 0; i < 1000; i++) {
        vm_reset(vm);
        Value bytecode_result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 4);
        TEST_ASSERT_EQ(26.0, AS_NUMBER(bytecode_result), "Bytecode result should be (2*3)+(4*5) = 26");
    }
    TEST_BENCHMARK_END("Bytecode execution (1000 iterations)");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Performance comparison should not leak memory");
    
    return true;
}

// Test hybrid execution modes
static bool test_hybrid_execution_modes(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create a graph that calls bytecode functions
    Graph* graph = graph_create();
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(10.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID func_call = graph_add_node(graph, NODE_FUNCTION_CALL, VALUE_STRING("add_function"));
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, func_call, 0);
    graph_add_edge(graph, const2, func_call, 1);
    graph_add_edge(graph, func_call, output, 0);
    
    // Define bytecode function
    uint8_t add_function[] = {
        OP_GET_LOCAL, 0,  // Get first argument
        OP_GET_LOCAL, 1,  // Get second argument
        OP_ADD,           // Add them
        OP_RETURN         // Return result
    };
    
    vm_register_function(vm, "add_function", add_function, sizeof(add_function));
    
    // Execute hybrid graph-bytecode computation
    Value result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_NUMBER(result), "Hybrid execution should return a number");
    TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Result should be 10 + 5 = 15");
    
    // Test bytecode calling graph
    uint8_t main_bytecode[] = {
        OP_CONSTANT, 0,      // Load 8.0
        OP_CONSTANT, 1,      // Load 7.0
        OP_EXECUTE_GRAPH, 0, // Execute graph with these inputs
        OP_RETURN            // Return result
    };
    Value main_constants[] = {VALUE_NUMBER(8.0), VALUE_NUMBER(7.0)};
    
    // Create a simple graph for bytecode to execute
    Graph* simple_graph = graph_create();
    NodeID input1 = graph_add_node(simple_graph, NODE_INPUT, VALUE_NUMBER(0)); // Input index 0
    NodeID input2 = graph_add_node(simple_graph, NODE_INPUT, VALUE_NUMBER(1)); // Input index 1
    NodeID multiply = graph_add_node(simple_graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID simple_output = graph_add_node(simple_graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(simple_graph, input1, multiply, 0);
    graph_add_edge(simple_graph, input2, multiply, 1);
    graph_add_edge(simple_graph, multiply, simple_output, 0);
    
    vm_register_graph(vm, 0, simple_graph);
    
    vm_reset(vm);
    Value hybrid_result = vm_execute_bytecode(vm, main_bytecode, sizeof(main_bytecode), main_constants, 2);
    TEST_ASSERT(IS_NUMBER(hybrid_result), "Bytecode-graph hybrid should return a number");
    TEST_ASSERT_EQ(56.0, AS_NUMBER(hybrid_result), "Result should be 8 * 7 = 56");
    
    graph_destroy(graph);
    graph_destroy(simple_graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Hybrid execution modes should not leak memory");
    
    return true;
}

// Test resource management integration
static bool test_resource_management_integration(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Create and execute many graphs to test resource management
    for (int i = 0; i < 100; i++) {
        Graph* graph = graph_create();
        NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i));
        NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i + 1));
        NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
        NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
        
        graph_add_edge(graph, const1, add, 0);
        graph_add_edge(graph, const2, add, 1);
        graph_add_edge(graph, add, output, 0);
        
        Value result = vm_execute_graph(vm, graph);
        TEST_ASSERT(IS_NUMBER(result), "Graph execution should return a number");
        TEST_ASSERT_EQ(2*i + 1, AS_NUMBER(result), "Result should be correct");
        
        graph_destroy(graph);
        
        // Trigger garbage collection periodically
        if (i % 10 == 0) {
            vm_collect_garbage(vm);
        }
    }
    
    // Final garbage collection
    vm_collect_garbage(vm);
    
    size_t final_memory = vm_get_memory_usage(vm);
    
    // Memory usage should be controlled
    TEST_ASSERT(final_memory < initial_memory + 1024 * 1024, "Memory usage should be controlled");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Resource management integration should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t graph_vm_integration_tests[] = {
    test_graph_vm_execution,
    test_bytecode_to_graph_conversion,
    test_graph_to_bytecode_conversion,
    test_graph_vm_memory_sharing,
    test_graph_vm_error_propagation,
    test_graph_vs_bytecode_performance,
    test_hybrid_execution_modes,
    test_resource_management_integration
};

test_suite_t graph_vm_integration_suite = {
    .name = "Graph-VM Integration Tests",
    .tests = graph_vm_integration_tests,
    .test_count = sizeof(graph_vm_integration_tests) / sizeof(graph_vm_integration_tests[0])
};
