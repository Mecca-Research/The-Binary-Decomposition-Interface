
#include "../../framework/test_framework.h"
#include "../../../vm/graph/graph.h"
#include "../../../vm/graph/graph_builder.h"
#include "../../../vm/graph/graph_executor.h"

// Test graph creation and basic operations
static bool test_graph_creation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Test initial state
    TEST_ASSERT_EQ(0, graph_node_count(graph), "New graph should have no nodes");
    TEST_ASSERT_EQ(0, graph_edge_count(graph), "New graph should have no edges");
    TEST_ASSERT(!graph_has_cycles(graph), "Empty graph should not have cycles");
    
    graph_destroy(graph);
    TEST_MEMORY_VERIFY("Graph creation should not leak memory");
    
    return true;
}

// Test node creation and management
static bool test_node_operations(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Create nodes
    NodeID node1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(42.0));
    NodeID node2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(3.14));
    NodeID node3 = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    
    TEST_ASSERT_NEQ(INVALID_NODE_ID, node1, "Node creation should succeed");
    TEST_ASSERT_NEQ(INVALID_NODE_ID, node2, "Node creation should succeed");
    TEST_ASSERT_NEQ(INVALID_NODE_ID, node3, "Node creation should succeed");
    TEST_ASSERT_EQ(3, graph_node_count(graph), "Graph should have 3 nodes");
    
    // Test node properties
    GraphNode* n1 = graph_get_node(graph, node1);
    TEST_ASSERT_NOT_NULL(n1, "Node retrieval should succeed");
    TEST_ASSERT_EQ(NODE_CONSTANT, n1->type, "Node type should match");
    TEST_ASSERT(IS_NUMBER(n1->value), "Node value should be a number");
    TEST_ASSERT_EQ(42.0, AS_NUMBER(n1->value), "Node value should match");
    
    // Test node removal
    bool removed = graph_remove_node(graph, node2);
    TEST_ASSERT(removed, "Node removal should succeed");
    TEST_ASSERT_EQ(2, graph_node_count(graph), "Graph should have 2 nodes after removal");
    
    GraphNode* removed_node = graph_get_node(graph, node2);
    TEST_ASSERT_NULL(removed_node, "Removed node should not be accessible");
    
    graph_destroy(graph);
    TEST_MEMORY_VERIFY("Node operations should not leak memory");
    
    return true;
}

// Test edge creation and management
static bool test_edge_operations(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Create nodes
    NodeID node1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID node2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(3.0));
    NodeID node3 = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    
    // Create edges
    EdgeID edge1 = graph_add_edge(graph, node1, node3, 0); // First input to ADD
    EdgeID edge2 = graph_add_edge(graph, node2, node3, 1); // Second input to ADD
    
    TEST_ASSERT_NEQ(INVALID_EDGE_ID, edge1, "Edge creation should succeed");
    TEST_ASSERT_NEQ(INVALID_EDGE_ID, edge2, "Edge creation should succeed");
    TEST_ASSERT_EQ(2, graph_edge_count(graph), "Graph should have 2 edges");
    
    // Test edge properties
    GraphEdge* e1 = graph_get_edge(graph, edge1);
    TEST_ASSERT_NOT_NULL(e1, "Edge retrieval should succeed");
    TEST_ASSERT_EQ(node1, e1->from, "Edge source should match");
    TEST_ASSERT_EQ(node3, e1->to, "Edge destination should match");
    TEST_ASSERT_EQ(0, e1->input_index, "Edge input index should match");
    
    // Test node connectivity
    TEST_ASSERT_EQ(1, graph_node_output_count(graph, node1), "Node1 should have 1 output");
    TEST_ASSERT_EQ(1, graph_node_output_count(graph, node2), "Node2 should have 1 output");
    TEST_ASSERT_EQ(2, graph_node_input_count(graph, node3), "Node3 should have 2 inputs");
    
    // Test edge removal
    bool removed = graph_remove_edge(graph, edge1);
    TEST_ASSERT(removed, "Edge removal should succeed");
    TEST_ASSERT_EQ(1, graph_edge_count(graph), "Graph should have 1 edge after removal");
    TEST_ASSERT_EQ(1, graph_node_input_count(graph, node3), "Node3 should have 1 input after edge removal");
    
    graph_destroy(graph);
    TEST_MEMORY_VERIFY("Edge operations should not leak memory");
    
    return true;
}

