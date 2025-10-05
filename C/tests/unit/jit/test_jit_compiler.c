
#include "../../framework/test_framework.h"
#include "../../../vm/jit/jit_compiler.h"
#include "../../../vm/jit/hot_path.h"
#include "../../../vm/jit/tiered_compilation.h"

// Test JIT compiler initialization
static bool test_jit_compiler_init(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Test initial state
    TEST_ASSERT_EQ(0, jit_get_compiled_function_count(jit), "Initial compiled function count should be 0");
    TEST_ASSERT_EQ(0, jit_get_cache_size(jit), "Initial cache size should be 0");
    TEST_ASSERT(jit_is_enabled(jit), "JIT should be enabled by default");
    
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT compiler init should not leak memory");
    
    return true;
}

// Test hotspot detection
static bool test_hotspot_detection(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Create a mock function for testing
    uint8_t bytecode[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_ADD, OP_RETURN};
    size_t bytecode_size = sizeof(bytecode);
    
    // Simulate multiple executions to trigger hotspot detection
    for (int i = 0; i < JIT_HOTSPOT_THRESHOLD + 1; i++) {
        jit_record_execution(jit, bytecode, bytecode_size);
    }
    
    // Check if hotspot was detected
    bool is_hotspot = jit_is_hotspot(jit, bytecode, bytecode_size);
    TEST_ASSERT(is_hotspot, "Function should be detected as hotspot after threshold executions");
    
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("Hotspot detection should not leak memory");
    
    return true;
}

// Test JIT compilation process
static bool test_jit_compilation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Create bytecode for a simple arithmetic operation
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load constant 0 (42.0)
        OP_CONSTANT, 1,  // Load constant 1 (3.14)
        OP_ADD,          // Add them
        OP_RETURN        // Return result
    };
    size_t bytecode_size = sizeof(bytecode);
    
    // Create constants
    Value constants[] = {VALUE_NUMBER(42.0), VALUE_NUMBER(3.14)};
    
    // Compile the bytecode
    CompiledFunction* compiled = jit_compile_function(jit, bytecode, bytecode_size, constants, 2);
    TEST_ASSERT_NOT_NULL(compiled, "JIT compilation should succeed");
    
    // Verify compiled function properties
    TEST_ASSERT_NOT_NULL(compiled->native_code, "Compiled function should have native code");
    TEST_ASSERT(compiled->code_size > 0, "Compiled function should have non-zero code size");
    TEST_ASSERT_EQ(bytecode_size, compiled->bytecode_size, "Bytecode size should match");
    
    // Test execution of compiled function
    VM* vm = vm_create();
    Value result = jit_execute_compiled(compiled, vm);
    TEST_ASSERT(IS_NUMBER(result), "Result should be a number");
    TEST_ASSERT_EQ(45.14, AS_NUMBER(result), "Result should be 42.0 + 3.14");
    
    vm_destroy(vm);
    jit_free_compiled_function(compiled);
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT compilation should not leak memory");
    
    return true;
}

// Test JIT code cache management
static bool test_jit_code_cache(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Compile multiple functions to test cache
    for (int i = 0; i < 10; i++) {
        uint8_t bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
        Value constants[] = {VALUE_NUMBER(i)};
        
        CompiledFunction* compiled = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 1);
        TEST_ASSERT_NOT_NULL(compiled, "JIT compilation should succeed");
        
        // Add to cache
        jit_cache_add(jit, bytecode, sizeof(bytecode), compiled);
    }
    
    TEST_ASSERT_EQ(10, jit_get_compiled_function_count(jit), "Cache should contain 10 functions");
    
    // Test cache lookup
    uint8_t test_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    CompiledFunction* cached = jit_cache_lookup(jit, test_bytecode, sizeof(test_bytecode));
    TEST_ASSERT_NOT_NULL(cached, "Cache lookup should find compiled function");
    
    // Test cache eviction (fill beyond capacity)
    for (int i = 10; i < JIT_CACHE_MAX_SIZE + 5; i++) {
        uint8_t bytecode[] = {OP_CONSTANT, 0, OP_POP, OP_RETURN};
        Value constants[] = {VALUE_NUMBER(i)};
        
        CompiledFunction* compiled = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 1);
        jit_cache_add(jit, bytecode, sizeof(bytecode), compiled);
    }
    
    TEST_ASSERT(jit_get_compiled_function_count(jit) <= JIT_CACHE_MAX_SIZE, "Cache should not exceed maximum size");
    
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT code cache should not leak memory");
    
    return true;
}

