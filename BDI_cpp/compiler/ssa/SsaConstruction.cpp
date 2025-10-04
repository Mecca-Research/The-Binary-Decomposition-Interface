
#include "SsaConstruction.hpp"
#include <algorithm>
#include <queue>
#include <sstream>

namespace bdi::compiler::ssa {

std::unique_ptr<SsaForm> SsaConstructor::convertToSsa(ControlFlowGraph& cfg) {
    auto ssa = std::make_unique<SsaForm>();
    
    // Phase 1: Compute dominance information
    auto dom_info = computeDominance(cfg);
    
    // Phase 2: Insert phi nodes
    insertPhiNodes(cfg, dom_info, *ssa);
    
    // Phase 3: Rename variables
    renameVariables(cfg, *ssa, dom_info);
    
    return ssa;
}

std::unordered_map<BasicBlockID, DominanceInfo> 
SsaConstructor::computeDominance(const ControlFlowGraph& cfg) {
    std::unordered_map<BasicBlockID, DominanceInfo> dom_info;
    
    // Initialize dominance info for all blocks
    for (const auto& [block_id, block] : cfg.getBlocks()) {
        dom_info[block_id] = DominanceInfo(block_id);
    }
    
    // Compute immediate dominators
    computeImmediateDominators(cfg, dom_info);
    
    // Compute dominance frontiers
    computeDominanceFrontiers(cfg, dom_info);
    
    return dom_info;
}

// Forward declaration
BasicBlockID findCommonDominator(
    BasicBlockID b1, BasicBlockID b2,
    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info);

void SsaConstructor::computeImmediateDominators(
    const ControlFlowGraph& cfg,
    std::unordered_map<BasicBlockID, DominanceInfo>& dom_info) {
    
    BasicBlockID entry = cfg.getEntryBlock();
    if (entry == 0) return;
    
    // Entry block dominates itself
    dom_info[entry].immediate_dominator = entry;
    
    // Initialize all other blocks to be dominated by all blocks
    std::unordered_set<BasicBlockID> all_blocks;
    for (const auto& [id, _] : cfg.getBlocks()) {
        all_blocks.insert(id);
    }
    
    // Iterative algorithm to compute dominators
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (const auto& [block_id, block] : cfg.getBlocks()) {
            if (block_id == entry) continue;
            
            // Find new immediate dominator
            BasicBlockID new_idom = 0;
            
            for (BasicBlockID pred : block->predecessors) {
                if (dom_info[pred].immediate_dominator != 0) {
                    if (new_idom == 0) {
                        new_idom = pred;
                    } else {
                        // Find common dominator
                        new_idom = findCommonDominator(new_idom, pred, dom_info);
                    }
                }
            }
            
            if (new_idom != 0 && dom_info[block_id].immediate_dominator != new_idom) {
                dom_info[block_id].immediate_dominator = new_idom;
                changed = true;
            }
        }
    }
    
    // Build dominated_blocks sets
    for (auto& [block_id, info] : dom_info) {
        if (block_id != entry && info.immediate_dominator != 0) {
            dom_info[info.immediate_dominator].dominated_blocks.insert(block_id);
        }
    }
}

void SsaConstructor::computeDominanceFrontiers(
    const ControlFlowGraph& cfg,
    std::unordered_map<BasicBlockID, DominanceInfo>& dom_info) {
    
    // For each block with multiple predecessors
    for (const auto& [block_id, block] : cfg.getBlocks()) {
        if (block->predecessors.size() >= 2) {
            // For each predecessor
            for (BasicBlockID pred : block->predecessors) {
                BasicBlockID runner = pred;
                
                // Walk up dominator tree until we reach block's idom
                while (runner != dom_info[block_id].immediate_dominator) {
                    dom_info[runner].dominance_frontier.insert(block_id);
                    runner = dom_info[runner].immediate_dominator;
                    if (runner == 0) break;
                }
            }
        }
    }
}

void SsaConstructor::insertPhiNodes(
    ControlFlowGraph& cfg,
    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info,
    SsaForm& ssa) {
    
    // Track which variables are defined in which blocks
    std::unordered_map<std::string, std::unordered_set<BasicBlockID>> var_defs;
    
    // Simplified: assume we have some variables to track
    // In a real implementation, this would come from analyzing the CFG
    std::vector<std::string> variables = {"x", "y", "z"}; // Example variables
    
    for (const std::string& var : variables) {
        // Collect all blocks where this variable is defined
        std::unordered_set<BasicBlockID> def_blocks;
        for (const auto& [block_id, block] : cfg.getBlocks()) {
            // In real implementation, check if var is defined in this block
            // For now, assume it's defined in some blocks
            if (block_id % 2 == 0) { // Simplified heuristic
                def_blocks.insert(block_id);
            }
        }
        
        // Insert phi nodes at dominance frontiers
        std::queue<BasicBlockID> worklist;
        for (BasicBlockID def_block : def_blocks) {
            worklist.push(def_block);
        }
        
        std::unordered_set<BasicBlockID> phi_inserted;
        
        while (!worklist.empty()) {
            BasicBlockID block = worklist.front();
            worklist.pop();
            
            // For each block in the dominance frontier
            auto it = dom_info.find(block);
            if (it != dom_info.end()) {
                for (BasicBlockID df_block : it->second.dominance_frontier) {
                    if (phi_inserted.find(df_block) == phi_inserted.end()) {
                        // Insert phi node
                        PhiNode phi;
                        phi.block = df_block;
                        phi.result = ssa.createVariable(var, 0, BDIType::INT32, df_block);
                        
                        // Add operands from predecessors
                        auto df_block_ptr = cfg.getBlock(df_block);
                        if (df_block_ptr) {
                            for (BasicBlockID pred : df_block_ptr->predecessors) {
                                // Create placeholder variable for now
                                SsaVariableID pred_var = ssa.createVariable(var, 0, BDIType::INT32, pred);
                                phi.addOperand(pred, pred_var);
                            }
                        }
                        
                        ssa.addPhiNode(phi);
                        phi_inserted.insert(df_block);
                        
                        // If this is a new definition site, add to worklist
                        if (def_blocks.find(df_block) == def_blocks.end()) {
                            worklist.push(df_block);
                            def_blocks.insert(df_block);
                        }
                    }
                }
            }
        }
    }
}