// Test graph validation
static bool test_graph_validation(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Create a valid graph
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(10.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(5.0));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    graph_add_edge(graph, add, output, 0);
    
    // Validate the graph
    bool is_valid = graph_validate(graph);
    TEST_ASSERT(is_valid, "Valid graph should pass validation");
    
    // Create an invalid graph (cycle)
    Graph* invalid_graph = graph_create();
    NodeID n1 = graph_add_node(invalid_graph, NODE_ADD, VALUE_NIL);
    NodeID n2 = graph_add_node(invalid_graph, NODE_MULTIPLY, VALUE_NIL);
    
    graph_add_edge(invalid_graph, n1, n2, 0);
    graph_add_edge(invalid_graph, n2, n1, 0); // Creates cycle
    
    bool is_invalid = graph_validate(invalid_graph);
    TEST_ASSERT(!is_invalid, "Invalid graph with cycle should fail validation");
    
    graph_destroy(graph);
    graph_destroy(invalid_graph);
    TEST_MEMORY_VERIFY("Graph validation should not leak memory");
    
    return true;
}

// Test topological sorting
static bool test_topological_sort(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Create a DAG
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(1.0));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(2.0));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    NodeID mult = graph_add_node(graph, NODE_MULTIPLY, VALUE_NIL);
    NodeID output = graph_add_node(graph, NODE_OUTPUT, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    graph_add_edge(graph, add, mult, 0);
    graph_add_edge(graph, const2, mult, 1);
    graph_add_edge(graph, mult, output, 0);
    
    // Get topological order
    size_t order_count;
    NodeID* topo_order = graph_topological_sort(graph, &order_count);
    TEST_ASSERT_NOT_NULL(topo_order, "Topological sort should succeed");
    TEST_ASSERT_EQ(5, order_count, "Should have 5 nodes in topological order");
    
    // Verify that constants come before operations that use them
    size_t const1_pos = SIZE_MAX, const2_pos = SIZE_MAX, add_pos = SIZE_MAX;
    for (size_t i = 0; i < order_count; i++) {
        if (topo_order[i] == const1) const1_pos = i;
        if (topo_order[i] == const2) const2_pos = i;
        if (topo_order[i] == add) add_pos = i;
    }
    
    TEST_ASSERT(const1_pos < add_pos, "const1 should come before add in topological order");
    TEST_ASSERT(const2_pos < add_pos, "const2 should come before add in topological order");
    
    free(topo_order);
    graph_destroy(graph);
    TEST_MEMORY_VERIFY("Topological sort should not leak memory");
    
    return true;
}

// Test graph cloning
static bool test_graph_cloning(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* original = graph_create();
    TEST_ASSERT_NOT_NULL(original, "Graph creation should succeed");
    
    // Create a graph
    NodeID const1 = graph_add_node(original, NODE_CONSTANT, VALUE_NUMBER(42.0));
    NodeID const2 = graph_add_node(original, NODE_CONSTANT, VALUE_NUMBER(3.14));
    NodeID add = graph_add_node(original, NODE_ADD, VALUE_NIL);
    
    graph_add_edge(original, const1, add, 0);
    graph_add_edge(original, const2, add, 1);
    
    // Clone the graph
    Graph* clone = graph_clone(original);
    TEST_ASSERT_NOT_NULL(clone, "Graph cloning should succeed");
    
    // Verify clone has same structure
    TEST_ASSERT_EQ(graph_node_count(original), graph_node_count(clone), "Clone should have same node count");
    TEST_ASSERT_EQ(graph_edge_count(original), graph_edge_count(clone), "Clone should have same edge count");
    
    // Verify nodes are equivalent but not identical
    NodeID* orig_nodes = graph_get_all_nodes(original);
    NodeID* clone_nodes = graph_get_all_nodes(clone);
    
    for (size_t i = 0; i < graph_node_count(original); i++) {
        GraphNode* orig_node = graph_get_node(original, orig_nodes[i]);
        GraphNode* clone_node = graph_get_node(clone, clone_nodes[i]);
        
        TEST_ASSERT_EQ(orig_node->type, clone_node->type, "Node types should match");
        TEST_ASSERT(values_equal(orig_node->value, clone_node->value), "Node values should match");
        TEST_ASSERT_NEQ(orig_node, clone_node, "Nodes should be different objects");
    }
    
    free(orig_nodes);
    free(clone_nodes);
    graph_destroy(original);
    graph_destroy(clone);
    TEST_MEMORY_VERIFY("Graph cloning should not leak memory");
    
    return true;
}

