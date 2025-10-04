
// ===================================================================
// Phase 5.1: Graph Optimization
// DESC: Graph simplification, fusion, and serialization for BDI graphs
// ===================================================================
#ifndef AEON_GRAPH_OPT_H
#define AEON_GRAPH_OPT_H

#include "../../c23_compat.h"
#include "../../graph/graph.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// --- Graph Serialization Format ---
#define GRAPH_MAGIC 0x42444947  // "BDIG"
#define GRAPH_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t node_count;
    uint64_t edge_count;
    uint64_t checksum;
} GraphHeader;

// --- Subgraph for Fusion ---
typedef struct {
    NodeId* nodes;
    size_t count;
    size_t capacity;
    float fusion_score;  // Heuristic score for fusion benefit
} Subgraph;

// --- Graph Optimization API ---

// Simplification: Remove dead nodes and merge constants
[[nodiscard]] int graph_simplify(BdiGraph* graph);
[[nodiscard]] int graph_remove_dead_nodes(BdiGraph* graph);
[[nodiscard]] int graph_merge_constants(BdiGraph* graph);

// Fusion: Identify and fuse subgraphs
[[nodiscard]] Subgraph* identify_fusible_subgraph(const BdiGraph* graph, size_t* out_count);
[[nodiscard]] int fuse_subgraph(BdiGraph* graph, const Subgraph* subgraph);
void subgraph_free(Subgraph* sg);

// Serialization: Save and load graphs
[[nodiscard]] int graph_serialize(const BdiGraph* graph, const char* path);
[[nodiscard]] BdiGraph* graph_deserialize(const char* path);

// Utility functions
[[nodiscard]] bool is_node_dead(const BdiGraph* graph, NodeId node_id);
[[nodiscard]] bool can_merge_constants(const GraphNode* n1, const GraphNode* n2);
[[nodiscard]] uint64_t compute_graph_checksum(const BdiGraph* graph);

#endif // AEON_GRAPH_OPT_H
