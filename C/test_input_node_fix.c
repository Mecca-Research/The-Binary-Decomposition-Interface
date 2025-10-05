#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "vm/graph/graph.h"
#include "vm/graph/graph_executor.h"

int main() {
    printf("Testing input node value propagation fix...\n");
    
    // Create a simple graph with one input node
    Graph* graph = graph_create("test_input_graph");
    assert(graph != NULL);
    
    // Add input node
    GraphNode* input_node = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    assert(input_node != NULL);
    
    printf("Created graph with input node (ID: %d)\n", input_node->id);
    
    // Create execution context manually
    GraphExecutionContext* context = graph_execution_context_create(graph);
    assert(context != NULL);
    
    // Set up input values
    GraphValue test_input;
    test_input.type = GRAPH_TYPE_F64;
    test_input.data.f64 = 42.5;
    
    context->input_values[0] = test_input;
    context->input_count = 1;
    
    printf("Set input value: %f\n", test_input.data.f64);
    
    // Test the input node execution directly
    GraphValue outputs[1];
    bool success = graph_execute_node(input_node, NULL, outputs, context);
    
    if (success) {
        printf("Input node execution succeeded!\n");
        printf("Output value: %f\n", outputs[0].data.f64);
        
        // Check if the output matches the input (should be 42.5, not 0.0)
        if (fabs(outputs[0].data.f64 - 42.5) < 0.001) {
            printf("SUCCESS: Input value was correctly propagated!\n");
            printf("Before fix: would have been 0.0\n");
            printf("After fix: correctly shows %f\n", outputs[0].data.f64);
        } else {
            printf("FAILURE: Expected 42.5, got %f\n", outputs[0].data.f64);
            return 1;
        }
    } else {
        printf("FAILURE: Input node execution failed\n");
        return 1;
    }
    
    // Test with a different input value
    printf("\nTesting with different input value...\n");
    test_input.data.f64 = -123.456;
    context->input_values[0] = test_input;
    
    success = graph_execute_node(input_node, NULL, outputs, context);
    
    if (success && fabs(outputs[0].data.f64 - (-123.456)) < 0.001) {
        printf("SUCCESS: Second test passed with value %f\n", outputs[0].data.f64);
    } else {
        printf("FAILURE: Second test failed\n");
        return 1;
    }
    
    // Cleanup
    graph_execution_context_destroy(context);
    graph_destroy(graph);
    
    printf("\nAll tests passed! Input node value propagation is working correctly.\n");
    return 0;
}
