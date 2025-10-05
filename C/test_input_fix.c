#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "vm/graph/graph.h"
#include "vm/graph/graph_executor.h"
#include "vm/graph/graph_builder.h"

int main() {
    printf("Testing input node value propagation fix...\n");
    
    // Create a simple graph: input -> output
    Graph* graph = graph_create("test_input_graph");
    assert(graph != NULL);
    
    // Add input node
    GraphNode* input_node = graph_add_input_node(graph, GRAPH_TYPE_F64, "input");
    assert(input_node != NULL);
    
    // Add output node
    GraphNode* output_node = graph_add_output_node(graph, GRAPH_TYPE_F64, "output");
    assert(output_node != NULL);
    
    // Connect input to output
    bool edge_added = graph_add_edge(graph, input_node->id, 0, output_node->id, 0);
    assert(edge_added);
    
    printf("Graph created with %d nodes and %d edges\n", graph->node_count, graph->edge_count);
    
    // Create executor
    GraphExecutor* executor = graph_executor_create();
    assert(executor != NULL);
    
    // Test input value propagation
    GraphValue test_input;
    test_input.type = GRAPH_TYPE_F64;
    test_input.data.f64 = 42.5;
    GraphValue inputs[] = {test_input};
    
    printf("Executing graph with input value: %f\n", test_input.data.f64);
    
    GraphExecutionResult result = graph_executor_execute(executor, graph, inputs, 1);
    
    if (result.success) {
        printf("Graph execution succeeded!\n");
        printf("Output count: %d\n", result.output_count);
        if (result.output_count > 0) {
            printf("Output value: %f\n", result.output_values[0].data.f64);
            
            // Check if the output matches the input (should be 42.5, not 0.0)
            if (fabs(result.output_values[0].data.f64 - 42.5) < 0.001) {
                printf("SUCCESS: Input value was correctly propagated!\n");
            } else {
                printf("FAILURE: Expected 42.5, got %f\n", result.output_values[0].data.f64);
                return 1;
            }
        } else {
            printf("FAILURE: No outputs produced\n");
            return 1;
        }
    } else {
        printf("FAILURE: Graph execution failed\n");
        if (result.error_message) {
            printf("Error: %s\n", result.error_message);
        }
        return 1;
    }
    
    // Cleanup
    graph_executor_destroy(executor);
    graph_destroy(graph);
    
    printf("Test completed successfully!\n");
    return 0;
}
