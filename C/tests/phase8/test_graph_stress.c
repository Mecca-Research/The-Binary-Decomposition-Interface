
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/graph/graph.h"
#include "../../vm/graph/graph_builder.h"
#include "../../vm/graph/graph_executor.h"
#include "../../vm/graph/graph_optimizer.h"

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

// Test 1: Large graph creation and management
void test_large_graph_creation(void) {
    TEST("Large Graph Creation and Management");
    
    const uint32_t NODE_COUNT = 1000;
    
    GraphBuilder* builder = graph_builder_create("large_graph");
    ASSERT(builder != NULL);
    
    // Create a chain of nodes
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t current = input;
    
    for (uint32_t i = 0; i < NODE_COUNT; i++) {
        GraphValue val = {0};
        val.type = GRAPH_TYPE_F64;
        val.data.f64 = 1.0;
        
        uint32_t constant = graph_builder_add_constant(builder, val, "const");
        uint32_t add = graph_builder_add(builder, current, constant, "add");
        current = add;
    }
    
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, current, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    ASSERT(graph->node_count > NODE_COUNT);
    
    // Test graph validation
    char* error = NULL;
    bool valid = graph_validate(graph, &error);
    ASSERT(valid == true);
    ASSERT(error == NULL);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 2: Deep recursion handling
void test_deep_recursion_handling(void) {
    TEST("Deep Recursion Handling");
    
    const uint32_t DEPTH = 500;
    
    Graph* graph = graph_create("deep_graph");
    ASSERT(graph != NULL);
    
    // Create a deep chain: A -> B -> C -> ... -> Z
    GraphNode* prev = NULL;
    for (uint32_t i = 0; i < DEPTH; i++) {
        char name[32];
        snprintf(name, sizeof(name), "node_%u", i);
        
        GraphNodeType type = (i == 0) ? GRAPH_NODE_INPUT : 
                           (i == DEPTH - 1) ? GRAPH_NODE_OUTPUT : GRAPH_NODE_ADD;
        
        GraphNode* node = graph_add_node(graph, type, name);
        ASSERT(node != NULL);
        
        if (prev) {
            graph_add_edge(graph, prev->id, 0, node->id, 0);
        }
        prev = node;
    }
    
    // Test topological sorting with deep graph
    uint32_t count;
    uint32_t* order = graph_topological_sort(graph, &count);
    ASSERT(order != NULL);
    ASSERT(count == DEPTH);
    
    free(order);
    graph_destroy(graph);
    
    TEST_END;
}

// Test 3: Memory stress test
void test_memory_stress(void) {
    TEST("Memory Stress Test");
    
    const uint32_t GRAPH_COUNT = 100;
    const uint32_t NODES_PER_GRAPH = 50;
    
    Graph** graphs = (Graph**)malloc(GRAPH_COUNT * sizeof(Graph*));
    ASSERT(graphs != NULL);
    
    // Create many graphs
    for (uint32_t g = 0; g < GRAPH_COUNT; g++) {
        char graph_name[32];
        snprintf(graph_name, sizeof(graph_name), "stress_graph_%u", g);
        
        graphs[g] = graph_create(graph_name);
        ASSERT(graphs[g] != NULL);
        
        // Add nodes to each graph
        for (uint32_t n = 0; n < NODES_PER_GRAPH; n++) {
            char node_name[32];
            snprintf(node_name, sizeof(node_name), "node_%u", n);
            
            GraphNode* node = graph_add_node(graphs[g], GRAPH_NODE_ADD, node_name);
            ASSERT(node != NULL);
        }
        
        ASSERT(graphs[g]->node_count == NODES_PER_GRAPH);
    }
    
    // Destroy all graphs
    for (uint32_t g = 0; g < GRAPH_COUNT; g++) {
        graph_destroy(graphs[g]);
    }
    
    free(graphs);
    
    TEST_END;
}

// Test 4: Complex graph execution stress
void test_complex_execution_stress(void) {
    TEST("Complex Graph Execution Stress");
    
    const uint32_t EXECUTION_COUNT = 100;
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create a moderately complex graph
    GraphBuilder* builder = graph_builder_create("complex_stress");
    
    // Create multiple inputs
    uint32_t inputs[5];
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "input_%d", i);
        inputs[i] = graph_builder_add_input(builder, GRAPH_TYPE_F64, name);
    }
    
    // Create a network of operations
    uint32_t add1 = graph_builder_add(builder, inputs[0], inputs[1], "add1");
    uint32_t add2 = graph_builder_add(builder, inputs[2], inputs[3], "add2");
    uint32_t mul1 = graph_builder_mul(builder, add1, add2, "mul1");
    uint32_t sub1 = graph_builder_sub(builder, mul1, inputs[4], "sub1");
    
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, sub1, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Execute multiple times with different inputs
    for (uint32_t i = 0; i < EXECUTION_COUNT; i++) {
        GraphValue input_values[5];
        for (int j = 0; j < 5; j++) {
            input_values[j].type = GRAPH_TYPE_F64;
            input_values[j].data.f64 = (double)(i + j + 1);
        }
        
        GraphExecutionResult result = graph_executor_execute(executor, graph, input_values, 5);
        ASSERT(result.success == true);
        ASSERT(result.output_count == 1);
        
        // Expected: ((input0 + input1) * (input2 + input3)) - input4
        double expected = ((input_values[0].data.f64 + input_values[1].data.f64) * 
                          (input_values[2].data.f64 + input_values[3].data.f64)) - 
                          input_values[4].data.f64;
        
        ASSERT(fabs(result.output_values[0].data.f64 - expected) < 1e-9);
        
        free(result.output_values);
        free(result.error_message);
    }
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 5: Optimization stress test
void test_optimization_stress(void) {
    TEST("Optimization Stress Test");
    
    const uint32_t OPTIMIZATION_ROUNDS = 50;
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Create a graph with many optimization opportunities
    GraphBuilder* builder = graph_builder_create("opt_stress");
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t current = input;
    
    // Create many redundant operations
    for (uint32_t i = 0; i < 20; i++) {
        // Add zero (should be optimized away)
        GraphValue zero = {0};
        zero.type = GRAPH_TYPE_F64;
        zero.data.f64 = 0.0;
        uint32_t zero_const = graph_builder_add_constant(builder, zero, "zero");
        uint32_t add_zero = graph_builder_add(builder, current, zero_const, "add_zero");
        
        // Multiply by one (should be optimized away)
        GraphValue one = {0};
        one.type = GRAPH_TYPE_F64;
        one.data.f64 = 1.0;
        uint32_t one_const = graph_builder_add_constant(builder, one, "one");
        uint32_t mul_one = graph_builder_mul(builder, add_zero, one_const, "mul_one");
        
        current = mul_one;
    }
    
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, current, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    uint32_t original_node_count = graph->node_count;
    
    // Run optimization multiple times
    for (uint32_t i = 0; i < OPTIMIZATION_ROUNDS; i++) {
        GraphOptimizationStats stats;
        bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
        ASSERT(optimized == true);
        
        // Graph should get smaller or stay the same
        ASSERT(graph->node_count <= original_node_count);
    }
    
    // Final node count should be significantly smaller
    ASSERT(graph->node_count < original_node_count);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 6: Concurrent access simulation
void test_concurrent_access_simulation(void) {
    TEST("Concurrent Access Simulation");
    
    const uint32_t THREAD_COUNT = 4;
    const uint32_t OPERATIONS_PER_THREAD = 25;
    
    // Simulate concurrent access by rapidly creating/destroying components
    for (uint32_t thread = 0; thread < THREAD_COUNT; thread++) {
        for (uint32_t op = 0; op < OPERATIONS_PER_THREAD; op++) {
            // Create and destroy graph
            Graph* graph = graph_create("concurrent_test");
            ASSERT(graph != NULL);
            
            GraphNode* node = graph_add_node(graph, GRAPH_NODE_ADD, "test_node");
            ASSERT(node != NULL);
            
            graph_destroy(graph);
            
            // Create and destroy executor
            GraphExecutor* executor = graph_executor_create();
            ASSERT(executor != NULL);
            graph_executor_destroy(executor);
            
            // Create and destroy optimizer
            GraphOptimizer* optimizer = graph_optimizer_create();
            ASSERT(optimizer != NULL);
            graph_optimizer_destroy(optimizer);
        }
    }
    
    TEST_END;
}

// Test 7: Edge case handling
void test_edge_case_handling(void) {
    TEST("Edge Case Handling");
    
    // Test empty graph
    Graph* empty_graph = graph_create("empty");
    ASSERT(empty_graph != NULL);
    
    char* error = NULL;
    bool valid = graph_validate(empty_graph, &error);
    ASSERT(valid == true); // Empty graph should be valid
    
    graph_destroy(empty_graph);
    
    // Test single node graph
    Graph* single_graph = graph_create("single");
    GraphNode* single_node = graph_add_node(single_graph, GRAPH_NODE_CONSTANT, "single");
    ASSERT(single_node != NULL);
    
    valid = graph_validate(single_graph, &error);
    ASSERT(valid == true);
    
    graph_destroy(single_graph);
    
    // Test graph with only input/output
    Graph* io_graph = graph_create("io_only");
    GraphNode* input = graph_add_input_node(io_graph, GRAPH_TYPE_F64, "input");
    GraphNode* output = graph_add_output_node(io_graph, GRAPH_TYPE_F64, "output");
    graph_add_edge(io_graph, input->id, 0, output->id, 0);
    
    valid = graph_validate(io_graph, &error);
    ASSERT(valid == true);
    
    graph_destroy(io_graph);
    
    TEST_END;
}

// Test 8: Performance regression test
void test_performance_regression(void) {
    TEST("Performance Regression Test");
    
    const uint32_t LARGE_NODE_COUNT = 500;
    const uint32_t BENCHMARK_ITERATIONS = 10;
    
    clock_t start_time = clock();
    
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        GraphBuilder* builder = graph_builder_create("perf_test");
        
        // Create a moderately complex graph
        uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
        uint32_t current = input;
        
        for (uint32_t i = 0; i < LARGE_NODE_COUNT; i++) {
            GraphValue val = {0};
            val.type = GRAPH_TYPE_F64;
            val.data.f64 = (double)(i + 1);
            
            uint32_t constant = graph_builder_add_constant(builder, val, "const");
            uint32_t add = graph_builder_add(builder, current, constant, "add");
            current = add;
        }
        
        uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
        graph_builder_connect(builder, current, 0, output, 0);
        
        Graph* graph = graph_builder_build(builder);
        ASSERT(graph != NULL);
        
        // Execute the graph
        GraphExecutor* executor = graph_executor_create();
        GraphValue input_val = {0};
        input_val.type = GRAPH_TYPE_F64;
        input_val.data.f64 = 1.0;
        
        GraphExecutionResult result = graph_executor_execute(executor, graph, &input_val, 1);
        ASSERT(result.success == true);
        
        free(result.output_values);
        free(result.error_message);
        graph_executor_destroy(executor);
        graph_destroy(graph);
        graph_builder_destroy(builder);
    }
    
    clock_t end_time = clock();
    double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Should complete within reasonable time (adjust threshold as needed)
    ASSERT(elapsed < 10.0); // 10 seconds threshold
    
    printf(" (%.2fs)", elapsed);
    
    TEST_END;
}

