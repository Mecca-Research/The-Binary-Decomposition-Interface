#ifndef CHIMERA_FRONTEND_DSL_MACROENGINE_HPP 
#define CHIMERA_FRONTEND_DSL_MACROENGINE_HPP 
#include <string> 
#include <vector> 
#include <any> // For representing parsed syntax elements generically 
namespace chimera::frontend::dsl { 
// Represents parsed macro definition or invocation data 
struct MacroSyntaxElement { 
// Content depends on the parser's output for macros 
// Could be a list of tokens, a dedicated AST structure, etc. 
std::any data; 
}; 
// Interface for the Macro Expansion Engine 
// Likely implemented in Rust for safety and advanced macro features (hygiene). 
class MacroEngine { 
public: 
virtual ~MacroEngine() = default; 
// Define a new macro 
virtual bool defineMacro(const std::string& name, const MacroSyntaxElement& definition) = 0; 
// Expand a macro invocation 
// Takes the invocation name and arguments (as syntax elements) 
// Returns a list of syntax elements resulting from the expansion, 
// or an empty list/error indicator on failure. 
virtual std::vector<MacroSyntaxElement> expandMacro(const std::string& name, const std::vector<MacroSyntaxElement>& arguments) = 0; 
// Potential methods for declarative DSL definition via macros 
// virtual bool defineDSLFromMacro(const MacroSyntaxElement& dsl_definition) = 0; 
// Check if macro exists 
virtual bool isMacroDefined(const std::string& name) const = 0; 
}; 
// --- Potential C++ Facade for a Rust Implementation --- 
// class RustMacroEngineFacade : public MacroEngine { 
// public:
 //    // Constructor loads Rust library? 
//    // Methods forward calls to Rust via C FFI (e.g., using cbindgen, cxx) 
//    bool defineMacro(const std::string& name, const MacroSyntaxElement& definition) override; 
//    std::vector<MacroSyntaxElement> expandMacro(const std::string& name, const std::vector<MacroSyntaxElement>& arguments) override; 
//    bool isMacroDefined(const std::string& name) const override; 
// private: 
//    // Handle to Rust object or FFI functions 
// };
 } // namespace chimera::frontend::dsl 
#endif // CHIMERA_FRONTEND_DSL_MACROENGINE_HPP 
