
#include "FunctionInlining.hpp"

namespace bdi::optimizer {

bool FunctionInlining::run(BDIGraph& graph) {
    bool modified = false;
    
    // Find all function call nodes
    std::vector<NodeID> call_nodes;
    for (auto& [node_id, node_ptr] : graph) {
        if (node_ptr->operation == BDIOperationType::FUNC_CALL) {
            call_nodes.push_back(node_id);
        }
    }
    
    // Try to inline each call
    for (NodeID call_node : call_nodes) {
        if (shouldInline(call_node, graph)) {
            inlineFunction(call_node, graph);
            modified = true;
        }
    }
    
    if (modified) {
        markGraphModified();
    }
    
    return modified;
}

bool FunctionInlining::shouldInline(NodeID call_node, const BDIGraph& graph) {
    // Check depth limit
    if (current_depth_ >= policy_.max_inline_depth) {
        return false;
    }
    
    // Estimate function size
    uint32_t size = estimateFunctionSize(call_node, graph);
    
    // Always inline small functions
    if (policy_.inline_small_functions && 
        size <= policy_.small_function_threshold) {
        return true;
    }
    
    // Check size limit
    if (size > policy_.max_inline_size) {
        return false;
    }
    
    // Additional heuristics could be added here:
    // - Call frequency
    // - Function complexity
    // - Optimization opportunities after inlining
    
    return true;
}

void FunctionInlining::inlineFunction(NodeID call_node, BDIGraph& graph) {
    auto call_node_ref = graph.getNode(call_node);
    if (!call_node_ref) return;
    
    // In a real implementation:
    // 1. Copy function body nodes into caller
    // 2. Remap parameter references to arguments
    // 3. Remap return values to call result
    // 4. Update control flow
    // 5. Remove call node
    
    // Simplified implementation
    current_depth_++;
    
    // Mark as modified
    markGraphModified();
    
    current_depth_--;
}

uint32_t FunctionInlining::estimateFunctionSize(NodeID function_node, 
                                                const BDIGraph& graph) {
    // Simplified size estimation
    // In real implementation, would traverse function body and count instructions
    return 50; // Placeholder
}

void FunctionInlining::visitNode(BDINode& node, BDIGraph& graph) {
    // Node-level processing if needed
}

} // namespace bdi::optimizer
