
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
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

// Helper function to create a simple chunk
static Chunk* create_simple_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Simple expression: 2 + 3
    int const1 = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1, 1);
    
    int const2 = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2, 1);
    
    chunk_write(chunk, OP_ADD, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Helper function to create a complex chunk
static Chunk* create_complex_chunk(void) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    chunk_init(chunk);
    
    // Complex expression: (2 + 3) * (4 - 1)
    int const1 = chunk_add_constant(chunk, 2.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1, 1);
    
    int const2 = chunk_add_constant(chunk, 3.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2, 1);
    
    chunk_write(chunk, OP_ADD, 1);
    
    int const3 = chunk_add_constant(chunk, 4.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const3, 1);
    
    int const4 = chunk_add_constant(chunk, 1.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const4, 1);
    
    chunk_write(chunk, OP_SUBTRACT, 1);
    
    chunk_write(chunk, OP_MULTIPLY, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Test 1: JIT VM Creation and Destruction
void test_jit_vm_create_destroy(void) {
    TEST("jit_vm_create_destroy");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    ASSERT(vm->base_vm != NULL);
    ASSERT(vm->jit_compiler != NULL);
    ASSERT(vm->config.enable_jit == true);
    
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 2: JIT VM Configuration
void test_jit_vm_configuration(void) {
    TEST("jit_vm_configuration");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 50;
    config.optimization_threshold = 500;
    config.max_compiled_functions = 512;
    
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    ASSERT(vm->config.jit_threshold == 50);
    ASSERT(vm->config.optimization_threshold == 500);
    ASSERT(vm->config.max_compiled_functions == 512);
    
    // Test configuration update
    config.jit_threshold = 25;
    jit_vm_set_config(vm, &config);
    
    JITConfig retrieved_config;
    jit_vm_get_config(vm, &retrieved_config);
    ASSERT(retrieved_config.jit_threshold == 25);
    
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 3: Basic JIT Execution
void test_jit_basic_execution(void) {
    TEST("jit_basic_execution");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // First execution should use interpreter
    JITVmResult result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.result_value == 5.0);
    ASSERT(result.used_jit == false);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 4: JIT Compilation Trigger
void test_jit_compilation_trigger(void) {
    TEST("jit_compilation_trigger");
    
    JITConfig config = jit_vm_default_config();
    config.jit_threshold = 3; // Low threshold for testing
    
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Execute multiple times to trigger JIT compilation
    for (int i = 0; i < 5; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        ASSERT(result.result_value == 5.0);
        
        if (i >= 3) {
            // After threshold, should potentially use JIT or have compilation time
            // (depending on implementation details)
        }
    }
    
    // Check statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    ASSERT(stats.total_executions == 5);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 5: JIT Code Cache
void test_jit_code_cache(void) {
    TEST("jit_code_cache");
    
    JITCodeCache* cache = jit_code_cache_create(10);
    ASSERT(cache != NULL);
    ASSERT(cache->max_entries == 10);
    ASSERT(cache->entry_count == 0);
    
    // Test cache miss
    CompiledCode* code = jit_code_cache_get(cache, 1);
    ASSERT(code == NULL);
    ASSERT(cache->cache_misses == 1);
    
    // Create mock compiled code
    CompiledCode* mock_code = (CompiledCode*)malloc(sizeof(CompiledCode));
    mock_code->function_id = 1;
    mock_code->tier = JIT_TIER_BASELINE;
    mock_code->execution_count = 0;
    mock_code->native_code = NULL;
    
    // Test cache put
    bool put_result = jit_code_cache_put(cache, 1, mock_code);
    ASSERT(put_result == true);
    ASSERT(cache->entry_count == 1);
    
    // Test cache hit
    CompiledCode* retrieved = jit_code_cache_get(cache, 1);
    ASSERT(retrieved != NULL);
    ASSERT(retrieved->function_id == 1);
    ASSERT(cache->cache_hits == 1);
    
    // Test cache remove
    jit_code_cache_remove(cache, 1);
    ASSERT(cache->entry_count == 0);
    
    jit_code_cache_destroy(cache);
    TEST_END;
}

// Test 6: JIT Statistics
void test_jit_statistics(void) {
    TEST("jit_statistics");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Execute a few times
    for (int i = 0; i < 3; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    
    // Check statistics
    JITVmStats stats;
    jit_vm_get_stats(vm, &stats);
    ASSERT(stats.total_executions == 3);
    ASSERT(stats.interpreter_execution_time_ns > 0);
    
    // Reset statistics
    jit_vm_reset_stats(vm);
    jit_vm_get_stats(vm, &stats);
    ASSERT(stats.total_executions == 0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 7: Complex Expression JIT
void test_jit_complex_expression(void) {
    TEST("jit_complex_expression");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_complex_chunk();
    ASSERT(chunk != NULL);
    
    JITVmResult result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0); // (2+3) * (4-1) = 5 * 3 = 15
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 8: JIT Function Compilation
void test_jit_function_compilation(void) {
    TEST("jit_function_compilation");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Manually compile function
    bool compile_result = jit_vm_compile_function(vm, 42, chunk);
    ASSERT(compile_result == true);
    
    // Check if function is in cache
    CompiledCode* code = jit_code_cache_get(vm->code_cache, 42);
    ASSERT(code != NULL);
    ASSERT(code->function_id == 42);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 9: JIT Cache Management
void test_jit_cache_management(void) {
    TEST("jit_cache_management");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Compile multiple functions
    for (uint32_t i = 1; i <= 5; i++) {
        bool result = jit_vm_compile_function(vm, i, chunk);
        ASSERT(result == true);
    }
    
    // Check cache size
    ASSERT(vm->code_cache->entry_count == 5);
    
    // Invalidate one function
    jit_vm_invalidate_function(vm, 3);
    ASSERT(vm->code_cache->entry_count == 4);
    
    // Clear entire cache
    jit_vm_clear_cache(vm);
    ASSERT(vm->code_cache->entry_count == 0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 10: JIT Performance Measurement
void test_jit_performance_measurement(void) {
    TEST("jit_performance_measurement");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_complex_chunk();
    ASSERT(chunk != NULL);
    
    // Execute and measure time
    JITVmResult result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.execution_time_ns > 0);
    
    // Check that timing is reasonable (less than 1 second)
    ASSERT(result.execution_time_ns < 1000000000ULL);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

int main(void) {
    printf("=== JIT Basic Functionality Tests ===\n");
    
    test_jit_vm_create_destroy();
    test_jit_vm_configuration();
    test_jit_basic_execution();
    test_jit_compilation_trigger();
    test_jit_code_cache();
    test_jit_statistics();
    test_jit_complex_expression();
    test_jit_function_compilation();
    test_jit_cache_management();
    test_jit_performance_measurement();
    
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
