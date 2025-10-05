
#include "graph_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

// Helper macros
#define OPTIMIZER_MAX_PASSES 32
#define OPTIMIZER_MAX_ITERATIONS 10
#define OPTIMIZER_CONVERGENCE_THRESHOLD 0.01

// Time measurement helper
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Core optimizer implementation
GraphOptimizer* graph_optimizer_create(void) {
    GraphOptimizerConfig config = graph_optimizer_default_config();
    return graph_optimizer_create_with_config(&config);
}

GraphOptimizer* graph_optimizer_create_with_config(const GraphOptimizerConfig* config) {
    GraphOptimizer* optimizer = (GraphOptimizer*)calloc(1, sizeof(GraphOptimizer));
    if (!optimizer) return NULL;
    
    if (config) {
        optimizer->config = *config;
    } else {
        optimizer->config = graph_optimizer_default_config();
    }
    
    optimizer->max_passes = OPTIMIZER_MAX_PASSES;
    optimizer->passes = (OptimizationPass*)calloc(optimizer->max_passes, sizeof(OptimizationPass));
    if (!optimizer->passes) {
        free(optimizer);
        return NULL;
    }
    
    // Add default passes based on configuration
    if (optimizer->config.enable_dead_code_elimination) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_DEAD_CODE_ELIMINATION,
                                graph_opt_dead_code_elimination, NULL);
    }
    
    if (optimizer->config.enable_constant_folding) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_CONSTANT_FOLDING,
                                graph_opt_constant_folding, NULL);
    }
    
    if (optimizer->config.enable_constant_propagation) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_CONSTANT_PROPAGATION,
                                graph_opt_constant_propagation, NULL);
    }
    
    if (optimizer->config.enable_cse) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_COMMON_SUBEXPRESSION_ELIMINATION,
                                graph_opt_common_subexpression_elimination, NULL);
    }
    
    if (optimizer->config.enable_algebraic_simplification) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_ALGEBRAIC_SIMPLIFICATION,
                                graph_opt_algebraic_simplification, NULL);
    }
    
    if (optimizer->config.enable_node_fusion) {
        graph_optimizer_add_pass(optimizer, OPT_PASS_NODE_FUSION,
                                graph_opt_node_fusion, NULL);
    }
    
    return optimizer;
}

void graph_optimizer_destroy(GraphOptimizer* optimizer) {
    if (!optimizer) return;
    
    free(optimizer->passes);
    free(optimizer->node_visited);
    free(optimizer->node_modified);
    free(optimizer->work_list);
    free(optimizer);
}

bool graph_optimizer_optimize(GraphOptimizer* optimizer, Graph* graph) {
    return graph_optimizer_optimize_with_stats(optimizer, graph, NULL);
}

bool graph_optimizer_optimize_with_stats(GraphOptimizer* optimizer, Graph* graph,
                                        GraphOptimizationStats* stats) {
    if (!optimizer || !graph) return false;
    
    uint64_t start_time = get_time_ns();
    
    // Reset statistics
    memset(&optimizer->stats, 0, sizeof(optimizer->stats));
    
    // Allocate working arrays
    if (!optimizer->node_visited || !optimizer->node_modified) {
        free(optimizer->node_visited);
        free(optimizer->node_modified);
        free(optimizer->work_list);
        
        optimizer->node_visited = (bool*)calloc(graph->node_count, sizeof(bool));
        optimizer->node_modified = (bool*)calloc(graph->node_count, sizeof(bool));
        optimizer->work_list = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
        
        if (!optimizer->node_visited || !optimizer->node_modified || !optimizer->work_list) {
            return false;
        }
    }
    
    bool changed = true;
    uint32_t iteration = 0;
    
    // Iterative optimization until convergence
    while (changed && iteration < optimizer->config.max_iterations) {
        changed = false;
        iteration++;
        
        // Run all enabled passes
        for (uint32_t i = 0; i < optimizer->pass_count; i++) {
            OptimizationPass* pass = &optimizer->passes[i];
            if (!pass->enabled) continue;
            
            uint64_t pass_start = get_time_ns();
            
            // Clear modification tracking
            memset(optimizer->node_modified, 0, graph->node_count * sizeof(bool));
            
            // Run the pass
            bool pass_changed = pass->function(graph, pass->pass_data, &optimizer->stats);
            
            // Update pass statistics
            pass->run_count++;
            pass->total_time_ns += get_time_ns() - pass_start;
            
            if (pass_changed) {
                changed = true;
                optimizer->stats.passes_run++;
            }
        }
        
        // Check convergence
        if (!changed) {
            optimizer->stats.converged = true;
            break;
        }
    }
    
    optimizer->stats.iterations = iteration;
    optimizer->stats.optimization_time_ns = get_time_ns() - start_time;
    
    // Mark graph as optimized
    graph->is_optimized = true;
    
    // Copy statistics if requested
    if (stats) {
        *stats = optimizer->stats;
    }
    
    return true;
}

