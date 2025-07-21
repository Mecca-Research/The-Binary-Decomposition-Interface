#include "ASTToIR.hpp"
#include "DSLCoreTypes.hpp" // For specific node types if DSLExpression holds them 
#include <stdexcept>
#include <vector> 
#include <map> 
#include <iostream> // Keep for debug/info logs, replace errors 
namespace ir::ir { 
// --- Assuming DSLExpression content variant includes --- 
// struct DSLLoopExpr { std::unique_ptr<DSLExpression> condition; std::unique_ptr<DSLExpression> body; }; // e.g., While 
// struct DSLFuncDefExpr { DSLDefinition signature; std::unique_ptr<DSLExpression> body; /* Maybe DSLExpressionSequence */ }; 
// struct DSLFuncCallExpr { Symbol function_name; std::vector<std::unique_ptr<DSLExpression>> arguments; }; 
// struct DSLAssignmentExpr { Symbol target; std::unique_ptr<DSLExpression> value; }; 
// struct DSLReturnExpr { std::unique_ptr<DSLExpression> value; }; // Optional value 
// --- ASTToIR --- 
ASTToIR::ASTToIR(TypeChecker& type_checker) : type_checker_(type_checker) {} 
std::unique_ptr<IRGraph> ASTToIR::convertFunction(const DSLFuncDefExpr& func_def, TypeChecker::CheckContext& parent_context) { 
     // ... (Setup graph, func_context as before) ... 
     IRNodeId entry_node = current_graph_->addNode(IROpCode::ENTRY, "func_entry"); 
     current_graph_->entry_node_id_ = entry_node; 
     IRNodeId current_cfg_node = entry_node; 
     // --- Generate Prologue --- 
     current_cfg_node = generateFunctionPrologue(func_def.signature, func_context, current_cfg_node); // Helper generates STACK/FP ops 
     // Process Parameters - associate PARAM nodes with stack locations relative to FP 
     // Assume parameters are pushed by caller or first things on stack frame 
     // size_t current_param_offset = ... // Calculate offset based on ABI (e.g., positive offset from FP) 
     // for(size_t i = 0; i < func_def.signature.parameters.size(); ++i) { 
     //     SymbolInfo& param_info = func_context.lookupSymbolInfo(func_def.signature.parameters[i].name); // Already added by type checker? 
     //     param_info.location = SymbolInfo::Location::STACK; 
     //     param_info.stack_offset = current_param_offset; 
     //     param_info.address_node_id = generateAddressCalculation(param_info.stack_offset, func_context, current_cfg_node); // Node calculat
     //     current_param_offset += getChimeraTypeSize(param_info.type); // Update for next param 
     // } 
     size_t frame_size = func_context.getFrameSize(); // Get size calculated during type checking 
     // Need nodes representing SP (Stack Pointer) and FP (Frame Pointer) registers/values 
     // IRNodeId current_sp = ... ; // Node representing SP before prologue 
     // IRNodeId current_fp = ... ; // Node representing FP before prologue 
     // 1. Push Old FP onto stack: STORE_MEM(SP - PtrSize, current_fp) 
     // 2. Set New FP:          FP = SP 
     // 3. Allocate Frame:      SP = SP - FrameSize (or ADD SP, -FrameSize) 
     // This requires adding specific IR opcodes or sequences for stack manipulation, 
     // or assuming lower-level details handled by IRToBDI. 
     // Let's add conceptual nodes: 
          IRNodeId push_fp_node = current_graph_->addNode(IROpCode::STACK_PUSH, "push_fp"); 
     // push_fp_node->inputs = { ... node for FP ... }; 
          current_graph_->addEdge(current_cfg_node, push_fp_node); current_cfg_node = push_fp_node; 
          IRNodeId set_fp_node = current_graph_->addNode(IROpCode::STACK_SET_FP, "set_fp"); // FP = SP 
          current_graph_->addEdge(current_cfg_node, set_fp_node); current_cfg_node = set_fp_node; 
          IRNodeId alloc_frame_node = current_graph_->addNode(IROpCode::STACK_ALLOC, "alloc_frame"); 
     // alloc_frame_node->operation_data = frame_size; // Store size 
          current_graph_->addEdge(current_cfg_node, alloc_frame_node); current_cfg_node = alloc_frame_node; 
     // --- Process Parameters (Assign locations relative to new FP) --- 
     // ... Create PARAM IR nodes, set their output type ... 
     // SymbolInfo for param needs location=STACK and correct offset relative to FP  
     // --- Convert function body --- 
          IRNodeId last_body_node = convertNode(func_def.body.get(), func_context, current_cfg_node); 
     // --- Generate Epilogue on RETURN ---    
     // The convertReturn helper needs modification: 
     // Before the RETURN_VALUE node, insert epilogue nodes: 
     // 1. Deallocate Frame: SP = FP 
     // 2. Pop Old FP:      FP = LOAD_MEM(SP - PtrSize); STACK_POP or adjust SP 
     // convertReturn(...) { 
     //    ... (convert return value expression) ... 
     //    IRNodeId dealloc_frame = current_graph_->addNode(IROpCode::STACK_DEALLOC, "dealloc_frame"); 
     //    current_graph_->addEdge(current_cfg_node, dealloc_frame); current_cfg_node = dealloc_frame; 
     //    IRNodeId pop_fp = current_graph_->addNode(IROpCode::STACK_POP, "pop_fp"); 
     //    // pop_fp output is old FP (if needed), also adjusts SP implicitly 
     //    current_graph_->addEdge(current_cfg_node, pop_fp); current_cfg_node = pop_fp; 
     //    // Create RETURN_VALUE node connected to pop_fp 
     //    IRNodeId ret_node_id = current_graph_->addNode(IROpCode::RETURN_VALUE, "return"); 
     //    // ... connect return value input ... 
     //    current_graph_->addEdge(current_cfg_node, ret_node_id); 
     //    return ret_node_id; 
     // } 
     return std::move(current_graph_); 
}     
     // --- Need implementations for --- 
     // convertMemberAccess(const DSLMemberAccessExpr&, CheckContext&, IRNodeId&); 
     // convertArrayIndex(const DSLArrayIndexExpr&, CheckContext&, IRNodeId&); 
     // convertStructLiteral(...); 
     // convertArrayLiteral(...); 
std::unique_ptr<IRGraph> ASTToIR::convertExpression(const DSLExpression* root_expr, TypeChecker::CheckContext& context) { 
    current_graph_ = std::make_unique<IRGraph>("ir_from_expr"),(func_def.signature.name.name); 
    next_node_id_ = 1; 
    TypeChecker::CheckContext func_context; 
    func_context.parent_scope = std::make_shared<TypeChecker::CheckContext>(parent_context); 
    // Add function symbol itself to parent context? Depends on scope rules. 
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
}uj-+
    
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
     // 2. Resolve signature types & Create PARAM nodes 
     std::vector<std::shared_ptr<ChimeraType>> param_types; 
     // Assuming func_def has a structure for parameters like std::vector<Symbol> params; 
     // for(size_t i = 0; i < func_def->parameters.size(); ++i) { 
     //     const auto& param_symbol = func_def->parameters[i]; 
     //     auto param_type = type_checker_.resolveTypeExpr(func_def->parameter_types[i], parent_context); // Resolve type 
     //     param_types.push_back(param_type); 
     //     IRNodeId param_node_id = current_graph_->addNode(IROpCode::PARAM, param_symbol.name); 
     //     auto* param_ir_node = getNode(param_node_id); 
     //     param_ir_node->output_type = param_type; 
     //     param_ir_node->operation_data = param_symbol; // Store symbol info 
     //     param_ir_node->operation_data = static_cast<uint32_t>(i); // Store param index in op_data? 
     //     func_context.addSymbol(param_symbol.name, param_type true, true, 0, param_node_id}); // Add to function scope (as immutable parameter) 
     //     current_graph_->addEdge(current_cfg_node, param_node_id); // Link control flow 
     //     current_cfg_node = param_node_id; 
     // } 
     // Store function type in context? func_context.addSymbol(func_def.signature.name.name, make_function_type(param_types, return_type)); 
     // 3. Convert function body 
     // Assume body is in func_def->value_expr (might be sequence) 
     IRNodeId last_body_node = convertNode(func_def->value_expr.get(), func_context, current_cfg_node); 
     // 4. Ensure proper termination (implicit RETURN for void functions?) 
     // For now, assume convertNode adds RETURN_VALUE if needed by sequence semantics. 
     // If the last node isn't a RETURN or other terminator, add one? Depends on language semantics. Or rely on checkExpression to add RETURN_VALUE?
     // Store function graph mapping? 
     // function_graphs_[func_def.signature.name.name] = current_graph_.get(); // Store raw ptr? Or manage elsewhere. 
     // This needs modification to `convertReturn` or graph post-processing 
     // Add implicit return for void functions if last node isn't RETURN 
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
              // Dispatch based on definition type (Var, Func, Struct, etc.) 
              // if (isFunctionDefinition(arg)) { convertFunctionDefinition(arg, context); } // Functions handled separately 
              // else if (isStructDefinition(arg)) { convertStructDefinition(arg, context); } 
              // else { convertVariableDefinition(arg, context, current_cfg_node); } 
              result_node_id = 0; // Definitions themselves don't yield a value node here                                                       
              else if constexpr (std::is_same_v<T, DSLMemberAccessExpr>) { result_node_id = convertMemberAccess(arg, context, current_cfg_n
              else if constexpr (std::is_same_v<T, DSLArrayIndexExpr>)   { result_node_id = convertArrayIndex(arg, context, current_cfg_nod    
              else if constexpr (std::is_same_v<T, DSLFuncCallExpr>)  { result_node_id = convertFunctionCall(arg, context, current_cfg_node); }
              else if constexpr (std::is_same_v<T, DSLReturnExpr>)   { result_node_id = convertReturn(arg, context, current_cfg_node); } 
              else if constexpr (std::is_same_v<T, DSLStructDefExpr>)  { convertStructDefinition(arg, context, current_cfg_node); result_node_i
              else if constexpr (std::is_same_v<T, DSLStructLiteralExpr>){ result_node_id = convertStructLiteral(arg, context, current_cfg_node
              else if constexpr (std::is_same_v<T, DSLArrayLiteralExpr>) { result_node_id = convertArrayLiteral(arg, context, current_cfg_node)
              else if constexpr (std::is_same_v<T, DSLForLoopExpr>)     { result_node_id = convertForLoop(arg, context, current_cfg_node); }    
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
    // --- Type checking phase already added symbol info to context --- 
    auto symbol_info = context.lookupSymbolInfo(def.name.name, true); // Look in current scope only 
    if (!symbol_info) throw BDIExecutionError("Internal Compiler Error: SymbolInfo missing after type check for " + def.name.name);                                                                             
    // 1. Convert the initializer expression (value_expr) 
    IRNodeId value_node_id = convertNode(def.value_expr.get(), context, current_cfg_node); 
if (value_node_id == 0) throw std::runtime_error("Cannot convert initializer for variable " + 
def.name.name); 
// ... error checks ...                                                                             
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
// Update symbol info with value node ID if immutable/SSA 
if (!symbol_info->is_mutable) { 
        symbol_info->location = SymbolInfo::Location::SSA_VALUE; 
        symbol_info->value_node_id = value_node_id; 
    } else { 
        // --- Generate ALLOC and STORE for mutable variables --- 
        // Determine size/alignment from symbol_info->type 
        size_t alloc_size = 8; // Placeholder: getChimeraTypeSize(symbol_info->type); 
        size_t alloc_align = 8; // Placeholder: getChimeraTypeAlignment(symbol_info->type); 
        // IR ALLOC node 
        IRNodeId alloc_node_id = current_graph_->addNode(IROpCode::ALLOC_MEM, "alloc_" + def.name.name); 
        auto* alloc_node = getNode(alloc_node_id); 
        // Store size/type info needed for allocation in operation_data 
        // alloc_node->operation_data = AllocInfo{alloc_size, alloc_align, symbol_info->type}; 
        alloc_node->output_type = std::make_shared<ChimeraType>(); // Type is POINTER to symbol_info->type 
        // Define pointer type properly: alloc_node->output_type = make_pointer_type(symbol_info->type); 
        current_graph_->addEdge(current_cfg_node, alloc_node_id); 
        current_cfg_node = alloc_node_id; 
        // Update SymbolInfo with allocation info 
        symbol_info->location = SymbolInfo::Location::STACK; // Or HEAP? Depends on scope/lifetime 
        symbol_info->address_node_id = alloc_node_id; // Store node producing address 
        // IR STORE node 
        IRNodeId store_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "init_" + def.name.name); 
        auto* store_node = getNode(store_node_id); 
        store_node->inputs.push_back({alloc_node_id, 0, alloc_node->output_type});           // Input 0: Address 
        store_node->inputs.push_back({value_node_id, 0, value_ir_node->output_type});      // Input 1: Value 
        store_node->operation_data = def.name; // Store target symbol 
        current_graph_->addEdge(current_cfg_node, store_node_id); 
        current_cfg_node = store_node_id; 
    }
    return 0; 
}                                                                                                                                             
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
        // Helper to get address calculation IR nodes (FramePtr + Offset) 
    IRNodeId ASTToIR::generateAddressCalculation(size_t offset, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
        // 1. Load Frame Pointer (assume available via special symbol or context info) 
        // IRNodeId fp_load_id = convertSymbolLoad(Symbol{"__FRAME_POINTER__"}, context, current_cfg_node); 
        // 2. Load Constant Offset 
        IRNodeId offset_const_id = current_graph_->addNode(IROpCode::LOAD_CONST, std::to_string(offset)); 
        auto* offset_node = getNode(offset_const_id); 
        offset_node->output_type = make_scalar(BDIType::UINT64); // Use pointer-sized uint for offset 
        offset_node->operation_data = BDIValueVariant{static_cast<uint64_t>(offset)}; 
        current_graph_->addEdge(current_cfg_node, offset_const_id); // Assume FP available before offset const 
        current_cfg_node = offset_const_id; 
        // 3. Add FP + Offset 
        IRNodeId add_addr_id = current_graph_->addNode(IROpCode::BINARY_OP, "addr_add"); 
        auto* add_node = getNode(add_addr_id); 
        // add_node->inputs.push_back({fp_load_id, 0, /* Pointer Type */ nullptr}); 
        add_node->inputs.push_back({offset_const_id, 0, offset_node->output_type}); 
        add_node->operation_data = Operator{"+"}; // Store operator 
        add_node->output_type = make_scalar(BDIType::POINTER); // Result is pointer 
        // current_graph_->addEdge(fp_load_id, add_addr_id); // Depends on FP load 
        current_graph_->addEdge(offset_const_id, add_addr_id); 
        current_cfg_node = add_addr_id; 
        return add_addr_id; // Return node producing the final address 
    }
    IRNodeId ASTToIR::convertVariableDefinition(const DSLDefinition& def, TypeChecker::CheckContext& context, IRNodeId& current_cfg_n
        auto symbol_info_ptr = context.lookupSymbolInfo(def.name.name, true); 
        if (!symbol_info_ptr) throw BDIExecutionError("Internal Compiler Error: SymbolInfo missing in convertVariableDefinition for " + def.na
        SymbolInfo& symbol_info = *symbol_info_ptr; // Modify context's version 
        IRNodeId value_node_id = convertNode(def.value_expr.get(), context, current_cfg_node); 
        // ... error checks ... 
        auto* value_ir_node = getNode(value_node_id); 
        if (symbol_info.is_mutable && symbol_info.location == SymbolInfo::Location::STACK) { 
            // Generate address calculation node 
            IRNodeId addr_node_id = generateAddressCalculation(symbol_info.stack_offset, context, current_cfg_node); 
            symbol_info.address_node_id = addr_node_id; // Store address source node ID 
            // Generate STORE_MEM 
            IRNodeId store_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "init_" + def.name.name); 
            auto* store_node = getNode(store_node_id); 
            store_node->inputs.push_back({addr_node_id, 0, getNode(addr_node_id)->output_type}); // Input 0: Address 
            store_node->inputs.push_back({value_node_id, 0, value_ir_node->output_type});         // Input 1: Value 
            store_node->operation_data = def.name; 
            current_graph_->addEdge(current_cfg_node, store_node_id); 
            current_cfg_node = store_node_id; 
        } else { // Immutable / SSA 
            symbol_info.value_node_id = value_node_id; // Just track the value source 
        } 
        return 0;
    }
IRNodeId ASTToIR::convertAssignment(const DSLAssignmentExpr& assign_expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_n
     // 1. Convert the RHS value expression 
     // ... (Check target symbol type and mutability in context) ... 
     // ... (Convert RHS value expression -> value_node_id) ... 
     // Find target variable info (assuming mutable needs memory) 
     // SymbolInfo target_info = context.lookupSymbolInfo(assign_expr.target.name); 
     // if (!target_info.is_mutable) throw BDIExecutionError("Cannot assign to immutable variable"); 
     // IRNodeId addr_node_id = target_info.allocation_node_id; // Get node that provided address 
     IRNodeId value_node_id = convertNode(assign_expr.value.get(), context, current_cfg_node); 
     if (value_node_id == 0) throw std::runtime_error("Cannot convert RHS of assignment"); 
     auto* value_ir_node = getNode(value_node_id); 
     // ... Convert RHS value -> value_node_id ... 
     if (!value_ir_node || !value_ir_node->output_type) throw std::runtime_error("RHS node invalid"); 
     // 2. Look up the target variable's type and address/value node 
     // Lookup symbol info 
     const Symbol& target_symbol = assign_expr.target; 
     auto target_type = context.lookupSymbolInfo(target_symbol.name); 
     if (!target_type) throw std::runtime_error("Assignment to undeclared variable: " + target_symbol.name, assign_expr.target.name); 
     if (!target_info || !target_info->is_mutable || target_info->address_node_id == 0) { 
         throw BDIExecutionError("Assignment target not found: " + target_symbol.name); 
     if (!target_info->is_mutable) throw BDIExecutionError("Cannot assign to immutable variable: " + target_symbol.name); 
     if (target_info->location != SymbolInfo::Location::STACK && target_info->location != SymbolInfo::Location::HEAP) { // Check if it has an a
         throw BDIExecutionError("Invalid assignment target location for: " + target_symbol.name); 
     }
     // Check if mutable? context.isMutable(target_symbol.name)? Requires enhancement. 
     // Check type compatibility 
     if (!type_checker_.checkTypeCompatibility(*target_type, *value_ir_node->output_type, context)) { 
           throw std::runtime_error("Type mismatch in assignment to: " + target_symbol.name); 
     } 
     // 3. Create STORE_MEM IR node (assuming mutable variables use memory) 
     // IRNodeId target_addr_node_id = context.getSymbolAddressNode(target_symbol.name); // Need mechanism 
     IRNodeId store_node_id = current_graph_->addNode(IROpCode::STORE_MEM, "assign_" + target_symbol.name, assign_expr.target.name); 
     auto* store_node = getNode(store_node_id); 
     // store_node->inputs.push_back({target_addr_node_id, 0, /* pointer type */}); // Input 0: Address Source
     store_node->inputs.push_back({target_info->address_node_id, 0, /* pointer type */ nullptr}); // Input 0: Address Source Node 
     store_node->inputs.push_back({value_node_id, 0, getNode(value_node_id)->output_type}); // Input 1: Value Source
     store_node->operation_data = assign_expr.target; // Store target symbol for BDI conversion 
     current_graph_->addEdge(current_cfg_node, store_node_id); // Store happens after RHS calculation 
     current_cfg_node = store_node_id; 
     // Assignment expression value? Usually void or the assigned value itself.
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
     IRNodeId cond_cfg_node = loop_header_id = current_graph_->addNode(IROpCode::JUMP, "loop_header");  // Start condition check from header 
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
     IRNodeId body_entry_node_id = current_graph_->addNode(IROpCode::SCOPE_BEGIN, "loop_body_entry"); // Placeholder for scope start 
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
     if (!symbol_info) throw BDIExecutionError("Undefined symbol load: " + symbol.name); 
     IRNodeId result_node_id = 0; 
     if (symbol_info->location == SymbolInfo::Location::STACK || symbol_info->location == SymbolInfo::Location::HEAP) { 
         // Mutable: Generate LOAD_MEM 
         result_node_id = current_graph_->addNode(IROpCode::LOAD_MEM, "load_" + symbol.name); 
         auto* node = getNode(result_node_id); 
         node->inputs.push_back({symbol_info->address_node_id, 0, getNode(symbol_info->address_node_id)->output_type}); /* pointer type */ nullptr}); // Input is the address source 
         node->operation_data = symbol; 
         node->output_type = symbol_info->type; // Load produces value of variable's type 
         if (current_cfg_node != 0) current_graph_->addEdge(current_cfg_node, result_node_id); 
         current_cfg_node = result_node_id; 
         return load_node_id; 
     } else if (symbol_info->location == SymbolInfo::Location::SSA_VALUE) { 
         // Immutable/SSA: Just return the ID of the node that originally produced the value 
         return symbol_info->value_node_id; // No new node, no CFG change here 
         result_node_id = symbol_info->value_node_id; 
         // No new node created, control flow doesn't necessarily advance here unless value node is later 
     } else { 
          throw BDIExecutionError("Cannot load symbol with unknown location: " + symbol.name); 
     } 
     return result_node_id; 
} 
 
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
         IRNodeId ASTToIR::convertFunctionCall(const DSLFuncCallExpr& call_expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_nod
         // 1. Lookup function signature in context 
         // FunctionType func_type = context.lookupFunctionType(call_expr.function_name.name); 
         // if (!func_type) throw BDIExecutionError("Call to undefined function: " + call_expr.function_name.name); 
         // 2. Convert arguments recursively 
         std::vector<IRValueRef> arg_refs; 
         IRNodeId last_arg_cfg_node = current_cfg_node; 
         // if (call_expr.arguments.size() != func_type.argument_types.size()) throw BDIExecutionError("Argument count mismatch for call to " + cal
         for(size_t i=0; i < call_expr.arguments.size(); ++i) { 
         IRNodeId arg_node_id = convertNode(call_expr.arguments[i].get(), context, last_arg_cfg_node); 
         auto* arg_node = getNode(arg_node_id); 
         // Check arg type against func_type.argument_types[i] 
         // if(!type_checker_.checkTypeCompatibility(*func_type.argument_types[i], *arg_node->output_type, context)) { throw ... } 
         arg_refs.push_back({arg_node_id, 0, arg_node->output_type}); 
     }
         // 3. Create CALL IR node 
         IRNodeId call_node_id = current_graph_->addNode(IROpCode::CALL, "call_" + call_expr.function_name.name); 
         auto* node = getNode(call_node_id); 
         node->inputs = std::move(arg_refs); 
         node->operation_data = call_expr.function_name; // Store function name/ID 
         // node->output_type = func_type.return_type; // Set expected return type 
         // 4. Connect control flow 
         current_graph_->addEdge(last_arg_cfg_node, call_node_id); 
         current_cfg_node = call_node_id; 
         return call_node_id; // Returns node representing the call itself (its output holds return value) 
     } 
         IRNodeId ASTToIR::convertReturn(const DSLReturnExpr& ret_expr, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node) { 
         IRNodeId ret_val_node_id = 0; 
         std::shared_ptr<ChimeraType> ret_type = make_scalar(BDIType::VOID); 
         if (ret_expr.value) { // If there is a return value 
         ret_val_node_id = convertNode(ret_expr.value.get(), context, current_cfg_node); 
         auto* val_node = getNode(ret_val_node_id); 
         if (!val_node || !val_node->output_type) throw BDIExecutionError("Invalid return value expression"); 
         ret_type = val_node->output_type; 
         current_cfg_node = ret_val_node_id; // CFG node before RETURN is the value node 
     }
         // TODO: Check ret_type against context.expected_return_type 
         IRNodeId ret_node_id = current_graph_->addNode(IROpCode::RETURN_VALUE, "return"); 
         auto* node = getNode(ret_node_id); 
         if (ret_val_node_id != 0) { // If returning a value 
         node->inputs.push_back({ret_val_node_id, 0, ret_type}); 
     } 
         current_graph_->addEdge(current_cfg_node, ret_node_id); 
         current_cfg_node = 0; // Return terminates this control flow path 
         return ret_node_id; 
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
          branch_node->operation_data = std::pair<IRNodeId, IRNodeId>{body_entry_node_id, exit_loop_node_id}; 
          // Connect control from branch after condition evaluation 
          current_graph_->addEdge(last_arg_cfg_node, branch_node_id); // Control flow after condition eval 
          // Convert True Branch 
          IRNodeId true_branch_start_cfg = branch_node_id; // Control starts from branch 
          IRNodeId true_branch_end_node = convertNode(op.arguments[1].get(), context, true_branch_start_cfg); 
          // Convert False Branch (if exists) 
          IRNodeId false_branch_end_node = 0; 
          IRNodeId false_branch_start_cfg = branch_node_id; 
          if (op.arguments.size() > 2) {
               false_branch_end_node = convertNode(op.arguments[2].get(), context, false_branch_start_cfg); 
          // Convert Loop Body 
          TypeChecker::CheckContext body_context; // New scope for body 
          body_context.parent_scope = std::make_shared<TypeChecker::CheckContext>(context); 
          IRNodeId body_cfg_node = body_entry_node_id; // Start body CFG from entry placeholder 
          current_graph_->addEdge(branch_node_id, body_cfg_node); // True path from branch goes to body entry 
          IRNodeId body_end_node = convertNode(loop_expr.body.get(), body_context, body_cfg_node); 
          // Jump back to header 
          IRNodeId back_jump_node_id = current_graph_->addNode(IROpCode::JUMP, "loop_back_jump"); 
          auto* back_jump_node = getNode(back_jump_node_id); 
          back_jump_node->operation_data = loop_header_id; // Target is header 
          current_graph_->addEdge(body_end_node, back_jump_node_id); // Connect body end to jump 
          // Connect Branch false path to exit node 
          current_graph_->addEdge(branch_node_id, exit_loop_node_id); // False path goes to exit 
          current_cfg_node = exit_loop_node_id; // Continue code gen after loop 
          return 0; 
     } 
          IRNodeId ASTToIR::convertMemberAccess(const DSLMemberAccessExpr& access_expr, TypeChecker::CheckContext& context, IRNodeId& curre
          // 1. Convert the base object expression to get its address node ID 
          //    (Assume structs are passed by reference or allocated on stack -> we need address) 
          IRNodeId base_addr_node_id = convertNode(access_expr.object.get(), context, current_cfg_node); 
          // Error check... Get base object type from type checker... 
          auto base_obj_type = type_checker_.checkExpression(access_expr.object.get(), context); // Re-check? Or store type during conversion? 
          if (!base_obj_type || !base_obj_type->isStruct()) throw BDIExecutionError("Internal Error: Base object for member access is not struct");
          const auto& struct_type = std::get<ChimeraStructType>(base_obj_type->content); 
          // 2. Find field offset 
          auto field_it = struct_type.field_name_to_index.find(access_expr.member_name.name); 
          if (field_it == struct_type.field_name_to_index.end()) throw BDIExecutionError("Internal Error: Field not found"); 
          size_t offset = struct_type.fields[field_it->second].offset; 
          // 3. Generate address calculation: BaseAddr + Offset 
          IRNodeId member_addr_node_id = generateAddressCalculation(offset, context, current_cfg_node, base_addr_node_id); // Pass base addre
          // 4. Return the node producing the MEMBER'S ADDRESS 
          //    The caller (e.g., convertAssignment or convertSymbolLoad needing member) will use this address node 
          //    with LOAD_MEM or STORE_MEM. 
          return member_addr_node_id; // Return node producing the MEMBER'S ADDRESS  
    }
          IRNodeId ASTToIR::convertArrayIndex(const DSLArrayIndexExpr& index_expr, TypeChecker::CheckContext& context, IRNodeId& current_cf
          // 1. Convert array base expression -> base_addr_node_id 
          IRNodeId base_addr_node_id = convertNode(index_expr.array.get(), context, current_cfg_node); 
          auto array_type = type_checker_.checkExpression(index_expr.array.get(), context); 
          if (!array_type || !array_type->isArray()) throw BDIExecutionError("Internal Error: Base object for index is not array"); 
          const auto& arr_type = std::get<ChimeraArrayType>(array_type->content); 
          size_t element_size = arr_type.element_size_bytes; 
          if (element_size == 0) throw BDIExecutionError("Internal Error: Array element size is zero"); 
          // 2. Convert index expression -> index_value_node_id 
          IRNodeId index_value_node_id = convertNode(index_expr.index.get(), context, current_cfg_node); 
          auto index_type = getNode(index_value_node_id)->output_type; // Get actual type of index node 
          // TODO: Convert index_value_node_id to pointer-sized unsigned integer if needed 
          // Error check... ensure index is integer type... 
          // 3. Generate address calculation: BaseAddr + (IndexValue * ElementSize) 
          //    a. Create CONST node for ElementSize 
          IRNodeId elem_size_const_id = current_graph_->addNode(IROpCode::LOAD_CONST); 
          auto* elem_size_node = getNode(elem_size_const_id); 
          elem_size_node->output_type = make_scalar(BDIType::UINT64); // Use uint64 for size/offset math 
          elem_size_node->operation_data = BDIValueVariant{static_cast<uint64_t>(element_size)}; 
          current_graph_->addEdge(current_cfg_node, elem_size_const_id); // Depends on previous cfg node 
          IRNodeId after_size_cfg = elem_size_const_id; 
          // ... set payload for element_size (as pointer-sized uint) ... set output type ... connect CFG ... 
          //    b. Create MUL node: IndexValue * ElementSize 
          IRNodeId offset_node_id = current_graph_->addNode(IROpCode::BINARY_OP); 
          auto* mul_node = getNode(offset_node_id); 
          mul_node->operation_data = Operator{"*"}; 
          mul_node->inputs.push_back({index_value_node_id, 0, index_type}); 
          mul_node->inputs.push_back({elem_size_const_id, 0, elem_size_node->output_type}); 
          mul_node->output_type = make_scalar(BDIType::UINT64); // Resulting offset is uint64 
          current_graph_->addEdge(getNode(index_value_node_id) ? current_cfg_node : 0, offset_node_id); // Depends on index calc 
          current_graph_->addEdge(elem_size_const_id, offset_node_id); // Depends on size const 
          IRNodeId after_mul_cfg = offset_node_id;   
          // ... set inputs (index_value_node_id, elem_size_const_id), op_data="*", output type ... connect CFG ... 
          //    c. Create ADD node: BaseAddr + Offset 
          IRNodeId element_addr_node_id = current_graph_->addNode(IROpCode::BINARY_OP); 
          auto* add_node = getNode(element_addr_node_id); 
          add_node->operation_data = Operator{"+"}; 
          add_node->inputs.push_back({base_addr_node_id, 0, getNode(base_addr_node_id)->output_type}); // Base Addr (Pointer) 
          add_node->inputs.push_back({offset_node_id, 0, mul_node->output_type}); // Offset (UInt64 -> Pointer size?) 
          add_node->output_type = make_scalar(BDIType::POINTER); // Result is pointer 
          current_graph_->addEdge(getNode(base_addr_node_id) ? current_cfg_node : 0, element_addr_node_id); // Depends on base addr calc 
          current_graph_->addEdge(after_mul_cfg, element_addr_node_id); // Depends on offset calc  
          // ... set inputs (base_addr_node_id, offset_node_id), op_data="+", output type POINTER ... connect CFG ... 
          current_cfg_node = element_addr_node_id; // Update CFG pos 
          return element_addr_node_id; // Return node producing the ELEMENT'S ADDRESS 
    }
          // Implement generateFunctionPrologue, generateFunctionEpilogue, convertStructDefinition etc. 
          // Helper to generate address calculation (Modified) 
          IRNodeId ASTToIR::generateAddressCalculation(size_t offset, TypeChecker::CheckContext& context, IRNodeId& current_cfg_node, Node
          IRNodeId current_base_node = 0; 
          if (base_addr_node_id != 0) { 
          current_base_node = base_addr_node_id; // Use provided base 
          } else { 
          // Load Frame Pointer if no base provided (assumes stack var) 
          // current_base_node = getNode(context.lookupSymbolInfo("__FRAME_POINTER__")->address_node_id); // Need FP tracking 
          }
          if (current_base_node == 0) throw BDIExecutionError("Cannot determine base address for offset calculation"); 
          // Load Constant Offset 
          IRNodeId offset_const_id = current_graph_->addNode(IROpCode::LOAD_CONST, std::to_string(offset)); 
          // ... set payload/type ... connect CFG ... 
          // Add Base + Offset 
          IRNodeId add_addr_id = current_graph_->addNode(IROpCode::BINARY_OP, "addr_add"); 
          // ... set inputs (current_base_node, offset_const_id), op="+", type=POINTER ... connect CFG ... 
          current_cfg_node = add_addr_id; 
          return add_addr_id; 
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
void ASTToChiIR::convertStructDefinition(const DSLStructDefExpr& struct_def, TypeChecker::CheckContext& context, ChiIRNodeId& current_cfg_node
    // Type checking phase should have already processed the struct definition 
    // and added its type information to the type registry / context. 
    // No ChiIR nodes are generated for the definition itself, only for its usage (literals, allocs). 
    std::cout << "ASTToChiIR: Processed struct definition for '" << struct_def.name.name << "' (No ChiIR generated)." << std::endl; 
} 
ChiIRNodeId ASTToChiIR::convertStructLiteral(const DSLStructLiteralExpr& literal_expr, TypeChecker::CheckContext& context, ChiIRNodeId& curren
    // 1. Resolve the struct type being instantiated 
    // auto struct_type_ptr = type_checker_.resolveTypeExpr(literal_expr.type_name, context); // Needs type resolution logic 
    auto struct_type_ptr = std::make_shared<ChimeraType>(); // Placeholder 
    if (!struct_type_ptr || !struct_type_ptr->isStruct()) throw BDIExecutionError("Invalid struct type for literal"); 
    const auto& struct_type = std::get<ChimeraStructType>(struct_type_ptr->content); 
    // 2. Allocate memory for the struct instance (on the stack for now) 
    ChiIRNodeId alloc_node_id = current_graph_->addNode(ChiIROpCode::ALLOC_MEM, "alloc_struct_" + struct_type.name); 
    auto* alloc_node = getNode(alloc_node_id); 
    // alloc_node->operation_data = AllocInfo{struct_type.total_size, struct_type.alignment, struct_type_ptr}; 
    alloc_node->output_type = std::make_shared<ChimeraType>(); // POINTER to struct_type 
    current_graph_->addEdge(current_cfg_node, alloc_node_id); 
    current_cfg_node = alloc_node_id; 
    ChiIRNodeId base_addr_node_id = alloc_node_id; // Node producing the base address 
    // 3. Convert and store each field initializer 
    // Assume literal_expr.fields is map<string, unique_ptr<DSLExpression>> 
    // for (const auto& field_pair : literal_expr.fields) { 
    //     const std::string& field_name = field_pair.first; 
    //     const DSLExpression* field_expr = field_pair.second.get(); 
    // 
    //     // Find field info in struct type 
    //     auto field_it = struct_type.field_name_to_index.find(field_name); 
    //     if (field_it == struct_type.field_name_to_index.end()) throw BDIExecutionError("Unknown field '" + field_name + "' in struct litera
    //     size_t field_offset = struct_type.fields[field_it->second].offset; 
    //     auto field_type = struct_type.fields[field_it->second].type; 
    // 
    //     // Convert initializer value 
    //     ChiIRNodeId value_node_id = convertNode(field_expr, context, current_cfg_node); 
    //     auto* value_ir_node = getNode(value_node_id); 
    //     // Check type compatibility(field_type, value_ir_node->output_type)... 
    // 
    //     // Calculate field address: BaseAddr + Offset 
    //     ChiIRNodeId field_addr_node_id = generateAddressCalculation(field_offset, context, current_cfg_node, base_addr_node_id); 
    // 
    //     // Generate STORE_MEM 
    //     ChiIRNodeId store_node_id = current_graph_->addNode(ChiIROpCode::STORE_MEM, "store_field_" + field_name); 
    //     // ... connect inputs (field_addr_node_id, value_node_id) ... connect CFG ... 
    //     current_cfg_node = store_node_id; 
    // } 
    return base_addr_node_id; // Return the node producing the address of the allocated struct 
}
// Implement convertArrayLiteral similarly (ALLOC + sequence of STOREs for elements) 
ChiIRNodeId ASTToChiIR::convertForLoop(const DSLForLoopExpr& for_loop, TypeChecker::CheckContext& context, ChiIRNodeId& current_cfg_node) { 
// Example C-style For: for (init; condition; increment) { body } 
// Structure: 
// -> Convert Init Expression 
// -> LOOP_HEADER (Jump Target) 
// -> Convert Condition Expression 
// -> BRANCH_COND (condition result) 
//      |      \ 
    //      |       -> EXIT_LOOP (Jump Target) 
//      -> LOOP_BODY_START 
//      -> Convert Body Sequence 
//      -> LOOP_INCREMENT_START (Jump Target) 
//      -> Convert Increment Expression 
//      -> JUMP back to LOOP_HEADER 
// -> EXIT_LOOP (Merge Point) 
// 1. Convert Init expression (creates variables in loop scope) 
    TypeChecker::CheckContext loop_context; // Create new scope for loop vars + body 
    loop_context.parent_scope = std::make_shared<TypeChecker::CheckContext>(context); 
    convertNode(for_loop.initializer.get(), loop_context, current_cfg_node); // Init runs once before loop 
// 2. Create Header, Condition, Branch (similar to While loop) 
    ChiIRNodeId loop_header_id = current_graph_->addNode(ChiIROpCode::JUMP, "for_header"); 
    current_graph_->addEdge(current_cfg_node, loop_header_id); 
    ChiIRNodeId cond_cfg_node = loop_header_id; 
    ChiIRNodeId cond_value_node_id = convertNode(for_loop.condition.get(), loop_context, cond_cfg_node); 
    ChiIRNodeId branch_node_id = current_graph_->addNode(ChiIROpCode::BRANCH_COND, "for_branch"); 
// ... connect condition to branch input ... connect cond_cfg_node to branch ... 
// 3. Create Body and Increment entry points (as JUMP targets/labels) 
    ChiIRNodeId body_entry_id = current_graph_->addNode(ChiIROpCode::JUMP, "for_body_entry"); 
    ChiIRNodeId increment_entry_id = current_graph_->addNode(ChiIROpCode::JUMP, "for_increment_entry"); 
    ChiIRNodeId exit_loop_id = current_graph_->addNode(ChiIROpCode::JUMP, "for_exit"); 
// 4. Set Branch targets 
auto* branch_node = getNode(branch_node_id); 
// branch_node->operation_data = std::pair<ChiIRNodeId, ChiIRNodeId>{body_entry_id, exit_loop_id}; 
    current_graph_->addEdge(branch_node_id, body_entry_id); // True -> Body 
    current_graph_->addEdge(branch_node_id, exit_loop_id); // False -> Exit 
// 5. Convert Body, connecting from body_entry 
    ChiIRNodeId body_cfg_node = body_entry_id; 
    ChiIRNodeId body_end_node = convertNode(for_loop.body.get(), loop_context, body_cfg_node); 
    current_graph_->addEdge(body_end_node, increment_entry_id); // End of body jumps to increment 
// 6. Convert Increment, connecting from increment_entry 
    ChiIRNodeId increment_cfg_node = increment_entry_id; 
    ChiIRNodeId increment_end_node = convertNode(for_loop.increment.get(), loop_context, increment_cfg_node); 
// 7. Jump from end of increment back to header 
    ChiIRNodeId back_jump_id = current_graph_->addNode(ChiIROpCode::JUMP, "for_back_jump"); 
    getNode(back_jump_id)->operation_data = loop_header_id; // Target header 
    current_graph_->addEdge(increment_end_node, back_jump_id); 
    current_cfg_node = exit_loop_id; // Continue after loop exit 
return 0; // Loops don't produce a value 
} 
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
