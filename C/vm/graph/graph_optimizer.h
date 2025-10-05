
#ifndef BDI_GRAPH_OPTIMIZER_H
#define BDI_GRAPH_OPTIMIZER_H

#include "graph.h"

// Forward declarations
typedef struct GraphOptimizer GraphOptimizer;
typedef struct OptimizationPass OptimizationPass;

// Optimization levels
typedef enum {
    GRAPH_OPT_NONE,
    GRAPH_OPT_BASIC,
    GRAPH_OPT_AGGRESSIVE,
    GRAPH_OPT_MAXIMUM
} GraphOptimizationLevel;

// Optimization pass types
typedef enum {
    OPT_PASS_DEAD_CODE_ELIMINATION,
    OPT_PASS_CONSTANT_FOLDING,
    OPT_PASS_CONSTANT_PROPAGATION,
    OPT_PASS_COMMON_SUBEXPRESSION_ELIMINATION,
    OPT_PASS_ALGEBRAIC_SIMPLIFICATION,
    OPT_PASS_LOOP_INVARIANT_CODE_MOTION,
    OPT_PASS_NODE_FUSION,
    OPT_PASS_MEMORY_OPTIMIZATION,
    OPT_PASS_CONTROL_FLOW_SIMPLIFICATION,
    OPT_PASS_CUSTOM
} OptimizationPassType;

// Optimization configuration
typedef struct {
    GraphOptimizationLevel level;
    bool enable_dead_code_elimination;
    bool enable_constant_folding;
    bool enable_constant_propagation;
    bool enable_cse;
    bool enable_algebraic_simplification;
    bool enable_loop_optimization;
    bool enable_node_fusion;
    bool enable_memory_optimization;
    uint32_t max_iterations;
    double convergence_threshold;
} GraphOptimizerConfig;

// Optimization statistics
typedef struct {
    uint32_t passes_run;
    uint32_t nodes_eliminated;
    uint32_t constants_folded;
    uint32_t expressions_eliminated;
    uint32_t nodes_fused;
    uint64_t optimization_time_ns;
    bool converged;
    uint32_t iterations;
} GraphOptimizationStats;

// Optimization pass function signature
typedef bool (*OptimizationPassFunc)(Graph* graph, void* pass_data, GraphOptimizationStats* stats);

// Optimization pass structure
struct OptimizationPass {
    OptimizationPassType type;
    const char* name;
    OptimizationPassFunc function;
    void* pass_data;
    bool enabled;
    uint32_t priority;
    
    // Statistics
    uint32_t run_count;
    uint64_t total_time_ns;
    uint32_t nodes_modified;
};

// Graph optimizer
struct GraphOptimizer {
    GraphOptimizerConfig config;
    
    // Optimization passes
    OptimizationPass* passes;
    uint32_t pass_count;
    uint32_t max_passes;
    
    // Statistics
    GraphOptimizationStats stats;
    
    // Working data
    bool* node_visited;
    bool* node_modified;
    uint32_t* work_list;
    uint32_t work_list_size;
};

// Core optimizer API
GraphOptimizer* graph_optimizer_create(void);
GraphOptimizer* graph_optimizer_create_with_config(const GraphOptimizerConfig* config);
void graph_optimizer_destroy(GraphOptimizer* optimizer);

// Optimization execution
bool graph_optimizer_optimize(GraphOptimizer* optimizer, Graph* graph);
bool graph_optimizer_optimize_with_stats(GraphOptimizer* optimizer, Graph* graph,
                                        GraphOptimizationStats* stats);

// Pass management
bool graph_optimizer_add_pass(GraphOptimizer* optimizer, OptimizationPassType type,
                             OptimizationPassFunc function, void* pass_data);
bool graph_optimizer_remove_pass(GraphOptimizer* optimizer, OptimizationPassType type);
bool graph_optimizer_enable_pass(GraphOptimizer* optimizer, OptimizationPassType type, bool enable);
void graph_optimizer_clear_passes(GraphOptimizer* optimizer);

