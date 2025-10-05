
#include "../../framework/test_framework.h"
#include "../../../vm/vm.h"
#include "../../../vm/jit/jit_compiler.h"
#include "../../../vm/vm_jit_integration.h"

// Test seamless VM-JIT execution transition
static bool test_vm_jit_transition(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(vm, "VM creation should succeed");
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Integrate JIT with VM
    vm_set_jit_compiler(vm, jit);
    TEST_ASSERT(vm_has_jit_compiler(vm), "VM should have JIT compiler after setting");
    
    // Create bytecode that will trigger JIT compilation
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load 10.0
        OP_CONSTANT, 1,  // Load 5.0
        OP_ADD,          // Add them
        OP_RETURN        // Return result
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Execute multiple times to trigger hotspot detection
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 2);
        TEST_ASSERT(IS_NUMBER(result), "Execution should return a number");
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Result should be 10 + 5 = 15");
    }
    
    // Verify that JIT compilation was triggered
    TEST_ASSERT(jit_is_hotspot(jit, bytecode, sizeof(bytecode)), "Bytecode should be detected as hotspot");
    CompiledFunction* compiled = jit_cache_lookup(jit, bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(compiled, "Compiled function should be in cache");
    
    // Test that subsequent executions use JIT-compiled code
    vm_reset(vm);
    TEST_BENCHMARK_START();
    Value jit_result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 2);
    TEST_BENCHMARK_END("JIT-compiled execution");
    
    TEST_ASSERT(IS_NUMBER(jit_result), "JIT execution should return a number");
    TEST_ASSERT_EQ(15.0, AS_NUMBER(jit_result), "JIT result should match interpreter result");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("VM-JIT transition should not leak memory");
    
    return true;
}

// Test JIT compilation performance improvement
static bool test_jit_performance_improvement(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create a more complex bytecode for better performance comparison
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load 1.0
        OP_CONSTANT, 1,  // Load 1000.0
        OP_CONSTANT, 2,  // Load 0.0 (counter)
        // Loop start
        OP_DUP,          // Duplicate counter
        OP_CONSTANT, 1,  // Load 1000.0
        OP_LESS,         // counter < 1000?
        OP_JUMP_IF_FALSE, 15, // Jump to end if false
        OP_CONSTANT, 0,  // Load 1.0
        OP_ADD,          // counter += 1
        OP_JUMP, 3,      // Jump back to loop start
        // Loop end
        OP_RETURN        // Return counter
    };
    Value constants[] = {VALUE_NUMBER(1.0), VALUE_NUMBER(1000.0), VALUE_NUMBER(0.0)};
    
    // Measure interpreter performance
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 3);
    }
    TEST_BENCHMARK_END("Interpreter execution (10 iterations)");
    
    // Trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 3);
    }
    
    // Measure JIT performance
    TEST_BENCHMARK_START();
    for (int i = 0; i < 10; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 3);
    }
    TEST_BENCHMARK_END("JIT execution (10 iterations)");
    
    // Verify JIT compilation occurred
    TEST_ASSERT(jit_is_hotspot(jit, bytecode, sizeof(bytecode)), "Complex bytecode should be hotspot");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT performance test should not leak memory");
    
    return true;
}

// Test JIT error handling and fallback
static bool test_jit_error_fallback(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create bytecode that might cause JIT compilation issues
    uint8_t problematic_bytecode[] = {
        OP_CONSTANT, 0,
        OP_CONSTANT, 1,
        OP_DIVIDE,       // Division might cause issues in JIT
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(0.0)}; // Division by zero
    
    // Execute to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, problematic_bytecode, sizeof(problematic_bytecode), constants, 2);
        // Should handle division by zero gracefully
        TEST_ASSERT(IS_NUMBER(result), "Should return a number even with division by zero");
    }
    
    // Test that VM continues to work even if JIT compilation fails
    TEST_ASSERT(!vm_has_error(vm), "VM should not have errors after JIT fallback");
    
    // Test with valid bytecode after error
    uint8_t valid_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value valid_constants[] = {VALUE_NUMBER(42.0)};
    
    vm_reset(vm);
    Value result = vm_execute_bytecode(vm, valid_bytecode, sizeof(valid_bytecode), valid_constants, 1);
    TEST_ASSERT(IS_NUMBER(result), "VM should work normally after JIT error");
    TEST_ASSERT_EQ(42.0, AS_NUMBER(result), "Result should be correct");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT error fallback should not leak memory");
    
    return true;
}

