#include "ArithmeticMapper.hpp" 
#include "../../core/graph/OperationTypes.hpp" 
#include "../../core/types/BDITypes.hpp" 
#include "../../core/payload/TypedPayload.hpp" 
#include "../../runtime/ExecutionContext.hpp" // For payload<->variant conversion 
#include <stdexcept>
 namespace bdi::frontend::dsl { 
// Remove previous mapToGraph definition if it existed 
// Implementation of mapToBDI using the base class interface signature is complex 
// because the base class doesn't know about ArithmeticExpr. 
// Option 1: Use std::any_cast inside mapToBDI (as done previously) 
// Option 2: Create a specific public method and have users call that. 
// Let's assume Option 1 for now. 
core::graph::NodeID ArithmeticMapper::mapToBDI(const DSLExpression& dsl_expr, 
                                              bdi::frontend::api::GraphBuilder& builder, 
                                              bdi::core::graph::NodeID& current_control_node) { 
// Helper recursive lambda (captures builder and control node by reference) 
std::function<NodeID(const ArithmeticExpr*)> mapRec = 
         [&](const ArithmeticExpr* expr) -> NodeID { 
if (!expr) return 0; 
switch (expr->op) { 
case ArithOp::CONST_I32: { 
                 NodeID const_node = builder.addNode(BDIOperationType::META_NOP, "CONST_I32"); 
                 builder.setNodePayload(const_node, TypedPayload::createFrom(expr->value)); 
                 builder.defineDataOutput(const_node, 0, BDIType::INT32); 
if (current_control_node != 0) builder.connectControl(current_control_node, const_node); 
                 current_control_node = const_node; 
return const_node; 
             } 
case ArithOp::ADD: 
case ArithOp::SUB: 
case ArithOp::MUL: 
case ArithOp::DIV: { 
                 NodeID lhs_bdi_id = mapRec(expr->lhs.get()); 
                 NodeID lhs_ctrl = current_control_node; // Capture control after LHS 
                 NodeID rhs_bdi_id = mapRec(expr->rhs.get()); 
                 NodeID rhs_ctrl = current_control_node; // Capture control after RHS 
if (lhs_bdi_id == 0 || rhs_bdi_id == 0) throw std::runtime_error("Failed mapping child"); 
                 BDIOperationType bdi_op; 
if (expr->op == ArithOp::ADD) bdi_op = BDIOperationType::ARITH_ADD; 
else if (expr->op == ArithOp::SUB) bdi_op = BDIOperationType::ARITH_SUB; 
else if (expr->op == ArithOp::MUL) bdi_op = BDIOperationType::ARITH_MUL; 
else if (expr->op == ArithOp::DIV) bdi_op = BDIOperationType::ARITH_DIV; 
else throw std::logic_error("Bad ArithOp");
                 NodeID op_node = builder.addNode(bdi_op); 
                 builder.defineDataOutput(op_node, 0, BDIType::INT32); // Assume INT32 result 
// Control flow: Both inputs must complete before op node runs 
                 builder.connectControl(lhs_ctrl, op_node); 
                 builder.connectControl(rhs_ctrl, op_node); 
// Data flow 
                 builder.connectData(lhs_bdi_id, 0, op_node, 0); 
                 builder.connectData(rhs_bdi_id, 0, op_node, 1); 
                 current_control_node = op_node; 
return op_node; 
             } 
default: throw std::runtime_error("Unknown ArithOp"); 
         }
     }; 
// End lambda mapRec 
// --- mapToBDI main logic --- 
// Need to cast the DSLExpression content back to ArithmeticExpr* 
// This indicates DSLExpression might not be the right container, 
// or the mapper needs specific knowledge. Assume caller ensures type. 
const ArithmeticExpr* root_expr = nullptr; 
try { 
// How to get ArithmeticExpr from DSLExpression variant? Needs structure change or specific handling. 
// Placeholder: Assume it's directly the operation for now. 
if(const auto* op_ptr = std::get_if<DSLOperation>(&dsl_expr.content)) { 
// Cannot directly convert DSLOperation back to ArithmeticExpr easily here... 
// This highlights a structural mismatch between generic DSLExpression and specific mapper. 
// Mapper likely needs to operate on the *original* DSL-specific AST node, not DSLExpression. 
throw std::runtime_error("ArithmeticMapper expects direct ArithmeticExpr, not generic DSLExpression (Refactor needed)"); 
         } 
         }
     } 
else { 
throw std::runtime_error("ArithmeticMapper input structure mismatch"); 
catch (const std::exception& e) { 
throw std::runtime_error("ArithmeticMapper input structure error: " + std::string(e.what())); 
     } 
// If casting worked (it won't with current structure): 
// return mapRec(root_expr); 
return 0; // Return error due to structural issue 
}
} // namespace chimera::frontend::dsl 
