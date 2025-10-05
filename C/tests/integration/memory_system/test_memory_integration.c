
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/gc/memory_pool.h"
#include "../../../vm/gc/mark_sweep.h"
#include "../../../vm/gc/generational_gc.h"

// Test cross-component memory allocation
static bool test_cross_component_memory_allocation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Allocate memory through different components
    
    // 1. VM stack allocations
    for (int i = 0; i < 100; i++) {
        vm_push_string(vm, "test string");
        vm_push_number(vm, i);
        vm_push_bool(vm, i % 2 == 0);
    }
    
    size_t after_stack = vm_get_memory_usage(vm);
    TEST_ASSERT(after_stack > initial_memory, "Stack allocations should increase memory usage");
    
    // 2. Graph node allocations
    Graph* graph = graph_create();
    for (int i = 0; i < 50; i++) {
        NodeID node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i));
        TEST_ASSERT_NEQ(INVALID_NODE_ID, node, "Node creation should succeed");
    }
    
    size_t after_graph = vm_get_memory_usage(vm);
    TEST_ASSERT(after_graph > after_stack, "Graph allocations should increase memory usage");
    
    // 3. JIT code allocations
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    uint8_t bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value constants[] = {VALUE_NUMBER(42.0)};
    
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 1);
    }
    
    size_t after_jit = vm_get_memory_usage(vm);
    TEST_ASSERT(after_jit >= after_graph, "JIT allocations should maintain or increase memory usage");
    
    // 4. Test memory sharing between components
    Value graph_result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_NUMBER(graph_result), "Graph execution should work with shared memory");
    
    // Clean up and verify memory management
    graph_destroy(graph);
    vm_collect_garbage(vm);
    
    size_t after_cleanup = vm_get_memory_usage(vm);
    TEST_ASSERT(after_cleanup < after_jit, "Memory should be reclaimed after cleanup");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Cross-component memory allocation should not leak memory");
    
    return true;
}

// Test garbage collection across all components
static bool test_system_wide_garbage_collection(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Create objects in different components that reference each other
    
    // 1. Create VM objects
    vm_set_global_string(vm, "shared_data", "Hello World");
    vm_push_string(vm, "stack_string");
    
    // 2. Create graph with references to VM globals
    Graph* graph = graph_create();
    NodeID global_ref = graph_add_node(graph, NODE_GLOBAL_GET, VALUE_STRING("shared_data"));
    NodeID string_len = graph_add_node(graph, NODE_STRING_LENGTH, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, global_ref, string_len, 0);
    graph_add_edge(graph, string_len, output, 0);
    
    // 3. Execute to create cross-references
    Value result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_NUMBER(result), "Cross-component execution should work");
    TEST_ASSERT_EQ(11.0, AS_NUMBER(result), "String length should be 11");
    
    size_t before_gc = vm_get_memory_usage(vm);
    
    // 4. Create many temporary objects
    for (int i = 0; i < 1000; i++) {
        vm_push_string(vm, "temporary");
        vm_pop_value(vm); // Immediately discard
        
        NodeID temp_node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i));
        graph_remove_node(graph, temp_node); // Immediately remove
    }
    
    size_t after_temp_objects = vm_get_memory_usage(vm);
    TEST_ASSERT(after_temp_objects > before_gc, "Temporary objects should increase memory");
    
    // 5. Trigger system-wide garbage collection
    vm_collect_garbage(vm);
    graph_collect_garbage(graph);
    
    size_t after_gc = vm_get_memory_usage(vm);
    TEST_ASSERT(after_gc < after_temp_objects, "GC should reclaim temporary objects");
    
    // 6. Verify that referenced objects are preserved
    Value preserved_result = vm_execute_graph(vm, graph);
    TEST_ASSERT(IS_NUMBER(preserved_result), "Referenced objects should be preserved");
    TEST_ASSERT_EQ(11.0, AS_NUMBER(preserved_result), "Preserved objects should work correctly");
    
    const char* preserved_global = vm_get_global_string(vm, "shared_data");
    TEST_ASSERT_STR_EQ("Hello World", preserved_global, "Global should be preserved");
    
    graph_destroy(graph);
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("System-wide GC should not leak memory");
    
    return true;
}