// Pass management
bool graph_optimizer_add_pass(GraphOptimizer* optimizer, OptimizationPassType type,
                             OptimizationPassFunc function, void* pass_data) {
    if (!optimizer || !function || optimizer->pass_count >= optimizer->max_passes) {
        return false;
    }
    
    OptimizationPass* pass = &optimizer->passes[optimizer->pass_count++];
    pass->type = type;
    pass->function = function;
    pass->pass_data = pass_data;
    pass->enabled = true;
    pass->priority = optimizer->pass_count; // Default priority
    
    // Set pass name
    switch (type) {
        case OPT_PASS_DEAD_CODE_ELIMINATION:
            pass->name = "Dead Code Elimination";
            break;
        case OPT_PASS_CONSTANT_FOLDING:
            pass->name = "Constant Folding";
            break;
        case OPT_PASS_CONSTANT_PROPAGATION:
            pass->name = "Constant Propagation";
            break;
        case OPT_PASS_COMMON_SUBEXPRESSION_ELIMINATION:
            pass->name = "Common Subexpression Elimination";
            break;
        case OPT_PASS_ALGEBRAIC_SIMPLIFICATION:
            pass->name = "Algebraic Simplification";
            break;
        case OPT_PASS_NODE_FUSION:
            pass->name = "Node Fusion";
            break;
        default:
            pass->name = "Custom Pass";
            break;
    }
    
    return true;
}

bool graph_optimizer_remove_pass(GraphOptimizer* optimizer, OptimizationPassType type) {
    if (!optimizer) return false;
    
    for (uint32_t i = 0; i < optimizer->pass_count; i++) {
        if (optimizer->passes[i].type == type) {
            // Shift remaining passes
            for (uint32_t j = i; j < optimizer->pass_count - 1; j++) {
                optimizer->passes[j] = optimizer->passes[j + 1];
            }
            optimizer->pass_count--;
            return true;
        }
    }
    
    return false;
}

bool graph_optimizer_enable_pass(GraphOptimizer* optimizer, OptimizationPassType type, bool enable) {
    if (!optimizer) return false;
    
    for (uint32_t i = 0; i < optimizer->pass_count; i++) {
        if (optimizer->passes[i].type == type) {
            optimizer->passes[i].enabled = enable;
            return true;
        }
    }
    
    return false;
}

void graph_optimizer_clear_passes(GraphOptimizer* optimizer) {
    if (!optimizer) return;
    
    optimizer->pass_count = 0;
}

// Built-in optimization passes
bool graph_opt_dead_code_elimination(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    uint32_t eliminated = 0;
    
    // Mark all nodes as potentially dead
    for (uint32_t i = 0; i < graph->node_count; i++) {
        graph->nodes[i]->is_dead = true;
    }
    
    // Mark output nodes and their dependencies as live
    for (uint32_t i = 0; i < graph->output_count; i++) {
        GraphNode* output = graph->outputs[i];
        if (output) {
            graph_mark_node_live(graph, output);
        }
    }
    
    // Remove dead nodes
    for (uint32_t i = 0; i < graph->node_count; ) {
        GraphNode* node = graph->nodes[i];
        if (node->is_dead && node->type != GRAPH_NODE_INPUT && node->type != GRAPH_NODE_OUTPUT) {
            if (graph_remove_node(graph, node->id)) {
                eliminated++;
                changed = true;
                // Don't increment i since we removed a node
            } else {
                i++;
            }
        } else {
            node->is_dead = false; // Reset for next iteration
            i++;
        }
    }
    
    stats->nodes_eliminated += eliminated;
    return changed;
}

