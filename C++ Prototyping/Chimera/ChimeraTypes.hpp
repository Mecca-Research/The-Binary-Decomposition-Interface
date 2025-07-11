#ifndef CHIMERA_FRONTEND_TYPES_CHIMERATYPES_HPP 
#define CHIMERA_FRONTEND_TYPES_CHIMERATYPES_HPP 
#include "../../core/types/BDITypes.hpp" // BDI Base types 
#include <vector> 
#include <string> 
#include <variant> 
#include <memory> // For Function representation? 
// --- Forward Declarations --- 
namespace bdi::meta { struct MetadataVariant; } // Annotation info 
namespace chimera::frontend::types { 
using namespace bdi::core::types; 
// --- High-Level Chimera Type Representation --- 
// Basic scalar value type 
struct ChimeraScalarType { 
    BDIType base_bdi_type; 
    bool is_const = false; 
    // Add potential range constraints? 
    bool operator==(const ChimeraScalarType&) const = default; 
}; 
// Tensor type (example, highly simplified) 
struct ChimeraTensorType { 
    BDIType element_type; 
    std::vector<size_t> shape; // Dimensions 
    std::string layout_hint = "RowMajor"; // e.g., "RowMajor", "ColMajor", "Contiguous" 
    bool is_const = false; 
    // Recurrence info? Update rules? Link to memory region properties? 
    bool operator==(const ChimeraTensorType&) const = default; 
};
// Graph type (represents user-level graphs) 
struct ChimeraGraphType {
 // Need definitions for Node/Edge types, potentially recursive ChimeraType 
// Placeholder for now 
std::string node_type_name; 
std::string edge_type_name; 
bool directed = true; 
bool operator==(const ChimeraGraphType&) const = default; 
}; 
// Memory Region handle type 
struct ChimeraMemoryRegionType { 
// The type of element stored in the region? Or just a handle? 
// Let's assume it's a typed handle for now. 
std::shared_ptr<struct ChimeraType> element_type; // Recursive type definition 
enum class Persistence { Volatile, Persistent, Adaptive } persistence = Persistence::Volatile; 
bool read_only = false; 
// Link to BDI RegionID? Maybe resolved later. 
bool operator==(const ChimeraMemoryRegionType&) const = default; // Need custom compare for shared_ptr 
}; 
// Function type 
struct ChimeraFunctionType { 
std::vector<std::shared_ptr<struct ChimeraType>> argument_types; 
std::shared_ptr<struct ChimeraType> return_type; 
bool is_pure = false; // Annotation hint 
// Proof context? Optimization constraints? 
bool operator==(const ChimeraFunctionType&) const = default; // Need custom compare 
}; 
// Agent type (high-level placeholder) 
struct ChimeraAgentType {
 std::shared_ptr<struct ChimeraType> state_type; // Type describing agent's state structure 
std::shared_ptr<struct ChimeraType> action_type; // Type describing possible actions 
// Links to learning / feedback mechanisms? 
bool operator==(const ChimeraAgentType&) const = default; // Need custom compare 
}; 
// --- Main ChimeraType Variant --- 
// Using std::shared_ptr to handle recursive type definitions (like Function args/return) 
struct ChimeraType {
 using Content = std::variant< 
std::monostate, // Unresolved / Error / Void 
         ChimeraScalarType, 
         ChimeraTensorType, 
         ChimeraGraphType, 
         ChimeraMemoryRegionType, 
         ChimeraFunctionType, 
         ChimeraAgentType 
// Add more types: Structs, Enums, Interfaces/Traits? 
     >; 
     Content content; 
std::string user_defined_name; // For named types (structs, etc.) 
// --- Helper Methods --- 
BDIType getBaseBDIType() const; // Get underlying BDI type if applicable 
bool isScalar() const { return std::holds_alternative<ChimeraScalarType>(content); } 
bool isTensor() const { return std::holds_alternative<ChimeraTensorType>(content); } 
// ... other type checks ... 
bool isResolved() const { return !std::holds_alternative<std::monostate>(content); } 
// Need custom comparison operator due to shared_ptr members in nested types 
bool operator==(const ChimeraType& other) const; 
bool operator!=(const ChimeraType& other) const { return !(*this == other); } 
}; 
} // namespace chimera::frontend::types 
#endif // CHIMERA_FRONTEND_TYPES_CHIMERATYPES_HPP 
