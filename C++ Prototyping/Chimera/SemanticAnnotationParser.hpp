#ifndef CHIMERA_FRONTEND_DSL_SEMANTICANNOTATIONPARSER_HPP 
#define CHIMERA_FRONTEND_DSL_SEMANTICANNOTATIONPARSER_HPP 
#include <string> 
#include <vector> 
#include <variant> 
#include <optional> 
// Include necessary BDI/Chimera types that annotations might refer to 
#include "../../core/types/BDITypes.hpp" // For BDIType
 namespace chimera::frontend::dsl { 
// --- Annotation Value Types --- 
using AnnotationValue = std::variant< 
    std::monostate, // Flag annotation (e.g., @Pure) 
    bool, 
    int64_t, 
    double, 
    std::string, 
    bdi::core::types::BDIType // e.g., @TargetType(INT32) 
    // Can add more complex types like lists or maps if needed 
>; 
// Structure to hold a parsed annotation 
struct ParsedAnnotation {
    std::string name; // e.g., "HardwareHint", "ProofGoal", "OptimizeConstraint" 
    std::vector<AnnotationValue> arguments; // Arguments passed to the annotation 
    // Add source location 
}; 
// Parses annotation strings (@Name(arg1, arg2="val")) into structured data 
class SemanticAnnotationParser { 
public: 
    SemanticAnnotationParser() = default; 
    // Parses a single annotation string (e.g., extracted by the main parser) 
    // Returns nullopt on parsing error. 
    std::optional<ParsedAnnotation> parseAnnotationString(const std::string& annotation_str); 
    // Parses multiple annotations often found together (e.g., above a function) 
    std::vector<ParsedAnnotation> parseAnnotationBlock(const std::string& block_str);
 private: 
    // Internal helper methods for parsing logic (e.g., splitting arguments, parsing values) 
    std::optional<AnnotationValue> parseValue(const std::string& value_str); 
}; 
} // namespace chimera::frontend::dsl 
#endif // CHIMERA_FRONTEND_DSL_SEMANTICANNOTATIONPARSER_HPP
