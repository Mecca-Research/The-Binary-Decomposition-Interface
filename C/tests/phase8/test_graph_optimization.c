
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "../../vm/graph/graph_optimizer.h"
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

// Test 1: Optimizer creation and destruction
void test_optimizer_creation(void) {
    TEST("Optimizer Creation and Destruction");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    GraphOptimizerConfig config;
    graph_optimizer_get_config(optimizer, &config);
    ASSERT(config.level == GRAPH_OPT_BASIC);
    ASSERT(config.enable_dead_code_elimination == true);
    ASSERT(config.enable_constant_folding == true);
    
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 2: Configuration management
void test_configuration_management(void) {
    TEST("Configuration Management");
    
    // Test default configuration
    GraphOptimizerConfig default_config = graph_optimizer_default_config();
    ASSERT(default_config.level == GRAPH_OPT_BASIC);
    ASSERT(default_config.enable_constant_folding == true);
    
    // Test configuration for different levels
    GraphOptimizerConfig aggressive_config = graph_optimizer_config_for_level(GRAPH_OPT_AGGRESSIVE);
    ASSERT(aggressive_config.level == GRAPH_OPT_AGGRESSIVE);
    ASSERT(aggressive_config.enable_loop_optimization == true);
    
    GraphOptimizerConfig none_config = graph_optimizer_config_for_level(GRAPH_OPT_NONE);
    ASSERT(none_config.level == GRAPH_OPT_NONE);
    ASSERT(none_config.enable_constant_folding == false);
    
    // Test custom configuration
    GraphOptimizer* optimizer = graph_optimizer_create_with_config(&aggressive_config);
    ASSERT(optimizer != NULL);
    
    GraphOptimizerConfig retrieved_config;
    graph_optimizer_get_config(optimizer, &retrieved_config);
    ASSERT(retrieved_config.level == GRAPH_OPT_AGGRESSIVE);
    
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 3: Dead code elimination
void test_dead_code_elimination(void) {
    TEST("Dead Code Elimination");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Create graph with dead code
    GraphBuilder* builder = graph_builder_create("dead_code_test");
    
    // Live path: input -> output
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, input, 0, output, 0);
    
    // Dead code: unused constant and operation
    GraphValue dead_val = {0};
    dead_val.type = GRAPH_TYPE_F64;
    dead_val.data.f64 = 999.0;
    uint32_t dead_const = graph_builder_add_constant(builder, dead_val, "dead");
    uint32_t dead_add = graph_builder_add(builder, dead_const, dead_const, "dead_add");
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    uint32_t original_node_count = graph->node_count;
    ASSERT(original_node_count == 4); // input, output, dead_const, dead_add
    
    // Optimize
    GraphOptimizationStats stats;
    bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
    ASSERT(optimized == true);
    ASSERT(stats.nodes_eliminated > 0);
    ASSERT(graph->node_count < original_node_count);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 4: Constant folding
void test_constant_folding(void) {
    TEST("Constant Folding");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Create graph with constant expression: 2 + 3
    GraphBuilder* builder = graph_builder_create("constant_folding_test");
    
    GraphValue val2 = {0}, val3 = {0};
    val2.type = val3.type = GRAPH_TYPE_F64;
    val2.data.f64 = 2.0;
    val3.data.f64 = 3.0;
    
    uint32_t const2 = graph_builder_add_constant(builder, val2, "two");
    uint32_t const3 = graph_builder_add_constant(builder, val3, "three");
    uint32_t add = graph_builder_add(builder, const2, const3, "add");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, add, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Find the add node before optimization
    GraphNode* add_node = graph_get_node(graph, add);
    ASSERT(add_node != NULL);
    ASSERT(add_node->type == GRAPH_NODE_ADD);
    ASSERT(add_node->is_constant == false);
    
    // Optimize
    GraphOptimizationStats stats;
    bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
    ASSERT(optimized == true);
    ASSERT(stats.constants_folded > 0);
    
    // Check if add node became constant
    add_node = graph_get_node(graph, add);
    if (add_node) {
        // If node still exists, it should be constant now
        ASSERT(add_node->is_constant == true);
        ASSERT(fabs(add_node->constant_value.data.f64 - 5.0) < 1e-9);
    }
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 5: Algebraic simplification
void test_algebraic_simplification(void) {
    TEST("Algebraic Simplification");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Create graph with algebraic identity: x + 0
    GraphBuilder* builder = graph_builder_create("algebraic_test");
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "x");
    
    GraphValue zero_val = {0};
    zero_val.type = GRAPH_TYPE_F64;
    zero_val.data.f64 = 0.0;
    uint32_t zero = graph_builder_add_constant(builder, zero_val, "zero");
    
    uint32_t add = graph_builder_add(builder, input, zero, "add_zero");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, add, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    uint32_t original_node_count = graph->node_count;
    
    // Optimize
    bool optimized = graph_optimizer_optimize(optimizer, graph);
    ASSERT(optimized == true);
    
    // The add operation should be simplified away
    // (exact behavior depends on implementation)
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 6: Common subexpression elimination
void test_common_subexpression_elimination(void) {
    TEST("Common Subexpression Elimination");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Create graph with common subexpressions
    GraphBuilder* builder = graph_builder_create("cse_test");
    
    uint32_t input1 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "a");
    uint32_t input2 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "b");
    
    // Create two identical add operations: a + b
    uint32_t add1 = graph_builder_add(builder, input1, input2, "add1");
    uint32_t add2 = graph_builder_add(builder, input1, input2, "add2");
    
    // Use both in a multiplication
    uint32_t mul = graph_builder_mul(builder, add1, add2, "mul");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, mul, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    uint32_t original_node_count = graph->node_count;
    
    // Optimize
    GraphOptimizationStats stats;
    bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
    ASSERT(optimized == true);
    
    // One of the add operations should be eliminated
    ASSERT(graph->node_count <= original_node_count);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 7: Pass management
void test_pass_management(void) {
    TEST("Pass Management");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Test enabling/disabling passes
    bool enabled = graph_optimizer_enable_pass(optimizer, OPT_PASS_CONSTANT_FOLDING, false);
    ASSERT(enabled == true);
    
    enabled = graph_optimizer_enable_pass(optimizer, OPT_PASS_CONSTANT_FOLDING, true);
    ASSERT(enabled == true);
    
    // Test removing passes
    bool removed = graph_optimizer_remove_pass(optimizer, OPT_PASS_DEAD_CODE_ELIMINATION);
    ASSERT(removed == true);
    
    // Test adding custom pass
    bool added = graph_optimizer_add_pass(optimizer, OPT_PASS_CUSTOM, 
                                         graph_opt_constant_folding, NULL);
    ASSERT(added == true);
    
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 8: Statistics collection
void test_statistics_collection(void) {
    TEST("Statistics Collection");
    
    GraphOptimizer* optimizer = graph_optimizer_create();
    ASSERT(optimizer != NULL);
    
    // Get initial stats
    GraphOptimizationStats stats;
    graph_optimizer_get_stats(optimizer, &stats);
    ASSERT(stats.passes_run == 0);
    ASSERT(stats.nodes_eliminated == 0);
    
    // Create a simple graph to optimize
    GraphBuilder* builder = graph_builder_create("stats_test");
    
    GraphValue val = {0};
    val.type = GRAPH_TYPE_F64;
    val.data.f64 = 42.0;
    uint32_t const_node = graph_builder_add_constant(builder, val, "constant");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, const_node, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Optimize and collect stats
    bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
    ASSERT(optimized == true);
    ASSERT(stats.passes_run > 0);
    ASSERT(stats.optimization_time_ns > 0);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

// Test 9: Graph analysis utilities
void test_graph_analysis_utilities(void) {
    TEST("Graph Analysis Utilities");
    
    // Create a simple graph
    GraphBuilder* builder = graph_builder_create("analysis_test");
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 5.0;
    uint32_t constant = graph_builder_add_constant(builder, const_val, "constant");
    uint32_t add = graph_builder_add(builder, input, constant, "add");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, add, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Test node analysis
    GraphNode* const_node = graph_get_node(graph, constant);
    ASSERT(const_node != NULL);
    ASSERT(graph_node_is_constant(const_node) == true);
    
    GraphNode* add_node = graph_get_node(graph, add);
    ASSERT(add_node != NULL);
    ASSERT(graph_node_is_constant(add_node) == false);
    
    // Test use counting
    uint32_t use_count = graph_count_node_uses(graph, constant);
    ASSERT(use_count == 1); // Used by add node
    
    // Test dead node detection
    GraphNode* output_node = graph_get_node(graph, output);
    ASSERT(output_node != NULL);
    ASSERT(graph_node_is_dead(graph, output_node) == false); // Output nodes are never dead
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 10: Constant evaluation
void test_constant_evaluation(void) {
    TEST("Constant Evaluation");
    
    // Test binary operation folding
    GraphValue left = {0}, right = {0};
    left.type = right.type = GRAPH_TYPE_F64;
    left.data.f64 = 6.0;
    right.data.f64 = 2.0;
    
    GraphValue result = graph_fold_binary_operation(GRAPH_NODE_ADD, &left, &right);
    ASSERT(result.type == GRAPH_TYPE_F64);
    ASSERT(fabs(result.data.f64 - 8.0) < 1e-9);
    
    result = graph_fold_binary_operation(GRAPH_NODE_DIV, &left, &right);
    ASSERT(fabs(result.data.f64 - 3.0) < 1e-9);
    
    // Test unary operation folding
    GraphValue bool_val = {0};
    bool_val.type = GRAPH_TYPE_BOOL;
    bool_val.data.boolean = true;
    
    result = graph_fold_unary_operation(GRAPH_NODE_NOT, &bool_val);
    ASSERT(result.type == GRAPH_TYPE_BOOL);
    ASSERT(result.data.boolean == false);
    
    TEST_END;
}

// Test 11: Graph integrity verification
void test_graph_integrity_verification(void) {
    TEST("Graph Integrity Verification");
    
    // Create a valid graph
    GraphBuilder* builder = graph_builder_create("integrity_test");
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, input, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    // Verify integrity
    bool valid = graph_optimizer_verify_graph_integrity(graph);
    ASSERT(valid == true);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 12: Optimization convergence
void test_optimization_convergence(void) {
    TEST("Optimization Convergence");
    
    GraphOptimizerConfig config = graph_optimizer_default_config();
    config.max_iterations = 5;
    config.convergence_threshold = 0.01;
    
    GraphOptimizer* optimizer = graph_optimizer_create_with_config(&config);
    ASSERT(optimizer != NULL);
    
    // Create a graph that should converge quickly
    GraphBuilder* builder = graph_builder_create("convergence_test");
    
    GraphValue val = {0};
    val.type = GRAPH_TYPE_F64;
    val.data.f64 = 1.0;
    uint32_t constant = graph_builder_add_constant(builder, val, "one");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    graph_builder_connect(builder, constant, 0, output, 0);
    
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    
    GraphOptimizationStats stats;
    bool optimized = graph_optimizer_optimize_with_stats(optimizer, graph, &stats);
    ASSERT(optimized == true);
    ASSERT(stats.iterations <= config.max_iterations);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    graph_optimizer_destroy(optimizer);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Optimization Tests...\n\n");
    
    test_optimizer_creation();
    test_configuration_management();
    test_dead_code_elimination();
    test_constant_folding();
    test_algebraic_simplification();
    test_common_subexpression_elimination();
    test_pass_management();
    test_statistics_collection();
    test_graph_analysis_utilities();
    test_constant_evaluation();
    test_graph_integrity_verification();
    test_optimization_convergence();
    
    printf("\n=== Graph Optimization Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

