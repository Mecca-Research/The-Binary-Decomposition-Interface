
/*
 * Graph Execution Fuzzing Harness
 * 
 * This harness targets the graph execution engine and optimization passes.
 * It fuzzes graph structures, node types, and execution orders to test:
 * - Graph construction and dependency resolution
 * - Execution scheduling and optimization passes
 * - Graph corruption and execution errors
 * - Graph traversal bugs and memory corruption
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include graph headers
#include "vm/graph/graph.h"
#include "vm/graph/graph_builder.h"
#include "vm/graph/graph_executor.h"
#include "vm/graph/graph_optimizer.h"
// Memory management handled internally

// Timeout protection
#include <signal.h>
#include <setjmp.h>

static jmp_buf graph_timeout_jmp;
static void graph_timeout_handler(int sig) {
    longjmp(graph_timeout_jmp, 1);
}

// Create a fuzzed graph from input data
static Graph* create_fuzz_graph(const uint8_t* data, size_t size) {
    if (size < 8) return NULL;
    
    GraphBuilder* builder = createGraphBuilder();
    if (!builder) return NULL;
    
    // Parse input to create graph structure
    size_t offset = 0;
    uint32_t num_nodes = (data[offset] % 16) + 1; // 1-16 nodes
    offset++;
    
    if (offset >= size) {
        freeGraphBuilder(builder);
        return NULL;
    }
    
    // Create nodes with fuzzed types
    NodeID* node_ids = malloc(num_nodes * sizeof(NodeID));
    if (!node_ids) {
        freeGraphBuilder(builder);
        return NULL;
    }
    
    for (uint32_t i = 0; i < num_nodes && offset < size; i++) {
        NodeType type = data[offset] % NODE_TYPE_COUNT;
        offset++;
        
        // Create node with fuzzed value
        Value node_value = NUMBER_VAL(0.0);
        if (offset + 4 <= size) {
            float fuzz_val = *(float*)(data + offset);
            node_value = NUMBER_VAL(fuzz_val);
            offset += 4;
        }
        
        node_ids[i] = addNode(builder, type, node_value);
    }
    
    // Add fuzzed edges
    uint32_t num_edges = (size - offset) / 2;
    if (num_edges > num_nodes * num_nodes) {
        num_edges = num_nodes * num_nodes;
    }
    
    for (uint32_t i = 0; i < num_edges && offset + 1 < size; i++) {
        uint32_t from_idx = data[offset] % num_nodes;
        uint32_t to_idx = data[offset + 1] % num_nodes;
        offset += 2;
        
        if (from_idx != to_idx) {
            addEdge(builder, node_ids[from_idx], node_ids[to_idx]);
        }
    }
    
    Graph* graph = buildGraph(builder);
    
    free(node_ids);
    freeGraphBuilder(builder);
    
    return graph;
}

// Test graph optimization
static void test_graph_optimization(Graph* graph) {
    if (!graph) return;
    
    GraphOptimizer* optimizer = createGraphOptimizer();
    if (!optimizer) return;
    
    // Apply various optimization passes
    optimizeGraph(optimizer, graph);
    
    freeGraphOptimizer(optimizer);
}

// Test graph execution
static void test_graph_execution(Graph* graph) {
    if (!graph) return;
    
    GraphExecutor* executor = createGraphExecutor();
    if (!executor) return;
    
    // Execute the graph
    ExecutionResult result = executeGraph(executor, graph);
    
    // Cleanup execution result
    if (result.success && result.output_values) {
        free(result.output_values);
    }
    
    freeGraphExecutor(executor);
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip invalid inputs
    if (size < 8 || size > 1024) return 0;
    
    // Set up timeout protection
    signal(SIGALRM, graph_timeout_handler);
    if (setjmp(graph_timeout_jmp) != 0) {
        alarm(0);
        return 0;
    }
    alarm(1);
    
    Graph* graph = create_fuzz_graph(data, size);
    if (!graph) {
        alarm(0);
        return 0;
    }
    
    // Test graph validation
    bool is_valid = validateGraph(graph);
    
    if (is_valid) {
        // Test optimization
        test_graph_optimization(graph);
        
        // Test execution
        test_graph_execution(graph);
    }
    
    // Cleanup
    freeGraph(graph);
    
    alarm(0);
    return 0;
}

// AFL entry point
#ifdef __AFL_COMPILER
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0 || size > 1024) {
        fclose(fp);
        return 0;
    }
    
    uint8_t* data = malloc(size);
    if (!data) {
        fclose(fp);
        return 1;
    }
    
    size_t read_size = fread(data, 1, size, fp);
    fclose(fp);
    
    if (read_size != size) {
        free(data);
        return 1;
    }
    
    int result = LLVMFuzzerTestOneInput(data, size);
    free(data);
    
    return result;
}
#endif
