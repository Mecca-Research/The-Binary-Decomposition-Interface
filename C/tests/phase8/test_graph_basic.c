
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../../vm/graph/graph.h"
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

// Test 1: Graph creation and destruction
void test_graph_creation(void) {
    TEST("Graph Creation and Destruction");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    ASSERT(graph->name != NULL);
    ASSERT(strcmp(graph->name, "test_graph") == 0);
    ASSERT(graph->node_count == 0);
    ASSERT(graph->edge_count == 0);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 2: Node creation and management
void test_node_creation(void) {
    TEST("Node Creation and Management");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Test constant node
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 42.0;
    
    GraphNode* const_node = graph_add_constant_node(graph, const_val, "constant");
    ASSERT(const_node != NULL);
    ASSERT(const_node->type == GRAPH_NODE_CONSTANT);
    ASSERT(const_node->is_constant == true);
    ASSERT(const_node->constant_value.data.f64 == 42.0);
    
    // Test input node
    GraphNode* input_node = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    ASSERT(input_node != NULL);
    ASSERT(input_node->type == GRAPH_NODE_INPUT);
    
    // Test output node
    GraphNode* output_node = graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    ASSERT(output_node != NULL);
    ASSERT(output_node->type == GRAPH_NODE_OUTPUT);
    
    // Test arithmetic node
    GraphNode* add_node = graph_add_node(graph, GRAPH_NODE_ADD, "add");
    ASSERT(add_node != NULL);
    ASSERT(add_node->type == GRAPH_NODE_ADD);
    
    ASSERT(graph->node_count == 4);
    ASSERT(graph->input_count == 1);
    ASSERT(graph->output_count == 1);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 3: Edge creation and management
void test_edge_creation(void) {
    TEST("Edge Creation and Management");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Create nodes
    GraphNode* node1 = graph_add_node(graph, GRAPH_NODE_CONSTANT, "node1");
    GraphNode* node2 = graph_add_node(graph, GRAPH_NODE_ADD, "node2");
    ASSERT(node1 != NULL && node2 != NULL);
    
    // Create edge
    GraphEdge* edge = graph_add_edge(graph, node1->id, 0, node2->id, 0);
    ASSERT(edge != NULL);
    ASSERT(edge->source == node1);
    ASSERT(edge->target == node2);
    ASSERT(edge->source_output == 0);
    ASSERT(edge->target_input == 0);
    
    ASSERT(graph->edge_count == 1);
    ASSERT(node2->dependency_count == 1);
    
    // Test edge removal
    bool removed = graph_remove_edge(graph, node1->id, 0, node2->id, 0);
    ASSERT(removed == true);
    ASSERT(graph->edge_count == 0);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 4: Graph validation
void test_graph_validation(void) {
    TEST("Graph Validation");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Valid graph
    GraphNode* input = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    GraphNode* output = graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    graph_add_edge(graph, input->id, 0, output->id, 0);
    
    char* error = NULL;
    bool valid = graph_validate(graph, &error);
    ASSERT(valid == true);
    ASSERT(error == NULL);
    
    // Test DAG property
    bool is_dag = graph_is_dag(graph);
    ASSERT(is_dag == true);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 5: Graph cloning
void test_graph_cloning(void) {
    TEST("Graph Cloning");
    
    Graph* original = graph_create("original");
    ASSERT(original != NULL);
    
    // Create a simple graph
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 3.14;
    
    GraphNode* const_node = graph_add_constant_node(original, const_val, "pi");
    GraphNode* output = graph_add_output_node(original, GRAPH_TYPE_F64, "output");
    graph_add_edge(original, const_node->id, 0, output->id, 0);
    
    // Clone the graph
    Graph* clone = graph_clone(original);
    ASSERT(clone != NULL);
    ASSERT(clone->node_count == original->node_count);
    ASSERT(clone->edge_count == original->edge_count);
    ASSERT(clone->input_count == original->input_count);
    ASSERT(clone->output_count == original->output_count);
    
    // Verify clone is independent
    ASSERT(clone != original);
    ASSERT(clone->nodes != original->nodes);
    
    graph_destroy(original);
    graph_destroy(clone);
    
    TEST_END;
}

// Test 6: Node lookup functions
void test_node_lookup(void) {
    TEST("Node Lookup Functions");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    GraphNode* node = graph_add_node(graph, GRAPH_NODE_ADD, "test_node");
    ASSERT(node != NULL);
    
    uint32_t node_id = node->id;
    
    // Test lookup by ID
    GraphNode* found_by_id = graph_get_node(graph, node_id);
    ASSERT(found_by_id == node);
    
    // Test lookup by name
    GraphNode* found_by_name = graph_find_node(graph, "test_node");
    ASSERT(found_by_name == node);
    
    // Test lookup of non-existent node
    GraphNode* not_found = graph_get_node(graph, 99999);
    ASSERT(not_found == NULL);
    
    GraphNode* not_found_by_name = graph_find_node(graph, "nonexistent");
    ASSERT(not_found_by_name == NULL);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 7: Graph statistics
void test_graph_statistics(void) {
    TEST("Graph Statistics");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Create a small graph
    GraphNode* input = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    GraphValue const_val = {0};
    const_val.type = GRAPH_TYPE_F64;
    const_val.data.f64 = 2.0;
    GraphNode* constant = graph_add_constant_node(graph, const_val, "constant");
    GraphNode* add = graph_add_node(graph, GRAPH_NODE_ADD, "add");
    GraphNode* output = graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    
    graph_add_edge(graph, input->id, 0, add->id, 0);
    graph_add_edge(graph, constant->id, 0, add->id, 1);
    graph_add_edge(graph, add->id, 0, output->id, 0);
    
    GraphStats stats;
    graph_get_stats(graph, &stats);
    
    ASSERT(stats.total_nodes == 4);
    ASSERT(stats.total_edges == 3);
    ASSERT(stats.input_nodes == 1);
    ASSERT(stats.output_nodes == 1);
    ASSERT(stats.constant_nodes == 1);
    ASSERT(stats.operation_nodes == 1);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 8: Graph value operations
void test_graph_values(void) {
    TEST("Graph Value Operations");
    
    // Test value creation
    double test_val = 42.5;
    GraphValue value = graph_value_create(GRAPH_TYPE_F64, &test_val);
    ASSERT(value.type == GRAPH_TYPE_F64);
    ASSERT(value.data.f64 == 42.5);
    
    // Test value equality
    GraphValue value2 = graph_value_create(GRAPH_TYPE_F64, &test_val);
    ASSERT(graph_value_equals(&value, &value2) == true);
    
    double different_val = 24.5;
    GraphValue value3 = graph_value_create(GRAPH_TYPE_F64, &different_val);
    ASSERT(graph_value_equals(&value, &value3) == false);
    
    // Test value to string
    char* str = graph_value_to_string(&value);
    ASSERT(str != NULL);
    ASSERT(strstr(str, "42.5") != NULL);
    free(str);
    
    TEST_END;
}

// Test 9: Graph memory management
void test_graph_memory_management(void) {
    TEST("Graph Memory Management");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    ASSERT(graph->memory_pool != NULL);
    ASSERT(graph->memory_used == 0);
    
    // Test allocation
    void* ptr1 = graph_alloc(graph, 64);
    ASSERT(ptr1 != NULL);
    ASSERT(graph->memory_used > 0);
    
    void* ptr2 = graph_alloc(graph, 128);
    ASSERT(ptr2 != NULL);
    ASSERT(ptr2 != ptr1);
    
    // Test memory reset
    graph_reset_memory_pool(graph);
    ASSERT(graph->memory_used == 0);
    
    graph_destroy(graph);
    
    TEST_END;
}

// Test 10: Graph topological sorting
void test_topological_sorting(void) {
    TEST("Graph Topological Sorting");
    
    Graph* graph = graph_create("test_graph");
    ASSERT(graph != NULL);
    
    // Create a DAG: A -> B -> C
    GraphNode* nodeA = graph_add_node(graph, GRAPH_NODE_INPUT, "A");
    GraphNode* nodeB = graph_add_node(graph, GRAPH_NODE_ADD, "B");
    GraphNode* nodeC = graph_add_node(graph, GRAPH_NODE_OUTPUT, "C");
    
    graph_add_edge(graph, nodeA->id, 0, nodeB->id, 0);
    graph_add_edge(graph, nodeB->id, 0, nodeC->id, 0);
    
    uint32_t count;
    uint32_t* order = graph_topological_sort(graph, &count);
    
    ASSERT(order != NULL);
    ASSERT(count == 3);
    
    // Verify topological order (A should come before B, B before C)
    int posA = -1, posB = -1, posC = -1;
    for (uint32_t i = 0; i < count; i++) {
        if (order[i] == nodeA->id) posA = i;
        else if (order[i] == nodeB->id) posB = i;
        else if (order[i] == nodeC->id) posC = i;
    }
    
    ASSERT(posA >= 0 && posB >= 0 && posC >= 0);
    ASSERT(posA < posB);
    ASSERT(posB < posC);
    
    free(order);
    graph_destroy(graph);
    
    TEST_END;
}

int main(void) {
    printf("Running Graph Basic Tests...\n\n");
    
    test_graph_creation();
    test_node_creation();
    test_edge_creation();
    test_graph_validation();
    test_graph_cloning();
    test_node_lookup();
    test_graph_statistics();
    test_graph_values();
    test_graph_memory_management();
    test_topological_sorting();
    
    printf("\n=== Graph Basic Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

