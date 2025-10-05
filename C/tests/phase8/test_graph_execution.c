
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/graph/graph_executor.h"
#include "../../vm/graph/graph_builder.h"

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

// Test 1: Executor creation and destruction
void test_executor_creation(void) {
    TEST("Executor Creation and Destruction");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    GraphExecutorConfig config;
    graph_executor_get_config(executor, &config);
    ASSERT(config.mode == GRAPH_EXEC_SEQUENTIAL);
    
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 2: Basic graph execution
void test_basic_execution(void) {
    TEST("Basic Graph Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create a simple graph: constant -> output
    GraphBuilder* builder = graph_builder_create("simple_test");
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 42.0;
    
    uint32_t const_node = graph_builder_add_constant(builder, const_val, "constant");
    uint32_t output_node = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, const_node, 0, output_node, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Execute the graph
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT(result.output_values != NULL);
    ASSERT_NEAR(result.output_values[0].data.f64, 42.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 3: Arithmetic execution
void test_arithmetic_execution(void) {
    TEST("Arithmetic Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create graph: 5 + 3 = 8
    GraphBuilder* builder = graph_builder_create("arithmetic_test");
    
    GraphValue val1 = {0}, val2 = {0};
    val1.type = val2.type = GRAPH_TYPE_F64;
    val1.data.f64 = 5.0;
    val2.data.f64 = 3.0;
    
    uint32_t const1 = graph_builder_add_constant(builder, val1, "five");
    uint32_t const2 = graph_builder_add_constant(builder, val2, "three");
    uint32_t add_node = graph_builder_add(builder, const1, const2, "add");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    
    graph_builder_connect(builder, add_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT_NEAR(result.output_values[0].data.f64, 8.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 4: Complex expression execution
void test_complex_expression(void) {
    TEST("Complex Expression Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create graph: (2 + 3) * 4 = 20
    GraphBuilder* builder = graph_builder_create("complex_test");
    
    GraphValue val2 = {0}, val3 = {0}, val4 = {0};
    val2.type = val3.type = val4.type = GRAPH_TYPE_F64;
    val2.data.f64 = 2.0;
    val3.data.f64 = 3.0;
    val4.data.f64 = 4.0;
    
    uint32_t const2 = graph_builder_add_constant(builder, val2, "two");
    uint32_t const3 = graph_builder_add_constant(builder, val3, "three");
    uint32_t const4 = graph_builder_add_constant(builder, val4, "four");
    
    uint32_t add_node = graph_builder_add(builder, const2, const3, "add");
    uint32_t mul_node = graph_builder_mul(builder, add_node, const4, "mul");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    
    graph_builder_connect(builder, mul_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT_NEAR(result.output_values[0].data.f64, 20.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 5: Input/output execution
void test_input_output_execution(void) {
    TEST("Input/Output Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create graph: input * 2
    GraphBuilder* builder = graph_builder_create("io_test");
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    
    GraphValue val2 = {0};
    val2.type = GRAPH_TYPE_F64;
    val2.data.f64 = 2.0;
    uint32_t const2 = graph_builder_add_constant(builder, val2, "two");
    
    uint32_t mul_node = graph_builder_mul(builder, input, const2, "mul");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    
    graph_builder_connect(builder, mul_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Prepare input values
    GraphValue input_val = {0};
    input_val.type = GRAPH_TYPE_F64;
    input_val.data.f64 = 7.0;
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, &input_val, 1);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT_NEAR(result.output_values[0].data.f64, 14.0, 1e-9);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 6: Logical operations execution
void test_logical_execution(void) {
    TEST("Logical Operations Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create graph: true AND false = false
    GraphBuilder* builder = graph_builder_create("logical_test");
    
    GraphValue val_true = {0}, val_false = {0};
    val_true.type = val_false.type = GRAPH_TYPE_BOOL;
    val_true.data.boolean = true;
    val_false.data.boolean = false;
    
    uint32_t const_true = graph_builder_add_constant(builder, val_true, "true");
    uint32_t const_false = graph_builder_add_constant(builder, val_false, "false");
    uint32_t and_node = graph_builder_and(builder, const_true, const_false, "and");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_BOOL, "result");
    
    graph_builder_connect(builder, and_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT(result.output_values[0].data.boolean == false);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 7: Comparison operations execution
void test_comparison_execution(void) {
    TEST("Comparison Operations Execution");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Create graph: 5 > 3 = true
    GraphBuilder* builder = graph_builder_create("comparison_test");
    
    GraphValue val5 = {0}, val3 = {0};
    val5.type = val3.type = GRAPH_TYPE_F64;
    val5.data.f64 = 5.0;
    val3.data.f64 = 3.0;
    
    uint32_t const5 = graph_builder_add_constant(builder, val5, "five");
    uint32_t const3 = graph_builder_add_constant(builder, val3, "three");
    uint32_t gt_node = graph_builder_gt(builder, const5, const3, "gt");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_BOOL, "result");
    
    graph_builder_connect(builder, gt_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    ASSERT(result.output_count == 1);
    ASSERT(result.output_values[0].data.boolean == true);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 8: Scheduler functionality
void test_scheduler_functionality(void) {
    TEST("Scheduler Functionality");
    
    // Create a simple DAG
    Graph* graph = graph_create("scheduler_test");
    ASSERT(graph != NULL);
    
    GraphNode* nodeA = graph_add_node(graph, GRAPH_NODE_INPUT, "A");
    GraphNode* nodeB = graph_add_node(graph, GRAPH_NODE_ADD, "B");
    GraphNode* nodeC = graph_add_node(graph, GRAPH_NODE_OUTPUT, "C");
    
    graph_add_edge(graph, nodeA->id, 0, nodeB->id, 0);
    graph_add_edge(graph, nodeB->id, 0, nodeC->id, 0);
    
    GraphScheduler* scheduler = graph_scheduler_create(graph);
    ASSERT(scheduler != NULL);
    
    bool scheduled = graph_scheduler_schedule(scheduler);
    ASSERT(scheduled == true);
    
    // Test scheduler state
    ASSERT(graph_scheduler_is_complete(scheduler) == false);
    
    // Simulate execution
    uint32_t next_node = graph_scheduler_get_next_ready(scheduler);
    ASSERT(next_node == nodeA->id); // Input node should be ready first
    
    graph_scheduler_mark_completed(scheduler, next_node);
    
    next_node = graph_scheduler_get_next_ready(scheduler);
    ASSERT(next_node == nodeB->id); // Add node should be ready after input
    
    graph_scheduler_destroy(scheduler);
    graph_destroy(graph);
    
    TEST_END;
}

// Test 9: Execution context management
void test_execution_context(void) {
    TEST("Execution Context Management");
    
    Graph* graph = graph_create("context_test");
    ASSERT(graph != NULL);
    
    // Add some nodes
    graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    
    GraphExecutionContext* context = graph_execution_context_create(graph);
    ASSERT(context != NULL);
    ASSERT(context->input_count == 1);
    ASSERT(context->output_count == 1);
    ASSERT(context->input_values != NULL);
    ASSERT(context->output_values != NULL);
    ASSERT(context->memory_pool != NULL);
    
    graph_execution_context_destroy(context);
    graph_destroy(graph);
    
    TEST_END;
}

// Test 10: Error handling
void test_error_handling(void) {
    TEST("Error Handling");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Test execution with NULL graph
    GraphExecutionResult result = graph_executor_execute(executor, NULL, NULL, 0);
    ASSERT(result.success == false);
    ASSERT(result.error_message != NULL);
    
    free(result.error_message);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 11: Statistics collection
void test_statistics_collection(void) {
    TEST("Statistics Collection");
    
    GraphExecutor* executor = graph_executor_create();
    ASSERT(executor != NULL);
    
    // Get initial stats
    GraphExecutorStats stats;
    graph_executor_get_stats(executor, &stats);
    ASSERT(stats.total_executions == 0);
    
    // Create and execute a simple graph
    GraphBuilder* builder = graph_builder_create("stats_test");
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 1.0;
    
    uint32_t const_node = graph_builder_add_constant(builder, const_val, "one");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    graph_builder_connect(builder, const_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    GraphExecutionResult result = graph_executor_execute(executor, graph, NULL, 0);
    ASSERT(result.success == true);
    
    // Check updated stats
    graph_executor_get_stats(executor, &stats);
    ASSERT(stats.total_executions == 1);
    ASSERT(stats.successful_executions == 1);
    ASSERT(stats.nodes_executed > 0);
    
    // Cleanup
    free(result.output_values);
    free(result.error_message);
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_executor_destroy(executor);
    
    TEST_END;
}

// Test 12: Configuration management
void test_executor_configuration(void) {
    TEST("Executor Configuration");
    
    GraphExecutorConfig config = graph_executor_default_config();
    ASSERT(config.mode == GRAPH_EXEC_SEQUENTIAL);
    ASSERT(config.max_threads == 1);
    
    // Test custom configuration
    config.enable_profiling = true;
    config.memory_limit = 2048;
    
    GraphExecutor* executor = graph_executor_create_with_config(&config);
    ASSERT(executor != NULL);
    
    GraphExecutorConfig retrieved_config;
    graph_executor_get_config(executor, &retrieved_config);
    ASSERT(retrieved_config.enable_profiling == true);
    ASSERT(retrieved_config.memory_limit == 2048);
    
    graph_executor_destroy(executor);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Execution Tests...\n\n");
    
    test_executor_creation();
    test_basic_execution();
    test_arithmetic_execution();
    test_complex_expression();
    test_input_output_execution();
    test_logical_execution();
    test_comparison_execution();
    test_scheduler_functionality();
    test_execution_context();
    test_error_handling();
    test_statistics_collection();
    test_executor_configuration();
    
    printf("\n=== Graph Execution Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