void SsaConstructor::renameVariables(
    ControlFlowGraph& cfg,
    SsaForm& ssa,
    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info) {
    
    // Stack of current versions for each variable
    std::unordered_map<std::string, std::stack<SsaVariableID>> var_stacks;
    
    // Start renaming from entry block
    BasicBlockID entry = cfg.getEntryBlock();
    if (entry != 0) {
        renameBlock(entry, cfg, ssa, dom_info, var_stacks);
    }
}

void SsaConstructor::renameBlock(
    BasicBlockID block_id,
    ControlFlowGraph& cfg,
    SsaForm& ssa,
    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info,
    std::unordered_map<std::string, std::stack<SsaVariableID>>& var_stacks) {
    
    auto block = cfg.getBlock(block_id);
    if (!block) return;
    
    // Process phi nodes in this block
    for (auto& phi : block->phi_nodes) {
        // The phi result gets a new version
        auto var = ssa.getVariable(phi.result);
        if (var) {
            var_stacks[var->base_name].push(phi.result);
        }
    }
    
    // Process instructions in this block (simplified)
    // In real implementation, would process each instruction
    
    // Update phi operands in successor blocks
    for (BasicBlockID succ : block->successors) {
        auto succ_block = cfg.getBlock(succ);
        if (succ_block) {
            for (auto& phi : succ_block->phi_nodes) {
                // Update the operand for this predecessor
                auto var = ssa.getVariable(phi.result);
                if (var && !var_stacks[var->base_name].empty()) {
                    phi.operands[block_id] = var_stacks[var->base_name].top();
                }
            }
        }
    }
    
    // Recursively process dominated blocks
    auto it = dom_info.find(block_id);
    if (it != dom_info.end()) {
        for (BasicBlockID dominated : it->second.dominated_blocks) {
            renameBlock(dominated, cfg, ssa, dom_info, var_stacks);
        }
    }
    
    // Pop variables defined in this block
    for (auto& phi : block->phi_nodes) {
        auto var = ssa.getVariable(phi.result);
        if (var && !var_stacks[var->base_name].empty()) {
            var_stacks[var->base_name].pop();
        }
    }
}

// Helper function to find common dominator
BasicBlockID findCommonDominator(
    BasicBlockID b1, BasicBlockID b2,
    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info) {
    
    std::unordered_set<BasicBlockID> b1_doms;
    BasicBlockID current = b1;
    
    // Collect all dominators of b1
    while (current != 0) {
        b1_doms.insert(current);
        auto it = dom_info.find(current);
        if (it == dom_info.end() || it->second.immediate_dominator == current) break;
        current = it->second.immediate_dominator;
    }
    
    // Find first common dominator in b2's dominator chain
    current = b2;
    while (current != 0) {
        if (b1_doms.find(current) != b1_doms.end()) {
            return current;
        }
        auto it = dom_info.find(current);
        if (it == dom_info.end() || it->second.immediate_dominator == current) break;
        current = it->second.immediate_dominator;
    }
    
    return 0;
}

// SsaUtils implementation
bool SsaUtils::validateSsa(const SsaForm& ssa, const ControlFlowGraph& cfg) {
    // Check that each variable is defined exactly once
    std::unordered_set<SsaVariableID> defined_vars;
    
    for (const auto& [id, var] : ssa.getVariables()) {
        if (defined_vars.find(id) != defined_vars.end()) {
            return false; // Variable defined multiple times
        }
        defined_vars.insert(id);
    }
    
    // Check phi nodes are valid
    for (const auto& phi : ssa.getPhiNodes()) {
        auto block = cfg.getBlock(phi.block);
        if (!block) return false;
        
        // Check that phi has operand for each predecessor
        if (phi.operands.size() != block->predecessors.size()) {
            return false;
        }
    }
    
    return true;
}

void SsaUtils::eliminateSsa(SsaForm& ssa, ControlFlowGraph& cfg) {
    // Convert phi nodes to copy operations
    // This is a simplified implementation
    for (const auto& phi : ssa.getPhiNodes()) {
        // In real implementation, insert copy operations at end of predecessor blocks
        // For now, just mark as processed
    }
}

std::string SsaUtils::printSsa(const SsaForm& ssa, const ControlFlowGraph& cfg) {
    std::ostringstream os;
    
    os << "SSA Form:\n";
    os << "Variables:\n";
    for (const auto& [id, var] : ssa.getVariables()) {
        os << "  " << var->getFullName() << " : " 
           << static_cast<int>(var->type) << "\n";
    }
    
    os << "\nPhi Nodes:\n";
    for (const auto& phi : ssa.getPhiNodes()) {
        auto result_var = ssa.getVariable(phi.result);
        if (result_var) {
            os << "  " << result_var->getFullName() << " = phi(";
            bool first = true;
            for (const auto& [block, var_id] : phi.operands) {
                if (!first) os << ", ";
                auto operand_var = ssa.getVariable(var_id);
                if (operand_var) {
                    os << "BB" << block << ": " << operand_var->getFullName();
                }
                first = false;
            }
            os << ")\n";
        }
    }
    
    return os.str();
}

} // namespace bdi::compiler::ssa
