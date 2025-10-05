
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
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

// Test 1: Builder creation and destruction
void test_builder_creation(void) {
    TEST("Builder Creation and Destruction");
    
    GraphBuilder* builder = graph_builder_create("test_graph");
    ASSERT(builder != NULL);
    
    uint32_t node_count = graph_builder_get_node_count(builder);
    ASSERT(node_count == 0);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 2: Basic node creation
void test_basic_node_creation(void) {
    TEST("Basic Node Creation");
    
    GraphBuilder* builder = graph_builder_create("test_graph");
    ASSERT(builder != NULL);
    
    // Test constant node
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 3.14;
    
    uint32_t const_id = graph_builder_add_constant(builder, const_val, "pi");
    ASSERT(const_id != 0);
    
    // Test input node
    uint32_t input_id = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    ASSERT(input_id != 0);
    ASSERT(input_id != const_id);
    
    // Test output node
    uint32_t output_id = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    ASSERT(output_id != 0);
    ASSERT(output_id != input_id);
    
    ASSERT(graph_builder_get_node_count(builder) == 3);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 3: Arithmetic operations
void test_arithmetic_operations(void) {
    TEST("Arithmetic Operations");
    
    GraphBuilder* builder = graph_builder_create("arithmetic_test");
    ASSERT(builder != NULL);
    
    // Create input nodes
    uint32_t input1 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "a");
    uint32_t input2 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "b");
    ASSERT(input1 != 0 && input2 != 0);
    
    // Test arithmetic operations
    uint32_t add_id = graph_builder_add(builder, input1, input2, "add");
    ASSERT(add_id != 0);
    
    uint32_t sub_id = graph_builder_sub(builder, input1, input2, "sub");
    ASSERT(sub_id != 0);
    
    uint32_t mul_id = graph_builder_mul(builder, input1, input2, "mul");
    ASSERT(mul_id != 0);
    
    uint32_t div_id = graph_builder_div(builder, input1, input2, "div");
    ASSERT(div_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 6);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 4: Logical operations
void test_logical_operations(void) {
    TEST("Logical Operations");
    
    GraphBuilder* builder = graph_builder_create("logical_test");
    ASSERT(builder != NULL);
    
    // Create boolean inputs
    uint32_t input1 = graph_builder_add_input(builder, GRAPH_TYPE_BOOL, "a");
    uint32_t input2 = graph_builder_add_input(builder, GRAPH_TYPE_BOOL, "b");
    ASSERT(input1 != 0 && input2 != 0);
    
    // Test logical operations
    uint32_t and_id = graph_builder_and(builder, input1, input2, "and");
    ASSERT(and_id != 0);
    
    uint32_t or_id = graph_builder_or(builder, input1, input2, "or");
    ASSERT(or_id != 0);
    
    uint32_t xor_id = graph_builder_xor(builder, input1, input2, "xor");
    ASSERT(xor_id != 0);
    
    uint32_t not_id = graph_builder_not(builder, input1, "not");
    ASSERT(not_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 6);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 5: Comparison operations
void test_comparison_operations(void) {
    TEST("Comparison Operations");
    
    GraphBuilder* builder = graph_builder_create("comparison_test");
    ASSERT(builder != NULL);
    
    uint32_t input1 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "a");
    uint32_t input2 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "b");
    ASSERT(input1 != 0 && input2 != 0);
    
    // Test comparison operations
    uint32_t eq_id = graph_builder_eq(builder, input1, input2, "eq");
    ASSERT(eq_id != 0);
    
    uint32_t ne_id = graph_builder_ne(builder, input1, input2, "ne");
    ASSERT(ne_id != 0);
    
    uint32_t lt_id = graph_builder_lt(builder, input1, input2, "lt");
    ASSERT(lt_id != 0);
    
    uint32_t le_id = graph_builder_le(builder, input1, input2, "le");
    ASSERT(le_id != 0);
    
    uint32_t gt_id = graph_builder_gt(builder, input1, input2, "gt");
    ASSERT(gt_id != 0);
    
    uint32_t ge_id = graph_builder_ge(builder, input1, input2, "ge");
    ASSERT(ge_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 8);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 6: Control flow operations
void test_control_flow_operations(void) {
    TEST("Control Flow Operations");
    
    GraphBuilder* builder = graph_builder_create("control_test");
    ASSERT(builder != NULL);
    
    uint32_t condition = graph_builder_add_input(builder, GRAPH_TYPE_BOOL, "condition");
    uint32_t true_val = graph_builder_add_input(builder, GRAPH_TYPE_F64, "true_val");
    uint32_t false_val = graph_builder_add_input(builder, GRAPH_TYPE_F64, "false_val");
    
    // Test select operation
    uint32_t select_id = graph_builder_select(builder, condition, true_val, false_val, "select");
    ASSERT(select_id != 0);
    
    // Test phi operation
    uint32_t inputs[] = {true_val, false_val};
    uint32_t phi_id = graph_builder_phi(builder, inputs, 2, "phi");
    ASSERT(phi_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 5);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 7: Memory operations
void test_memory_operations(void) {
    TEST("Memory Operations");
    
    GraphBuilder* builder = graph_builder_create("memory_test");
    ASSERT(builder != NULL);
    
    // Create size input for allocation
    GraphValue size_val = {0};
    size_val.type = GRAPH_TYPE_I64;
    size_val.data.i64 = 64;
    uint32_t size_node = graph_builder_add_constant(builder, size_val, "size");
    
    // Test allocation
    uint32_t alloc_id = graph_builder_alloc(builder, size_node, GRAPH_TYPE_F64, "alloc");
    ASSERT(alloc_id != 0);
    
    // Test store
    uint32_t value = graph_builder_add_input(builder, GRAPH_TYPE_F64, "value");
    uint32_t store_id = graph_builder_store(builder, alloc_id, value, "store");
    ASSERT(store_id != 0);
    
    // Test load
    uint32_t load_id = graph_builder_load(builder, alloc_id, GRAPH_TYPE_F64, "load");
    ASSERT(load_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 5);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 8: Type operations
void test_type_operations(void) {
    TEST("Type Operations");
    
    GraphBuilder* builder = graph_builder_create("type_test");
    ASSERT(builder != NULL);
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_I32, "input");
    ASSERT(input != 0);
    
    // Test cast operation
    uint32_t cast_id = graph_builder_cast(builder, input, GRAPH_TYPE_I32, GRAPH_TYPE_F64, "cast");
    ASSERT(cast_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 2);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 9: Function operations
void test_function_operations(void) {
    TEST("Function Operations");
    
    GraphBuilder* builder = graph_builder_create("function_test");
    ASSERT(builder != NULL);
    
    // Create arguments
    uint32_t arg1 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "arg1");
    uint32_t arg2 = graph_builder_add_input(builder, GRAPH_TYPE_F64, "arg2");
    uint32_t args[] = {arg1, arg2};
    
    // Test function call
    uint32_t call_id = graph_builder_call(builder, "test_function", args, 2, "call");
    ASSERT(call_id != 0);
    
    // Test return
    uint32_t return_id = graph_builder_return(builder, call_id, "return");
    ASSERT(return_id != 0);
    
    ASSERT(graph_builder_get_node_count(builder) == 4);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 10: Connection management
void test_connection_management(void) {
    TEST("Connection Management");
    
    GraphBuilder* builder = graph_builder_create("connection_test");
    ASSERT(builder != NULL);
    
    uint32_t input = graph_builder_add_input(builder, GRAPH_TYPE_F64, "input");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "output");
    
    // Test manual connection
    bool connected = graph_builder_connect(builder, input, 0, output, 0);
    ASSERT(connected == true);
    
    // Test edge existence
    bool has_edge = graph_builder_has_edge(builder, input, output);
    ASSERT(has_edge == true);
    
    // Test disconnection
    bool disconnected = graph_builder_disconnect(builder, input, output);
    ASSERT(disconnected == true);
    
    has_edge = graph_builder_has_edge(builder, input, output);
    ASSERT(has_edge == false);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 11: Graph building and validation
void test_graph_building(void) {
    TEST("Graph Building and Validation");
    
    GraphBuilder* builder = graph_builder_create("build_test");
    ASSERT(builder != NULL);
    
    // Create a simple expression: (a + b) * c
    uint32_t a = graph_builder_add_input(builder, GRAPH_TYPE_F64, "a");
    uint32_t b = graph_builder_add_input(builder, GRAPH_TYPE_F64, "b");
    uint32_t c = graph_builder_add_input(builder, GRAPH_TYPE_F64, "c");
    
    uint32_t add = graph_builder_add(builder, a, b, "add");
    uint32_t mul = graph_builder_mul(builder, add, c, "mul");
    uint32_t output = graph_builder_add_output(builder, GRAPH_TYPE_F64, "result");
    
    graph_builder_connect(builder, mul, 0, output, 0);
    
    // Validate before building
    char* error = NULL;
    bool valid = graph_builder_validate(builder, &error);
    ASSERT(valid == true);
    ASSERT(error == NULL);
    
    // Build the graph
    Graph* graph = graph_builder_build(builder);
    ASSERT(graph != NULL);
    ASSERT(graph->node_count == 6);
    ASSERT(graph->input_count == 3);
    ASSERT(graph->output_count == 1);
    
    graph_destroy(graph);
    graph_builder_destroy(builder);
    
    TEST_END;
}

// Test 12: Configuration management
void test_configuration_management(void) {
    TEST("Configuration Management");
    
    GraphBuilderConfig config = graph_builder_default_config();
    ASSERT(config.validate_on_build == true);
    ASSERT(config.enable_type_checking == true);
    
    // Test custom configuration
    config.auto_optimize = true;
    config.initial_node_capacity = 128;
    
    GraphBuilder* builder = graph_builder_create_with_config("config_test", &config);
    ASSERT(builder != NULL);
    
    GraphBuilderConfig retrieved_config;
    graph_builder_get_config(builder, &retrieved_config);
    ASSERT(retrieved_config.auto_optimize == true);
    ASSERT(retrieved_config.initial_node_capacity == 128);
    
    graph_builder_destroy(builder);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Builder Tests...\n\n");
    
    test_builder_creation();
    test_basic_node_creation();
    test_arithmetic_operations();
    test_logical_operations();
    test_comparison_operations();
    test_control_flow_operations();
    test_memory_operations();
    test_type_operations();
    test_function_operations();
    test_connection_management();
    test_graph_building();
    test_configuration_management();
    
    printf("\n=== Graph Builder Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