// Test 9: Memory leak detection
void test_memory_leak_detection(void) {
    TEST("Memory Leak Detection");
    
    const uint32_t ALLOCATION_CYCLES = 100;
    
    // Repeatedly create and destroy components to check for leaks
    for (uint32_t cycle = 0; cycle < ALLOCATION_CYCLES; cycle++) {
        // Graph creation/destruction cycle
        Graph* graph = graph_create("leak_test");
        
        for (int i = 0; i < 10; i++) {
            GraphNode* node = graph_add_node(graph, GRAPH_NODE_ADD, "node");
            ASSERT(node != NULL);
        }
        
        graph_destroy(graph);
        
        // Builder creation/destruction cycle
        GraphBuilder* builder = graph_builder_create("leak_test");
        
        uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
        uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
        graph_builder_connect(builder, input, 0, output, 0);
        
        Graph* built_graph = graph_builder_build(builder);
        ASSERT(built_graph != NULL);
        
        graph_destroy(built_graph);
        graph_builder_destroy(builder);
        
        // Executor creation/destruction cycle
        GraphExecutor* executor = graph_executor_create();
        ASSERT(executor != NULL);
        graph_executor_destroy(executor);
        
        // Optimizer creation/destruction cycle
        GraphOptimizer* optimizer = graph_optimizer_create();
        ASSERT(optimizer != NULL);
        graph_optimizer_destroy(optimizer);
    }
    
    TEST_END;
}

