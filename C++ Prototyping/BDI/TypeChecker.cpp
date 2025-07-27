#include "TypeChecker.hpp" 
#include "DSLRegistry.hpp" // Potentially needed for operator info 
#include <iostream> 
#include <memory> // For make_shared 
namespace chimera::frontend::types { 
// --- CheckContext Methods --- (As before) 
// --- Type Comparison (Implementation in ChimeraTypes.cpp potentially or header) --- 
// bool ChimeraType::operator==(const ChimeraType& other) const { ... } // Need full impl comparing variants 
// Assume AST includes DSLMemberAccessExpr, DSLArrayIndexExpr, DSLStructDefExpr etc. 
// --- TypeChecker Methods --- 
void ChimeraStructType::calculateLayout() { 
        total_size = 0; 
        alignment = 1; 
        field_name_to_index.clear(); 
        size_t current_offset = 0; 
        for (size_t i = 0; i < fields.size(); ++i) { 
            auto& field = fields[i]; 
            if (!field.type || !field.type->isResolved()) throw BDIExecutionError("Cannot layout struct with unresolved field types"); 
            size_t field_size = getChimeraTypeSize(*field.type); 
            size_t field_align = getChimeraTypeAlignment(*field.type); 
            // Align current offset UP 
            current_offset = (current_offset + field_align - 1) / field_align * field_align; 
            field.offset = current_offset; 
            current_offset += field_size; 
            alignment = std::max(alignment, field_align); // Struct alignment is max of field alignments 
            field_name_to_index[field.name] = i; 
        } 
        // Align total size UP to struct alignment 
        total_size = (current_offset + alignment - 1) / alignment * alignment; 
    }
    void ChimeraArrayType::calculateLayout() { 
        if (!element_type || !element_type->isResolved()) throw BDIExecutionError("Cannot layout array with unresolved element type"); 
        element_size_bytes = getChimeraTypeSize(*element_type); 
        alignment = getChimeraTypeAlignment(*element_type); 
        total_size = element_size_bytes * count; 
        // Array total size doesn't necessarily need element alignment padding *between* elements if accessed via index math 
    }
        // --- TypeChecker Updates --- 
        // Assume AST includes nodes like: 
        // struct DSLMemberAccessExpr { std::unique_ptr<DSLExpression> object; Symbol member_name; }; 
        // struct DSLArrayIndexExpr { std::unique_ptr<DSLExpression> array; std::unique_ptr<DSLExpression> index; }; 
        // struct DSLStructDefExpr { ... }; struct DSLArrayDefExpr { ... }; 
        // Helper to create unresolved type 
        auto make_unresolved() { return std::make_shared<ChimeraType>(); } 
        // Helper to create scalar type easily 
        auto make_scalar(BDIType bdi_t, bool is_const = false) { 
        auto t = std::make_shared<ChimeraType>(); 
        t->content = ChimeraScalarType{bdi_t, is_const}; 
        return t; 
    } 
        // Add get size/alignment helpers based on ChimeraType (simplified) 
        size_t getChimeraTypeSize(const ChimeraType& type) { 
        if (type.isScalar()) { return getBdiTypeSize(std::get<ChimeraScalarType>(type.content).base_bdi_type); } 
        // Add cases for Tensor, Struct etc. based on their definitions 
        return 8; // Default/placeholder 
    }
        size_t getChimeraTypeAlignment(const ChimeraType& type) { 
        if (type.isScalar()) { return getBdiTypeSize(std::get<ChimeraScalarType>(type.content).base_bdi_type); } // Often size==alignment 
        return 8; // Default/placeholder 
     } 
    std::shared_ptr<ChimeraType> TypeChecker::checkMemberAccess(const DSLMemberAccessExpr& access_expr, CheckContext& context) { 
        auto object_type = checkExpression(access_expr.object.get(), context); 
        if (!object_type || !object_type->isStruct()) { 
            throw BDIExecutionError("Type Error: Member access '.' requires a struct type, got " /* + typeToString(object_type) */ ); 
        } 
        const auto& struct_type = std::get<ChimeraStructType>(object_type->content); 
        auto field_it = struct_type.field_name_to_index.find(access_expr.member_name.name); 
        if (field_it == struct_type.field_name_to_index.end()) { 
            throw BDIExecutionError("Type Error: Struct '" + struct_type.name + "' has no member named '" + access_expr.member_name.name + "'"
        } 
        size_t field_index = field_it->second; 
        return struct_type.fields[field_index].type; // Return the type of the member 
    }
     std::shared_ptr<ChimeraType> TypeChecker::checkArrayIndex(const DSLArrayIndexExpr& index_expr, CheckContext& context) { 
        auto array_type = checkExpression(index_expr.array.get(), context); 
        auto index_type = checkExpression(index_expr.index.get(), context); 
        if (!array_type || !array_type->isArray()) { 
             throw BDIExecutionError("Type Error: Array index '[]' requires an array type."); 
        } 
         if (!index_type || !index_type->isScalar() || !TypeSystem::isInteger(std::get<ChimeraScalarType>(index_type->content).base_bdi_type))
             throw BDIExecutionError("Type Error: Array index must be an integer type."); 
         }
         // Optional: Add static bounds check if array size and index are constants? 
        const auto& arr_type = std::get<ChimeraArrayType>(array_type->content); 
        return arr_type.element_type; // Return the element type 
    }
     // Need checkDefinition variations for struct/array definitions 
}
     std::shared_ptr<ChimeraType> TypeChecker::checkExpression(const DSLExpression* expr, CheckContext& context) { 
     if (!expr) return make_unresolved(); 
     try { 
     return std::visit([&](auto&& arg) -> std::shared_ptr<ChimeraType> {  
     using T = std::decay_t<decltype(arg)>; 
     if constexpr (std::is_same_v<T, std::monostate>) { return make_unresolved(); } 
     else if constexpr (std::is_same_v<T, Symbol>)        { return checkSymbol(arg, context); } 
     else if constexpr (std::is_same_v<T, DSLLiteral>)     { return checkLiteral(arg); } 
     else if constexpr (std::is_same_v<T, DSLOperation>)    { return checkOperation(arg, context); } 
     else if constexpr (std::is_same_v<T, DSLExpressionSequence>) { return checkSequence(arg, context); } 
     else if constexpr (std::is_same_v<T, DSLDefinition>)  { checkDefinition(arg, context); return make_scalar(BDIType::VOID); 
     else { std::cerr << "Type Error: Unknown expression variant type." << std::endl; return make_unresolved(); } 
              }, expr->content); 
} 
     catch (const std::exception& e) { 
     std::cerr << "Type Error during checkExpression: " << e.what() << std::endl; 
     return make_unresolved(); 
     } 
} 
bool TypeChecker::checkFunctionDefinition(/* Function definition struct func_def */ CheckContext& parent_context) { 
     // std::cerr << "TypeChecker::checkFunctionDefinition - STUBBED" << std::endl; 
     CheckContext func_context; 
     func_context.parent_scope = std::make_shared<CheckContext>(parent_context); // Inherit scope 
     // 1. Resolve parameter types and add to func_context.symbol_table 
     // ... requires parsing func_def.parameters ... 
     // Example: func_context.addSymbol("param_name", resolved_param_type); 
     // 2. Resolve return type annotation (if any) 
     // std::shared_ptr<ChimeraType> expected_return = resolveTypeExpr(func_def.return_type_expr, parent_context); // Need resolveTypeExpr hel
     // func_context.expected_return_type = expected_return; 
     // 3. Check function body 
     // std::shared_ptr<ChimeraType> actual_return_type = checkExpression(func_def.body, func_context); 
     // 4. Verify return type compatibility 
     // if (!checkTypeCompatibility(*expected_return, *actual_return_type, func_context)) { 
     //      std::cerr << "Type Error: Function return type mismatch." << std::endl; 
     //      return false; 
     // } 
     return true; // Placeholder 
} 
bool TypeChecker::checkModule(/* Module structure mod */) { 
    // std::cerr << "TypeChecker::checkModule - STUBBED" << std::endl; 
    CheckContext global_context; 
    bool success = true; 
    // for (const auto& item : mod.top_level_items) { 
    //      if (/* item is function */) { success &= checkFunctionDefinition(item, global_context); } 
    //      else if (/* item is global definition */) { checkDefinition(item, global_context); } // Definitions add to context 
    //      else { success &= checkExpression(item, global_context)->isResolved(); /* Top level expr must resolve? */ } 
    // } 
    return success; // Placeholder 
} 
// --- Private Helpers --- 
std::shared_ptr<ChimeraType> TypeChecker::checkLiteral(const DSLLiteral& literal) { 
     auto checked_type = make_unresolved(); 
     std::visit([&](auto&& arg) { 
         using T = std::decay_t<decltype(arg)>; 
         // Assign default concrete types to literals 
         if constexpr (std::is_same_v<T, long long>) { checked_type = make_scalar(BDIType::INT64); } // Default to largest int 
         else if constexpr (std::is_same_v<T, double>) { checked_type = make_scalar(BDIType::FLOAT64); } // Default to largest float 
         else if constexpr (std::is_same_v<T, bool>) { checked_type = make_scalar(BDIType::BOOL); } 
         else if constexpr (std::is_same_v<T, std::string>) { /* checked_type = make_string_type(); // Need String type */ } 
     }, literal); 
     return checked_type;
 } 
std::shared_ptr<ChimeraType> TypeChecker::checkSymbol(const Symbol& symbol, CheckContext& context) { 
    auto found_type = context.lookupSymbol(symbol.name); 
    if (!found_type) { 
         throw std::runtime_error("Type Error: Undefined symbol '" + symbol.name + "'"); 
    }
    return found_type; 
} 
std::shared_ptr<ChimeraType> TypeChecker::checkOperation(const DSLOperation& operation, CheckContext& context) { 
    // std::cerr << "TypeChecker::checkOperation for '" << operation.op.representation << "'." << std::endl; 
    // 1. Recursively check types of all arguments 
    std::vector<std::shared_ptr<ChimeraType>> arg_types; 
    arg_types.reserve(operation.arguments.size()); 
    for (const auto& arg_expr : operation.arguments) { 
        auto arg_type = checkExpression(arg_expr.get(), context); 
        if (!arg_type->isResolved()) { 
             throw std::runtime_error("Type Error: Could not resolve type for argument in operation '" + operation.op.representation + "'"); 
        } 
        arg_types.push_back(arg_type); 
    }
    // 2. Look up operator signature & determine result type (basic built-ins for now) 
    // TODO: Integrate with DSLRegistry for custom operators 
    const std::string& op_name = operation.op.representation; 
    if (op_name == "+" || op_name == "-" || op_name == "*" || op_name == "/") { 
        if (arg_types.size() != 2) throw std::runtime_error("Type Error: Binary operator '" + op_name + "' requires 2 arguments."); 
        // Promote and return result type (assuming scalar numeric for now) 
        if (!arg_types[0]->isScalar() || !arg_types[1]->isScalar()) throw std::runtime_error("Type Error: Operands for '" + op_name + "' must 
        BDIType type1 = std::get<ChimeraScalarType>(arg_types[0]->content).base_bdi_type; 
        BDIType type2 = std::get<ChimeraScalarType>(arg_types[1]->content).base_bdi_type; 
        BDIType result_bdi_type = TypeSystem::getPromotedType(type1, type2); 
        if (result_bdi_type == BDIType::UNKNOWN || !TypeSystem::isNumeric(result_bdi_type)) { 
             throw std::runtime_error("Type Error: Invalid operands for arithmetic operator '" + op_name + "'"); 
        } 
         return make_scalar(result_bdi_type); 
    } else if (op_name == "==" || op_name == "!=" || op_name == "<" || op_name == "<=" || op_name == ">" || op_name == ">=") { 
         if (arg_types.size() != 2) throw std::runtime_error("Type Error: Comparison operator '" + op_name + "' requires 2 arguments."); 
         // Basic check: ensure types are promotable/comparable 
         if (!arg_types[0]->isScalar() || !arg_types[1]->isScalar()) throw std::runtime_error("Type Error: Operands for '" + op_name + "' must
         BDIType type1 = std::get<ChimeraScalarType>(arg_types[0]->content).base_bdi_type; 
         BDIType type2 = std::get<ChimeraScalarType>(arg_types[1]->content).base_bdi_type; 
         if (TypeSystem::getPromotedType(type1, type2) == BDIType::UNKNOWN && type1 != type2) { // Allow comparing same non-numeric types? 
              throw std::runtime_error("Type Error: Incompatible operands for comparison operator '" + op_name + "'"); 
        return make_scalar(BDIType::BOOL); // Comparisons return bool 
    }
    // Add cases for function calls (lookup function type, check args, return its return type) 
    // Add cases for other built-in operators (logical, bitwise, assignment etc.) 
    else {
        throw std::runtime_error("Type Error: Unknown or unsupported operator '" + op_name + "'"); 
    }
 } 
std::shared_ptr<ChimeraType> TypeChecker::checkSequence(const DSLExpressionSequence& seq, CheckContext& context) { 
    std::shared_ptr<ChimeraType> last_type = make_scalar(BDIType::VOID); // Default to void 
    CheckContext sequence_context; // Create new scope for sequence 
    sequence_context.parent_scope = std::make_shared<CheckContext>(context); 
    for (const auto& expr : seq.expressions) { 
         last_type = checkExpression(expr.get(), sequence_context); // Check in sequence scope 
    }
    // Type of a sequence is the type of its last evaluated expression 
    return last_type; 
} 
std::shared_ptr<ChimeraType> TypeChecker::checkDefinition(const DSLDefinition& definition, CheckContext& context) { 
     // 1. Check the value expression first (allows type inference) 
     // ... check value_type, annotation_type ... 
        bool is_mutable = true; // Determine mutability 
        SymbolInfo info(final_type, is_mutable, false, context.getScopeLevel()); 
        if (is_mutable) { // Allocate stack space only for mutable locals for now 
            info.location = SymbolInfo::Location::STACK; 
            size_t size = getChimeraTypeSize(*final_type); 
            size_t alignment = getChimeraTypeAlignment(*final_type); 
            info.stack_offset = context.allocateStackSpace(size, alignment); 
            std::cout << "TypeCheck INFO: Allocated stack space for '" << def.name.name << "' at offset " << info.stack_offset << std::endl; 
        } else { 
            info.location = SymbolInfo::Location::SSA_VALUE; // Mark as value source 
        } 
        if (!context.addSymbol(def.name.name, info)) { /* Error */ } 
        return make_scalar(BDIType::VOID); 
        } 
     auto value_type = checkExpression(definition.value_expr.get(), context); 
     if (!value_type->isResolved()) { 
         throw std::runtime_error("Type Error: Cannot resolve type for definition of '" + definition.name.name + "'"); 
     } 
     // --- CheckContext Stack Allocation --- 
     size_t TypeChecker::CheckContext::allocateStackSpace(const std::string& var_name, size_t size_bytes, size_t alignment) { 
     if (!stack_frame) { // Should only allocate within function frames 
     if (parent_scope) return parent_scope->allocateStackSpace(var_name, size_bytes, alignment); // Delegate up? Risky. 
     throw BDIExecutionError("Compiler Error: Attempting stack allocation outside function scope"); 
         }
     // Align current offset UP 
     stack_frame->current_offset = (stack_frame->current_offset + alignment - 1) / alignment * alignment; 
     size_t allocated_offset = stack_frame->current_offset; 
     stack_frame->current_offset += size_bytes; 
     stack_frame->total_size = std::max(stack_frame->total_size, stack_frame->current_offset); // Update total size needed 
     stack_frame->max_alignment = std::max(stack_frame->max_alignment, alignment); 
     stack_frame->variable_offsets[var_name] = allocated_offset; // Store offset 
     } 
// std::cout << "StackAlloc: " << var_name << " at offset " << allocated_offset << " (Size " << size_bytes << ")" << std::endl; 
return allocated_offset; 
std::optional<size_t> TypeChecker::CheckContext::getStackOffset(const std::string& name) const { 
if (stack_frame) { 
auto it = stack_frame->variable_offsets.find(name); 
if (it != stack_frame->variable_offsets.end()) { 
return it->second; 
         }
     } 
if (parent_scope) { 
return parent_scope->getStackOffset(name); // Look in parent 
     } 
return std::nullopt; 
} 
size_t TypeChecker::CheckContext::getFrameSize() const { 
if (stack_frame) return stack_frame->total_size; 
if (parent_scope) return parent_scope->getFrameSize(); // Delegate up? Or error? Error safer. 
throw BDIExecutionError("Compiler Error: Cannot get frame size outside function scope"); 
} 
// --- TypeChecker Updates --- 
// Modify checkDefinition to use new allocateStackSpace 
std::shared_ptr<ChimeraType> TypeChecker::checkDefinition(const DSLDefinition& def, CheckContext& context) { 
// ... check value_type, annotation_type -> final_type ... 
bool is_mutable = true; 
SymbolInfo info(final_type, is_mutable, false, context.getScopeLevel()); 
if (is_mutable /* && isLocalVariable not parameter/global? */) { 
        info.location = SymbolInfo::Location::STACK; 
size_t size = getChimeraTypeSize(*final_type); 
size_t alignment = getChimeraTypeAlignment(*final_type); 
// Allocate space and store the calculated offset IN THE SymbolInfo 
        info.stack_offset = context.allocateStackSpace(
 def.name.name, size, alignment); 
    } 
else { /* ... handle SSA ... */ } 
if (!context.addSymbol(
 def.name.name, info)) { /* Error */ } 
return make_scalar(BDIType::VOID); 
}    
bool TypeChecker::checkTypeCompatibility(const ChimeraType& expected, const ChimeraType& actual, CheckContext& context) { 
    // std::cerr << "TypeChecker::checkTypeCompatibility - STUBBED" << std::endl; 
    if (!expected.isResolved() || !actual.isResolved()) return false; // Cannot compare unresolved 
    // Basic scalar check 
    if (expected.isScalar() && actual.isScalar()) { 
        const auto& scalar_expected = std::get<ChimeraScalarType>(expected.content); 
        const auto& scalar_actual = std::get<ChimeraScalarType>(actual.content); 
        // Check if actual can implicitly convert to expected 
        return TypeSystem::canImplicitlyConvert(scalar_actual.base_bdi_type, scalar_expected.base_bdi_type); 
    }
    // Add checks for Tensor compatibility (shape, element type), Function types, etc. 
    // Fallback: Require exact type match for non-scalars for now 
    return expected == actual; 
} 
    // Called when processing struct definition AST node 
    bool TypeChecker::checkStructDefinition(const DSLStructDefExpr& def, CheckContext& context) { 
        std::cout << "TypeCheck INFO: Checking struct definition '" << def.name.name << "'" << std::endl; 
        auto new_struct_type = std::make_shared<ChimeraType>(); 
        ChimeraStructType st_content; 
        st_content.name = def.name.name; 
        // Create a temporary context for resolving field types relative to struct scope? (If needed) 
        // CheckContext struct_context(&context); 
        for (const auto& field_def : def.fields) { // Assume def.fields is vector<DSLDefinition> or similar 
            // Resolve field type annotation 
            // auto field_type = resolveTypeExpr(field_def.type_expr.get(), context); // Use resolve helper 
            auto field_type = make_scalar(BDIType::INT32); // Placeholder type resolution 
            if (!field_type || !field_type->isResolved()) { 
                throw BDIExecutionError("Type Error: Cannot resolve type for field '" + field_def.name.name + "' in struct '" + def.name.name 
            } 
            st_content.fields.push_back({field_def.name.name, field_type, 0}); // Offset calculated later 
        } 
        st_content.calculateLayout(); // Calculate offsets, size, alignment 
        new_struct_type->content = std::move(st_content); 
        new_struct_type->user_defined_name = def.name.name; 
        // Add struct type definition to the current scope/type registry 
        // context.addTypeDefinition(def.name.name, new_struct_type); // Need type registry in context 
        return true;
    }
    // Check Member Access (called from checkExpression visitor) 
    std::shared_ptr<ChimeraType> TypeChecker::checkMemberAccess(const DSLMemberAccessExpr& access_expr, CheckContext& context) { 
        auto object_type = checkExpression(access_expr.object.get(), context); 
        if (!object_type || !object_type->isStruct()) { 
            throw BDIExecutionError("Type Error: Member access '.' requires a struct type."); 
        } 
        // Use std::get to access the struct type content safely 
        const auto& struct_type = std::get<ChimeraStructType>(object_type->content); 
        auto field_it = struct_type.field_name_to_index.find(access_expr.member_name.name); 
        if (field_it == struct_type.field_name_to_index.end()) { 
            throw BDIExecutionError("Type Error: Struct '" + struct_type.name + "' has no member '" + access_expr.member_name.name + "'"); 
        } 
        size_t field_index = field_it->second; 
        if (field_index >= struct_type.fields.size()) { 
             throw BDIExecutionError("Internal Compiler Error: Invalid field index for struct member."); 
        } 
        return struct_type.fields[field_index].type; // Return the type of the member 
    }
     // Check Array Index (called from checkExpression visitor) 
     std::shared_ptr<ChimeraType> TypeChecker::checkArrayIndex(const DSLArrayIndexExpr& index_expr, CheckContext& context) { 
        auto array_type = checkExpression(index_expr.array.get(), context); 
        auto index_type = checkExpression(index_expr.index.get(), context); 
        if (!array_type || !array_type->isArray()) { 
             throw BDIExecutionError("Type Error: Array index '[]' requires an array type."); 
        } 
         if (!index_type || !index_type->isScalar() || !TypeSystem::isInteger(std::get<ChimeraScalarType>(index_type->content).base_bdi_type))
             throw BDIExecutionError("Type Error: Array index must be an integer type."); 
         }
        const auto& arr_type = std::get<ChimeraArrayType>(array_type->content); 
        if (!arr_type.element_type) { 
            throw BDIExecutionError("Internal Compiler Error: Array type has null element type."); 
        } 
        return arr_type.element_type; // Return the element type 
    }
// Assume TypeChecker has access to a Type Registry storing struct/array definitions 
std::shared_ptr<ChimeraType> TypeChecker::resolveTypeExpr(const DSLExpression* type_expr, CheckContext& context) { 
// Simplified: Assume type expression is just a Symbol for now 
if(const auto* sym = std::get_if<Symbol>(&type_expr->content)) { 
// Lookup type name in registry/context 
// return context.lookupTypeDefinition(sym->name); // Needs type registry 
// Placeholder: Return a pre-defined struct/array type for testing 
if (sym->name == "Point") { 
auto point_type = std::make_shared<ChimeraType>(); 
                ChimeraStructType st; 
st.name = "Point"; 
                st.fields.push_back({"x", make_scalar(BDIType::FLOAT32)}); 
                st.fields.push_back({"y", make_scalar(BDIType::FLOAT32)}); 
                st.calculateLayout(); // Calculate offsets/size 
                point_type->content = std::move(st); 
return point_type; 
            } 
if (sym->name == "IntArray3") { 
auto arr_type = std::make_shared<ChimeraType>(); 
                 ChimeraArrayType at; 
                 at.element_type = make_scalar(BDIType::INT32); 
                 at.count = 3; 
                 at.calculateLayout(); 
                        arr_type->content = std::move(at); 
                 return arr_type; 
            } 
        } 
        throw BDIExecutionError("Type Error: Cannot resolve type expression."); 
    }
    std::shared_ptr<ChimeraType> TypeChecker::checkDefinition(const DSLDefinition& def, CheckContext& context) { 
        // ... (Existing value checking) ... 
        std::shared_ptr<ChimeraType> final_type = value_type; // Type inferred from value first 
        if (def.type_expr) { // If explicit type annotation exists 
           auto annotation_type = resolveTypeExpr(def.type_expr.get(), context); 
           if (!annotation_type || !annotation_type->isResolved()) { 
                reportError("Invalid type annotation for '" + def.name.name + "'", /*loc*/); 
           } else if (!checkTypeCompatibility(*annotation_type, *value_type, context)) { // Check compatibility 
                reportError("Value type incompatible with annotation for '" + def.name.name + "'", /*loc*/); 
           } else { 
               final_type = annotation_type; // Use annotation type if compatible 
           } 
        } 
         // ... (Handle mutability, allocate stack space using final_type size/alignment) ... 
         SymbolInfo info(final_type, /*mutability*/ true, false, context.getScopeLevel()); 
         if (info.is_mutable) { 
             info.location = SymbolInfo::Location::STACK; 
             info.stack_offset = context.allocateStackSpace(def.name.name, getTypeSize(*final_type, *this), getTypeAlignment(*final_type, *thi
         } else { /* SSA */ } 
         context.addSymbol(def.name.name, info); 
         return make_scalar(BDIType::VOID); 
    }
// --- ChimeraType Method Implementations --- 
bool ChimeraType::operator==(const ChimeraType& other) const { 
     if (content.index() != other.content.index()) return false; 
     // Add specific comparisons for each variant type 
     if (const auto* this_scalar = std::get_if<ChimeraScalarType>(&content)) { 
         return *this_scalar == std::get<ChimeraScalarType>(other.content); 
     } else if (const auto* this_tensor = std::get_if<ChimeraTensorType>(&content)) { 
         return *this_tensor == std::get<ChimeraTensorType>(other.content); 
     } // Add other cases... 
     return true; // If index matches and it's monostate or unhandled 
} 
     std::shared_ptr<ChimeraType> final_type = value_type; 
     // 2. Check type annotation if present 
     if (definition.type_expr) { 
        // Need a way to resolve type expressions (e.g., looking up type names) 
        // std::shared_ptr<ChimeraType> annotation_type = resolveTypeExpr(definition.type_expr.get(), context); 
        std::shared_ptr<ChimeraType> annotation_type = make_unresolved(); // Placeholder 
        if (!annotation_type->isResolved()) { 
             throw std::runtime_error("Type Error: Invalid type annotation for '" + definition.name.name + "'"); 
        } 
        // Check compatibility 
        if (!checkTypeCompatibility(*annotation_type, *value_type, context)) { 
             throw std::runtime_error("Type Error: Value type incompatible with type annotation for '" + definition.name.name + "'"); 
        } 
        final_type = annotation_type; // Use annotation type if compatible 
     } 
     // 3. Add symbol to context (use final_type) 
     context.addSymbol(definition.name.name, final_type); 
     return make_scalar(BDIType::VOID); // Definitions themselves evaluate to void 
} 
// Add checkMemberAccess, checkArrayIndex ... 
// Implement checkStructLiteral, checkArrayLiteral similarly, ensuring field/element types match definition. 
} // namespace chimera::frontend::types 