// Test JIT code cache management
static bool test_jit_cache_integration(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create multiple different bytecode sequences
    uint8_t bytecode1[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_ADD, OP_RETURN};
    uint8_t bytecode2[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_MULTIPLY, OP_RETURN};
    uint8_t bytecode3[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_SUBTRACT, OP_RETURN};
    
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Execute each bytecode sequence enough times to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode1, sizeof(bytecode1), constants, 2);
        
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode2, sizeof(bytecode2), constants, 2);
        
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode3, sizeof(bytecode3), constants, 2);
    }
    
    // Verify all three are compiled and cached
    TEST_ASSERT(jit_cache_lookup(jit, bytecode1, sizeof(bytecode1)) != NULL, "Bytecode1 should be cached");
    TEST_ASSERT(jit_cache_lookup(jit, bytecode2, sizeof(bytecode2)) != NULL, "Bytecode2 should be cached");
    TEST_ASSERT(jit_cache_lookup(jit, bytecode3, sizeof(bytecode3)) != NULL, "Bytecode3 should be cached");
    
    TEST_ASSERT_EQ(3, jit_get_compiled_function_count(jit), "Should have 3 compiled functions");
    
    // Test cache hit performance
    TEST_BENCHMARK_START();
    for (int i = 0; i < 100; i++) {
        vm_reset(vm);
        vm_execute_bytecode(vm, bytecode1, sizeof(bytecode1), constants, 2);
    }
    TEST_BENCHMARK_END("Cache hit performance (100 executions)");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT cache integration should not leak memory");
    
    return true;
}

// Test JIT with complex control flow
static bool test_jit_control_flow(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create bytecode with conditional jumps
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,      // Load input value
        OP_CONSTANT, 1,      // Load 0
        OP_GREATER,          // input > 0?
        OP_JUMP_IF_FALSE, 8, // Jump to else branch
        OP_CONSTANT, 2,      // Load 1 (positive result)
        OP_JUMP, 10,         // Jump to end
        OP_CONSTANT, 3,      // Load -1 (negative/zero result)
        OP_RETURN            // Return result
    };
    
    // Test with positive input
    Value constants_pos[] = {VALUE_NUMBER(5.0), VALUE_NUMBER(0.0), VALUE_NUMBER(1.0), VALUE_NUMBER(-1.0)};
    
    // Execute enough times to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants_pos, 4);
        TEST_ASSERT(IS_NUMBER(result), "Should return a number");
        TEST_ASSERT_EQ(1.0, AS_NUMBER(result), "Positive input should return 1");
    }
    
    // Test with negative input using JIT-compiled code
    Value constants_neg[] = {VALUE_NUMBER(-3.0), VALUE_NUMBER(0.0), VALUE_NUMBER(1.0), VALUE_NUMBER(-1.0)};
    vm_reset(vm);
    Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants_neg, 4);
    TEST_ASSERT(IS_NUMBER(result), "Should return a number");
    TEST_ASSERT_EQ(-1.0, AS_NUMBER(result), "Negative input should return -1");
    
    // Verify JIT compilation handled control flow correctly
    TEST_ASSERT(jit_is_hotspot(jit, bytecode, sizeof(bytecode)), "Control flow bytecode should be hotspot");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT control flow should not leak memory");
    
    return true;
}

