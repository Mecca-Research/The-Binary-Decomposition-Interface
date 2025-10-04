
#include "InstructionSelection.hpp"
#include <sstream>

namespace bdi::compiler::backend {

std::string MachineInstruction::toString() const {
    std::ostringstream os;
    os << opcode;
    
    if (!operands.empty()) {
        os << " ";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) os << ", ";
            os << "r" << operands[i];
        }
    }
    
    if (!comment.empty()) {
        os << "  # " << comment;
    }
    
    return os.str();
}

InstructionSelector::InstructionSelector(const Architecture& arch)
    : arch_(arch) {
    initializePatterns();
}

void InstructionSelector::initializePatterns() {
    // Initialize patterns based on architecture
    if (arch_.getType() == ArchType::X86_64) {
        // x86-64 patterns
        patterns_.emplace_back("add_reg_reg", BDIOperationType::ARITH_ADD,
                              std::vector<std::string>{"add"}, 1);
        patterns_.emplace_back("sub_reg_reg", BDIOperationType::ARITH_SUB,
                              std::vector<std::string>{"sub"}, 1);
        patterns_.emplace_back("mul_reg_reg", BDIOperationType::ARITH_MUL,
                              std::vector<std::string>{"imul"}, 3);
        patterns_.emplace_back("div_reg_reg", BDIOperationType::ARITH_DIV,
                              std::vector<std::string>{"idiv"}, 10);
        patterns_.emplace_back("mov_reg_reg", BDIOperationType::META_NOP,
                              std::vector<std::string>{"mov"}, 1);
    } else if (arch_.getType() == ArchType::ARM64) {
        // ARM64 patterns
        patterns_.emplace_back("add_reg_reg", BDIOperationType::ARITH_ADD,
                              std::vector<std::string>{"add"}, 1);
        patterns_.emplace_back("sub_reg_reg", BDIOperationType::ARITH_SUB,
                              std::vector<std::string>{"sub"}, 1);
        patterns_.emplace_back("mul_reg_reg", BDIOperationType::ARITH_MUL,
                              std::vector<std::string>{"mul"}, 3);
        patterns_.emplace_back("sdiv_reg_reg", BDIOperationType::ARITH_DIV,
                              std::vector<std::string>{"sdiv"}, 10);
        patterns_.emplace_back("mov_reg_reg", BDIOperationType::META_NOP,
                              std::vector<std::string>{"mov"}, 1);
    }
}

void InstructionSelector::addPattern(const InstructionPattern& pattern) {
    patterns_.push_back(pattern);
}

const InstructionPattern* InstructionSelector::findBestPattern(
    NodeID node_id, const BDIGraph& graph) const {
    
    auto node_ref = graph.getNode(node_id);
    if (!node_ref) return nullptr;
    
    const BDINode& node = node_ref->get();
    
    // Find all matching patterns
    const InstructionPattern* best = nullptr;
    uint32_t best_cost = UINT32_MAX;
    
    for (const auto& pattern : patterns_) {
        if (pattern.bdi_op == node.operation) {
            if (pattern.cost < best_cost) {
                best = &pattern;
                best_cost = pattern.cost;
            }
        }
    }
    
    return best;
}

std::vector<MachineInstruction> InstructionSelector::selectInstructions(
    const SsaForm& ssa, const BDIGraph& graph) {
    
    std::vector<MachineInstruction> instructions;
    
    // Process each SSA variable definition
    for (const auto& [var_id, var] : ssa.getVariables()) {
        // Find the node that defines this variable
        // In real implementation, would have mapping from SSA var to graph node
        
        // For now, create placeholder instructions
        MachineInstruction instr;
        instr.opcode = "mov";
        instr.operands = {var_id % 16}; // Simplified register assignment
        instr.comment = var->getFullName();
        instructions.push_back(instr);
    }
    
    return instructions;
}

void InstructionSelector::selectForNode(
    NodeID node_id, const BDIGraph& graph,
    std::vector<MachineInstruction>& instructions) {
    
    const InstructionPattern* pattern = findBestPattern(node_id, graph);
    if (!pattern) return;
    
    auto node_ref = graph.getNode(node_id);
    if (!node_ref) return;
    
    const BDINode& node = node_ref->get();
    
    // Generate instructions based on pattern
    for (const std::string& op : pattern->machine_ops) {
        MachineInstruction instr(op);
        
        // Add operands based on node inputs
        for (const auto& input : node.data_inputs) {
            instr.operands.push_back(input.node_id % 16); // Simplified
        }
        
        instructions.push_back(instr);
    }
}

} // namespace bdi::compiler::backend