// Test 10: Boundary condition testing
void test_boundary_conditions(void) {
    TEST("Boundary Condition Testing");
    
    // Test maximum values
    GraphValue max_val = {0};
    max_val.type = GRAPH_TYPE_F64;
    max_val.data.f64 = 1e308; // Near double max
    
    Graph* graph = graph_create("boundary_test");
    GraphNode* max_node = graph_add_constant_node(graph, max_val, "max");
    ASSERT(max_node != NULL);
    ASSERT(max_node->constant_value.data.f64 == 1e308);
    
    // Test minimum values
    GraphValue min_val = {0};
    min_val.type = GRAPH_TYPE_F64;
    min_val.data.f64 = -1e308; // Near double min
    
    GraphNode* min_node = graph_add_constant_node(graph, min_val, "min");
    ASSERT(min_node != NULL);
    ASSERT(min_node->constant_value.data.f64 == -1e308);
    
    // Test zero
    GraphValue zero_val = {0};
    zero_val.type = GRAPH_TYPE_F64;
    zero_val.data.f64 = 0.0;
    
    GraphNode* zero_node = graph_add_constant_node(graph, zero_val, "zero");
    ASSERT(zero_node != NULL);
    ASSERT(zero_node->constant_value.data.f64 == 0.0);
    
    graph_destroy(graph);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Stress Tests...\n\n");
    
    test_large_graph_creation();
    test_deep_recursion_handling();
    test_memory_stress();
    test_complex_execution_stress();
    test_optimization_stress();
    test_concurrent_access_simulation();
    test_edge_case_handling();
    test_performance_regression();
    test_memory_leak_detection();
    test_boundary_conditions();
    
    printf("\n=== Graph Stress Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