// Helper function for dead code elimination
static void graph_mark_node_live(Graph* graph, GraphNode* node) {
    if (!node || !node->is_dead) return;
    
    node->is_dead = false;
    
    // Mark all input nodes as live
    for (uint32_t i = 0; i < node->input_count; i++) {
        GraphEdge* edge = node->inputs[i];
        if (edge && edge->source) {
            graph_mark_node_live(graph, edge->source);
        }
    }
}

bool graph_opt_constant_folding(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    uint32_t folded = 0;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        
        // Skip if already constant or has no inputs
        if (node->is_constant || node->input_count == 0) continue;
        
        // Check if all inputs are constants
        bool all_inputs_constant = true;
        GraphValue* input_values = (GraphValue*)malloc(node->input_count * sizeof(GraphValue));
        if (!input_values) continue;
        
        for (uint32_t j = 0; j < node->input_count; j++) {
            GraphEdge* edge = node->inputs[j];
            if (!edge || !edge->source || !edge->source->is_constant) {
                all_inputs_constant = false;
                break;
            }
            input_values[j] = edge->source->constant_value;
        }
        
        if (all_inputs_constant) {
            // Try to evaluate the expression
            GraphValue result = graph_evaluate_constant_expression(node, input_values);
            
            if (graph_value_is_valid(&result)) {
                // Replace node with constant
                node->type = GRAPH_NODE_CONSTANT;
                node->constant_value = result;
                node->is_constant = true;
                
                // Remove input edges
                for (uint32_t j = 0; j < node->input_count; j++) {
                    if (node->inputs[j]) {
                        GraphEdge* edge = node->inputs[j];
                        graph_remove_edge(graph, edge->source->id, edge->source_output,
                                         node->id, j);
                    }
                }
                node->input_count = 0;
                
                folded++;
                changed = true;
            }
        }
        
        free(input_values);
    }
    
    stats->constants_folded += folded;
    return changed;
}