// Built-in optimization passes
bool graph_opt_dead_code_elimination(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_constant_folding(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_constant_propagation(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_common_subexpression_elimination(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_algebraic_simplification(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_loop_invariant_code_motion(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_node_fusion(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_memory_optimization(Graph* graph, void* pass_data, GraphOptimizationStats* stats);
bool graph_opt_control_flow_simplification(Graph* graph, void* pass_data, GraphOptimizationStats* stats);

// Configuration
void graph_optimizer_set_config(GraphOptimizer* optimizer, const GraphOptimizerConfig* config);
void graph_optimizer_get_config(const GraphOptimizer* optimizer, GraphOptimizerConfig* config);
GraphOptimizerConfig graph_optimizer_default_config(void);
GraphOptimizerConfig graph_optimizer_config_for_level(GraphOptimizationLevel level);

// Statistics
void graph_optimizer_get_stats(const GraphOptimizer* optimizer, GraphOptimizationStats* stats);
void graph_optimizer_reset_stats(GraphOptimizer* optimizer);
void graph_optimizer_print_stats(const GraphOptimizer* optimizer);

// Analysis utilities
bool graph_node_is_dead(const Graph* graph, const GraphNode* node);
bool graph_node_is_constant(const GraphNode* node);
bool graph_nodes_are_equivalent(const GraphNode* node1, const GraphNode* node2);
bool graph_can_fuse_nodes(const GraphNode* node1, const GraphNode* node2);
uint32_t graph_count_node_uses(const Graph* graph, uint32_t node_id);
GraphNode** graph_get_node_users(const Graph* graph, uint32_t node_id, uint32_t* user_count);

// Transformation utilities
bool graph_replace_node_uses(Graph* graph, uint32_t old_node_id, uint32_t new_node_id);
bool graph_merge_nodes(Graph* graph, uint32_t node1_id, uint32_t node2_id, uint32_t* result_id);
bool graph_split_node(Graph* graph, uint32_t node_id, uint32_t* result_ids, uint32_t result_count);
bool graph_move_node(Graph* graph, uint32_t node_id, uint32_t new_position);

// Constant evaluation
GraphValue graph_evaluate_constant_expression(const GraphNode* node, const GraphValue* inputs);
bool graph_is_constant_expression(const Graph* graph, const GraphNode* node);
GraphValue graph_fold_binary_operation(GraphNodeType op, const GraphValue* left, const GraphValue* right);
GraphValue graph_fold_unary_operation(GraphNodeType op, const GraphValue* operand);

// Pattern matching
typedef struct {
    GraphNodeType* pattern;
    uint32_t pattern_length;
    GraphNodeType replacement;
    bool (*matcher)(const GraphNode** nodes, uint32_t count);
    GraphNode* (*replacer)(Graph* graph, const GraphNode** nodes, uint32_t count);
} GraphOptimizationPattern;

bool graph_match_pattern(const Graph* graph, const GraphNode* root, 
                        const GraphOptimizationPattern* pattern);
bool graph_apply_pattern(Graph* graph, GraphNode* root, 
                        const GraphOptimizationPattern* pattern);

// Algebraic simplification patterns
extern const GraphOptimizationPattern ALGEBRAIC_PATTERNS[];
extern const uint32_t ALGEBRAIC_PATTERN_COUNT;

// Loop analysis
typedef struct {
    GraphNode** header_nodes;
    GraphNode** body_nodes;
    GraphNode** exit_nodes;
    uint32_t header_count;
    uint32_t body_count;
    uint32_t exit_count;
    bool is_natural_loop;
    uint32_t nesting_level;
} GraphLoop;

GraphLoop* graph_find_loops(const Graph* graph, uint32_t* loop_count);
void graph_loop_destroy(GraphLoop* loop);
bool graph_is_loop_invariant(const GraphLoop* loop, const GraphNode* node);
GraphNode** graph_find_loop_invariant_nodes(const GraphLoop* loop, uint32_t* count);

// Memory analysis
typedef struct {
    GraphNode** alloc_nodes;
    GraphNode** load_nodes;
    GraphNode** store_nodes;
    uint32_t alloc_count;
    uint32_t load_count;
    uint32_t store_count;
    bool has_aliasing;
    bool has_side_effects;
} GraphMemoryInfo;

GraphMemoryInfo* graph_analyze_memory_usage(const Graph* graph);
void graph_memory_info_destroy(GraphMemoryInfo* info);
bool graph_memory_operations_alias(const GraphNode* op1, const GraphNode* op2);
bool graph_can_reorder_memory_operations(const GraphNode* op1, const GraphNode* op2);

// Profiling-guided optimization
typedef struct {
    uint32_t node_id;
    uint64_t execution_count;
    uint64_t execution_time_ns;
    double hotness_score;
} GraphNodeProfile;

typedef struct {
    GraphNodeProfile* profiles;
    uint32_t profile_count;
    uint64_t total_execution_time_ns;
} GraphExecutionProfile;

bool graph_optimizer_apply_profile_guided_optimization(GraphOptimizer* optimizer, 
                                                      Graph* graph,
                                                      const GraphExecutionProfile* profile);

// Debugging and visualization
void graph_optimizer_dump_pass_info(const GraphOptimizer* optimizer);
char* graph_optimizer_generate_optimization_report(const GraphOptimizer* optimizer,
                                                  const GraphOptimizationStats* stats);
bool graph_optimizer_verify_graph_integrity(const Graph* graph);

// Custom optimization pass helpers
typedef struct {
    const char* name;
    OptimizationPassFunc function;
    void* data;
    uint32_t priority;
} CustomOptimizationPass;

bool graph_optimizer_register_custom_pass(GraphOptimizer* optimizer,
                                         const CustomOptimizationPass* pass);

#endif // BDI_GRAPH_OPTIMIZER_H

