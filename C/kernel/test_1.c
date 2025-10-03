// ===================================================================
// DESC: Integration test for the Aeon Kernel
//       This test creates a simple graph (c = a + b), allocates memory
//       with HAM, and executes the nodes on the CPU device.
// ===================================================================
#include "c23_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graph.h"
#include "ham.h"
#include "device.h"

// Link against the implementations in our other .c files
extern HamVTable HAM_DEFAULT_IMPL;
extern DeviceVTable CPU_DEVICE_IMPL;

int main(void) {
    printf("--- Aeon-0 Kernel: M0 Integration Test ---\n");

    // 1. Get Interfaces to Kernel Services
    HamVTable* ham = &HAM_DEFAULT_IMPL;
    DeviceVTable* cpu = &CPU_DEVICE_IMPL;
    printf("-> Fetched HAM and CPU Device interfaces.\n");

    // 2. Create the BDI Graph
    BdiGraph* g = aeon_graph_create();
    assert(g != nullptr && "Failed to create graph.");
    printf("-> BDI Graph created.\n");

    // 3. Allocate Memory Regions using HAM for our variables a, b, and c
    RegionId region_a_id, region_b_id, region_c_id;
    void *ptr_a, *ptr_b, *ptr_c;

    assert(ham->alloc(&region_a_id, HAM_ACTIVE, sizeof(float), &ptr_a) == 0);
    assert(ham->alloc(&region_b_id, HAM_ACTIVE, sizeof(float), &ptr_b) == 0);
    assert(ham->alloc(&region_c_id, HAM_ACTIVE, sizeof(float), &ptr_c) == 0);
    printf("-> Allocated HAM regions for inputs (a, b) and output (c).\n");

    // 4. Populate Input Memory
    *(float*)ptr_a = 15.5f;
    *(float*)ptr_b = 24.5f;
    printf("-> Populated inputs: a = %f, b = %f\n", *(float*)ptr_a, *(float*)ptr_b);

    // 5. Build the BDI Graph Nodes
    // Node 1: Constant 'a' ( conceptually, points to data in region_a )
    GraphNode node_a = {.op = OP_CONST, .region_hint = region_a_id};
    NodeId node_a_id = aeon_graph_add_node(g, node_a);

    // Node 2: Constant 'b' ( conceptually, points to data in region_b )
    GraphNode node_b = {.op = OP_CONST, .region_hint = region_b_id};
    NodeId node_b_id = aeon_graph_add_node(g, node_b);

    // Node 3: The ADD operation
    GraphNode node_add = {
        .op = OP_ADD,
        .input_count = 2,
        .inputs = {node_a_id, node_b_id},
        .region_hint = region_c_id // Output goes to region 'c'
    };
    NodeId node_add_id = aeon_graph_add_node(g, node_add);
    printf("-> Constructed BDI graph with 3 nodes (CONST, CONST, ADD).\n");

    // 6. Simulate Scheduler and Execute Nodes
    printf("-> Simulating scheduler dispatch...\n");
    void* kernel_ptr;
    
    // "Execute" the CONST nodes (this is a no-op in our M0 model)
    cpu->lower(&g->nodes[node_a_id-1], &kernel_ptr);
    cpu->enqueue(kernel_ptr, nullptr, 0);
    
    cpu->lower(&g->nodes[node_b_id-1], &kernel_ptr);
    cpu->enqueue(kernel_ptr, nullptr, 0);

    // Execute the ADD node
    // A real scheduler would build this list based on node->inputs
    HamRegion region_a = {.id = region_a_id, .base = ptr_a};
    HamRegion region_b = {.id = region_b_id, .base = ptr_b};
    HamRegion region_c = {.id = region_c_id, .base = ptr_c};
    const HamRegion* add_op_regions[] = {&region_a, &region_b, &region_c};

    cpu->lower(&g->nodes[node_add_id-1], &kernel_ptr);
    cpu->enqueue(kernel_ptr, add_op_regions, 3);

    // 7. Verify the Result
    float result = *(float*)ptr_c;
    printf("-> Computation complete. Result in region c: %f\n", result);
    assert(result == 40.0f && "Result of 15.5 + 24.5 should be 40.0");
    printf("-> Verification successful!\n");

    // 8. Cleanup
    ham->free(region_a_id);
    ham->free(region_b_id);
    ham->free(region_c_id);
    aeon_graph_free(g);
    printf("-> Cleaned up graph and HAM regions.\n");

    printf("\n--- M0 Integration Test PASSED ---\n");

    return 0;
}