// Test graph serialization
static bool test_graph_serialization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    // Create a graph
    NodeID const1 = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(123.45));
    NodeID const2 = graph_add_node(graph, NODE_CONSTANT, VALUE_BOOL(true));
    NodeID add = graph_add_node(graph, NODE_ADD, VALUE_NIL);
    
    graph_add_edge(graph, const1, add, 0);
    graph_add_edge(graph, const2, add, 1);
    
    // Serialize graph
    size_t serialized_size;
    uint8_t* serialized_data = graph_serialize(graph, &serialized_size);
    TEST_ASSERT_NOT_NULL(serialized_data, "Graph serialization should succeed");
    TEST_ASSERT(serialized_size > 0, "Serialized data should have non-zero size");
    
    // Deserialize graph
    Graph* deserialized = graph_deserialize(serialized_data, serialized_size);
    TEST_ASSERT_NOT_NULL(deserialized, "Graph deserialization should succeed");
    
    // Verify deserialized graph matches original
    TEST_ASSERT_EQ(graph_node_count(graph), graph_node_count(deserialized), "Node counts should match");
    TEST_ASSERT_EQ(graph_edge_count(graph), graph_edge_count(deserialized), "Edge counts should match");
    
    free(serialized_data);
    graph_destroy(graph);
    graph_destroy(deserialized);
    TEST_MEMORY_VERIFY("Graph serialization should not leak memory");
    
    return true;
}

// Test graph performance with large graphs
static bool test_graph_performance(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Graph* graph = graph_create();
    TEST_ASSERT_NOT_NULL(graph, "Graph creation should succeed");
    
    TEST_BENCHMARK_START();
    
    // Create a large graph (1000 nodes)
    NodeID* nodes = malloc(1000 * sizeof(NodeID));
    for (int i = 0; i < 1000; i++) {
        nodes[i] = graph_add_node(graph, NODE_CONSTANT, VALUE_NUMBER(i));
    }
    
    // Add edges to create a chain
    for (int i = 0; i < 999; i++) {
        NodeID add_node = graph_add_node(graph, NODE_ADD, VALUE_NIL);
        graph_add_edge(graph, nodes[i], add_node, 0);
        graph_add_edge(graph, nodes[i+1], add_node, 1);
    }
    
    TEST_BENCHMARK_END("Large graph creation (1000 nodes)");
    
    // Test operations on large graph
    TEST_BENCHMARK_START();
    bool is_valid = graph_validate(graph);
    TEST_BENCHMARK_END("Large graph validation");
    
    TEST_ASSERT(is_valid, "Large graph should be valid");
    TEST_ASSERT_EQ(1000, graph_node_count(graph), "Should have 1000 constant nodes");
    
    free(nodes);
    graph_destroy(graph);
    TEST_MEMORY_VERIFY("Graph performance test should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t graph_basic_tests[] = {
    test_graph_creation,
    test_node_operations,
    test_edge_operations,
    test_graph_validation,
    test_topological_sort,
    test_graph_cloning,
    test_graph_serialization,
    test_graph_performance
};

test_suite_t graph_test_suite = {
    .name = "Graph Basic Tests",
    .tests = graph_basic_tests,
    .test_count = sizeof(graph_basic_tests) / sizeof(graph_basic_tests[0])
};
