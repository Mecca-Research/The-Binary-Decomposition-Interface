#ifndef CHIMERA_FRONTEND_DSL_DSLREGISTRY_HPP 
#define CHIMERA_FRONTEND_DSL_DSLREGISTRY_HPP 
#include "DSLCoreTypes.hpp" 
#include <string> 
#include <unordered_map> 
#include <memory> // For unique_ptr or shared_ptr 
#include <optional> 
#include <shared_mutex> // For thread-safe access if dynamic loading occurs 
// Forward declare the mapper interface (belongs likely in chimera/ir/) 
namespace bdi::frontend::api { class GraphBuilder; } // Forward BDI builder 
namespace chimera::ir { class ChiIRNode; } // Forward ChiIR node 
namespace chimera::frontend::dsl { 
// Interface for mapping a recognized DSL construct to ChiIR or directly to BDI 
class IDSLMapper { 
public: 
virtual ~IDSLMapper() = default; 
// Option 1: Map to ChiIR (preferred for high-level opts) 
// virtual std::unique_ptr<chimera::ir::ChiIRNode> mapToChiIR(const DSLExpression& dsl_expr) = 0; 
// Option 2: Map directly to BDI (simpler initially, less opt potential) 
virtual bdi::core::graph::NodeID mapToBDI(const DSLExpression& dsl_expr, 
                                             bdi::frontend::api::GraphBuilder& builder, 
                                             bdi::core::graph::NodeID& current_control_node) = 0; 
}; 
// Manages defined DSLs and their associated mappers 
class DSLRegistry { 
public: 
    DSLRegistry() = default; 
// Register a DSL specification and its associated mapper 
bool registerDSL(std::shared_ptr<DSLSpecification> dsl_spec, std::shared_ptr<IDSLMapper> mapper); 
// Get the specification for a named DSL 
std::shared_ptr<const DSLSpecification> getDSLSpecification(const std::string& dsl_name) const; 
// Get the mapper for a named DSL 
std::shared_ptr<IDSLMapper> getDSLMapper(const std::string& dsl_name) const; 
// Check if a DSL is registered 
bool isRegistered(const std::string& dsl_name) const; 
// TODO: Methods for dynamic loading/unloading? 
private: 
mutable std::shared_mutex registry_mutex_; // For thread safety 
std::unordered_map<std::string, std::shared_ptr<DSLSpecification>> specifications_; 
std::unordered_map<std::string, std::shared_ptr<IDSLMapper>> mappers_; 
}; 
} // namespace chimera::frontend::dsl 
#endif // CHIMERA_FRONTEND_DSL_DSLREGISTRY_HPP 