bool graph_opt_constant_propagation(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    
    // Find constant nodes and propagate their values
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        
        if (!node->is_constant) continue;
        
        // For each output edge, check if we can propagate the constant
        for (uint32_t j = 0; j < node->output_count; j++) {
            GraphEdge* edge = node->outputs[j];
            if (!edge || !edge->target) continue;
            
            GraphNode* target = edge->target;
            
            // Simple propagation: if target is an identity operation, replace it
            if (target->type == GRAPH_NODE_ADD && target->input_count == 2) {
                // Check if one input is zero
                GraphEdge* other_edge = NULL;
                for (uint32_t k = 0; k < target->input_count; k++) {
                    if (target->inputs[k] != edge) {
                        other_edge = target->inputs[k];
                        break;
                    }
                }
                
                if (other_edge && other_edge->source && other_edge->source->is_constant) {
                    GraphValue* const_val = &other_edge->source->constant_value;
                    if (const_val->type == GRAPH_TYPE_F64 && const_val->data.f64 == 0.0) {
                        // Replace target with the non-zero input
                        if (graph_replace_node_uses(graph, target->id, node->id)) {
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    
    return changed;
}

bool graph_opt_common_subexpression_elimination(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    uint32_t eliminated = 0;
    
    // Compare all pairs of nodes for equivalence
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node1 = graph->nodes[i];
        
        for (uint32_t j = i + 1; j < graph->node_count; j++) {
            GraphNode* node2 = graph->nodes[j];
            
            // Check if nodes are equivalent
            if (graph_nodes_are_equivalent(node1, node2)) {
                // Replace all uses of node2 with node1
                if (graph_replace_node_uses(graph, node2->id, node1->id)) {
                    // Remove node2
                    if (graph_remove_node(graph, node2->id)) {
                        eliminated++;
                        changed = true;
                        j--; // Adjust index since we removed a node
                    }
                }
            }
        }
    }
    
    stats->expressions_eliminated += eliminated;
    return changed;
}

bool graph_opt_algebraic_simplification(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    
    // Apply algebraic simplification patterns
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        
        // x + 0 = x
        if (node->type == GRAPH_NODE_ADD && node->input_count == 2) {
            GraphEdge* edge1 = node->inputs[0];
            GraphEdge* edge2 = node->inputs[1];
            
            if (edge1 && edge2 && edge1->source && edge2->source) {
                // Check if one operand is zero
                GraphNode* zero_node = NULL;
                GraphNode* other_node = NULL;
                
                if (edge1->source->is_constant && 
                    edge1->source->constant_value.type == GRAPH_TYPE_F64 &&
                    edge1->source->constant_value.data.f64 == 0.0) {
                    zero_node = edge1->source;
                    other_node = edge2->source;
                } else if (edge2->source->is_constant && 
                          edge2->source->constant_value.type == GRAPH_TYPE_F64 &&
                          edge2->source->constant_value.data.f64 == 0.0) {
                    zero_node = edge2->source;
                    other_node = edge1->source;
                }
                
                if (zero_node && other_node) {
                    // Replace node with other_node
                    if (graph_replace_node_uses(graph, node->id, other_node->id)) {
                        changed = true;
                    }
                }
            }
        }
        
        // x * 1 = x
        if (node->type == GRAPH_NODE_MUL && node->input_count == 2) {
            GraphEdge* edge1 = node->inputs[0];
            GraphEdge* edge2 = node->inputs[1];
            
            if (edge1 && edge2 && edge1->source && edge2->source) {
                // Check if one operand is one
                GraphNode* one_node = NULL;
                GraphNode* other_node = NULL;
                
                if (edge1->source->is_constant && 
                    edge1->source->constant_value.type == GRAPH_TYPE_F64 &&
                    edge1->source->constant_value.data.f64 == 1.0) {
                    one_node = edge1->source;
                    other_node = edge2->source;
                } else if (edge2->source->is_constant && 
                          edge2->source->constant_value.type == GRAPH_TYPE_F64 &&
                          edge2->source->constant_value.data.f64 == 1.0) {
                    one_node = edge2->source;
                    other_node = edge1->source;
                }
                
                if (one_node && other_node) {
                    // Replace node with other_node
                    if (graph_replace_node_uses(graph, node->id, other_node->id)) {
                        changed = true;
                    }
                }
            }
        }
        
        // x * 0 = 0
        if (node->type == GRAPH_NODE_MUL && node->input_count == 2) {
            GraphEdge* edge1 = node->inputs[0];
            GraphEdge* edge2 = node->inputs[1];
            
            if (edge1 && edge2 && edge1->source && edge2->source) {
                GraphNode* zero_node = NULL;
                
                if (edge1->source->is_constant && 
                    edge1->source->constant_value.type == GRAPH_TYPE_F64 &&
                    edge1->source->constant_value.data.f64 == 0.0) {
                    zero_node = edge1->source;
                } else if (edge2->source->is_constant && 
                          edge2->source->constant_value.type == GRAPH_TYPE_F64 &&
                          edge2->source->constant_value.data.f64 == 0.0) {
                    zero_node = edge2->source;
                }
                
                if (zero_node) {
                    // Replace node with zero_node
                    if (graph_replace_node_uses(graph, node->id, zero_node->id)) {
                        changed = true;
                    }
                }
            }
        }
    }
    
    return changed;
}

bool graph_opt_loop_invariant_code_motion(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    // TODO: Implement loop invariant code motion
    return false;
}

bool graph_opt_node_fusion(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    if (!graph || !stats) return false;
    
    bool changed = false;
    uint32_t fused = 0;
    
    // Look for fusable node patterns
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node1 = graph->nodes[i];
        
        // Look for arithmetic chains that can be fused
        if (node1->type == GRAPH_NODE_ADD || node1->type == GRAPH_NODE_MUL) {
            for (uint32_t j = 0; j < node1->output_count; j++) {
                GraphEdge* edge = node1->outputs[j];
                if (!edge || !edge->target) continue;
                
                GraphNode* node2 = edge->target;
                
                // Check if we can fuse these nodes
                if (graph_can_fuse_nodes(node1, node2)) {
                    uint32_t fused_id;
                    if (graph_merge_nodes(graph, node1->id, node2->id, &fused_id)) {
                        fused++;
                        changed = true;
                        break; // Node structure changed, restart
                    }
                }
            }
        }
    }
    
    stats->nodes_fused += fused;
    return changed;
}

bool graph_opt_memory_optimization(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    // TODO: Implement memory optimization
    return false;
}

bool graph_opt_control_flow_simplification(Graph* graph, void* pass_data, GraphOptimizationStats* stats) {
    // TODO: Implement control flow simplification
    return false;
}

// Configuration
GraphOptimizerConfig graph_optimizer_default_config(void) {
    GraphOptimizerConfig config = {
        .level = GRAPH_OPT_BASIC,
        .enable_dead_code_elimination = true,
        .enable_constant_folding = true,
        .enable_constant_propagation = true,
        .enable_cse = true,
        .enable_algebraic_simplification = true,
        .enable_loop_optimization = false,
        .enable_node_fusion = false,
        .enable_memory_optimization = false,
        .max_iterations = OPTIMIZER_MAX_ITERATIONS,
        .convergence_threshold = OPTIMIZER_CONVERGENCE_THRESHOLD
    };
    return config;
}

GraphOptimizerConfig graph_optimizer_config_for_level(GraphOptimizationLevel level) {
    GraphOptimizerConfig config = graph_optimizer_default_config();
    
    switch (level) {
        case GRAPH_OPT_NONE:
            config.enable_dead_code_elimination = false;
            config.enable_constant_folding = false;
            config.enable_constant_propagation = false;
            config.enable_cse = false;
            config.enable_algebraic_simplification = false;
            config.max_iterations = 1;
            break;
            
        case GRAPH_OPT_BASIC:
            // Use default config
            break;
            
        case GRAPH_OPT_AGGRESSIVE:
            config.enable_loop_optimization = true;
            config.enable_node_fusion = true;
            config.max_iterations = 20;
            break;
            
        case GRAPH_OPT_MAXIMUM:
            config.enable_loop_optimization = true;
            config.enable_node_fusion = true;
            config.enable_memory_optimization = true;
            config.max_iterations = 50;
            break;
    }
    
    config.level = level;
    return config;
}

void graph_optimizer_set_config(GraphOptimizer* optimizer, const GraphOptimizerConfig* config) {
    if (optimizer && config) {
        optimizer->config = *config;
    }
}

void graph_optimizer_get_config(const GraphOptimizer* optimizer, GraphOptimizerConfig* config) {
    if (optimizer && config) {
        *config = optimizer->config;
    }
}

// Statistics
void graph_optimizer_get_stats(const GraphOptimizer* optimizer, GraphOptimizationStats* stats) {
    if (optimizer && stats) {
        *stats = optimizer->stats;
    }
}

void graph_optimizer_reset_stats(GraphOptimizer* optimizer) {
    if (!optimizer) return;
    
    memset(&optimizer->stats, 0, sizeof(optimizer->stats));
    
    // Reset pass statistics
    for (uint32_t i = 0; i < optimizer->pass_count; i++) {
        optimizer->passes[i].run_count = 0;
        optimizer->passes[i].total_time_ns = 0;
        optimizer->passes[i].nodes_modified = 0;
    }
}

void graph_optimizer_print_stats(const GraphOptimizer* optimizer) {
    if (!optimizer) return;
    
    const GraphOptimizationStats* stats = &optimizer->stats;
    
    printf("Graph Optimization Statistics:\n");
    printf("  Passes Run: %u\n", stats->passes_run);
    printf("  Iterations: %u\n", stats->iterations);
    printf("  Converged: %s\n", stats->converged ? "Yes" : "No");
    printf("  Nodes Eliminated: %u\n", stats->nodes_eliminated);
    printf("  Constants Folded: %u\n", stats->constants_folded);
    printf("  Expressions Eliminated: %u\n", stats->expressions_eliminated);
    printf("  Nodes Fused: %u\n", stats->nodes_fused);
    printf("  Optimization Time: %.3f ms\n", 
           (double)stats->optimization_time_ns / 1000000.0);
    
    printf("\nPass Statistics:\n");
    for (uint32_t i = 0; i < optimizer->pass_count; i++) {
        const OptimizationPass* pass = &optimizer->passes[i];
        printf("  %s:\n", pass->name);
        printf("    Run Count: %u\n", pass->run_count);
        printf("    Total Time: %.3f ms\n", (double)pass->total_time_ns / 1000000.0);
        printf("    Enabled: %s\n", pass->enabled ? "Yes" : "No");
    }
}

// Analysis utilities
bool graph_node_is_dead(const Graph* graph, const GraphNode* node) {
    if (!graph || !node) return false;
    
    // Output nodes are never dead
    if (node->type == GRAPH_NODE_OUTPUT) return false;
    
    // Check if any other node uses this node
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge && edge->source == node) {
            return false; // Node is used
        }
    }
    
    return true; // Node is not used
}

