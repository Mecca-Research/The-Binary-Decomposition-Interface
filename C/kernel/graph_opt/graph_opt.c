
// ===================================================================
// Phase 5.1: Graph Optimization Implementation
// ===================================================================
#include "graph_opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Graph Simplification ---

int graph_simplify(BdiGraph* graph) {
    if (!graph) return -1;
    
    int result = 0;
    result |= graph_remove_dead_nodes(graph);
    result |= graph_merge_constants(graph);
    
    return result;
}

int graph_remove_dead_nodes(BdiGraph* graph) {
    if (!graph || !graph->nodes) return -1;
    
    size_t removed = 0;
    
    // Mark live nodes (reachable from outputs)
    bool* live = calloc(graph->node_count, sizeof(bool));
    if (!live) return -1;
    
    // Simple liveness: mark all nodes as live for M0
    // TODO: Implement proper reachability analysis
    for (size_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].opcode == OP_RET) {
            live[i] = true;
        }
    }
    
    // Backward propagation of liveness
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < graph->node_count; i++) {
            if (live[i]) {
                for (size_t j = 0; j < graph->nodes[i].input_count; j++) {
                    NodeId input_id = graph->nodes[i].inputs[j];
                    if (input_id < graph->node_count && !live[input_id]) {
                        live[input_id] = true;
                        changed = true;
                    }
                }
            }
        }
    }
    
    // Remove dead nodes
    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < graph->node_count; read_idx++) {
        if (live[read_idx]) {
            if (write_idx != read_idx) {
                graph->nodes[write_idx] = graph->nodes[read_idx];
            }
            write_idx++;
        } else {
            removed++;
        }
    }
    
    graph->node_count = write_idx;
    free(live);
    
    return (int)removed;
}

int graph_merge_constants(BdiGraph* graph) {
    if (!graph || !graph->nodes) return -1;
    
    size_t merged = 0;
    
    // Find constant nodes
    for (size_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].opcode != OP_CONST) continue;
        
        for (size_t j = i + 1; j < graph->node_count; j++) {
            if (graph->nodes[j].opcode != OP_CONST) continue;
            
            if (can_merge_constants(&graph->nodes[i], &graph->nodes[j])) {
                // Redirect all uses of j to i
                for (size_t k = 0; k < graph->node_count; k++) {
                    for (size_t inp = 0; inp < graph->nodes[k].input_count; inp++) {
                        if (graph->nodes[k].inputs[inp] == graph->nodes[j].id) {
                            graph->nodes[k].inputs[inp] = graph->nodes[i].id;
                        }
                    }
                }
                
                // Mark j as dead (will be removed by dead node elimination)
                graph->nodes[j].input_count = 0;  // Mark as having no inputs (dead node)
                graph->nodes[j].opcode = OP_CONST;  // Keep as CONST for safety that won't be treated as live
                merged++;
            }
        }
    }
    
    return (int)merged;
}

bool is_node_dead(const BdiGraph* graph, NodeId node_id) {
    if (!graph || node_id >= graph->node_count) return true;
    
    // A node is dead if it has no users and is not an output
    const GraphNode* node = &graph->nodes[node_id];
    if (node->op == OP_RET) return false;
    
    // Check if any node uses this node
    for (size_t i = 0; i < graph->node_count; i++) {
        for (size_t j = 0; j < graph->nodes[i].input_count; j++) {
            if (graph->nodes[i].inputs[j] == node_id) {
                return false;
            }
        }
    }
    
    return true;
}

bool can_merge_constants(const GraphNode* n1, const GraphNode* n2) {
    if (!n1 || !n2) return false;
    if (n1->opcode != OP_CONST || n2->opcode != OP_CONST) return false;
    if (n1->type.id != n2->type.id) return false;
    
    // Compare constant values (simplified)
    return n1->const_value == n2->const_value;
}

// --- Graph Fusion ---

Subgraph* identify_fusible_subgraph(const BdiGraph* graph, size_t* out_count) {
    if (!graph || !out_count) return NULL;
    
    // Allocate subgraphs array
    Subgraph* subgraphs = malloc(sizeof(Subgraph) * 16);
    if (!subgraphs) return NULL;
    
    size_t sg_count = 0;
    
    // Identify chains of fusible operations
    for (size_t i = 0; i < graph->node_count; i++) {
        const GraphNode* node = &graph->nodes[i];
        
        // Look for arithmetic chains
        if (node->op == OP_ADD || node->op == OP_MUL) {
            Subgraph sg = {0};
            sg.nodes = malloc(sizeof(NodeId) * 8);
            sg.capacity = 8;
            sg.count = 0;
            sg.fusion_score = 1.0f;
            
            // Add this node
            sg.nodes[sg.count++] = node->id;
            
            // Try to extend the chain
            for (size_t j = 0; j < node->input_count; j++) {
                NodeId input_id = node->inputs[j];
                if (input_id < graph->node_count) {
                    const GraphNode* input_node = &graph->nodes[input_id];
                    if (input_node->op == OP_ADD || input_node->op == OP_MUL) {
                        if (sg.count >= sg.capacity) {
                            sg.capacity *= 2;
                            sg.nodes = realloc(sg.nodes, sizeof(NodeId) * sg.capacity);
                        }
                        sg.nodes[sg.count++] = input_id;
                        sg.fusion_score += 0.5f;
                    }
                }
            }
            
            if (sg.count > 1) {
                subgraphs[sg_count++] = sg;
            } else {
                free(sg.nodes);
            }
        }
    }
    
    *out_count = sg_count;
    return subgraphs;
}

