
#include "LoopInvariantCodeMotion.hpp"
#include <queue>
#include <algorithm>

namespace bdi::optimizer {

bool LoopInvariantCodeMotion::run(BDIGraph& graph) {
    // Build CFG from graph (simplified)
    ControlFlowGraph cfg;
    
    // Detect loops
    auto loops = detectLoops(cfg);
    
    // Process each loop
    bool modified = false;
    for (auto& loop : loops) {
        hoistInvariants(loop, graph);
        if (wasGraphModified()) {
            modified = true;
        }
    }
    
    return modified;
}

std::vector<Loop> LoopInvariantCodeMotion::detectLoops(const ControlFlowGraph& cfg) {
    std::vector<Loop> loops;
    
    // Use depth-first search to find back edges
    std::unordered_set<BasicBlockID> visited;
    std::unordered_set<BasicBlockID> in_stack;
    
    std::function<void(BasicBlockID)> dfs = [&](BasicBlockID block_id) {
        visited.insert(block_id);
        in_stack.insert(block_id);
        
        auto block = cfg.getBlock(block_id);
        if (block) {
            for (BasicBlockID succ : block->successors) {
                if (in_stack.find(succ) != in_stack.end()) {
                    // Found a back edge: block_id -> succ
                    // succ is a loop header
                    Loop loop(succ);
                    loop.backedges.push_back(block_id);
                    
                    // Find all blocks in the loop using worklist algorithm
                    std::queue<BasicBlockID> worklist;
                    worklist.push(block_id);
                    loop.blocks.insert(succ);
                    loop.blocks.insert(block_id);
                    
                    while (!worklist.empty()) {
                        BasicBlockID current = worklist.front();
                        worklist.pop();
                        
                        auto current_block = cfg.getBlock(current);
                        if (current_block) {
                            for (BasicBlockID pred : current_block->predecessors) {
                                if (loop.blocks.find(pred) == loop.blocks.end()) {
                                    loop.blocks.insert(pred);
                                    worklist.push(pred);
                                }
                            }
                        }
                    }
                    
                    loops.push_back(loop);
                } else if (visited.find(succ) == visited.end()) {
                    dfs(succ);
                }
            }
        }
        
        in_stack.erase(block_id);
    };
    
    // Start DFS from entry block
    BasicBlockID entry = cfg.getEntryBlock();
    if (entry != 0) {
        dfs(entry);
    }
    
    return loops;
}

bool LoopInvariantCodeMotion::isLoopInvariant(
    NodeID node_id, const Loop& loop, const BDIGraph& graph,
    std::unordered_set<NodeID>& invariant_nodes) {
    
    // Already determined to be invariant
    if (invariant_nodes.find(node_id) != invariant_nodes.end()) {
        return true;
    }
    
    auto node_ref = graph.getNode(node_id);
    if (!node_ref) return false;
    
    const BDINode& node = node_ref->get();
    
    // Constants are always invariant
    if (node.operation == BDIOperationType::META_NOP && 
        !node.payload.isEmpty()) {
        return true;
    }
    
    // Check if all operands are loop invariant
    for (const auto& input : node.data_inputs) {
        // If operand is defined outside loop, it's invariant
        // If operand is defined inside loop but is itself invariant, continue
        // Otherwise, not invariant
        
        if (!isLoopInvariant(input.node_id, loop, graph, invariant_nodes)) {
            return false;
        }
    }
    
    // All operands are invariant, so this node is invariant
    invariant_nodes.insert(node_id);
    return true;
}

void LoopInvariantCodeMotion::hoistInvariants(Loop& loop, BDIGraph& graph) {
    std::unordered_set<NodeID> invariant_nodes;
    std::vector<NodeID> to_hoist;
    
    // Find all loop invariant instructions
    for (BasicBlockID block_id : loop.blocks) {
        // In real implementation, iterate through instructions in block
        // For now, simplified
    }
    
    // Hoist invariant instructions to loop preheader
    for (NodeID node_id : to_hoist) {
        // Move node to preheader
        // In real implementation, would modify control flow
        markGraphModified();
    }
}

void LoopInvariantCodeMotion::visitNode(BDINode& node, BDIGraph& graph) {
    // Node-level processing if needed
}

} // namespace bdi::optimizer
