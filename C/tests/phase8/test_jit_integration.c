

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../../vm/vm_jit_integration.h"
#include "../../vm/bci_chunk.h"
#include "../../vm/gc/generational_gc.h"
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
    
    int const1_idx = chunk_add_constant(chunk, 10.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const1_idx, 1);
    int const2_idx = chunk_add_constant(chunk, 5.0);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, const2_idx, 1);
    chunk_write(chunk, OP_ADD, 1);
    chunk_write(chunk, OP_RETURN, 1);
    
    return chunk;
}

// Test 1: JIT Integration with Enhanced VM
void test_jit_enhanced_vm_integration(void) {
    TEST("jit_enhanced_vm_integration");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    ASSERT(vm->base_vm != NULL);
    ASSERT(vm->base_vm->gc != NULL);
    ASSERT(vm->jit_compiler != NULL);
    
    // Test that enhanced VM features are accessible
    enhanced_vm_enable_gc(vm->base_vm, true);
    enhanced_vm_enable_profiling(vm->base_vm, true);
    
    Chunk* chunk = create_simple_chunk();
    JITVmResult result = jit_vm_execute(vm, chunk);
    
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 2: JIT with Garbage Collection Integration
void test_jit_gc_integration(void) {
    TEST("jit_gc_integration");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Enable GC
    enhanced_vm_enable_gc(vm->base_vm, true);
    
    // Allocate some objects through the VM
    void* obj1 = vm_alloc(vm->base_vm, 100);
    void* obj2 = vm_alloc(vm->base_vm, 200);
    ASSERT(obj1 != NULL);
    ASSERT(obj2 != NULL);
    
    // Execute JIT code while GC is active
    Chunk* chunk = create_simple_chunk();
    JITVmResult result = jit_vm_execute(vm, chunk);
    
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0);
    
    // Trigger GC manually
    vm_gc_collect(vm->base_vm);
    
    // Execute again after GC
    result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0);
    
    // Check GC statistics
    uint64_t gc_collections, gc_allocated, gc_freed;
    size_t young_used, old_used;
    enhanced_vm_get_gc_stats(vm->base_vm, &gc_collections, &gc_allocated, &gc_freed, &young_used, &old_used);
    
    ASSERT(gc_collections > 0);
    ASSERT(gc_allocated >= 300); // At least the objects we allocated
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 3: JIT with Hot Path Detection Integration
void test_jit_hot_path_integration(void) {
    TEST("jit_hot_path_integration");
    
    JITConfig config = jit_vm_default_config();
    config.enable_hotspot_detection = true;
    config.jit_threshold = 5;
    
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    ASSERT(vm->hot_path_detector != NULL);
    
    Chunk* chunk = create_simple_chunk();
    
    // Execute multiple times to trigger hot path detection
    for (int i = 0; i < 10; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        ASSERT(result.result_value == 15.0);
    }
    
    // Check hot path detector statistics
    uint64_t total_executions, hot_hits, cold_hits;
    hot_path_detector_get_stats(vm->hot_path_detector, &total_executions, &hot_hits, &cold_hits);
    
    ASSERT(total_executions == 10);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 4: JIT with Tiered Compilation Integration
void test_jit_tiered_compilation_integration(void) {
    TEST("jit_tiered_compilation_integration");
    
    JITConfig config = jit_vm_default_config();
    config.enable_tiered_compilation = true;
    config.jit_threshold = 3;
    config.optimization_threshold = 8;
    
    JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    ASSERT(vm->tiered_compilation != NULL);
    
    Chunk* chunk = create_simple_chunk();
    
    // Execute multiple times to trigger tiered compilation
    for (int i = 0; i < 15; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        ASSERT(result.result_value == 15.0);
    }
    
    // Check tiered compilation statistics
    uint64_t tier_transitions, deoptimizations, compilation_decisions;
    tiered_compilation_get_stats(vm->tiered_compilation, &tier_transitions, &deoptimizations, &compilation_decisions);
    
    printf("\n  Tier transitions: %lu", tier_transitions);
    printf("\n  Compilation decisions: %lu", compilation_decisions);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 5: JIT Pipeline Integration
void test_jit_pipeline_integration(void) {
    TEST("jit_pipeline_integration");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Test that JIT works with the existing pipeline
    // This would normally involve the full compilation pipeline
    // For now, test basic integration
    
    Chunk* chunk = create_simple_chunk();
    
    // Execute with JIT integration
    JITVmResult result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0);
    
    // Compare with enhanced VM direct execution
    EnhancedVmResult enhanced_result = enhanced_vm_execute_with_result(vm->base_vm, chunk);
    ASSERT(enhanced_result.success == true);
    ASSERT(enhanced_result.result_value == result.result_value);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 6: Multi-Component Integration Test
void test_multi_component_integration(void) {
    TEST("multi_component_integration");
    
    JITConfig config = jit_vm_default_config();
    config.enable_jit = true;
    config.enable_hotspot_detection = true;
    config.enable_tiered_compilation = true;
    config.jit_threshold = 5;
    
    JITIntegratedVM* vm = jit_vm_create_with_config(2 * 1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    // Enable all features
    enhanced_vm_enable_gc(vm->base_vm, true);
    enhanced_vm_enable_profiling(vm->base_vm, true);
    
    // Allocate some objects
    void* obj1 = vm_alloc(vm->base_vm, 500);
    void* obj2 = vm_alloc(vm->base_vm, 300);
    ASSERT(obj1 != NULL);
    ASSERT(obj2 != NULL);
    
    Chunk* chunk = create_simple_chunk();
    
    // Execute multiple times with all components active
    for (int i = 0; i < 20; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        ASSERT(result.result_value == 15.0);
        
        // Occasionally trigger GC
        if (i % 5 == 0) {
            vm_gc_collect(vm->base_vm);
        }
    }
    
    // Check comprehensive statistics
    JITVmStats jit_stats;
    jit_vm_get_stats(vm, &jit_stats);
    
    uint64_t gc_collections, gc_allocated, gc_freed;
    size_t young_used, old_used;
    enhanced_vm_get_gc_stats(vm->base_vm, &gc_collections, &gc_allocated, &gc_freed, &young_used, &old_used);
    
    printf("\n  JIT executions: %lu", jit_stats.total_executions);
    printf("\n  GC collections: %lu", gc_collections);
    printf("\n  Cache hits: %lu", jit_stats.cache_hits);
    
    ASSERT(jit_stats.total_executions == 20);
    ASSERT(gc_collections >= 4); // At least the manual collections
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 7: JIT Error Handling Integration
void test_jit_error_handling_integration(void) {
    TEST("jit_error_handling_integration");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Test with NULL chunk
    JITVmResult result = jit_vm_execute(vm, NULL);
    ASSERT(result.success == false);
    
    // Test with invalid function compilation
    bool compile_result = jit_vm_compile_function(vm, 999, NULL);
    ASSERT(compile_result == false);
    
    // Test cache operations with invalid IDs
    jit_vm_invalidate_function(vm, 999); // Should not crash
    
    // Test with valid chunk after errors
    Chunk* chunk = create_simple_chunk();
    result = jit_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.result_value == 15.0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

// Test 8: JIT Resource Management Integration
void test_jit_resource_management_integration(void) {
    TEST("jit_resource_management_integration");
    
    // Test multiple VM creation and destruction
    for (int i = 0; i < 5; i++) {
        JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
        ASSERT(vm != NULL);
        
        Chunk* chunk = create_simple_chunk();
        
        // Compile some functions
        for (uint32_t j = 1; j <= 3; j++) {
            bool result = jit_vm_compile_function(vm, j, chunk);
            ASSERT(result == true);
        }
        
        // Execute
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
        
        chunk_free(chunk);
        jit_vm_destroy(vm);
    }
    
    TEST_END;
}

// Test 9: JIT Configuration Integration
void test_jit_configuration_integration(void) {
    TEST("jit_configuration_integration");
    
    // Test different configurations
    JITConfig configs[] = {
        {true, true, true, 10, 100, 50},    // Full JIT
        {true, false, false, 5, 50, 25},    // JIT only
        {false, false, false, 0, 0, 0},     // Interpreter only
    };
    
    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
        JITIntegratedVM* vm = jit_vm_create_with_config(1024 * 1024, &configs[i]);
        ASSERT(vm != NULL);
        
        Chunk* chunk = create_simple_chunk();
        
        // Execute multiple times
        for (int j = 0; j < 10; j++) {
            JITVmResult result = jit_vm_execute(vm, chunk);
            ASSERT(result.success == true);
            ASSERT(result.result_value == 15.0);
        }
        
        // Check that configuration is respected
        JITConfig retrieved_config;
        jit_vm_get_config(vm, &retrieved_config);
        ASSERT(retrieved_config.enable_jit == configs[i].enable_jit);
        ASSERT(retrieved_config.enable_hotspot_detection == configs[i].enable_hotspot_detection);
        
        chunk_free(chunk);
        jit_vm_destroy(vm);
    }
    
    TEST_END;
}

// Test 10: JIT Statistics Integration
void test_jit_statistics_integration(void) {
    TEST("jit_statistics_integration");
    
    JITIntegratedVM* vm = jit_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    
    // Execute and compile
    for (int i = 0; i < 15; i++) {
        JITVmResult result = jit_vm_execute(vm, chunk);
        ASSERT(result.success == true);
    }
    
    // Compile some functions manually
    for (uint32_t i = 1; i <= 5; i++) {
        jit_vm_compile_function(vm, i, chunk);
    }
    
    // Get comprehensive statistics
    JITVmStats jit_stats;
    jit_vm_get_stats(vm, &jit_stats);
    
    uint64_t total_exec, jit_exec, interp_exec;
    enhanced_vm_get_stats(vm->base_vm, &total_exec, &jit_exec, &interp_exec);
    
    printf("\n  JIT VM total executions: %lu", jit_stats.total_executions);
    printf("\n  Enhanced VM total executions: %lu", total_exec);
    printf("\n  JIT compilations: %lu", jit_stats.jit_compilations);
    printf("\n  Cache entries: %zu", vm->code_cache->entry_count);
    
    ASSERT(jit_stats.total_executions == 15);
    ASSERT(jit_stats.jit_compilations >= 5);
    
    // Test statistics reset
    jit_vm_reset_stats(vm);
    jit_vm_get_stats(vm, &jit_stats);
    ASSERT(jit_stats.total_executions == 0);
    
    chunk_free(chunk);
    jit_vm_destroy(vm);
    TEST_END;
}

int main(void) {
    printf("=== JIT Integration Tests ===\n");
    
    test_jit_enhanced_vm_integration();
    test_jit_gc_integration();
    test_jit_hot_path_integration();
    test_jit_tiered_compilation_integration();
    test_jit_pipeline_integration();
    test_multi_component_integration();
    test_jit_error_handling_integration();
    test_jit_resource_management_integration();
    test_jit_configuration_integration();
    test_jit_statistics_integration();
    
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

