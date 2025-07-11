#ifndef CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP 
#define CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP 
#include "ChimeraTypes.hpp" 
#include "../dsl/DSLCoreTypes.hpp" // Need DSL expression structure 
#include <unordered_map> 
#include <string> 
#include <memory> // For expression tree pointers 
namespace chimera::frontend::types { 
using namespace chimera::frontend::dsl; 
// Performs static type checking on Chimera code representations (e.g., ChiAST or ChiIR) 
class TypeChecker { 
public: 
// Context for type checking (symbol tables, scope info) 
struct CheckContext {
// Map symbol names to their resolved ChimeraType within current scope 
std::unordered_map<std::string, std::shared_ptr<ChimeraType>> symbol_table; 
std::shared_ptr<CheckContext> parent_scope = nullptr; // For nested scopes
        // Expected return type (for checking function bodies) 
        std::shared_ptr<ChimeraType> expected_return_type = nullptr; 
        // Lookup symbol resolving through parent scopes 
        std::shared_ptr<ChimeraType> lookupSymbol(const std::string& name) const; 
        // Add symbol to current scope 
        void addSymbol(const std::string& name, std::shared_ptr<ChimeraType> type); 
    }; 
    TypeChecker() = default; // Potentially load base types/functions 
    // Check a DSL expression tree (or ChiAST/ChiIR node) within a given context 
    // Returns the resolved ChimeraType of the expression, or an unresolved type on error. 
    std::shared_ptr<ChimeraType> checkExpression(const DSLExpression* expr, CheckContext& context); 
    // Check a function definition 
    bool checkFunctionDefinition(/* Function definition structure */ CheckContext& parent_context); 
    // Check a whole module/file 
    bool checkModule(/* Module structure */); 
private: 
    // Recursive checking helpers for different expression types 
    std::shared_ptr<ChimeraType> checkLiteral(const DSLLiteral& literal); 
    std::shared_ptr<ChimeraType> checkSymbol(const Symbol& symbol, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkOperation(const DSLOperation& operation, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkSequence(const DSLExpressionSequence& seq, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkDefinition(const DSLDefinition& definition, CheckContext& context); 
    // Type compatibility and promotion logic for ChimeraTypes 
    bool checkTypeCompatibility(const ChimeraType& expected, const ChimeraType& actual, CheckContext& context); 
    std::shared_ptr<ChimeraType> promoteTypesForOperation(const Operator& op, const std::vector<std::shared_ptr<ChimeraType>>& arg_types); 
    // Type registry for user-defined types? 
    // std::unordered_map<std::string, std::shared_ptr<ChimeraType>> user_type_registry_; 
}; 
} // namespace chimera::frontend::types 
#endif // CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP
