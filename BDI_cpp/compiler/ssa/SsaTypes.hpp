
#ifndef BDI_COMPILER_SSA_SSATYPES_HPP
#define BDI_COMPILER_SSA_SSATYPES_HPP

#include "core/types/TypeSystem.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace bdi::compiler::ssa {

using bdi::core::types::BDIType;
using BasicBlockID = uint64_t;
using SsaVariableID = uint64_t;

/**
 * @brief Represents a versioned SSA variable
 * 
 * In SSA form, each variable is assigned exactly once. This structure
 * represents a specific version of a variable with its type information.
 */
struct SsaVariable {
    SsaVariableID id;           ///< Unique identifier for this SSA variable
    std::string base_name;      ///< Original variable name (e.g., "x")
    uint32_t version;           ///< Version number (e.g., x_0, x_1, x_2)
    BDIType type;               ///< Type of the variable
    BasicBlockID defining_block; ///< Block where this variable is defined
    
    SsaVariable(SsaVariableID id_ = 0, std::string name = "", 
                uint32_t ver = 0, BDIType t = BDIType::UNKNOWN,
                BasicBlockID block = 0)
        : id(id_), base_name(std::move(name)), version(ver), 
          type(t), defining_block(block) {}
    
    std::string getFullName() const {
        return base_name + "_" + std::to_string(version);
    }
};

/**
 * @brief Represents a phi node in SSA form
 * 
 * Phi nodes are inserted at control flow merge points to select
 * the appropriate variable version based on which predecessor block
 * was executed.
 */
struct PhiNode {
    SsaVariableID result;       ///< The SSA variable produced by this phi
    BasicBlockID block;         ///< Block containing this phi node
    
    /// Maps predecessor block ID to the SSA variable from that path
    std::unordered_map<BasicBlockID, SsaVariableID> operands;
    
    PhiNode(SsaVariableID res = 0, BasicBlockID blk = 0)
        : result(res), block(blk) {}
    
    void addOperand(BasicBlockID pred_block, SsaVariableID var) {
        operands[pred_block] = var;
    }
};

/**
 * @brief Represents a basic block in the control flow graph
 */
struct BasicBlock {
    BasicBlockID id;
    std::string label;
    std::vector<BasicBlockID> predecessors;
    std::vector<BasicBlockID> successors;
    std::vector<PhiNode> phi_nodes;
    
    // Instructions in this block (simplified representation)
    std::vector<uint64_t> instructions; // Could be NodeIDs from BDIGraph
    
    BasicBlock(BasicBlockID id_ = 0, std::string lbl = "")
        : id(id_), label(std::move(lbl)) {}
};

/**
 * @brief Dominance information for SSA construction
 */
struct DominanceInfo {
    BasicBlockID block_id;
    BasicBlockID immediate_dominator; ///< Immediate dominator (idom)
    std::unordered_set<BasicBlockID> dominance_frontier; ///< DF(block)
    std::unordered_set<BasicBlockID> dominated_blocks; ///< Blocks dominated by this
    
    DominanceInfo(BasicBlockID id = 0) 
        : block_id(id), immediate_dominator(0) {}
};

/**
 * @brief Control Flow Graph representation
 */
class ControlFlowGraph {
public:
    ControlFlowGraph() : next_block_id_(1), entry_block_(0) {}
    
    BasicBlockID createBlock(const std::string& label = "");
    BasicBlock* getBlock(BasicBlockID id);
    const BasicBlock* getBlock(BasicBlockID id) const;
    
    void addEdge(BasicBlockID from, BasicBlockID to);
    void setEntryBlock(BasicBlockID id) { entry_block_ = id; }
    BasicBlockID getEntryBlock() const { return entry_block_; }
    
    const std::unordered_map<BasicBlockID, std::unique_ptr<BasicBlock>>& getBlocks() const {
        return blocks_;
    }
    
    size_t getBlockCount() const { return blocks_.size(); }
    
private:
    std::unordered_map<BasicBlockID, std::unique_ptr<BasicBlock>> blocks_;
    BasicBlockID next_block_id_;
    BasicBlockID entry_block_;
};

/**
 * @brief SSA form representation
 */
class SsaForm {
public:
    SsaForm() : next_variable_id_(1) {}
    
    SsaVariableID createVariable(const std::string& base_name, 
                                  uint32_t version, BDIType type,
                                  BasicBlockID defining_block);
    
    SsaVariable* getVariable(SsaVariableID id);
    const SsaVariable* getVariable(SsaVariableID id) const;
    
    void addPhiNode(const PhiNode& phi);
    const std::vector<PhiNode>& getPhiNodes() const { return phi_nodes_; }
    std::vector<PhiNode>& getPhiNodes() { return phi_nodes_; }
    
    const std::unordered_map<SsaVariableID, std::unique_ptr<SsaVariable>>& 
    getVariables() const { return variables_; }
    
private:
    std::unordered_map<SsaVariableID, std::unique_ptr<SsaVariable>> variables_;
    std::vector<PhiNode> phi_nodes_;
    SsaVariableID next_variable_id_;
};

} // namespace bdi::compiler::ssa

#endif // BDI_COMPILER_SSA_SSATYPES_HPP