// Test memory pressure handling system-wide
static bool test_memory_pressure_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Set up memory pressure monitoring
    vm_set_memory_pressure_threshold(vm, 1024 * 1024); // 1MB threshold
    
    size_t initial_memory = vm_get_memory_usage(vm);
    bool pressure_detected = false;
    
    // Gradually increase memory usage
    for (int batch = 0; batch < 100; batch++) {
        // Allocate in VM
        for (int i = 0; i < 10; i++) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Large string %d-%d with lots of content to use memory", batch, i);
            vm_push_string(vm, buffer);
        }
        
        // Allocate in graph
        Graph* temp_graph = graph_create();
        for (int i = 0; i < 20; i++) {
            graph_add_node(temp_graph, NODE_CONSTANT, VALUE_STRING("memory pressure test"));
        }
        
        // Check for memory pressure
        if (vm_is_memory_pressure_detected(vm)) {
            pressure_detected = true;
            printf("Memory pressure detected at batch %d\n", batch);
            
            // Trigger aggressive cleanup
            vm_collect_garbage(vm);
            graph_destroy(temp_graph);
            
            // Verify pressure is relieved
            vm_update_memory_pressure_status(vm);
            if (!vm_is_memory_pressure_detected(vm)) {
                printf("Memory pressure relieved after cleanup\n");
            }
            
            break;
        }
        
        graph_destroy(temp_graph);
        
        // Periodic cleanup to prevent excessive memory usage
        if (batch % 10 == 0) {
            vm_collect_garbage(vm);
        }
    }
    
    TEST_ASSERT(pressure_detected, "Memory pressure should be detected during test");
    
    // Final cleanup and verification
    vm_collect_garbage(vm);
    size_t final_memory = vm_get_memory_usage(vm);
    
    printf("Memory usage - Initial: %zu, Final: %zu\n", initial_memory, final_memory);
    TEST_ASSERT(final_memory < initial_memory + 2 * 1024 * 1024, "Final memory should be controlled");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Memory pressure handling should not leak memory");
    
    return true;
}

// Test memory leak detection in integrated system
static bool test_memory_leak_detection(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Enable memory leak detection
    vm_enable_leak_detection(vm, true);
    
    size_t baseline_memory = vm_get_memory_usage(vm);
    
    // Simulate potential memory leaks in different components
    for (int cycle = 0; cycle < 10; cycle++) {
        // 1. VM operations that might leak
        for (int i = 0; i < 100; i++) {
            vm_push_string(vm, "potential leak");
            vm_push_number(vm, i);
            // Intentionally don't pop some values to simulate leaks
            if (i % 10 != 0) {
                vm_pop_value(vm);
                vm_pop_value(vm);
            }
        }
        
        // 2. Graph operations that might leak
        Graph* graph = graph_create();
        for (int i = 0; i < 50; i++) {
            NodeID node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i));
            // Create edges that might not be properly cleaned up
            if (i > 0) {
                graph_add_edge(graph, node - 1, node, 0);
            }
        }
        
        // 3. JIT operations that might leak
        JITCompiler* jit = jit_compiler_create();
        uint8_t bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
        Value constants[] = {VALUE_NUMBER(cycle)};
        
        CompiledFunction* compiled = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 1);
        // Intentionally don't free compiled function to simulate leak
        
        jit_compiler_destroy(jit); // This should clean up, but let's test
        graph_destroy(graph);
        
        // Check for leaks periodically
        if (cycle % 3 == 0) {
            MemoryLeakReport* report = vm_detect_memory_leaks(vm);
            if (report && report->leak_count > 0) {
                printf("Detected %zu memory leaks in cycle %d\n", report->leak_count, cycle);
                
                // Verify leak details
                for (size_t i = 0; i < report->leak_count; i++) {
                    MemoryLeak* leak = &report->leaks[i];
                    TEST_ASSERT_NOT_NULL(leak->allocation_site, "Leak should have allocation site");
                    TEST_ASSERT(leak->size > 0, "Leak should have positive size");
                }
            }
            memory_leak_report_destroy(report);
        }
        
        // Trigger cleanup
        vm_collect_garbage(vm);
    }
    
    // Final leak detection
    MemoryLeakReport* final_report = vm_detect_memory_leaks(vm);
    if (final_report) {
        printf("Final leak report: %zu leaks detected\n", final_report->leak_count);
        memory_leak_report_destroy(final_report);
    }
    
    size_t final_memory = vm_get_memory_usage(vm);
    printf("Memory usage - Baseline: %zu, Final: %zu\n", baseline_memory, final_memory);
    
    // Memory growth should be limited despite potential leaks
    TEST_ASSERT(final_memory < baseline_memory + 5 * 1024 * 1024, "Memory growth should be limited");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Memory leak detection should not leak memory");
    
    return true;
}

