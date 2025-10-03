// ===================================================================
// DESC: Implements the basic memory management for BDI graph
//       structures.
// ===================================================================
#include "c23_compat.h"
#include "graph.h"
#include <stdlib.h>
#include <string.h>
[[nodiscard]] 
BdiGraph* aeon_graph_create() {
    BdiGraph* g = (BdiGraph*)calloc(1, sizeof(BdiGraph));
    if (!g) return nullptr;
    // Initialize update spec array
    g->updates = nullptr;
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

// --- Implementation of aeon_attach_meta ---
int aeon_attach_meta(BdiGraph* g, NodeId node_id, const NodeMeta* meta) {
    if (!g || node_id == 0 || node_id > g->node_count || !meta) return -1;

    // 1. Allocate space in the metadata arena.
    if (g->meta_size + sizeof(NodeMeta) > g->meta_capacity) {
        size_t new_capacity = g->meta_capacity < 64 ? 64 : g->meta_capacity * 2;
        uint8_t* new_arena = (uint8_t*)realloc(g->meta_arena, new_capacity);
        if (!new_arena) return -1; // Allocation failure
        g->meta_arena = new_arena;
        g->meta_capacity = new_capacity;
    }

    // 2. Copy the metadata into the arena.
    size_t offset = g->meta_size;
    memcpy(g->meta_arena + offset, meta, sizeof(NodeMeta));
    g->meta_size += sizeof(NodeMeta);

    // 3. Update the node to point to this new metadata.
    g->nodes[node_id - 1].meta_off = offset;
    return 0; // Success
}