// Test JIT optimization passes
static bool test_jit_optimization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Create bytecode with optimization opportunities
    uint8_t bytecode[] = {
        OP_CONSTANT, 0,  // Load 5.0
        OP_CONSTANT, 1,  // Load 3.0
        OP_ADD,          // Add (can be constant folded)
        OP_CONSTANT, 2,  // Load 2.0
        OP_MULTIPLY,     // Multiply
        OP_RETURN
    };
    Value constants[] = {VALUE_NUMBER(5.0), VALUE_NUMBER(3.0), VALUE_NUMBER(2.0)};
    
    // Compile without optimization
    jit_set_optimization_level(jit, JIT_OPT_NONE);
    CompiledFunction* unoptimized = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 3);
    TEST_ASSERT_NOT_NULL(unoptimized, "Unoptimized compilation should succeed");
    
    // Compile with optimization
    jit_set_optimization_level(jit, JIT_OPT_AGGRESSIVE);
    CompiledFunction* optimized = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 3);
    TEST_ASSERT_NOT_NULL(optimized, "Optimized compilation should succeed");
    
    // Test that both produce the same result
    VM* vm = vm_create();
    Value result1 = jit_execute_compiled(unoptimized, vm);
    Value result2 = jit_execute_compiled(optimized, vm);
    
    TEST_ASSERT(IS_NUMBER(result1) && IS_NUMBER(result2), "Both results should be numbers");
    TEST_ASSERT_EQ(AS_NUMBER(result1), AS_NUMBER(result2), "Results should be identical");
    TEST_ASSERT_EQ(16.0, AS_NUMBER(result1), "Result should be (5+3)*2 = 16");
    
    // Optimized version might be smaller due to constant folding
    TEST_ASSERT(optimized->code_size <= unoptimized->code_size, "Optimized code should not be larger");
    
    vm_destroy(vm);
    jit_free_compiled_function(unoptimized);
    jit_free_compiled_function(optimized);
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT optimization should not leak memory");
    
    return true;
}

// Test JIT error handling
static bool test_jit_error_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Test compilation of invalid bytecode
    uint8_t invalid_bytecode[] = {0xFF, 0xFF, 0xFF}; // Invalid opcodes
    CompiledFunction* compiled = jit_compile_function(jit, invalid_bytecode, sizeof(invalid_bytecode), NULL, 0);
    TEST_ASSERT_NULL(compiled, "Compilation of invalid bytecode should fail");
    
    // Test error state
    TEST_ASSERT(jit_has_error(jit), "JIT should have error after failed compilation");
    const char* error = jit_get_error(jit);
    TEST_ASSERT_NOT_NULL(error, "Error message should be available");
    
    // Clear error and test recovery
    jit_clear_error(jit);
    TEST_ASSERT(!jit_has_error(jit), "Error should be cleared");
    
    // Test successful compilation after error recovery
    uint8_t valid_bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value constants[] = {VALUE_NUMBER(42.0)};
    compiled = jit_compile_function(jit, valid_bytecode, sizeof(valid_bytecode), constants, 1);
    TEST_ASSERT_NOT_NULL(compiled, "Compilation should succeed after error recovery");
    
    jit_free_compiled_function(compiled);
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT error handling should not leak memory");
    
    return true;
}

// Test JIT performance profiling
static bool test_jit_profiling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    JITCompiler* jit = jit_compiler_create();
    TEST_ASSERT_NOT_NULL(jit, "JIT compiler creation should succeed");
    
    // Enable profiling
    jit_enable_profiling(jit, true);
    TEST_ASSERT(jit_is_profiling_enabled(jit), "Profiling should be enabled");
    
    // Compile and execute a function
    uint8_t bytecode[] = {OP_CONSTANT, 0, OP_RETURN};
    Value constants[] = {VALUE_NUMBER(42.0)};
    
    CompiledFunction* compiled = jit_compile_function(jit, bytecode, sizeof(bytecode), constants, 1);
    TEST_ASSERT_NOT_NULL(compiled, "Compilation should succeed");
    
    // Execute multiple times to gather profile data
    VM* vm = vm_create();
    for (int i = 0; i < 100; i++) {
        jit_execute_compiled(compiled, vm);
    }
    
    // Check profiling data
    JITProfileData* profile = jit_get_profile_data(jit, bytecode, sizeof(bytecode));
    TEST_ASSERT_NOT_NULL(profile, "Profile data should be available");
    TEST_ASSERT_EQ(100, profile->execution_count, "Execution count should be tracked");
    TEST_ASSERT(profile->total_time > 0, "Total execution time should be recorded");
    
    vm_destroy(vm);
    jit_free_compiled_function(compiled);
    jit_compiler_destroy(jit);
    TEST_MEMORY_VERIFY("JIT profiling should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t jit_compiler_tests[] = {
    test_jit_compiler_init,
    test_hotspot_detection,
    test_jit_compilation,
    test_jit_code_cache,
    test_jit_optimization,
    test_jit_error_handling,
    test_jit_profiling
};

test_suite_t jit_test_suite = {
    .name = "JIT Compiler Tests",
    .tests = jit_compiler_tests,
    .test_count = sizeof(jit_compiler_tests) / sizeof(jit_compiler_tests[0])
};
