#ifndef CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP 
#define CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP 
#include "ChimeraTypes.hpp" 
#include "DSLCoreTypes.hpp" // Need DSL expression structure 
#include <unordered_map> 
#include <string> 
#include <memory> // For expression tree pointers 
namespace chimera::frontend::types { 
using namespace chimera::frontend::dsl; 
// Performs static type checking on Chimera code representations (e.g., ChiAST or ChiIR) 
class TypeChecker::CheckContext { 
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
    // --- Stack Frame Layout --- 
    struct StackFrameInfo { 
    size_t current_offset = 0; // Relative to Frame Pointer (grows downwards typically) 
    size_t max_alignment = 1; 
    size_t total_size = 0; // Calculated size of the frame 
    std::map<std::string, size_t> variable_offsets; // Track offset for each local stack var 
        }; 
        CheckContext(std::shared_ptr<CheckContext> parent = nullptr, int level = 0, bool is_function_scope = false) 
          : parent_scope(parent), scope_level_(level), is_function_scope_(is_function_scope) { 
    if (is_function_scope_) { 
                stack_frame = std::make_unique<StackFrameInfo>(); 
    // Convention: Reserve space for old FP and return address? Depends on ABI. 
            } 
            } 
        }
     else if (parent) { 
                stack_frame = parent->stack_frame; // Inner scopes share frame (or sub-allocate?) 
    // Allocate space within the *current* function's frame 
    size_t allocateStackSpace(const std::string& var_name, size_t size_bytes, size_t alignment); 
    // Get offset for a variable allocated on the stack 
    std::optional<size_t> getStackOffset(const std::string& name) const; 
    // Get total calculated frame size (only valid for function scope context) 
    size_t getFrameSize() const; 
private: 
    // ... symbol_table ... 
    bool is_function_scope_; 
    // Use pointer to share frame info down scope chain, owned by function context 
    std::shared_ptr<StackFrameInfo> stack_frame = nullptr; 
    }; 
    // Recursive checking helpers for different expression types 
    // ... TypeChecker ... 
    std::shared_ptr<ChimeraType> checkLiteral(const DSLLiteral& literal); 
    std::shared_ptr<ChimeraType> checkSymbol(const Symbol& symbol, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkOperation(const DSLOperation& operation, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkSequence(const DSLExpressionSequence& seq, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkDefinition(const DSLDefinition& definition, CheckContext& context); 
    std::shared_ptr<ChimeraType> checkMemberAccess(const DSLMemberAccessExpr&, CheckContext&); 
    std::shared_ptr<ChimeraType> checkArrayIndex(const DSLArrayIndexExpr&, CheckContext&);
    // Type compatibility and promotion logic for ChimeraTypes 
    bool checkTypeCompatibility(const ChimeraType& expected, const ChimeraType& actual, CheckContext& context); 
    std::shared_ptr<ChimeraType> promoteTypesForOperation(const Operator& op, const std::vector<std::shared_ptr<ChimeraType>>& arg_types); 
    // Type registry for user-defined types? 
    // std::unordered_map<std::string, std::shared_ptr<ChimeraType>> user_type_registry_; 
}; 
} // namespace chimera::frontend::types 
#endif // CHIMERA_FRONTEND_TYPES_TYPECHECKER_HPP