bool graph_node_is_constant(const GraphNode* node) {
    return node && node->is_constant;
}

bool graph_nodes_are_equivalent(const GraphNode* node1, const GraphNode* node2) {
    if (!node1 || !node2) return false;
    
    // Must have same type
    if (node1->type != node2->type) return false;
    
    // Must have same number of inputs
    if (node1->input_count != node2->input_count) return false;
    
    // For constant nodes, values must be equal
    if (node1->type == GRAPH_NODE_CONSTANT) {
        return graph_value_equals(&node1->constant_value, &node2->constant_value);
    }
    
    // For other nodes, inputs must be equivalent
    for (uint32_t i = 0; i < node1->input_count; i++) {
        GraphEdge* edge1 = node1->inputs[i];
        GraphEdge* edge2 = node2->inputs[i];
        
        if (!edge1 || !edge2) {
            if (edge1 != edge2) return false;
            continue;
        }
        
        // Input sources must be the same node
        if (edge1->source != edge2->source) return false;
        if (edge1->source_output != edge2->source_output) return false;
    }
    
    return true;
}

bool graph_can_fuse_nodes(const GraphNode* node1, const GraphNode* node2) {
    if (!node1 || !node2) return false;
    
    // Simple fusion rules
    switch (node1->type) {
        case GRAPH_NODE_ADD:
            return (node2->type == GRAPH_NODE_MUL);
        case GRAPH_NODE_MUL:
            return (node2->type == GRAPH_NODE_ADD);
        default:
            return false;
    }
}

