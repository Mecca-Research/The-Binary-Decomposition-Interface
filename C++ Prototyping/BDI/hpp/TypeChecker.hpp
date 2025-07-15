#include "BDITypes.hpp"
#include "BDITypes.hpp"
#include "BinaryEncoding.hpp" // Placeholder
#include <vector>
#include <numeric> // For std::common_type 
#include <cstddef>
#include <stdexcept>
#include <concepts>
#include <bit> // For std::bit_cast
#include <cstring> // For memcpy fallback
#include <array> // For std::bit_cast target
#include <type_traits> // For std::is_same_v
#include <map> // Use map for ordered iteration maybe? Or stick to unordered_map 
namespace chimera::frontend::types { 
// ... ChimeraType ... 
// --- Symbol Information --- 
struct SymbolInfo { 
std::shared_ptr<ChimeraType> type = nullptr; 
bool is_mutable = false; 
bool is_parameter = false; 
int scope_level = 0; // Helps differentiate shadowed variables 
// Location Info (relative to scope/frame) 
enum class Location { UNKNOWN, STACK, HEAP, REGISTER, SSA_VALUE } location = Location::UNKNOWN; 
size_t stack_offset = 0;         
// Offset from frame pointer if on stack 
    ir::IRNodeId value_node_id = 0; // ID of IR node producing the value (for immutable/SSA) 
    ir::IRNodeId address_node_id = 0;// ID of IR node producing the address (ALLOC/PARAM) 
    SymbolInfo(std::shared_ptr<ChimeraType> t, bool mut = false, bool param = false, int scope = 0) 
        : type(t), is_mutable(mut), is_parameter(param), scope_level(scope) {} 
}; 
// Context for type checking & symbol management 
class TypeChecker::CheckContext { 
public: 
    CheckContext(std::shared_ptr<CheckContext> parent = nullptr, int level = 0) 
      : parent_scope(parent), scope_level_(level) {} 
std::shared_ptr<SymbolInfo> lookupSymbolInfo(const std::string& name, bool current_scope_only = false) const; 
bool addSymbol(const std::string& name, SymbolInfo info); // Returns false on redeclaration in same scope 
std::shared_ptr<CheckContext> parent_scope = nullptr; 
std::shared_ptr<ChimeraType> expected_return_type = nullptr; 
    int getScopeLevel() const { return scope_level_; } 
    // --- Stack Layout (Conceptual) --- 
    size_t current_stack_offset = 0; // Track current offset for allocation 
    size_t allocateStackSpace(size_t size_bytes, size_t alignment = 8); // Returns offset 
private: 
    std::unordered_map<std::string, SymbolInfo> symbol_table; // Name -> Info 
    int scope_level_; 
}; 
// ... TypeChecker class declaration ... 
} // namespace chimera::frontend::types 
    
      // File: chimera/frontend/types/TypeChecker.cpp (SymbolInfo Context Implementation) 
namespace chimera::frontend::types { 
// --- CheckContext Methods --- 
std::shared_ptr<SymbolInfo> TypeChecker::CheckContext::lookupSymbolInfo(const std::string& name, bool current_scope_only) const { 
    auto it = symbol_table.find(name); 
    if (it != symbol_table.end()) { 
        // Return shared_ptr to modifiable copy? Or const ref? Return copy for safety. 
        return std::make_shared<SymbolInfo>(it->second); 
    } else if (parent_scope && !current_scope_only) { 
        return parent_scope->lookupSymbolInfo(name, false); // Recursive lookup 
    } else { 
        return nullptr; // Symbol not found 
    }
 } 
bool TypeChecker::CheckContext::addSymbol(const std::string& name, SymbolInfo info) { 
    if (symbol_table.count(name)) { 
         // Allow shadowing from parent, but not redeclaration in same scope 
         // Check scope level? For now, allow overwrite within scope too easily. 
         // throw BDIExecutionError("Type Error: Redeclaration of symbol '" + name + "' in the same scope."); 
         // return false; 
    }
    symbol_table[name] = std::move(info); 
    return true; 
} 
size_t TypeChecker::CheckContext::allocateStackSpace(size_t size_bytes, size_t alignment) { 
    // Align current offset UP to the required alignment 
    size_t aligned_offset = (current_stack_offset + alignment - 1) / alignment * alignment; 
    current_stack_offset = aligned_offset + size_bytes; 
    // In a real compiler, manage max stack size per function etc. 
    return aligned_offset; 
} 
// ... TypeChecker methods using SymbolInfo ... 
std::shared_ptr<ChimeraType> TypeChecker::checkSymbol(const Symbol& symbol, CheckContext& context) { 
    auto info = context.lookupSymbolInfo(symbol.name); 
    if (!info) { 
         throw BDIExecutionError("Type Error: Undefined symbol '" + symbol.name + "'"); 
    }
    return info->type; // Return the resolved type 
} 
// Modify checkDefinition and checkAssignment to use/update SymbolInfo 
std::shared_ptr<ChimeraType> TypeChecker::checkDefinition(const DSLDefinition& def, CheckContext& context) { 
    // ... check value_type, annotation_type ... 
    bool is_mutable = true; // Determine mutability based on 'let'/'var' or annotations 
    SymbolInfo info(final_type, is_mutable, false, context.getScopeLevel()); 
    // ... potentially calculate stack offset here if type size known ... 
    // info.location = SymbolInfo::Location::STACK; 
    // info.stack_offset = context.allocateStackSpace( getChimeraTypeSize(final_type) ); // Need size function 
    if (!context.addSymbol(def.name.name, info)) { 
        // Handle redeclaration error 
    }
    return make_scalar(BDIType::VOID); 
} 
// ... (checkAssignment needs to lookup SymbolInfo, check mutability) ... 
} // namespace chimera::frontend::types 
