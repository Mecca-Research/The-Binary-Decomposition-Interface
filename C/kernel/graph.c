// ===================================================================
// DESC: Implements the basic memory management for BDI graph
//       structures.
// ===================================================================
#include "graph.h"
#include <stdlib.h>
#include <string.h>

BdiGraph* aion_graph_create() {
    BdiGraph* g = (BdiGraph*)calloc(1, sizeof(BdiGraph));
    if (!g) return NULL;
    return g;
}

void aion_graph_free(BdiGraph* g) {
    if (!g) return;
    free(g->nodes);
    free(g->edges);
    free(g->meta_arena);
    free(g);
}

NodeId aion_graph_add_node(BdiGraph* g, GraphNode node) {
    if (g->node_count >= g->node_capacity) {
        size_t new_capacity = g->node_capacity < 8 ? 8 : g->node_capacity * 2;
        GraphNode* new_nodes = (GraphNode*)realloc(g->nodes, new_capacity * sizeof(GraphNode));
        if (!new_nodes) return 0; // Return invalid ID on failure
        g->nodes = new_nodes;
        g->node_capacity = new_capacity;
    }
    NodeId id = g->node_count + 1; // Use 1-based indexing for IDs
    node.id = id;
    g->nodes[g->node_count++] = node;
    return id;
}
