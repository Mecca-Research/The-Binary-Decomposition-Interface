
#ifndef BDI_OPTIMIZER_PASSES_LOOPINVARIANTCODEMOTION_HPP
#define BDI_OPTIMIZER_PASSES_LOOPINVARIANTCODEMOTION_HPP

#include "optimizer/engine/OptimizationPassBase.hpp"
#include "compiler/ssa/SsaTypes.hpp"
#include <unordered_set>
#include <vector>

namespace bdi::optimizer {

using bdi::compiler::ssa::BasicBlockID;
using bdi::compiler::ssa::ControlFlowGraph;

/**
 * @brief Represents a loop in the control flow graph
 */
struct Loop {
    BasicBlockID header;                    ///< Loop header block
    std::unordered_set<BasicBlockID> blocks; ///< All blocks in the loop
    std::vector<BasicBlockID> exits;        ///< Exit blocks from the loop
    std::vector<BasicBlockID> backedges;    ///< Blocks with backedges to header
    
    Loop(BasicBlockID h = 0) : header(h) {}
    
    bool contains(BasicBlockID block) const {
        return blocks.find(block) != blocks.end();
    }
};

/**
 * @brief Loop Invariant Code Motion optimization pass
 * 
 * Moves computations that produce the same result on every loop iteration
 * out of the loop to improve performance.
 */
class LoopInvariantCodeMotion : public OptimizationPassBase {
public:
    LoopInvariantCodeMotion() 
        : OptimizationPassBase("LoopInvariantCodeMotion") {}
    
    bool run(BDIGraph& graph) override;
    
    /**
     * @brief Detect loops in the control flow graph
     */
    std::vector<Loop> detectLoops(const ControlFlowGraph& cfg);
    
    /**
     * @brief Check if an instruction is loop invariant
     */
    bool isLoopInvariant(NodeID node, const Loop& loop, 
                        const BDIGraph& graph,
                        std::unordered_set<NodeID>& invariant_nodes);
    
    /**
     * @brief Hoist loop invariant instructions to preheader
     */
    void hoistInvariants(Loop& loop, BDIGraph& graph);
    
private:
    void visitNode(BDINode& node, BDIGraph& graph) override;
};

} // namespace bdi::optimizer

#endif // BDI_OPTIMIZER_PASSES_LOOPINVARIANTCODEMOTION_HPP
