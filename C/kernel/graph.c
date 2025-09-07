// ===================================================================
// DESC: Implements the basic memory management for BDI graph
//       structures.
// ===================================================================
#include "graph.h"
#include <stdlib.h>
#include <string.h>

BdiGraph* aeon_graph_create() {
    BdiGraph* g = (BdiGraph*)calloc(1, sizeof(BdiGraph));
    if (!g) return NULL;
    // Initialize update spec array
    g->updates = NULL;
    g->update_count = 0;
    g->update_capacity = 0;
    return g;
}

void aeon_graph_free(BdiGraph* g) {
    if (!g) return;
    free(g->nodes);
    free(g->edges);
    free(g->meta_arena);
    free(g->updates); // Free update specs
    free(g);
}

NodeId aeon_graph_add_node(BdiGraph* g, GraphNode node) {
    if (g->node_count >= g->node_capacity) {
        size_t new_capacity = g->node_capacity < 8 ? 8 : g->node_capacity * 2;
        GraphNode* new_nodes = (GraphNode*)realloc(g->nodes, new_capacity * sizeof(GraphNode));
        if (!new_nodes) return 0;
        g->nodes = new_nodes;
        g->node_capacity = new_capacity;
    }
    NodeId id = g->node_count + 1;
    node.id = id;
    g->nodes[g->node_count++] = node;
    return id;
}

// --- Implementation of aeon_bind_update ---
int aeon_bind_update(BdiGraph* g, const UpdateSpec* spec) {
    if (!g || !spec) return -1;
    if (g->update_count >= g->update_capacity) {
        size_t new_capacity = g->update_capacity < 4 ? 4 : g->update_capacity * 2;
        UpdateSpec* new_updates = (UpdateSpec*)realloc(g->updates, new_capacity * sizeof(UpdateSpec));
        if (!new_updates) return -1; // Allocation failure
        g->updates = new_updates;
        g->update_capacity = new_capacity;
    }
    g->updates[g->update_count++] = *spec;
    return 0; // Success
}
