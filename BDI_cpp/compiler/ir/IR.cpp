#include "IR.hpp"
#include <stdexcept> // For errors 
namespace chimera::ir { 
// ---IRNode --- 
bool IRNode::validate() const { 
// Add basic validation: e.g., does node ID exist? Are opcodes valid? 
// Do inputs have types? Does output type match opcode? 
return true; // Placeholder 
} 
// --- IRGraph --- 
IRNodeId IRGraph::addNode(IROpCode opcode, std::string label) { 
    IRNodeId id = next_node_id_++; 
auto node = std::make_unique<IRNode>(); 
    node->id = id; 
    node->opcode = opcode; 
    node->label = std::move(label); 
    nodes_[id] = std::move(node); 
// Set entry node if first node added? 
if (!entry_node_id_.has_value()) { 
        entry_node_id_ = id; 
    }
 return id; 
} 
bool IRGraph::addEdge(IRNodeId from, IRNodeId to) { 
// Add explicit edge representation if needed, or just manage via node successors 
auto from_node = getNode(from); 
auto to_node = getNode(to); 
if (from_node && to_node) { 
// Example: Add to successor list 
// from_node->control_successors.push_back(to); 
return true; 
    }
 return false;
} 
IRNode* IRGraph::getNode(IRNodeId id) { 
     auto it = nodes_.find(id); 
     return (it != nodes_.end()) ? it->second.get() : nullptr; 
} 
const IRNode* IRGraph::getNode(IRNodeId id) const { 
      auto it = nodes_.find(id); 
     return (it != nodes_.end()) ? it->second.get() : nullptr; 
} 
std::optional<IRNodeId> IRGraph::getEntryNode() const { 
    return entry_node_id_; 
} 
std::vector<IRNodeId> IRGraph::getExitNodes() const { 
    std::vector<IRNodeId> exits;
     for (const auto& pair : nodes_) { 
         if (pair.second->opcode == IROpCode::EXIT || pair.second->opcode == IROpCode::RETURN) { 
             exits.push_back(pair.first); 
         }
     } 
     return exits; 
} 
// --- ASTToIR --- 
ASTToIR::ASTToIR(TypeChecker& type_checker) : type_checker_(type_checker) {} 
std::unique_ptr<IRGraph> ASTToIR::convertExpression(const DSLExpression* root_expr, TypeChecker::CheckContext& context) { 
    current_graph_ = std::make_unique<IRGraph>("ir_from_expr"); 
    next_node_id_ = 1; // Reset local counter for this graph 
    IRNodeId entry_cfg_node = current_graph_->addNode(IROpCode::ENTRY, "entry"); 
    IRNodeId last_node = convertNode(root_expr, context, entry_cfg_node); // Start conversion 
    // Add an implicit RETURN_VALUE or EXIT node? Depends on semantics. 
    if (last_node != 0) {
        // Example: Add return value node 
        // IRNodeId ret_val_node = current_graph_->addNode(IROpCode::RETURN_VALUE); 
        // getNode(ret_val_node)->inputs.push_back({last_node, 0, /* type from last_node */}); 
        // current_graph_->addEdge(last_node, ret_val_node); // Add control flow edge 
    }
    return std::move(current_graph_); 
} 
std::unique_ptr<IRGraph> ASTToIR::convertFunction(/* Func Def AST */ TypeChecker::CheckContext& context) { 
     current_graph_ = std::make_unique<IRGraph>(/* func_def->name */); 
     next_node_id_ = 1; 
     // 1. Create ENTRY node 
     // 2. Create PARAM nodes for each parameter, link from ENTRY, store their types 
     // 3. Convert function body expression(s) using convertNode, linking from last PARAM node 
     // 4. Ensure RETURN nodes are handled correctly 
     return std::move(current_graph_); 
} 
// --- Recursive Conversion Helpers (Stubs) --- 
IRNodeId ASTToIR::convertNode(const DSLExpression* expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     if (!expr) return 0; 
     IRNodeId result_node_id = 0; 
     // Get type first (might be needed for some conversions) 
     auto chimera_type = type_checker_.checkExpression(expr, context); // Assuming checkExpression doesn't modify graph state needed here 
     std::visit([&](auto&& arg) { 
         using T = std::decay_t<decltype(arg)>; 
              if constexpr (std::is_same_v<T, Symbol>) { 
                  // Create LOAD_SYMBOL node 
                  result_node_id = current_graph_->addNode(IROpCode::LOAD_SYMBOL, arg.name); 
                  auto* node = getNode(result_node_id); 
                  node->operation_data = arg; 
                  node->output_type = chimera_type; 
              } else if constexpr (std::is_same_v<T, DSLLiteral>) { 
                   // Create LOAD_CONST node 
                   result_node_id = current_graph_->addNode(IROpCode::LOAD_CONST); 
                   auto* node = getNode(result_node_id); 
                   // Convert DSL literal to BDIValueVariant for storage 
                   // node->operation_data = convertLiteralToVariant(arg); // Need helper 
                   node->output_type = chimera_type; 
              } else if constexpr (std::is_same_v<T, DSLOperation>) { 
                   result_node_id = convertOperation(arg, context, current_cfg_node); // Handle recursively 
              } else if constexpr (std::is_same_v<T, std::shared_ptr<IDSLSpecificASTNode>>) { 
                    // Create DSL_BLOCK node 
                    result_node_id = current_graph_->addNode(IROpCode::DSL_BLOCK, arg->getNodeTypeName()); 
                    auto* node = getNode(result_node_id); 
                    // Store pointer or identifier for the specific DSL node in operation_data 
                    // node->operation_data = arg; // Store shared_ptr? Risky. Store identifier/copy? 
                    node->output_type = chimera_type; // Type determined by checker 
                    // Recursively convert inputs *before* this node 
                    // ... complex dependency handling ... 
              } 
              // Add other cases: Definition, Sequence, etc. 
              else {
                   // std::cerr << "ASTToIR: Unhandled expression type in convertNode." << std::endl; 
              } 
     }, expr->content);
// Link control flow (simple sequential assumption here) 
if (result_node_id != 0 && current_cfg_node != 0) { 
         current_graph_->addEdge(current_cfg_node, result_node_id); 
     } 
if (result_node_id != 0) { 
          current_cfg_node = result_node_id; // Update current position 
      } 
return result_node_id; // Return ID of node producing the value 
} 
IRNodeId ASTToIR::convertOperation(const DSLOperation& op, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
// 1. Convert arguments recursively 
std::vector<IRValueRef> arg_refs; 
     IRNodeId last_arg_cfg_node = current_cfg_node; // Track control flow through args 
for(const auto& arg_expr : op.arguments) { 
          IRNodeId arg_node_id = convertNode(arg_expr.get(), context, last_arg_cfg_node); 
if (arg_node_id == 0) throw std::runtime_error("Failed converting operation argument"); 
auto* arg_node = getNode(arg_node_id); // Assume getNode exists in ASTToIR 
          arg_refs.push_back({arg_node_id, 0, arg_node->output_type}); // Assume arg produces single output 0 
     } 
// 2. Create operation node 
// Map DSL Operator string to IROpCode (e.g., BINARY_OP, UNARY_OP, CALL) 
     IROpCode op_code = IROpCode::BINARY_OP; // Default assumption 
// ... logic to determine opcode based on op.op.representation ... 
     IRNodeId op_node_id = current_graph_->addNode(op_code, op.op.representation); 
auto* node = getNode(op_node_id); 
     node->inputs = std::move(arg_refs); 
     node->operation_data = op.op; // Store original operator info 
     node->output_type = type_checker_.checkOperation(op, context); // Get result type 
// 3. Connect control flow (last argument computation -> operation node) 
     current_graph_->addEdge(last_arg_cfg_node, op_node_id); 
     current_cfg_node = op_node_id; 
return op_node_id; 
} 
// ... Implement convertDefinition, convertSequence etc. ... 
IRNode* ASTToIR::getNode(IRNodeId id) { 
return current_graph_ ? current_graph_->getNode(id) : nullptr; 
} 
} // namespace chimera::ir 
