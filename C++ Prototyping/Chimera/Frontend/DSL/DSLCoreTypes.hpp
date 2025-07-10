#ifndef CHIMERA_FRONTEND_DSL_DSLCORETYPES_HPP 
#define CHIMERA_FRONTEND_DSL_DSLCORETYPES_HPP 
#include <string> 
#include <vector> 
#include <variant> 
#include <memory> // For potential smart pointers 
namespace chimera::frontend::dsl { 
// --- Basic DSL Elements --- 
// Represents a symbolic identifier within a DSL 
struct Symbol { 
std::string name; 
// Potentially add source location, scope information 
bool operator==(const Symbol&) const = default; 
}; 
// Represents a literal value within a DSL (distinct from runtime BDIValueVariant) 
// Could hold strings, numbers before type resolution. 
using DSLLiteral = std::variant< 
std::monostate, // Empty 
long long,      
// Integer literal (can represent various sizes initially) 
double,         
bool, 
std::string     
// Floating point literal 
// String literal 
// Add other literal types as needed (e.g., character, byte array) 
>; 
// Represents a basic operator or keyword in a DSL 
struct Operator { 
std::string representation; // e.g., "+", "*", "if", "define" 
// Potentially add arity, precedence, associativity info 
bool operator==(const Operator&) const = default; 
}; 
// --- DSL Structure Definitions --- 
// Forward declaration for recursive structures 
struct DSLExpression; 
// Represents a function call or operation within a DSL expression tree 
struct DSLOperation { 
    Operator op; 
std::vector<std::unique_ptr<DSLExpression>> arguments; 
}; 
// Represents a sequence of expressions (e.g., a block) 
struct DSLExpressionSequence {
 std::vector<std::unique_ptr<DSLExpression>> expressions; 
}; 
// Represents a definition within a DSL (variable, function, type) 
struct DSLDefinition { 
    Symbol name; 
std::unique_ptr<DSLExpression> type_expr; // Optional type annotation 
std::unique_ptr<DSLExpression> value_expr; // The definition body or value 
// Add flags: const, public, etc. 
}; 
// Represents the overall structure of a DSL expression/statement 
// This is a simplified AST node representation for DSL content 
struct DSLExpression { 
using Content = std::variant< 
std::monostate, 
         Symbol, 
         DSLLiteral, 
         DSLOperation, 
         DSLExpressionSequence, 
         DSLDefinition 
// Add other expression types: IfExpr, LambdaExpr, MatchExpr etc. 
     >; 
     Content content; 
// Add source location info (line, column, file) 
// Add potential type annotation handle (resolved later) 
}; 
// --- DSL Definition Elements --- 
// Structure to define the syntax and semantics of a specific DSL operator 
struct DSLOperatorDefinition {
    Operator op; 
// Parameters: Expected argument types/patterns 
// Mapping: How this operator translates to ChiIR or BDI (e.g., function name, BDI op sequence) 
// Type signature: Result type based on input types 
// Constraints: Pre/post conditions 
}; 
// Structure defining a complete DSL 
struct DSLSpecification {
 std::string name; 
// Set of keywords, operators, syntax rules (could be BNF-like) 
std::vector<DSLOperatorDefinition> operator_definitions; 
// Rules for type checking within the DSL 
}; 
// Mapping rules to ChiIR/BDI 
} // namespace chimera::frontend::dsl 
#endif // CHIMERA_FRONTEND_DSL_DSLCORETYPES_HPP 