// Test performance under memory constraints
static bool test_performance_under_memory_constraints(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    // Set strict memory limits
    vm_set_memory_limit(vm, 2 * 1024 * 1024); // 2MB limit
    
    // Create a computation that uses significant memory
    uint8_t memory_intensive_bytecode[] = {
        OP_NEW_ARRAY, 200,   // Create large array
        OP_CONSTANT, 0,      // Load 0 (index)
        OP_CONSTANT, 1,      // Load 200 (limit)
        // Fill array loop
        OP_DUP2,
        OP_LESS,
        OP_JUMP_IF_FALSE, 15,
        OP_DUP,              // Duplicate index
        OP_DUP,              // Duplicate index again
        OP_CONSTANT, 2,      // Load value
        OP_ARRAY_SET,        // Set array element
        OP_CONSTANT, 3,      // Load 1
        OP_ADD,              // Increment index
        OP_JUMP, 4,          // Loop back
        OP_POP,              // Remove index
        OP_RETURN            // Return array
    };
    Value constants[] = {VALUE_NUMBER(0.0), VALUE_NUMBER(200.0), VALUE_NUMBER(42.0), VALUE_NUMBER(1.0)};
    
    // Benchmark performance under memory constraints
    TEST_BENCHMARK_START();
    
    bool memory_limit_hit = false;
    for (int i = 0; i < 20; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, memory_intensive_bytecode, sizeof(memory_intensive_bytecode), constants, 4);
        
        if (IS_NIL(result) && vm_has_error(vm)) {
            const char* error = vm_get_error(vm);
            if (strstr(error, "memory") || strstr(error, "limit")) {
                memory_limit_hit = true;
                printf("Memory limit hit at iteration %d\n", i);
                vm_clear_error(vm);
            }
        } else {
            TEST_ASSERT(IS_ARRAY(result), "Should return array when memory allows");
        }
        
        // Force garbage collection to free memory
        vm_collect_garbage(vm);
    }
    
    TEST_BENCHMARK_END("Memory-constrained execution (20 iterations)");
    
    // Test that system gracefully handles memory limits
    TEST_ASSERT(memory_limit_hit, "Memory limit should be enforced");
    
    // Verify system recovers after memory pressure
    uint8_t simple_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value simple_constants[] = {VALUE_NUMBER(123.0)};
    
    vm_reset(vm);
    Value recovery_result = vm_execute_bytecode(vm, simple_bytecode, sizeof(simple_bytecode), simple_constants, 1);
    TEST_ASSERT(IS_NUMBER(recovery_result), "System should recover after memory pressure");
    TEST_ASSERT_EQ(123.0, AS_NUMBER(recovery_result), "Recovery should produce correct results");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Performance under memory constraints should not leak memory");
    
    return true;
}

