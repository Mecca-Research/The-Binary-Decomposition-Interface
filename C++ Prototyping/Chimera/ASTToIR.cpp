#include "ASTToIR.hpp"
#include "DSLCoreTypes.hpp" // For specific node types if DSLExpression holds them 
#include <stdexcept>
#include <iostream> // Keep for debug/info logs, replace errors 
namespace ir::ir { 
// --- ASTToIR --- 
ASTToIR::ASTToIR(TypeChecker& type_checker) : type_checker_(type_checker) {} 
std::unique_ptr<IRGraph> ASTToIR::convertExpression(const DSLExpression* root_expr, TypeChecker::CheckContext& context) { 
    current_graph_ = std::make_unique<IRGraph>("ir_from_expr"); 
    next_node_id_ = 1; 
    IRNodeId entry_cfg_node = current_graph_->addNode(IROpCode::ENTRY, "entry"); 
    current_graph_->entry_node_id_ = entry_cfg_node; // Mark entry 
    IRNodeId last_expr_node = convertNode(root_expr, context, entry_cfg_node); 
// Implicit return or exit? Add RETURN_VALUE or EXIT node. 
if (last_expr_node != 0) {
        IRNodeId exit_node = current_graph_->addNode(IROpCode::RETURN_VALUE, "return"); 
auto* ret_ir_node = getNode(exit_node); 
auto* last_expr_ir_node = getNode(last_node); 
if (last_expr_ir_node && last_expr_ir_node->output_type) { 
            ret_ir_node->inputs.push_back({last_node, 0, last_expr_ir_node->output_type}); // Assume output 0 is result 
        } 
        current_graph_->addEdge(last_node, exit_node); // Control flow 
    } 
else { 
// Handle empty expression case? Add EXIT directly? 
        current_graph_->addNode(IROpCode::EXIT, "empty_exit"); 
    }
    return std::move(current_graph_); 
} 
std::unique_ptr<IRGraph> ASTToIR::convertFunction(const DSLDefinition* func_def, TypeChecker::CheckContext& parent_context) { 
     if (!func_def || !std::holds_alternative<std::monostate>(func_def->value_expr->content) /* check if value holds function body */) { 
         throw std::runtime_error("Invalid function definition AST node passed to convertFunction"); 
     } 
     current_graph_ = std::make_unique<IRGraph>(func_def->name.name); 
     next_node_id_ = 1; 
     TypeChecker::CheckContext func_context; // Create new scope 
     func_context.parent_scope = std::make_shared<TypeChecker::CheckContext>(parent_context); 
     // 1. Create ENTRY node 
     IRNodeId entry_node = current_graph_->addNode(IROpCode::ENTRY, "func_entry"); 
     current_graph_->entry_node_id_ = entry_node; 
     IRNodeId current_cfg_node = entry_node; 
     // 2. Create PARAM nodes for parameters 
     // Assuming func_def has a structure for parameters like std::vector<Symbol> params; 
     // for(size_t i = 0; i < func_def->parameters.size(); ++i) { 
     //     const auto& param_symbol = func_def->parameters[i]; 
     //     auto param_type = type_checker_.resolveTypeExpr(func_def->parameter_types[i], parent_context); // Resolve type 
     //     IRNodeId param_node_id = current_graph_->addNode(IROpCode::PARAM, param_symbol.name); 
     //     auto* param_ir_node = getNode(param_node_id); 
     //     param_ir_node->output_type = param_type; 
     //     param_ir_node->operation_data = param_symbol; // Store symbol info 
     //     func_context.addSymbol(param_symbol.name, param_type); // Add to function scope 
     //     current_graph_->addEdge(current_cfg_node, param_node_id); // Link control flow 
     //     current_cfg_node = param_node_id; 
     // } 
     // 3. Convert function body 
     // Assume body is in func_def->value_expr (might be sequence) 
     IRNodeId last_body_node = convertNode(func_def->value_expr.get(), func_context, current_cfg_node); 
     // 4. Ensure RETURN nodes are handled 
     // If the last node wasn't explicitly RETURN, add one? Or rely on checkExpression to add RETURN_VALUE? 
     // For now, assume convertNode adds RETURN_VALUE if needed by sequence semantics. 
     return std::move(current_graph_); 
} 
// Recursive Conversion Helpers 
// struct DSLLoopExpr { DSLExpression condition; DSLExpressionSequence body; }; 
// struct DSLFuncDefExpr { DSLDefinition signature; DSLExpressionSequence body; }; // Simplified 
// struct DSLAssignmentExpr { Symbol target; DSLExpression value; }; 
IRNodeId ASTToIR::convertNode(const DSLExpression* expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     if (!expr) return 0; 
     IRNodeId result_node_id = 0; // ID of node producing the value of this expression 
     auto chimera_type = type_checker_.checkExpression(expr, context); // Get type first 
     std::visit([&](auto&& arg) { 
     // ... (Literal, Symbol, Operation cases) ... 
         using T = std::decay_t<decltype(arg)>; 
              if constexpr (std::is_same_v<T, std::monostate>) { /* No node */ }
              else if constexpr (std::is_same_v<T, Symbol>) { result_node_id = convertSymbolLoad(arg, context, current_cfg_node); } 
              else if constexpr (std::is_same_v<T, DSLLiteral>) { result_node_id = convertLiteral(arg, context, current_cfg_node); } 
              else if constexpr (std::is_same_v<T, DSLOperation>) { result_node_id = convertOperation(arg, context, current_cfg_node); } 
              else if constexpr (std::is_same_v<T, DSLExpressionSequence>) { result_node_id = convertSequence(arg, context, current_cfg_node);
              else if constexpr (std::is_same_v<T, DSLDefinition>) { convertDefinition(arg, context, current_cfg_node); result_node_id = 0; 
              // Handle different kinds of definitions 
              // if (isFunctionDefinition(arg)) { convertFunctionDefinition(arg, context, current_cfg_node); } 
              // else { convertVariableDefinition(arg, context, current_cfg_node); } 
              result_node_id = 0; // Definitions usually don't produce a value node directly 
              } 
              // Example: Handling an assignment AST node 
              // else if constexpr (std::is_same_v<T, DSLAssignmentExpr>) { 
              //    result_node_id = convertAssignment(arg, context, current_cfg_node); 
              // } 
              // Example: Handling a loop AST node 
              // else if constexpr (std::is_same_v<T, DSLLoopExpr>) { // e.g., a 'while' loop 
              //    result_node_id = convertWhileLoop(arg, context, current_cfg_node); 
              // }                                       
              else if constexpr (std::is_same_v<T, std::shared_ptr<IDSLSpecificASTNode>>) { result_node_id = convertDSLBlock(arg.get(), contex
              else { throw std::runtime_error("ASTToIR: Unknown expression variant type."); } 
     }, expr->content); 
     // Attach annotations from AST node to resulting IR node(s) if applicable 
     // if (result_node_id != 0 && expr->hasAnnotations()) { ... getNode(result_node_id)->annotations = ... } 
     return result_node_id; 
} 
IRNodeId ASTToIR::convertVariableDefinition(const DSLDefinition& def, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node)
 // 1. Convert the initializer expression (value_expr) 
    IRNodeId value_node_id = convertNode(def.value_expr.get(), context, current_cfg_node); 
if (value_node_id == 0) throw std::runtime_error("Cannot convert initializer for variable " + 
def.name.name); 
auto* value_ir_node = getNode(value_node_id); 
if (!value_ir_node || !value_ir_node->output_type) throw std::runtime_error("Initializer node invalid"); 
// 2. (Optional) Resolve type annotation if present and check compatibility 
std::shared_ptr<ChimeraType> var_type = value_ir_node->output_type; 
if (def.type_expr) { 
// var_type = type_checker_.resolveTypeExpr(def.type_expr.get(), context); 
// if (!type_checker_.checkTypeCompatibility(*var_type, *value_ir_node->output_type, context)) { /* Error */ } 
    }
 // 3. Add symbol to context *with its type* 
    context.addSymbol(
 def.name.name, var_type);   
    // 4. Create IR Node for Storage (if necessary) 
    // Depending on strategy: 
    //   - Variables might just be SSA values (represented by the node producing them). 
    //   - Mutable variables might require explicit ALLOC_MEM + STORE_MEM IR nodes. 
    // Let's assume mutable requires ALLOC/STORE for now. 
    if (/* variable is mutable? Check annotations or language rules */ true) { 
         // a. Create ALLOC_MEM node 
         IRNodeId alloc_node_id = current_graph_->addNode(IROpCode::ALLOC_MEM, "alloc_" + def.name.name); 
         auto* alloc_node = getNode(alloc_node_id); 
         alloc_node->output_type = std::make_shared<ChimeraType>(); // Type should be a pointer/ref type 
         // alloc_node->operation_data = var_type; // Store allocated type info 
         current_graph_->addEdge(current_cfg_node, alloc_node_id); // Alloc happens after previous step 
         current_cfg_node = alloc_node_id; 
         // b. Create STORE_MEM node 
          IRNodeId store_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "store_" + def.name.name); 
          auto* store_node = getNode(store_node_id); 
          store_node->inputs.push_back({alloc_node_id, 0, alloc_node->output_type}); // Input 0: Address 
          store_node->inputs.push_back({value_node_id, 0, value_ir_node->output_type}); // Input 1: Value 
          store_node->operation_data = def.name; // Store symbol being stored for clarity 
          current_graph_->addEdge(current_cfg_node, store_node_id); // Store happens after alloc 
          current_cfg_node = store_node_id; 
          // Store address associated with symbol in context for later LOAD_SYMBOL 
          // context.addSymbolAddress(def.name.name, alloc_node_id); // Need mechanism in CheckContext 
    } else { 
        // Immutable variable: just associate symbol name with the value-producing node ID 
        // context.addSymbolValueNode(def.name.name, value_node_id); // Need mechanism 
    }
    return 0; // Definition itself doesn't produce a value usable by next expression 
} 
IRNodeId ASTToIR::convertAssignment(const DSLAssignmentExpr& assign_expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_n
     // 1. Convert the RHS value expression 
     IRNodeId value_node_id = convertNode(assign_expr.value.get(), context, current_cfg_node); 
     if (value_node_id == 0) throw std::runtime_error("Cannot convert RHS of assignment"); 
     auto* value_ir_node = getNode(value_node_id); 
     if (!value_ir_node || !value_ir_node->output_type) throw std::runtime_error("RHS node invalid"); 
     // 2. Look up the target variable's type and address/value node 
     const Symbol& target_symbol = assign_expr.target; 
     auto target_type = context.lookupSymbol(target_symbol.name); 
     if (!target_type) throw std::runtime_error("Assignment to undeclared variable: " + target_symbol.name); 
     // Check if mutable? context.isMutable(target_symbol.name)? Requires enhancement. 
     // Check type compatibility 
     if (!type_checker_.checkTypeCompatibility(*target_type, *value_ir_node->output_type, context)) { 
           throw std::runtime_error("Type mismatch in assignment to: " + target_symbol.name); 
     } 
     // 3. Create STORE_MEM IR node (assuming mutable variables use memory) 
     // IRNodeId target_addr_node_id = context.getSymbolAddressNode(target_symbol.name); // Need mechanism 
     IRNodeId store_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "assign_" + target_symbol.name); 
     auto* store_node = getNode(store_node_id); 
     // store_node->inputs.push_back({target_addr_node_id, 0, /* pointer type */}); // Input 0: Address 
     store_node->inputs.push_back({value_node_id, 0, value_ir_node->output_type}); // Input 1: Value 
     store_node->operation_data = target_symbol; 
     current_graph_->addEdge(current_cfg_node, store_node_id); // Store happens after RHS calculation 
     current_cfg_node = store_node_id; 
     // Assignment expression value? Usually void or the assigned value itself. Let's say void. 
     return 0; 
} 
IRNodeId ASTToIR::convertWhileLoop(const DSLLoopExpr& loop_expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     // Structure: -> Header -> Condition -> Branch -> Body -> Jump -> Header  
     // ... code before loop ... 
     // -> LOOP_HEADER (Jump Target) 
     // -> Convert Condition Expression 
     // -> BRANCH_COND (condition result) 
     //      |      \ 
     //      |       -> EXIT_LOOP (Jump Target) 
     //      -> LOOP_BODY_START 
     //      -> Convert Body Sequence 
     //      -> JUMP back to LOOP_HEADER 
     // -> EXIT_LOOP (Merge Point) 
     // ... code after loop ... 
     //      -> Exit Merge <-/ 
     IRNodeId loop_header_id = current_graph_->addNode(IROpCode::JUMP, "loop_header"); // Acts as label/merge 
     current_graph_->addEdge(current_cfg_node, loop_header_id); // Edge into loop check 
     // Convert Condition 
     // Condition - control flow starts from header 
     IRNodeId cond_cfg_node = loop_header_id = current_graph_->addNode(ChiIROpCode::JUMP, "loop_header");  // Start condition check from header 
     current_graph_->addEdge(current_cfg_node, loop_header_id); // Edge into loop check 
     IRNodeId cond_value_node_id = convertNode(&loop_expr.condition, context, cond_cfg_node); 
     if (cond_value_node_id == 0 || getNode(cond_value_node_id)->output_type->getBaseBDIType() != BDIType::BOOL) { 
         throw std::runtime_error("Loop condition must evaluate to boolean"); 
     } 
     // Create Branch 
     // Branch - control flow comes from end of condition evaluation 
     IRNodeId branch_node_id = current_graph_->addNode(IROpCode::BRANCH_COND, "loop_branch"); 
     auto* branch_node = getNode(branch_node_id); 
     branch_node->inputs.push_back({cond_value_node_id, 0, getNode(cond_value_node_id)->output_type}); 
     current_graph_->addEdge(cond_cfg_node, branch_node_id); // Connect condition result to branch 
     // Loop Body - control flow starts conceptually AFTER branch (on true path) 
     // Create Loop Body Scope and Convert Body 
     TypeChecker::CheckContext body_context;  
     body_context.parent_scope = std::make_shared<TypeChecker::CheckContext>(context); 
     IRNodeId body_entry_placeholder = branch_node_id; // We'll actually start body exec after branch node 
     IRNodeId body_start_cfg_node = branch_node_id; // Control enters body from branch (true path) 
     IRNodeId body_end_node = convertNode(&loop_expr.body, body_context, body_entry_placeholder, body_start_cfg_node); // Assuming body is a sequence, Body CFG starts here 
     // Add Jump from end of body back to header 
     IRNodeId back_jump_node_id = current_graph_->addNode(IROpCode::JUMP, "loop_back_jump"); 
     auto* back_jump_node = getNode(back_jump_node_id); 
     // back_jump_node->operation_data = loop_header_id; // Store target (Alternative way) 
     current_graph_->addEdge(body_end_node, back_jump_node_id); // Connect body end to jump 
     current_graph_->addEdge(back_jump_node_id, loop_header_id); // Connect jump to header 
     // Create Exit Node (Merge Point) 
     IRNodeId exit_loop_node_id = current_graph_->addNode(IROpCode::JUMP, "loop_exit"); // Merge point after loop, Acts as merge label 
     // Connect Branch false path to exit node 
     // Need to store targets in BRANCH node operation_data or successors 
     // branch_node->operation_data = std::pair<IRNodeId, IRNodeId>{ body_start_cfg_node_after_branch , exit_loop_node_id }; 
     current_graph_->addEdge(branch_node_id, exit_loop_node_id); // Add edge conceptually for false path 
     current_cfg_node = exit_loop_node_id; // Continue code generation after the loop exit 
     return 0; // Loops typically don't produce a single value node 
} 
IRNodeId ASTToIR::convertLiteral(const DSLLiteral& literal, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     IRNodeId node_id = current_graph_->addNode(IROpCode::LOAD_CONST); 
     auto* node = getNode(node_id); 
     node->output_type = type_checker_.checkLiteral(literal); // Get type from checker 
     // Convert literal to BDIValueVariant and store 
     std::visit([&](auto&& arg) { 
          using LT = std::decay_t<decltype(arg)>; 
          if constexpr (!std::is_same_v<LT, std::monostate>) { node->operation_data = BDIValueVariant{arg}; } 
     }, literal); 
     if (current_cfg_node != 0) current_graph_->addEdge(current_cfg_node, node_id); 
     current_cfg_node = node_id; 
     return node_id;
 } 
IRNodeId ASTToIR::convertSymbolLoad(const Symbol& symbol, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     auto symbol_type = context.lookupSymbol(symbol.name); 
     if (!symbol_type) throw std::runtime_error("ASTToIR Error: Undefined symbol '" + symbol.name + "'"); 
     IRNodeId node_id = current_graph_->addNode(IROpCode::LOAD_SYMBOL, symbol.name); 
     auto* node = getNode(node_id); 
     node->operation_data = symbol; 
     node->output_type = symbol_type; 
     if (current_cfg_node != 0) current_graph_->addEdge(current_cfg_node, node_id); 
     current_cfg_node = node_id; 
     return node_id;
} 
IRNodeId ASTToIR::convertOperation(const DSLOperation& op, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
     // 1. Convert arguments first, linking their control flow sequentially 
     std::vector<IRValueRef> arg_refs; 
     IRNodeId last_arg_cfg_node = current_cfg_node; 
     for(const auto& arg_expr : op.arguments) { 
         IRNodeId arg_node_id = convertNode(arg_expr.get(), context, last_arg_cfg_node); // last_arg_cfg_node updated here 
         if (arg_node_id == 0) throw std::runtime_error("Failed converting operation argument"); 
         auto* arg_node = getNode(arg_node_id); 
         if (!arg_node || !arg_node->output_type) throw std::runtime_error("Invalid argument node created"); 
         arg_refs.push_back({arg_node_id, 0, arg_node->output_type}); 
     } 
     // 2. Handle specific operation types (Control Flow, Assignment, Calls, Binary/Unary Ops) 
     const std::string& op_name = op.op.representation; 
     // Example: Assignment (=) - Assume AST structure `DSLOperation{"=", {LHS_Symbol, RHS_Expr}}` 
     if (op_name == "=" && op.arguments.size() == 2 && std::holds_alternative<Symbol>(op.arguments[0]->content)) { 
         // LHS (arg 0) is handled by looking up symbol, RHS (arg 1) is value node ID from arg_refs[1] 
         const Symbol& target_symbol = std::get<Symbol>(op.arguments[0]->content); 
         // Check type compatibility between target symbol and RHS value 
         auto target_type = context.lookupSymbol(target_symbol.name); 
         if (!target_type) throw std::runtime_error("Assignment to undeclared variable: " + target_symbol.name); 
         if (!type_checker_.checkTypeCompatibility(*target_type, *arg_refs[1].type, context)) { 
              throw std::runtime_error("Type mismatch in assignment to: " + target_symbol.name); 
         }
         // Create STORE_SYMBOL or similar IR node? Or handle directly in BDI conversion? 
         // For now, let's assume assignment doesn't create a separate IR node but affects context. 
         // Or create an ASSIGN operation node. Let's do that. 
         IRNodeId assign_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "assign_" + target_symbol.name); // Use STORE_MEM concep
         auto* node = getNode(assign_node_id); 
         node->operation_data = target_symbol; // Store target symbol 
         node->inputs.push_back(arg_refs[1]);   // Value to store 
         node->output_type = make_scalar(BDIType::VOID); // Assignment has no value 
         current_graph_->addEdge(last_arg_cfg_node, assign_node_id); // Control flow after RHS calculation 
         current_cfg_node = assign_node_id; 
         return assign_node_id; // Return assign node ID (or 0 if no value?) 
     } 
     // Example: If/Else - Assume AST `DSLOperation{"if", {Cond, TrueBranch, FalseBranch}}` (Branches are Sequences) 
     else if (op_name == "if" && op.arguments.size() >= 2) { 
          // Node for condition already created in arg_refs[0] 
          IRNodeId cond_node_id = arg_refs[0].node_id; 
          if (arg_refs[0].type->getBaseBDIType() != BDIType::BOOL) throw std::runtime_error("If condition must be boolean"); 
          // Create BRANCH node 
          IRNodeId branch_node_id = current_graph_->addNode(IROpCode::BRANCH_COND, "if_branch"); 
          auto* branch_node = getNode(branch_node_id); 
          branch_node->inputs.push_back(arg_refs[0]); // Condition input 
          current_graph_->addEdge(last_arg_cfg_node, branch_node_id); // Control flow after condition eval 
          // Convert True Branch 
          IRNodeId true_branch_start_cfg = branch_node_id; // Control starts from branch 
          IRNodeId true_branch_end_node = convertNode(op.arguments[1].get(), context, true_branch_start_cfg); 
          // Convert False Branch (if exists) 
          IRNodeId false_branch_end_node = 0; 
          IRNodeId false_branch_start_cfg = branch_node_id; 
          if (op.arguments.size() > 2) {
               false_branch_end_node = convertNode(op.arguments[2].get(), context, false_branch_start_cfg); 
          } 
          // Create Merge Node (if necessary) 
          // Set branch targets (Successors) 
          branch_node->control_successors.push_back(body_entry_placeholder == branch_node_id ? // If body is empty, need first real node 
                                  (getNode(body_end_node) ? body_end_node : back_jump_node_id) // Find start of actual body code
                                  : getNode(body_entry_placeholder)->control_successors[0]);   // True Path: First node generated 
          branch_node->control_successors.push_back(exit_loop_node_id); // False Path: Exit loop 
          current_cfg_node = exit_loop_node_id; // Continue code generation after the loop exit 
         return 0; // Loops don't produce a value 
     } 
     // Similar logic needs to be implemented for If/Else using BRANCH_COND and merge points. 
     // Function Calls: Create CALL node, link arguments, CFG edge to CALL, CFG edge from CALL to successor. 
    // Function Defs: Convert body, ensure RETURN nodes exist.
          IRNodeId merge_node_id = current_graph_->addNode(IROpCode::JUMP, "if_merge"); // Use JUMP as simple merge 
          // Connect branch targets (True -> True Branch Start, False -> False Branch Start) - Handled by BRANCH node data 
          branch_node->operation_data = std::pair<IRNodeId, IRNodeId>{ 
               getNode(true_branch_start_cfg)->control_successors[0], // Node *after* branch feeding true branch 
               (false_branch_end_node != 0) ? getNode(false_branch_start_cfg)->control_successors[0] : merge_node_id // Node after branch feed
          }; 
          // Connect end of branches to merge node 
           current_graph_->addEdge(true_branch_end_node, merge_node_id); 
           if (false_branch_end_node != 0) { 
               current_graph_->addEdge(false_branch_end_node, merge_node_id); 
           } else { 
               // If no false branch, connect branch directly to merge for false case? Needs refinement. 
                // The BRANCH node itself needs to encode both targets. IR structure needs update. 
           } 
          current_cfg_node = merge_node_id; 
          // Result type of 'if' depends on language rules (e.g., must branches match? C-style void?) 
          // node->output_type = ...; 
          return merge_node_id; // Return merge node (or last node of dominant branch?) 
     } 
     // Example: Standard Binary/Unary Ops 
     else { 
         // Determine IR opcode based on op_name (e.g., BINARY_OP, UNARY_OP) 
         IROpCode op_code = (op.arguments.size() == 2) ? IROpCode::BINARY_OP : 
                               (op.arguments.size() == 1) ? IROpCode::UNARY_OP : IROpCode::CALL; // Basic guess 
         IRNodeId op_node_id = current_graph_->addNode(op_code, op_name); 
         auto* node = getNode(op_node_id); 
         node->inputs = std::move(arg_refs); 
         node->operation_data = op.op; // Store DSL operator 
         node->output_type = type_checker_.checkOperation(op, context); // Reuse type checker logic for result type 
         current_graph_->addEdge(last_arg_cfg_node, op_node_id); // Control flow after last arg 
         current_cfg_node = op_node_id;
         return op_node_id; 
     } 
} 
// ... Implement convertDefinition, convertSequence, convertDSLBlock, convertSymbolLoad etc. ... 
// --- IRToBDI --- (Structure - Implementation heavily stubbed) --- 
IRToBDI::IRToBDI(GraphBuilder& builder, DSLRegistry& dsl_registry, MetadataStore& meta_store) 
    : builder_(builder), dsl_registry_(dsl_registry), meta_store_(meta_store) {} 
