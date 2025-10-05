
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/vm_graph_integration.h"
#include "../../vm/bci_chunk.h"

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

#define ASSERT_NEAR(a, b, epsilon) \
    ASSERT(fabs((a) - (b)) < (epsilon))

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

// Test 1: Graph VM creation and destruction
void test_graph_vm_creation(void) {
    TEST("Graph VM Creation and Destruction");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    ASSERT(vm->base_vm != NULL);
    ASSERT(vm->graph_executor != NULL);
    ASSERT(vm->graph_optimizer != NULL);
    ASSERT(vm->graph_cache != NULL);
    
    // Test validation
    bool valid = graph_vm_validate_integration(vm);
    ASSERT(valid == true);
    
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 2: Configuration management
void test_configuration_management(void) {
    TEST("Configuration Management");
    
    GraphVMConfig default_config = graph_vm_default_config();
    ASSERT(default_config.mode == GRAPH_VM_MODE_HYBRID);
    ASSERT(default_config.enable_graph_optimization == true);
    
    // Test custom configuration
    GraphVMConfig custom_config = default_config;
    custom_config.mode = GRAPH_VM_MODE_GRAPH_ONLY;
    custom_config.graph_cache_size = 512;
    
    GraphIntegratedVM* vm = graph_vm_create_with_config(1024 * 1024, &custom_config);
    ASSERT(vm != NULL);
    
    GraphVMConfig retrieved_config;
    graph_vm_get_config(vm, &retrieved_config);
    ASSERT(retrieved_config.mode == GRAPH_VM_MODE_GRAPH_ONLY);
    ASSERT(retrieved_config.graph_cache_size == 512);
    
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 3: Graph cache functionality
void test_graph_cache_functionality(void) {
    TEST("Graph Cache Functionality");
    
    GraphCache* cache = graph_cache_create(10);
    ASSERT(cache != NULL);
    ASSERT(cache->max_entries == 10);
    ASSERT(cache->entry_count == 0);
    
    // Create a simple graph
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Test cache operations
    uint32_t function_id = 123;
    
    // Initially not in cache
    Graph* cached = graph_cache_get(cache, function_id);
    ASSERT(cached == NULL);
    
    // Add to cache
    bool added = graph_cache_put(cache, function_id, graph);
    ASSERT(added == true);
    ASSERT(cache->entry_count == 1);
    
    // Retrieve from cache
    cached = graph_cache_get(cache, function_id);
    ASSERT(cached == graph);
    
    // Remove from cache
    graph_cache_remove(cache, function_id);
    ASSERT(cache->entry_count == 0);
    
    cached = graph_cache_get(cache, function_id);
    ASSERT(cached == NULL);
    
    graph_cache_destroy(cache);
    
    TEST_END;
}

// Test 4: Bytecode to graph conversion
void test_bytecode_to_graph_conversion(void) {
    TEST("Bytecode to Graph Conversion");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Test conversion capability
    bool can_convert = graph_vm_can_convert_to_graph(chunk);
    ASSERT(can_convert == true);
    
    // Convert to graph
    Graph* graph = graph_vm_convert_bytecode_to_graph(vm, chunk);
    ASSERT(graph != NULL);
    ASSERT(graph->node_count > 0);
    
    graph_destroy(graph);
    chunk_free(chunk);
    free(chunk);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 5: Graph execution through VM
void test_graph_execution_through_vm(void) {
    TEST("Graph Execution Through VM");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Create a simple graph manually
    GraphBuilder* builder = graph_builder_create("vm_test");
    
    GraphValue val1 = {0}, val2 = {0};
    val1.type = val2.type = GRAPH_TYPE_F64;
    val1.data.f64 = 4.0;
    val2.data.f64 = 6.0;
    
    uint32_t const1 = graph_builder_add_constant(builder, val1, "four");
    uint32_t const2 = graph_builder_add_constant(builder, val2, "six");
    uint32_t add = graph_builder_add(builder, const1, const2, "add");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, add, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Execute through VM
    GraphVMResult result = graph_vm_execute_graph(vm, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.used_graph_execution == true);
    ASSERT(result.output_count == 1);
    ASSERT_NEAR(result.output_values[0].data.f64, 10.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 6: Hybrid execution mode
void test_hybrid_execution_mode(void) {
    TEST("Hybrid Execution Mode");
    
    GraphVMConfig config = graph_vm_default_config();
    config.mode = GRAPH_VM_MODE_HYBRID;
    
    GraphIntegratedVM* vm = graph_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Execute in hybrid mode
    GraphVMResult result = graph_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT_NEAR(result.output_values[0].data.f64, 5.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    chunk_free(chunk);
    free(chunk);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 7: Function compilation to graph
void test_function_compilation_to_graph(void) {
    TEST("Function Compilation to Graph");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    uint32_t function_id = 456;
    Chunk* chunk = create_simple_chunk();
    ASSERT(chunk != NULL);
    
    // Compile function to graph
    bool compiled = graph_vm_compile_function_to_graph(vm, function_id, chunk);
    ASSERT(compiled == true);
    
    // Check if graph is cached
    Graph* cached_graph = graph_vm_get_function_graph(vm, function_id);
    ASSERT(cached_graph != NULL);
    
    // Execute the cached graph
    GraphVMResult result = graph_vm_execute_function(vm, function_id);
    ASSERT(result.success == true);
    ASSERT(result.used_graph_execution == true);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    chunk_free(chunk);
    free(chunk);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 8: Graph optimization integration
void test_graph_optimization_integration(void) {
    TEST("Graph Optimization Integration");
    
    GraphVMConfig config = graph_vm_default_config();
    config.enable_graph_optimization = true;
    
    GraphIntegratedVM* vm = graph_vm_create_with_config(1024 * 1024, &config);
    ASSERT(vm != NULL);
    
    uint32_t function_id = 789;
    
    // Create a graph with optimization opportunities
    GraphBuilder* builder = graph_builder_create("opt_test");
    
    // Create: x + 0 (should be optimized to x)
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "x");
    GraphValue zero_val = {0};
    zero_val.type = GRAPH_TYPE_F64;
    zero_val.data.f64 = 0.0;
    uint32_t zero = graph_builder_add_constant(builder, zero_val, "zero");
    uint32_t add = graph_builder_add(builder, input, zero, "add_zero");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, add, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Register and optimize
    bool registered = graph_vm_register_graph(vm, function_id, graph);
    ASSERT(registered == true);
    
    bool optimized = graph_vm_optimize_graph(vm, function_id);
    ASSERT(optimized == true);
    
    graph_builder_destroy(builder);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 9: Statistics collection
void test_statistics_collection(void) {
    TEST("Statistics Collection");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Get initial stats
    GraphVMStats stats;
    graph_vm_get_stats(vm, &stats);
    ASSERT(stats.total_executions == 0);
    ASSERT(stats.graph_executions == 0);
    
    // Execute something
    Chunk* chunk = create_simple_chunk();
    GraphVMResult result = graph_vm_execute(vm, chunk);
    ASSERT(result.success == true);
    
    // Check updated stats
    graph_vm_get_stats(vm, &stats);
    ASSERT(stats.total_executions == 1);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    chunk_free(chunk);
    free(chunk);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 10: Error handling
void test_error_handling(void) {
    TEST("Error Handling");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Test execution with NULL chunk
    GraphVMResult result = graph_vm_execute(vm, NULL);
    ASSERT(result.success == false);
    ASSERT(result.error_message != NULL);
    
    const char* error_str = graph_vm_error_to_string(&result);
    ASSERT(error_str != NULL);
    
    free(result.error_message);
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 11: Memory management
void test_memory_management(void) {
    TEST("Memory Management");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Test memory allocation
    void* ptr = graph_vm_alloc_graph_memory(vm, 256);
    ASSERT(ptr != NULL);
    
    // Test memory limit checking
    bool within_limit = graph_vm_check_graph_memory_limit(vm, 512);
    ASSERT(within_limit == true);
    
    // Free memory
    graph_vm_free_graph_memory(vm, ptr);
    
    graph_vm_destroy(vm);
    
    TEST_END;
}

// Test 12: Builder integration
void test_builder_integration(void) {
    TEST("Builder Integration");
    
    GraphIntegratedVM* vm = graph_vm_create(1024 * 1024);
    ASSERT(vm != NULL);
    
    // Create builder through VM
    GraphBuilder* builder = graph_vm_create_builder(vm, "integration_test");
    ASSERT(builder != NULL);
    
    // Build a simple graph
    GraphValue val = {0};
    val.type = GRAPH_TYPE_F64;
    val.data.f64 = 123.0;
    uint32_t constant = graph_builder_add_constant(builder, val, "value");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, constant, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Register with VM
    uint32_t function_id = 999;
    bool registered = graph_vm_register_graph(vm, function_id, graph);
    ASSERT(registered == true);
    
    // Execute
    GraphVMResult result = graph_vm_execute_function(vm, function_id);
    ASSERT(result.success == true);
    ASSERT_NEAR(result.output_values[0].data.f64, 123.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_builder_destroy(builder);
    graph_vm_destroy(vm);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Integration Tests...\n\n");
    
    test_graph_vm_creation();
    test_configuration_management();
    test_graph_cache_functionality();
    test_bytecode_to_graph_conversion();
    test_graph_execution_through_vm();
    test_hybrid_execution_mode();
    test_function_compilation_to_graph();
    test_graph_optimization_integration();
    test_statistics_collection();
    test_error_handling();
    test_memory_management();
    test_builder_integration();
    
    printf("\n=== Graph Integration Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