int fuse_subgraph(BdiGraph* graph, const Subgraph* subgraph) {
    if (!graph || !subgraph || subgraph->count == 0) return -1;
    
    // Create a fused node
    GraphNode fused = {0};
    fused.id = graph->node_count;
    fused.opcode = OP_SUBGRAPH_BEGIN;  // Mark as fused subgraph
    fused.input_count = 0;
    
    // Collect all external inputs
    for (size_t i = 0; i < subgraph->count; i++) {
        NodeId node_id = subgraph->nodes[i];
        if (node_id >= graph->node_count) continue;
        
        const GraphNode* node = &graph->nodes[node_id];
        for (size_t j = 0; j < node->input_count; j++) {
            NodeId input_id = node->inputs[j];
            
            // Check if input is external to subgraph
            bool is_external = true;
            for (size_t k = 0; k < subgraph->count; k++) {
                if (subgraph->nodes[k] == input_id) {
                    is_external = false;
                    break;
                }
            }
            
            if (is_external && fused.input_count < 8) {
                fused.inputs[fused.input_count++] = input_id;
            }
        }
    }
    
    // Add fused node to graph
    if (graph->node_count >= graph->node_capacity) {
        return -1;  // Graph full
    }
    
    graph->nodes[graph->node_count++] = fused;
    
    return 0;
}

void subgraph_free(Subgraph* sg) {
    if (sg) {
        free(sg->nodes);
        sg->nodes = NULL;
        sg->count = 0;
        sg->capacity = 0;
    }
}

// --- Graph Serialization ---

uint64_t compute_graph_checksum(const BdiGraph* graph) {
    if (!graph) return 0;
    
    uint64_t checksum = 0;
    
    // Simple checksum: XOR of all node IDs and opcodes
    for (size_t i = 0; i < graph->node_count; i++) {
        checksum ^= graph->nodes[i].id;
        checksum ^= (uint64_t)graph->nodes[i].opcode << 32;
    }
    
    return checksum;
}

int graph_serialize(const BdiGraph* graph, const char* path) {
    if (!graph || !path) return -1;
    
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    
    // Write header
    GraphHeader header = {
        .magic = GRAPH_MAGIC,
        .version = GRAPH_VERSION,
        .node_count = graph->node_count,
        .edge_count = graph->edge_count,
        .checksum = compute_graph_checksum(graph)
    };
    
    if (fwrite(&header, sizeof(GraphHeader), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    
    // Write nodes
    if (fwrite(graph->nodes, sizeof(GraphNode), graph->node_count, f) != graph->node_count) {
        fclose(f);
        return -1;
    }
    
    // Write edges
    if (fwrite(graph->edges, sizeof(GraphEdge), graph->edge_count, f) != graph->edge_count) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return 0;
}

BdiGraph* graph_deserialize(const char* path) {
    if (!path) return NULL;
    
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    // Read header
    GraphHeader header;
    if (fread(&header, sizeof(GraphHeader), 1, f) != 1) {
        fclose(f);
        return NULL;
    }
    
    // Validate header
    if (header.magic != GRAPH_MAGIC || header.version != GRAPH_VERSION) {
        fclose(f);
        return NULL;
    }
    
    // Allocate graph
    BdiGraph* graph = malloc(sizeof(BdiGraph));
    if (!graph) {
        fclose(f);
        return NULL;
    }
    
    graph->node_count = header.node_count;
    graph->node_capacity = header.node_count;
    graph->edge_count = header.edge_count;
    graph->edge_capacity = header.edge_count;
    
    // Allocate and read nodes
    graph->nodes = malloc(sizeof(GraphNode) * graph->node_count);
    if (!graph->nodes || fread(graph->nodes, sizeof(GraphNode), graph->node_count, f) != graph->node_count) {
        free(graph->nodes);
        free(graph);
        fclose(f);
        return NULL;
    }
    
    // Allocate and read edges
    graph->edges = malloc(sizeof(GraphEdge) * graph->edge_count);
    if (!graph->edges || fread(graph->edges, sizeof(GraphEdge), graph->edge_count, f) != graph->edge_count) {
        free(graph->nodes);
        free(graph->edges);
        free(graph);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    
    // Verify checksum
    if (compute_graph_checksum(graph) != header.checksum) {
        free(graph->nodes);
        free(graph->edges);
        free(graph);
        return NULL;
    }
    
    return graph;
}