bool IRToBDI::convertGraph(const IRGraph& ir_graph) { 
     ir_to_bdi_node_map_.clear(); 
     ir_output_to_bdi_port_map_.clear(); 
     // TODO: Perform topological sort or handle dependencies correctly 
     // Simple iteration for now 
     std::vector<IRNodeId> nodes_to_visit; 
     // Assume entry node exists and start traversal 
     if (auto entry_id = ir_graph.getEntryNode()) { 
         // Need a proper traversal algorithm (DFS, BFS) 
         nodes_to_visit.push_back(*entry_id); // Example: Start with entry 
     } else { 
          throw std::runtime_error("IR graph has no entry node"); 
     } 
     std::set<IRNodeId> visited;
     std::vector<IRNodeId> processing_order; // Result of traversal 
     // --- Placeholder for Traversal --- 
     // Perform DFS or BFS starting from entry to get a reasonable processing order 
     // For now, just process entry node as example 
      if (!nodes_to_visit.empty()) { 
          const IRNode* start_node = ir_graph.getNode(nodes_to_visit[0]); 
          if (start_node) { 
              if (!convertNode(*start_node)) return false; 
          } 
      } 
      // --- End Placeholder --- 
     // After converting all nodes, connect control flow edges in BDI 
     // for (const auto& pair : ir_graph.nodes_) { ... iterate successors ... connectControl ... } 
     return true; // Placeholder 
} 
bool IRToBDI::convertNode(const IRNode& ir_node) { 
     if (ir_to_bdi_node_map_.count(ir_node.id)) { 
         return true; // Already processed 
     } 
     // Recursively ensure inputs are converted first (if not using topo sort) 
     for(const auto& input_ref : ir_node.inputs) { 
         if (!ir_to_bdi_node_map_.count(input_ref.node_id)) { 
             const IRNode* input_ir_node = current_graph_->getNode(input_ref.node_id); // Need access to IR graph 
             if (!input_ir_node || !convertNode(*input_ir_node)) return false; 
         }
     } 
     NodeID bdi_node_id = 0; 
     BDIOperationType bdi_op = BDIOperationType::META_NOP; 
     std::cout << "Converting IR Node " << ir_node.id << " (Op: " << static_cast<int>(ir_node.opcode) << ")" << std::endl; 
     // --- Map IR Opcode to BDI Op + Build Node --- 
     switch (ir_node.opcode) { 
         case IROpCode::ENTRY: bdi_op = BDIOperationType::META_START; break; 
         case IROpCode::EXIT: bdi_op = BDIOperationType::META_END; break; 
         case IROpCode::RETURN_VALUE: bdi_op = BDIOperationType::CTRL_RETURN; break; // RETURN takes value input 
         case IROpCode::LOAD_CONST: bdi_op = BDIOperationType::META_CONST; break; // Use new const type 
         case IROpCode::LOAD_SYMBOL: /* Requires memory load based on symbol lookup */ bdi_op = BDIOperationType::MEM_LOAD; /* Placeholder 
         case IROpCode::STORE_MEM: /* Assignment */ bdi_op = BDIOperationType::MEM_STORE; break; 
         case IROpCode::BINARY_OP: { 
              const auto* op = std::get_if<Operator>(&ir_node.operation_data); 
              if (op) { // Map operator name to BDI OpType 
                   if (op->representation == "+") bdi_op = BDIOperationType::ARITH_ADD; 
                   else if (op->representation == "-") bdi_op = BDIOperationType::ARITH_SUB; 
                   // ... Map ALL operators ... 
                   else { std::cerr << "IRToBDI Error: Unknown binary operator " << op->representation << std::endl; return false; } 
              } else return false; 
              break;
         }
          case IROpCode::BRANCH_COND: bdi_op = BDIOperationType::CTRL_BRANCH_COND; break; 
          case IROpCode::JUMP: bdi_op = BDIOperationType::CTRL_JUMP; break; 
          case IROpCode::CALL: bdi_op = BDIOperationType::CTRL_CALL; break; 
          // ... other cases ... 
         default: 
              std::cerr << "IRToBDI Error: Unhandled IR Opcode: " << static_cast<int>(ir_node.opcode) << std::endl; 
              return false; 
     } 
     // Create the main BDI node 
     bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
     ir_to_bdi_node_map_[ir_node.id] = bdi_node_id; 
     // Set Payload for CONST 
 if (ir_node.opcode == IROpCode::LOAD_CONST) { 
          if (const auto* val_var = std::get_if<BDIValueVariant>(&ir_node.operation_data)) { 
              builder_.setNodePayload(bdi_node_id, ExecutionContext::variantToPayload(*val_var)); 
          } else return false;
     } 
     // Define Output Port(s) - assume single output 0 for simplicity 
      if (ir_node.output_type && ir_node.output_type->isResolved() && ir_node.output_type->getBaseBDIType() != BDIType::VOID) { 
          builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType(), ir_node.label + "_out"); 
          // Store mapping for this output value 
          // Ensure vector exists before accessing element 
          if(ir_output_to_bdi_port_map_[ir_node.id].empty()) { 
              ir_output_to_bdi_port_map_[ir_node.id].resize(1); 
          } 
          ir_output_to_bdi_port_map_[ir_node.id][0] = {bdi_node_id, 0}; 
      } 
     // Connect Data Inputs 
     for (size_t i = 0; i < ir_node.inputs.size(); ++i) { 
         const auto& input_ref = ir_node.inputs[i]; 
         auto bdi_port_ref_opt = getBDIPortRef(input_ref); // Find BDI source port 
         if (!bdi_port_ref_opt) { 
              std::cerr << "IRToBDI Error: Failed to find BDI source for IR input." << std::endl; 
             return false; 
         }
         if (!builder_.connectData(bdi_port_ref_opt.value().node_id, bdi_port_ref_opt.value().port_index, 
                              bdi_node_id, static_cast<PortIndex>(i))) { 
              std::cerr << "IRToBDI Error: Failed to connect data edge." << std::endl; 
              return false; // Failed to connect 
         }
     } 
     // Handle control flow connection later after all nodes are mapped 
     // TODO: Translate and add annotations as BDI metadata 
     return true; 
} 
// ... getBDIPortRef implementation ... 
} // namespace ir::ir 
