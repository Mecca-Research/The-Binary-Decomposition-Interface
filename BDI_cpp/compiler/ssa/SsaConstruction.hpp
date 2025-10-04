
#ifndef BDI_COMPILER_SSA_SSACONSTRUCTION_HPP
#define BDI_COMPILER_SSA_SSACONSTRUCTION_HPP

#include "compiler/ssa/SsaTypes.hpp"
#include "core/graph/BDIGraph.hpp"
#include <unordered_map>
#include <stack>

namespace bdi::compiler::ssa {

using bdi::core::graph::BDIGraph;
using bdi::core::graph::NodeID;

/**
 * @brief SSA Construction Engine
 * 
 * Implements the classic Cytron et al. algorithm for SSA construction:
 * 1. Compute dominance information
 * 2. Insert phi nodes at dominance frontiers
 * 3. Rename variables to ensure single assignment
 */
class SsaConstructor {
public:
    SsaConstructor() = default;
    
    /**
     * @brief Convert a control flow graph to SSA form
     * @param cfg The control flow graph to convert
     * @return SSA form representation
     */
    std::unique_ptr<SsaForm> convertToSsa(ControlFlowGraph& cfg);
    
    /**
     * @brief Compute dominance information for all blocks
     * @param cfg The control flow graph
     * @return Map of block ID to dominance information
     */
    std::unordered_map<BasicBlockID, DominanceInfo> 
    computeDominance(const ControlFlowGraph& cfg);
    
    /**
     * @brief Insert phi nodes at appropriate join points
     * @param cfg The control flow graph
     * @param dom_info Dominance information
     * @param ssa The SSA form being constructed
     */
    void insertPhiNodes(ControlFlowGraph& cfg,
                       const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info,
                       SsaForm& ssa);
    
private:
    /**
     * @brief Compute immediate dominators using iterative algorithm
     */
    void computeImmediateDominators(const ControlFlowGraph& cfg,
                                    std::unordered_map<BasicBlockID, DominanceInfo>& dom_info);
    
    /**
     * @brief Compute dominance frontiers from immediate dominators
     */
    void computeDominanceFrontiers(const ControlFlowGraph& cfg,
                                   std::unordered_map<BasicBlockID, DominanceInfo>& dom_info);
    
    /**
     * @brief Rename variables in SSA form (third phase of SSA construction)
     */
    void renameVariables(ControlFlowGraph& cfg,
                        SsaForm& ssa,
                        const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info);
    
    /**
     * @brief Helper for recursive variable renaming
     */
    void renameBlock(BasicBlockID block_id,
                    ControlFlowGraph& cfg,
                    SsaForm& ssa,
                    const std::unordered_map<BasicBlockID, DominanceInfo>& dom_info,
                    std::unordered_map<std::string, std::stack<SsaVariableID>>& var_stacks);
};

/**
 * @brief Utility functions for SSA manipulation
 */
class SsaUtils {
public:
    /**
     * @brief Check if SSA form is valid
     */
    static bool validateSsa(const SsaForm& ssa, const ControlFlowGraph& cfg);
    
    /**
     * @brief Convert SSA form back to non-SSA (for code generation)
     */
    static void eliminateSsa(SsaForm& ssa, ControlFlowGraph& cfg);
    
    /**
     * @brief Print SSA form for debugging
     */
    static std::string printSsa(const SsaForm& ssa, const ControlFlowGraph& cfg);
};

} // namespace bdi::compiler::ssa

#endif // BDI_COMPILER_SSA_SSACONSTRUCTION_HPP
