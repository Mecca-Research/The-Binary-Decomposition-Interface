#include "TypeChecker.hpp" 
#include "../dsl/DSLRegistry.hpp" // Potentially needed for operator info 
#include <iostream> 
#include <memory> // For make_shared 
namespace chimera::frontend::types { 
// --- CheckContext Methods --- (As before) 
// --- Type Comparison (Implementation in ChimeraTypes.cpp potentially or header) --- 
// bool ChimeraType::operator==(const ChimeraType& other) const { ... } // Need full impl comparing variants 
// --- TypeChecker Methods --- 
// Helper to create unresolved type 
auto make_unresolved() { return std::make_shared<ChimeraType>(); } 
// Helper to create scalar type easily 
auto make_scalar(BDIType bdi_t, bool is_const = false) { 
auto t = std::make_shared<ChimeraType>(); 
    t->content = ChimeraScalarType{bdi_t, is_const}; 
return t; 
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
else if constexpr (std::is_same_v<T, DSLDefinition>)  { checkDefinition(arg, context); return make_scalar(BDIType::VOID); /*
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
     auto value_type = checkExpression(definition.value_expr.get(), context); 
     if (!value_type->isResolved()) { 
         throw std::runtime_error("Type Error: Cannot resolve type for definition of '" + definition.name.name + "'"); 
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
} // namespace chimera::frontend::types 
         }