uint32_t graph_count_node_uses(const Graph* graph, uint32_t node_id) {
    if (!graph) return 0;
    
    uint32_t use_count = 0;
    
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge && edge->source && edge->source->id == node_id) {
            use_count++;
        }
    }
    
    return use_count;
}

GraphNode** graph_get_node_users(const Graph* graph, uint32_t node_id, uint32_t* user_count) {
    if (!graph || !user_count) return NULL;
    
    *user_count = 0;
    
    // Count users first
    uint32_t count = 0;
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge && edge->source && edge->source->id == node_id) {
            count++;
        }
    }
    
    if (count == 0) return NULL;
    
    // Allocate array
    GraphNode** users = (GraphNode**)malloc(count * sizeof(GraphNode*));
    if (!users) return NULL;
    
    // Fill array
    uint32_t index = 0;
    for (uint32_t i = 0; i < graph->edge_count && index < count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge && edge->source && edge->source->id == node_id) {
            users[index++] = edge->target;
        }
    }
    
    *user_count = count;
    return users;
}

// Transformation utilities
bool graph_replace_node_uses(Graph* graph, uint32_t old_node_id, uint32_t new_node_id) {
    if (!graph) return false;
    
    GraphNode* old_node = graph_get_node(graph, old_node_id);
    GraphNode* new_node = graph_get_node(graph, new_node_id);
    
    if (!old_node || !new_node) return false;
    
    bool changed = false;
    
    // Replace all uses of old_node with new_node
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (edge && edge->source == old_node) {
            edge->source = new_node;
            changed = true;
        }
    }
    
    return changed;
}

bool graph_merge_nodes(Graph* graph, uint32_t node1_id, uint32_t node2_id, uint32_t* result_id) {
    // TODO: Implement node merging
    return false;
}

bool graph_split_node(Graph* graph, uint32_t node_id, uint32_t* result_ids, uint32_t result_count) {
    // TODO: Implement node splitting
    return false;
}

bool graph_move_node(Graph* graph, uint32_t node_id, uint32_t new_position) {
    // TODO: Implement node moving
    return false;
}

// Constant evaluation
GraphValue graph_evaluate_constant_expression(const GraphNode* node, const GraphValue* inputs) {
    GraphValue result = {0};
    
    if (!node) return result;
    
    switch (node->type) {
        case GRAPH_NODE_ADD:
            if (inputs) {
                result = graph_value_add(&inputs[0], &inputs[1]);
            }
            break;
            
        case GRAPH_NODE_SUB:
            if (inputs) {
                result = graph_value_sub(&inputs[0], &inputs[1]);
            }
            break;
            
        case GRAPH_NODE_MUL:
            if (inputs) {
                result = graph_value_mul(&inputs[0], &inputs[1]);
            }
            break;
            
        case GRAPH_NODE_DIV:
            if (inputs) {
                result = graph_value_div(&inputs[0], &inputs[1]);
            }
            break;
            
        case GRAPH_NODE_NOT:
            if (inputs) {
                result = graph_value_not(&inputs[0]);
            }
            break;
            
        default:
            // Cannot evaluate
            break;
    }
    
    return result;
}

bool graph_is_constant_expression(const Graph* graph, const GraphNode* node) {
    if (!graph || !node) return false;
    
    // Constant nodes are constant expressions
    if (node->is_constant) return true;
    
    // Check if all inputs are constant expressions
    for (uint32_t i = 0; i < node->input_count; i++) {
        GraphEdge* edge = node->inputs[i];
        if (!edge || !edge->source) return false;
        
        if (!graph_is_constant_expression(graph, edge->source)) {
            return false;
        }
    }
    
    // All inputs are constant, so this is a constant expression
    return true;
}

GraphValue graph_fold_binary_operation(GraphNodeType op, const GraphValue* left, const GraphValue* right) {
    GraphValue result = {0};
    
    if (!left || !right) return result;
    
    switch (op) {
        case GRAPH_NODE_ADD:
            result = graph_value_add(left, right);
            break;
        case GRAPH_NODE_SUB:
            result = graph_value_sub(left, right);
            break;
        case GRAPH_NODE_MUL:
            result = graph_value_mul(left, right);
            break;
        case GRAPH_NODE_DIV:
            result = graph_value_div(left, right);
            break;
        case GRAPH_NODE_MOD:
            result = graph_value_mod(left, right);
            break;
        case GRAPH_NODE_AND:
            result = graph_value_and(left, right);
            break;
        case GRAPH_NODE_OR:
            result = graph_value_or(left, right);
            break;
        case GRAPH_NODE_XOR:
            result = graph_value_xor(left, right);
            break;
        case GRAPH_NODE_EQ:
            result = graph_value_eq(left, right);
            break;
        case GRAPH_NODE_NE:
            result = graph_value_ne(left, right);
            break;
        case GRAPH_NODE_LT:
            result = graph_value_lt(left, right);
            break;
        case GRAPH_NODE_LE:
            result = graph_value_le(left, right);
            break;
        case GRAPH_NODE_GT:
            result = graph_value_gt(left, right);
            break;
        case GRAPH_NODE_GE:
            result = graph_value_ge(left, right);
            break;
        default:
            break;
    }
    
    return result;
}

