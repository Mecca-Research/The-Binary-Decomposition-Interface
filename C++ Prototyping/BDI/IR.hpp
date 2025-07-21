#ifndef IR_IR_HPP 
#define IR_IR_HPP 
#include "ChimeraTypes.hpp" 
#include "DSLCoreTypes.hpp" // For Operator, Symbol 
#include <vector> 
#include <variant> 
#include <memory> 
#include <string> 
#include <cstdint>
#include <map> 
#include <optional> 
namespace ir::ir { 
// --- IR Node Structure --- 
// ... ChiIRNodeId, ChiIROpCode ... 
/** 
 * @brief Represents Intermediate Representation codes for Chimera operations. 
 * These are higher-level than BDI operations and closer to source constructs. 
 */ 
using IRNodeId = uint64_t; 
enum class IROpCode { 
    // Meta 
    ENTRY,        //!< Function or graph entry point.  
    EXIT,         //!< Function or graph exit point (no return value).       
    PARAM,        //!< Represents a function input parameter.      
    RETURN_VALUE, //!< Represents the value returned by a function (takes one input).  
    SCOPE_BEGIN, 
    SCOPE_END, 
    // Literals/Symbols 
    LOAD_CONST, LOAD_SYMBOL, 
    // Memory/State 
    ALLOC_MEM, LOAD_MEM, STORE_MEM, GET_STATE, SET_STATE, // For agents maybe 
    // Arithmetic/Logical/Bitwise (higher level than BDI initially) 
    BINARY_OP, UNARY_OP, 
    // Control Flow 
    JUMP, BRANCH_COND, 
    // Functions 
    CALL, DEFINE_FUNC, RETURN, 
    // DSL Specific (Placeholder) 
    DSL_BLOCK, 
    // Annotations / Metadata Nodes 
    ANNOTATION_MARKER 
}; 
// Represents a reference to an output value of another IR node 
/** 
 * @brief Represents a reference to a value produced by another ChiIR node's output. 
 * Includes the source node ID, the specific output port index, and the expected type. 
 */ 
struct IRValueRef { 
    IRNodeId node_id = 0; //!< ID of the node producing the value. 
    uint32_t output_index = 0; //!< Index of the output port on the source node. 
    std::shared_ptr<ChimeraType> type = nullptr; //!< Resolved ChimeraType of the value.  
    bool operator==(const IRValueRef&) const = default; 
}; 
/** 
 * @brief Represents a node in the Chimera Intermediate Representation graph. 
 * 
 * IR nodes represent operations, control flow constructs, or data references 
 * at a level higher than BDI but lower than the source AST. They carry type 
 * information and links to source annotations. 
 */ 
struct IRNode { 
    // ... id, opcode, label, inputs, output_type ... 
    IRNodeId id; //!< Unique identifier for this node. 
    IROpCode opcode; //!< The operation this node performs. 
    std::string label; //!< Optional debug label 
    // Inputs/Operands represented as references to previous node outputs 
    std::vector<IRValueRef> inputs; //!< Values consumed by this node. 
    // Type of the primary output value produced by this node 
    std::shared_ptr<ChimeraType> output_type = nullptr; //!< Type of the primary value produced.
    // Operation specific data 
    std::variant< 
        std::monostate, 
        BDIValueVariant,        // For LOAD_CONST (stores runtime value) 
        Symbol,                 // For LOAD_SYMBOL, DEFINE_FUNC name, PARAM 
        Operator,               // For BINARY_OP, UNARY_OP 
        IRNodeId,               // For JUMP, BRANCH targets, CALL target, RETURN source (value node)  
        std::pair<IRNodeId, IRNodeId> // BRANCH_COND targets (true_target_node, false_target_node)
        // Add structures for ALLOC_MEM (size, type), DSL_BLOCK (dsl_name, content) etc. 
    > operation_data; 
    // Control flow links (could be part of operation_data for jumps/branches) 
    std::vector<IRNodeId> control_successors; // Explicit successors needed? 
    // Source mapping and annotations 
    // SourceLocation source_loc; 
    std::vector<ParsedAnnotation> annotations; //!< Annotations from source code. 
    /** 
     * @brief Performs basic validation checks on the node structure. 
     * @return True if the node appears structurally valid, false otherwise. 
     */  
    bool validate() const; 
}; 
// --- IR Graph Structure --- 
// Represents a function or a basic block potentially 
class IRGraph { 
public: 
    IRGraph(std::string name = "ir_graph") : name_(std::move(name)) {} 
    IRNodeId addNode(IROpCode opcode, std::string label = ""); 
    bool addEdge(IRNodeId from, IRNodeId to); // Control flow edge 
    // Add methods for setting node inputs, data, type, annotations etc. 
    // ... ChiIRGraph ... (addEdge might populate successors) 
    bool ChiIRGraph::addEdge(ChiIRNodeId from, ChiIRNodeId to) { 
    auto from_node = getNode(from); 
    auto to_node = getNode(to); // Ensure target exists 
    if (from_node && to_node) { 
    from_node->control_successors.push_back(to); 
    // Optionally add predecessor link to 'to_node' if needed 
    return true; 
    }
    return false; 
}
    IRNode* getNode(IRNodeId id); 
    const IRNode* getNode(IRNodeId id) const; 
    // Entry/Exit points? 
    std::optional<IRNodeId> getEntryNode() const; 
    std::vector<IRNodeId> getExitNodes() const; // Might have multiple returns 
private: 
    std::string name_; 
    std::unordered_map<IRNodeId, std::unique_ptr<IRNode>> nodes_; 
    IRNodeId next_node_id_ = 1; 
    // Adjacency list or other structure for edges if needed explicitly 
    std::optional<IRNodeId> entry_node_id_; 
}; 
// --- AST to IR Converter --- 
class ASTToIR { 
public: 
    ASTToIR(TypeChecker& type_checker /* Needs type info */); 
// Convert a DSL expression (AST root) into a IR graph (representing e.g., a function body) 
std::unique_ptr<IRGraph> convertExpression(const DSLExpression* root_expr, TypeChecker::CheckContext& context); 
// Convert a function definition 
std::unique_ptr<IRGraph> convertFunction(/* Func Def AST */ TypeChecker::CheckContext& context); 
private: 
    TypeChecker& type_checker_; 
std::unique_ptr<IRGraph> current_graph_; 
    IRNodeId next_node_id_ = 1; // Local counter during conversion? 
// Recursive helper to convert AST nodes, returning ID of last node in sequence / result node 
IRNodeId convertNode(const DSLExpression* expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node); 
IRNodeId convertOperation(const DSLOperation& op, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node); 
IRNodeId convertDefinition(const DSLDefinition& def, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node); 
// ... other helpers ... 
}; 
// ... ASTToChiIR ... 
} // namespace ir::ir 
#endif // IR_IR_HPP 