// Test JIT with function calls
static bool test_jit_function_calls(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create bytecode that calls a function
    uint8_t main_bytecode[] = {
        OP_CONSTANT, 0,      // Load argument 1
        OP_CONSTANT, 1,      // Load argument 2
        OP_CALL, 2,          // Call function with 2 arguments
        OP_RETURN            // Return result
    };
    
    uint8_t function_bytecode[] = {
        OP_GET_LOCAL, 0,     // Get first argument
        OP_GET_LOCAL, 1,     // Get second argument
        OP_ADD,              // Add them
        OP_RETURN            // Return result
    };
    
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Register the function with the VM
    vm_register_function(vm, "add", function_bytecode, sizeof(function_bytecode));
    
    // Execute main function multiple times to trigger JIT compilation
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, main_bytecode, sizeof(main_bytecode), constants, 2);
        TEST_ASSERT(IS_NUMBER(result), "Function call should return a number");
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Function call result should be 10 + 5 = 15");
    }
    
    // Verify both main and called function can be JIT compiled
    TEST_ASSERT(jit_is_hotspot(jit, main_bytecode, sizeof(main_bytecode)), "Main function should be hotspot");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT function calls should not leak memory");
    
    return true;
}

// Test JIT memory management integration
static bool test_jit_memory_integration(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Create bytecode that allocates objects
    uint8_t bytecode[] = {
        OP_NEW_OBJECT,       // Allocate new object
        OP_CONSTANT, 0,      // Load property name
        OP_CONSTANT, 1,      // Load property value
        OP_SET_PROPERTY,     // Set property
        OP_RETURN            // Return object
    };
    Value constants[] = {VALUE_STRING("test_prop"), VALUE_NUMBER(42.0)};
    
    size_t initial_memory = vm_get_memory_usage(vm);
    
    // Execute multiple times to trigger JIT compilation and test memory management
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 5; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 2);
        TEST_ASSERT(IS_OBJECT(result), "Should return an object");
        
        // Trigger garbage collection periodically
        if (i % 10 == 0) {
            vm_collect_garbage(vm);
        }
    }
    
    // Final garbage collection
    vm_collect_garbage(vm);
    
    size_t final_memory = vm_get_memory_usage(vm);
    
    // Memory usage should be reasonable (not growing unboundedly)
    TEST_ASSERT(final_memory < initial_memory + 1024 * 1024, "Memory usage should be controlled");
    
    // Verify JIT compilation occurred
    TEST_ASSERT(jit_is_hotspot(jit, bytecode, sizeof(bytecode)), "Object allocation bytecode should be hotspot");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT memory integration should not leak memory");
    
    return true;
}

// Test JIT debugging and profiling integration
static bool test_jit_debugging_integration(void) {
    TEST_MEMORY_CHECKPOINT();
    
    VM* vm = vm_create();
    JITCompiler* jit = jit_compiler_create();
    vm_set_jit_compiler(vm, jit);
    
    // Enable debugging and profiling
    vm_set_debug_mode(vm, true);
    jit_enable_profiling(jit, true);
    
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,
        OP_CONSTANT, 1,
        OP_ADD,
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(10.0), VALUE_NUMBER(5.0)};
    
    // Execute with debugging enabled
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        vm_reset(vm);
        Value result = vm_execute_bytecode(vm, bytecode, sizeof(bytecode), constants, 2);
        TEST_ASSERT(IS_NUMBER(result), "Debugging should not affect execution");
        TEST_ASSERT_EQ(15.0, AS_NUMBER(result), "Result should be correct with debugging");
    }
    
    // Check that profiling data was collected
    JITProfileData* profile = jit_get_profile_data(jit, bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(profile, "Profile data should be available");
    TEST_ASSERT(profile->execution_count > JIT_HOTSPOT_THRESHOLD, "Execution count should be tracked");
    TEST_ASSERT(profile->total_time > 0, "Execution time should be recorded");
    
    // Test debugging information preservation
    TEST_ASSERT(vm_has_debug_info(vm), "VM should have debug information");
    
    vm_destroy(vm);
    TEST_MEMORY_VERIFY("JIT debugging integration should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t vm_jit_integration_tests[] = {
    test_vm_jit_transition,
    test_jit_performance_improvement,
    test_jit_error_fallback,
    test_jit_cache_integration,
    test_jit_control_flow,
    test_jit_function_calls,
    test_jit_memory_integration,
    test_jit_debugging_integration
};

test_suite_t vm_jit_integration_suite = {
    .name = "VM-JIT Integration Tests",
    .tests = vm_jit_integration_tests,
    .test_count = sizeof(vm_jit_integration_tests) / sizeof(vm_jit_integration_tests[0])
};