GraphValue graph_fold_unary_operation(GraphNodeType op, const GraphValue* operand) {
    GraphValue result = {0};
    
    if (!operand) return result;
    
    switch (op) {
        case GRAPH_NODE_NOT:
            result = graph_value_not(operand);
            break;
        default:
            break;
    }
    
    return result;
}

// Debugging and verification
bool graph_optimizer_verify_graph_integrity(const Graph* graph) {
    if (!graph) return false;
    
    // Check all edges have valid source and target nodes
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        GraphEdge* edge = graph->edges[i];
        if (!edge || !edge->source || !edge->target) {
            return false;
        }
        
        // Check that source and target nodes exist in the graph
        bool source_found = false, target_found = false;
        for (uint32_t j = 0; j < graph->node_count; j++) {
            if (graph->nodes[j] == edge->source) source_found = true;
            if (graph->nodes[j] == edge->target) target_found = true;
        }
        
        if (!source_found || !target_found) {
            return false;
        }
    }
    
    // Check for cycles (DAG property)
    if (graph_has_cycles(graph)) {
        return false;
    }
    
    return true;
}

void graph_optimizer_dump_pass_info(const GraphOptimizer* optimizer) {
    if (!optimizer) return;
    
    printf("Graph Optimizer Pass Information:\n");
    printf("  Total Passes: %u\n", optimizer->pass_count);
    
    for (uint32_t i = 0; i < optimizer->pass_count; i++) {
        const OptimizationPass* pass = &optimizer->passes[i];
        printf("  Pass %u: %s\n", i, pass->name);
        printf("    Type: %d\n", pass->type);
        printf("    Enabled: %s\n", pass->enabled ? "Yes" : "No");
        printf("    Priority: %u\n", pass->priority);
        printf("    Run Count: %u\n", pass->run_count);
        printf("    Total Time: %.3f ms\n", (double)pass->total_time_ns / 1000000.0);
    }
}

char* graph_optimizer_generate_optimization_report(const GraphOptimizer* optimizer,
                                                  const GraphOptimizationStats* stats) {
    if (!optimizer || !stats) return NULL;
    
    size_t buffer_size = 2048;
    char* report = (char*)malloc(buffer_size);
    if (!report) return NULL;
    
    int pos = snprintf(report, buffer_size,
        "Graph Optimization Report\n"
        "========================\n"
        "Optimization Level: %d\n"
        "Passes Run: %u\n"
        "Iterations: %u\n"
        "Converged: %s\n"
        "Total Time: %.3f ms\n\n"
        "Transformations Applied:\n"
        "  Nodes Eliminated: %u\n"
        "  Constants Folded: %u\n"
        "  Expressions Eliminated: %u\n"
        "  Nodes Fused: %u\n\n",
        optimizer->config.level,
        stats->passes_run,
        stats->iterations,
        stats->converged ? "Yes" : "No",
        (double)stats->optimization_time_ns / 1000000.0,
        stats->nodes_eliminated,
        stats->constants_folded,
        stats->expressions_eliminated,
        stats->nodes_fused);
    
    // Add pass details
    pos += snprintf(report + pos, buffer_size - pos, "Pass Details:\n");
    for (uint32_t i = 0; i < optimizer->pass_count && pos < buffer_size - 100; i++) {
        const OptimizationPass* pass = &optimizer->passes[i];
        pos += snprintf(report + pos, buffer_size - pos,
            "  %s: %u runs, %.3f ms\n",
            pass->name, pass->run_count,
            (double)pass->total_time_ns / 1000000.0);
    }
    
    return report;
}