// Test memory safety in concurrent scenarios
static bool test_memory_safety_concurrent(void) {
    TEST_MEMORY_CHECKPOINT();
    
    // Simulate concurrent access with multiple VMs
    VM* vm1 = vm_create();
    VM* vm2 = vm_create();
    TEST_ASSERT_NOT_NULL(vm1, "First VM creation should succeed");
    TEST_ASSERT_NOT_NULL(vm2, "Second VM creation should succeed");
    
    // Create shared memory pool for testing
    MemoryPool* shared_pool = memory_pool_create(1024 * 1024);
    memory_pool_set_thread_safe(shared_pool, true);
    
    // Test concurrent allocations
    void* ptrs1[100];
    void* ptrs2[100];
    
    // Simulate concurrent allocation patterns
    for (int i = 0; i < 100; i++) {
        ptrs1[i] = memory_pool_alloc(shared_pool, 64 + (i % 32));
        ptrs2[i] = memory_pool_alloc(shared_pool, 96 + (i % 48));
        
        TEST_ASSERT_NOT_NULL(ptrs1[i], "Concurrent allocation 1 should succeed");
        TEST_ASSERT_NOT_NULL(ptrs2[i], "Concurrent allocation 2 should succeed");
        TEST_ASSERT_NEQ(ptrs1[i], ptrs2[i], "Concurrent allocations should be different");
    }
    
    // Test concurrent operations on VMs
    uint8_t concurrent_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CONSTANT, 1,
        OP_ADD,
        OP_RETURN
    };
    Value constants1[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    Value constants2[] = {VALUE_NUMBER(20.0), VALUE_NUMBER(7.0)};
    
    // Interleaved execution
    for (int i = 0; i < 50; i++) {
        vm_reset(vm1);
        Value result1 = vm_execute_bytecode(vm1, concurrent_bytecode, sizeof(concurrent_bytecode), constants1, 2);
        
        vm_reset(vm2);
        Value result2 = vm_execute_bytecode(vm2, concurrent_bytecode, sizeof(concurrent_bytecode), constants2, 2);
        
        TEST_ASSERT(IS_NUMBER(result1) && IS_NUMBER(result2), "Concurrent execution should return numbers");
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result1), "VM1 result should be correct");
        TEST_ASSERT_EQ(27.0, AS_NUMBER(result2), "VM2 result should be correct");
    }
    
    // Test concurrent memory cleanup
    for (int i = 0; i < 100; i++) {
        if (i % 2 == 0) {
            memory_pool_free(shared_pool, ptrs1[i]);
        } else {
            memory_pool_free(shared_pool, ptrs2[i]);
        }
    }
    
    // Verify memory pool integrity
    MemoryPoolStats stats = memory_pool_get_stats(shared_pool);
    TEST_ASSERT_EQ(100, stats.free_count, "All allocations should be freed");
    
    memory_pool_destroy(shared_pool);
    vm_destroy(vm1);
    vm_destroy(vm2);
    TEST_MEMORY_VERIFY("Memory safety concurrent test should not leak memory");
    
    return true;
}

// Test resource cleanup in error conditions
static bool test_resource_cleanup_on_errors(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Test cleanup after VM errors
    for (int error_type = 0; error_type < 5; error_type++) {
        // Set up resources before error
        vm_push_string(vm, "resource before error");
        Graph* graph = graph_create();
        NodeID node = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(42.0));
        
        // Trigger different types of errors
        switch (error_type) {
            case 0: // Stack overflow
                for (int i = 0; i < 10000; i++) {
                    if (!vm_push_number(vm, i)) break;
                }
                break;
                
            case 1: // Invalid bytecode
                {
                    uint8_t invalid_bytecode[] = {0xFF, 0xFF, 0xFF};
                    vm_execute_bytecode(vm, invalid_bytecode, sizeof(invalid_bytecode), NULL, 0);
                }
                break;
                
            case 2: // Division by zero
                {
                    uint8_t div_zero_bytecode[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_DIVIDE, OP_RETURN};
                    Value div_constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(0.0)};
                    vm_execute_bytecode(vm, div_zero_bytecode, sizeof(div_zero_bytecode), div_constants, 2);
                }
                break;
                
            case 3: // Invalid graph operation
                graph_add_edge(graph, node, INVALID_NODE_ID, 0);
                vm_execute_graph(vm, graph);
                break;
                
            case 4: // Memory allocation failure simulation
                vm_simulate_memory_allocation_failure(vm, true);
                vm_push_string(vm, "this should fail");
                vm_simulate_memory_allocation_failure(vm, false);
                break;
        }
        
        // Verify error state
        bool has_error = vm_has_error(vm);
        if (has_error) {
            printf("Error type %d: %s\n", error_type, vm_get_error(vm));
        }
        
        // Clean up resources
        graph_destroy(graph);
        vm_clear_error(vm);
        vm_reset(vm);
        vm_collect_garbage(vm);
        
        // Verify memory is cleaned up
        size_t current_memory = vm_get_memory_usage(vm);
        TEST_ASSERT(current_memory <= initial_memory + 1024 * 1024, "Memory should be cleaned up after errors");
    }
    
    // Final verification
    size_t final_memory = vm_get_memory_usage(vm);
    TEST_ASSERT(final_memory <= initial_memory + 512 * 1024, "Final memory should be close to initial");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("Resource cleanup on errors should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t memory_system_integration_tests[] = {
    test_cross_component_memory_allocation,
    test_system_wide_garbage_collection,
    test_memory_pressure_handling,
    test_memory_leak_detection,
    test_performance_under_memory_constraints,
    test_memory_safety_concurrent,
    test_resource_cleanup_on_errors
};

test_suite_t memory_system_integration_suite = {
    .name = "Memory System Integration Tests",
    .tests = memory_system_integration_tests,
    .test_count = sizeof(memory_system_integration_tests) / sizeof(memory_system_integration_tests[0])
};
