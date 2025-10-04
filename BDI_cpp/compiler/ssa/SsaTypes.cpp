
#include "SsaTypes.hpp"

namespace bdi::compiler::ssa {

// ControlFlowGraph implementation
BasicBlockID ControlFlowGraph::createBlock(const std::string& label) {
    BasicBlockID id = next_block_id_++;
    auto block = std::make_unique<BasicBlock>(id, label);
    blocks_[id] = std::move(block);
    return id;
}

BasicBlock* ControlFlowGraph::getBlock(BasicBlockID id) {
    auto it = blocks_.find(id);
    return (it != blocks_.end()) ? it->second.get() : nullptr;
}

const BasicBlock* ControlFlowGraph::getBlock(BasicBlockID id) const {
    auto it = blocks_.find(id);
    return (it != blocks_.end()) ? it->second.get() : nullptr;
}

void ControlFlowGraph::addEdge(BasicBlockID from, BasicBlockID to) {
    auto from_block = getBlock(from);
    auto to_block = getBlock(to);
    
    if (from_block && to_block) {
        from_block->successors.push_back(to);
        to_block->predecessors.push_back(from);
    }
}

// SsaForm implementation
SsaVariableID SsaForm::createVariable(const std::string& base_name,
                                      uint32_t version, BDIType type,
                                      BasicBlockID defining_block) {
    SsaVariableID id = next_variable_id_++;
    auto var = std::make_unique<SsaVariable>(id, base_name, version, type, defining_block);
    variables_[id] = std::move(var);
    return id;
}

SsaVariable* SsaForm::getVariable(SsaVariableID id) {
    auto it = variables_.find(id);
    return (it != variables_.end()) ? it->second.get() : nullptr;
}

const SsaVariable* SsaForm::getVariable(SsaVariableID id) const {
    auto it = variables_.find(id);
    return (it != variables_.end()) ? it->second.get() : nullptr;
}

void SsaForm::addPhiNode(const PhiNode& phi) {
    phi_nodes_.push_back(phi);
}

} // namespace bdi::compiler::ssa
