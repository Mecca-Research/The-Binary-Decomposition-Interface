
#ifndef BDI_COMPILER_BACKEND_CODEGEN_INSTRUCTIONSELECTION_HPP
#define BDI_COMPILER_BACKEND_CODEGEN_INSTRUCTIONSELECTION_HPP

#include "compiler/ssa/SsaTypes.hpp"
#include "compiler/backend/Architecture.hpp"
#include "core/graph/BDIGraph.hpp"
#include <vector>
#include <memory>
#include <string>

namespace bdi::compiler::backend {

using bdi::compiler::ssa::SsaForm;
using bdi::compiler::ssa::SsaVariableID;
using bdi::core::graph::BDIGraph;
using bdi::core::graph::NodeID;

/**
 * @brief Machine instruction representation
 */
struct MachineInstruction {
    std::string opcode;
    std::vector<uint32_t> operands;  // Register IDs or immediate values
    std::string comment;
    
    MachineInstruction(std::string op = "", std::string cmt = "")
        : opcode(std::move(op)), comment(std::move(cmt)) {}
    
    std::string toString() const;
};

/**
 * @brief Instruction pattern for pattern matching
 */
struct InstructionPattern {
    std::string pattern_name;
    BDIOperationType bdi_op;
    std::vector<std::string> machine_ops;  // Sequence of machine instructions
    uint32_t cost;  // Cost metric for this pattern
    
    InstructionPattern(std::string name = "", BDIOperationType op = BDIOperationType::META_NOP,
                      std::vector<std::string> ops = {}, uint32_t c = 1)
        : pattern_name(std::move(name)), bdi_op(op), 
          machine_ops(std::move(ops)), cost(c) {}
};

/**
 * @brief Instruction selector using pattern matching
 * 
 * Translates SSA form to target-specific machine instructions
 * using a pattern matching approach.
 */
class InstructionSelector {
public:
    explicit InstructionSelector(const Architecture& arch);
    
    /**
     * @brief Select instructions for SSA form
     * @param ssa The SSA form to translate
     * @param graph The original BDI graph
     * @return Vector of machine instructions
     */
    std::vector<MachineInstruction> selectInstructions(
        const SsaForm& ssa, const BDIGraph& graph);
    
    /**
     * @brief Add a pattern to the pattern database
     */
    void addPattern(const InstructionPattern& pattern);
    
    /**
     * @brief Find best matching pattern for a node
     */
    const InstructionPattern* findBestPattern(
        NodeID node_id, const BDIGraph& graph) const;
    
private:
    const Architecture& arch_;
    std::vector<InstructionPattern> patterns_;
    
    void initializePatterns();
    void selectForNode(NodeID node_id, const BDIGraph& graph,
                      std::vector<MachineInstruction>& instructions);
};

} // namespace bdi::compiler::backend

#endif // BDI_COMPILER_BACKEND_CODEGEN_INSTRUCTIONSELECTION_HPP
