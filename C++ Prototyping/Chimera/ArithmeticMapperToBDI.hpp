#include "DSLCoreTypes.hpp" // For IDSLSpecificASTNode 
namespace chimera::frontend::dsl { 
// ... ArithmeticExpr definition (now inherits IDSLSpecificASTNode) ... 
class ArithmeticMapper : public IDSLMapper { 
public: 
// Implement the updated mapToBDI 
    bdi::core::graph::NodeID mapToBDI(const IDSLSpecificASTNode* dsl_node, 
                                     bdi::frontend::api::GraphBuilder& builder, 
                                     bdi::core::graph::NodeID& current_control_node) override; 
bool handlesNodeType(const std::string& node_type_name) const override { 
return node_type_name == "ArithmeticExpr"; 
    }
 private: 
// Recursive helper now takes specific type 
     bdi::core::graph::NodeID mapExpression(const ArithmeticExpr* expr, 
                                           bdi::frontend::api::GraphBuilder& builder, 
                                           bdi::core::graph::NodeID& current_control_node); 
};
} // namespace chimera::frontend::dsl 
