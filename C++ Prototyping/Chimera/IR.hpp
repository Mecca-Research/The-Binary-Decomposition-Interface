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
using IRNodeId = uint64_t; 
enum class IROpCode { 
    // Meta 
    ENTRY, EXIT, PARAM, RETURN_VALUE, SCOPE_BEGIN, SCOPE_END, 
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
struct IRValueRef { 
    IRNodeId node_id = 0; 
    uint32_t output_index = 0; // Typically 0 for single-output nodes 
    std::shared_ptr<ChimeraType> type = nullptr; // Type of the value being referenced 
    bool operator==(const IRValueRef&) const = default; 
}; 
struct IRNode { 
    IRNodeId id; 
    IROpCode opcode; 
    std::string label; // Optional debug label 
    // Inputs/Operands represented as references to previous node outputs 
    std::vector<IRValueRef> inputs; 
    // Type of the primary output value produced by this node 
    std::shared_ptr<ChimeraType> output_type = nullptr;
    // Operation specific data 
    std::variant< 
        std::monostate, 
        BDIValueVariant,        // For LOAD_CONST (stores runtime value) 
        Symbol,                 // For LOAD_SYMBOL, DEFINE_FUNC name 
        Operator,               // For BINARY_OP, UNARY_OP 
        IRNodeId,               // For JUMP, BRANCH targets, CALL target 
        std::pair<IRNodeId, IRNodeId> // For BRANCH true/false targets 
        // Add structures for ALLOC_MEM (size, type), DSL_BLOCK (dsl_name, content) etc. 
    > operation_data; 
    // Control flow links (could be part of operation_data for jumps/branches) 
    std::vector<IRNodeId> control_successors; // Explicit successors needed? 
    // Source mapping and annotations 
    // SourceLocation source_loc; 
    std::vector<ParsedAnnotation> annotations; // Annotations parsed earlier 
    // Basic validation 
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
} // namespace ir::ir 
#endif // IR_IR_HPP 
