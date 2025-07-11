#include "TypeChecker.hpp" 
#include <iostream> 
namespace chimera::frontend::types { 
// --- CheckContext Methods --- 
std::shared_ptr<ChimeraType> TypeChecker::CheckContext::lookupSymbol(const std::string& name) const { 
    auto it = symbol_table.find(name); 
    if (it != symbol_table.end()) { 
        return it->second; 
    } else if (parent_scope) {
        return parent_scope->lookupSymbol(name); 
    } else { 
        return nullptr; // Symbol not found 
    }
 } 
void TypeChecker::CheckContext::addSymbol(const std::string& name, std::shared_ptr<ChimeraType> type) { 
    // Check for redeclaration in current scope? 
    symbol_table[name] = type; 
} 
// --- TypeChecker Methods (Stubs) --- 
std::shared_ptr<ChimeraType> TypeChecker::checkExpression(const DSLExpression* expr, CheckContext& context) { 
    if (!expr) return std::make_shared<ChimeraType>(); // Return unresolved on null 
     // Dispatch based on the variant content 
     return std::visit([&](auto&& arg) -> std::shared_ptr<ChimeraType> { 
         using T = std::decay_t<decltype(arg)>; 
              if constexpr (std::is_same_v<T, std::monostate>) { return std::make_shared<ChimeraType>(); /* Unresolved */ } 
              else if constexpr (std::is_same_v<T, Symbol>)        { return checkSymbol(arg, context); } 
              else if constexpr (std::is_same_v<T, DSLLiteral>)     { return checkLiteral(arg); } 
              else if constexpr (std::is_same_v<T, DSLOperation>)    { return checkOperation(arg, context); } 
              else if constexpr (std::is_same_v<T, DSLExpressionSequence>) { return checkSequence(arg, context); } 
              else if constexpr (std::is_same_v<T, DSLDefinition>)  { /* Definitions usually don't have a return type in sequence */ checkDefi
              else { /* Unknown variant type */ return std::make_shared<ChimeraType>(); } 
     }, expr->content); 
} 
bool TypeChecker::checkFunctionDefinition(/* Function def */ CheckContext& parent_context) { 
     std::cerr << "TypeChecker::checkFunctionDefinition - STUBBED" << std::endl; 
     // 1. Create new scope context inheriting parent_context 
     // 2. Add function parameters to new scope's symbol table with their types 
     // 3. Set expected_return_type in new context 
     // 4. checkExpression(function_body, new_context) 
     // 5. Check if body's return type matches expected_return_type 
     return true; // Placeholder 
} 
bool TypeChecker::checkModule(/* Module structure */) { 
     std::cerr << "TypeChecker::checkModule - STUBBED" << std::endl; 
     CheckContext global_context; 
     // Iterate through top-level definitions/expressions in module 
     // checkExpression(item, global_context) or checkFunctionDefinition(item, global_context) 
     return true; // Placeholder 
} 
// --- Private Helpers (Stubs) --- 
std::shared_ptr<ChimeraType> TypeChecker::checkLiteral(const DSLLiteral& literal) { 
     // Determine ChimeraType based on literal variant type 
     auto checked_type = std::make_shared<ChimeraType>(); 
     std::visit([&](auto&& arg) { 
         using T = std::decay_t<decltype(arg)>; 
         if constexpr (std::is_same_v<T, long long>) { checked_type->content = ChimeraScalarType{BDIType::INT64}; } // Default int literal typ
         else if constexpr (std::is_same_v<T, double>) { checked_type->content = ChimeraScalarType{BDIType::FLOAT64}; } // Default float liter
         else if constexpr (std::is_same_v<T, bool>) { checked_type->content = ChimeraScalarType{BDIType::BOOL}; } 
         else if constexpr (std::is_same_v<T, std::string>) { /* String Type needed */ } 
     }, literal); 
     return checked_type;
} 
std::shared_ptr<ChimeraType> TypeChecker::checkSymbol(const Symbol& symbol, CheckContext& context) { 
    auto found_type = context.lookupSymbol(symbol.name); 
    if (!found_type) { 
         std::cerr << "Type Error: Undefined symbol '" << symbol.name << "'" << std::endl; 
         return std::make_shared<ChimeraType>(); // Return unresolved 
    }
    return found_type; 
} 
std::shared_ptr<ChimeraType> TypeChecker::checkOperation(const DSLOperation& operation, CheckContext& context) { 
     std::cerr << "TypeChecker::checkOperation for '" << operation.op.representation << "' - STUBBED" << std::endl; 
     // 1. Recursively check types of all arguments: checkExpression(arg, context) 
     // 2. Look up the operator definition (e.g., from DSLRegistry or built-ins) 
     // 3. Check if argument types match operator signature 
     // 4. Determine result type based on operator definition and arg types (using promotion logic) 
     return std::make_shared<ChimeraType>(); // Placeholder 
} 
std::shared_ptr<ChimeraType> TypeChecker::checkSequence(const DSLExpressionSequence& seq, CheckContext& context) { 
    // Type of sequence is usually the type of its *last* expression 
    std::shared_ptr<ChimeraType> last_type = std::make_shared<ChimeraType>(); // Default to void/unresolved 
     CheckContext sequence_context; // Create new scope for sequence? Or use parent? Depends on language rules. 
     sequence_context.parent_scope = std::make_shared<CheckContext>(context); // Example: New scope 
    for (const auto& expr : seq.expressions) { 
         last_type = checkExpression(expr.get(), sequence_context); // Check in sequence scope 
    }
    return last_type; 
} 
std::shared_ptr<ChimeraType> TypeChecker::checkDefinition(const DSLDefinition& definition, CheckContext& context) { 
     std::cerr << "TypeChecker::checkDefinition for '" << definition.name.name << "' - STUBBED" << std::endl; 
     // 1. Check the type annotation (if present): checkExpression(definition.type_expr) 
     // 2. Check the value expression: value_type = checkExpression(definition.value_expr) 
     // 3. Check if value_type is compatible with annotation type 
     // 4. Add symbol to context: context.addSymbol(definition.name.name, resolved_type) 
     return std::make_shared<ChimeraType>(); // Definitions don't typically return a value here 
} 
bool TypeChecker::checkTypeCompatibility(const ChimeraType& expected, const ChimeraType& actual, CheckContext& context) { 
     std::cerr << "TypeChecker::checkTypeCompatibility - STUBBED" << std::endl; 
     // Compare ChimeraType variants and their contents recursively if needed 
     // Use TypeSystem::canImplicitlyConvert for scalar base types 
     return true; // Placeholder 
} 
std::shared_ptr<ChimeraType> TypeChecker::promoteTypesForOperation(const Operator& op, const std::vector<std::shared_ptr<ChimeraType>>& arg_ty
     std::cerr << "TypeChecker::promoteTypesForOperation - STUBBED" << std::endl; 
     // Look up operator, check argument count/types 
     // Use TypeSystem::getPromotedType for base BDI types if applicable 
     // Return the calculated result type 
     return std::make_shared<ChimeraType>(); // Placeholder 
} 
// --- ChimeraType Implementation Stubs --- 
BDIType ChimeraType::getBaseBDIType() const { 
    if (const auto* scalar = std::get_if<ChimeraScalarType>(&content)) { 
        return scalar->base_bdi_type; 
    } else if (const auto* tensor = std::get_if<ChimeraTensorType>(&content)) { 
         return tensor->element_type; 
    }
    // Add cases for other types that might have a base BDI type 
    return BDIType::UNKNOWN; 
} 
bool ChimeraType::operator==(const ChimeraType& other) const { 
     // Need proper implementation comparing variant content recursively 
     return content.index() == other.content.index(); // Basic check for now 
} 
} // namespace chimera::frontend::types 
