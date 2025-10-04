
#include "CommonSubexpressionElimination.hpp"
#include <sstream>

namespace bdi::optimizer {

bool CommonSubexpressionElimination::run(BDIGraph& graph) {
    expression_map_.clear();
    bool modified = false;
    
    // Visit all nodes and find common subexpressions
    visitGraph(graph);
    
    // Replace redundant computations
    for (const auto& [expr_key, node_ids] : expression_map_) {
        if (node_ids.size() > 1) {
            // Keep first occurrence, replace others
            NodeID canonical = node_ids[0];
            
            for (size_t i = 1; i < node_ids.size(); ++i) {
                replaceNodeUses(node_ids[i], canonical, graph);
                modified = true;
            }
        }
    }
    
    if (modified) {
        markGraphModified();
    }
    
    return modified;
}

void CommonSubexpressionElimination::visitNode(BDINode& node, BDIGraph& graph) {
    // Skip meta nodes and nodes with side effects
    if (node.operation == BDIOperationType::META_START ||
        node.operation == BDIOperationType::META_END ||
        node.operation == BDIOperationType::META_NOP) {
        return;
    }
    
    // Create expression key
    std::string expr_key = createExpressionKey(node);
    
    // Add to expression map
    expression_map_[expr_key].push_back(node.id);
}

std::string CommonSubexpressionElimination::createExpressionKey(const BDINode& node) {
    std::ostringstream key;
    
    // Include operation type
    key << static_cast<int>(node.operation) << ":";
    
    // Include input node IDs (sorted for commutativity)
    std::vector<NodeID> inputs;
    for (const auto& input : node.data_inputs) {
        inputs.push_back(input.node_id);
    }
    
    // Sort for commutative operations
    if (isCommutative(node.operation)) {
        std::sort(inputs.begin(), inputs.end());
    }
    
    for (NodeID input : inputs) {
        key << input << ",";
    }
    
    return key.str();
}

bool CommonSubexpressionElimination::isCommutative(BDIOperationType op) {
    return op == BDIOperationType::ARITH_ADD ||
           op == BDIOperationType::ARITH_MUL ||
           op == BDIOperationType::LOGIC_AND ||
           op == BDIOperationType::LOGIC_OR ||
           op == BDIOperationType::BITWISE_AND ||
           op == BDIOperationType::BITWISE_OR ||
           op == BDIOperationType::BITWISE_XOR;
}

void CommonSubexpressionElimination::replaceNodeUses(
    NodeID old_node, NodeID new_node, BDIGraph& graph) {
    
    // Find all nodes that use old_node and redirect to new_node
    for (auto& [node_id, node_ptr] : graph) {
        bool modified_node = false;
        
        // Update data inputs
        for (auto& input : node_ptr->data_inputs) {
            if (input.node_id == old_node) {
                input.node_id = new_node;
                modified_node = true;
            }
        }
        
        if (modified_node) {
            markGraphModified();
        }
    }
    
    // Remove old node
    graph.removeNode(old_node);
}

} // namespace bdi::optimizer
