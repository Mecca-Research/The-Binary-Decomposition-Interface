#ifndef IR_IR_IRTOBDI_HPP 
#define IR_IR_IRTOBDI_HPP 
#include "IR.hpp"
 #include "../frontend/api/GraphBuilder.hpp" 
#include "../frontend/dsl/DSLRegistry.hpp" // To get DSL mappers 
#include "../meta/MetadataStore.hpp" // For adding metadata 
namespace chimera::ir { 
// Converts a IR graph into a BDI graph 
class IRToBDI { 
public: 
// Requires GraphBuilder (for target BDI graph), DSLRegistry (for DSL blocks), MetadataStore 
    IRToBDI(bdi::frontend::api::GraphBuilder& builder, 
               frontend::dsl::DSLRegistry& dsl_registry, 
               bdi::meta::MetadataStore& meta_store); 
// Convert a IR graph. Returns entry BDI node ID? Or modifies builder directly. 
bool convertGraph(const IRGraph& ir_graph); 
private: 
    bdi::frontend::api::GraphBuilder& builder_; 
    frontend::dsl::DSLRegistry& dsl_registry_; 
    bdi::meta::MetadataStore& meta_store_; 
// Map IR Node IDs to corresponding BDI Node IDs during conversion 
std::unordered_map<IRNodeId, bdi::core::graph::NodeID> ir_to_bdi_node_map_; 
// Map IR Value Refs to the BDI PortRef producing the value 
std::unordered_map<IRNodeId, std::vector<bdi::core::graph::PortRef>> ir_output_to_bdi_port_map_; 
// Recursive / iterative function to convert IR nodes 
bool convertNode(const IRNode& ir_node); 
// Helper to get the BDI PortRef corresponding to a IRValueRef input 
std::optional<bdi::core::graph::PortRef> getBDIPortRef(const IRValueRef& value_ref); 
}; 
} // namespace chimera::ir 
#endif // IR_IR_IRTOBDI_HPP ---------------------------------------------------------------------------------------------------------
// File: chimera/ir/IRToBDI.cpp (Stubs) 
#include "IRToBDI.hpp"
 #include "../core/graph/OperationTypes.hpp" // BDI Ops 
#include <iostream> 
namespace chimera::ir { 
IRToBDI::IRToBDI(bdi::frontend::api::GraphBuilder& builder, 
                       frontend::dsl::DSLRegistry& dsl_registry,
                       bdi::meta::MetadataStore& meta_store) 
    : builder_(builder), dsl_registry_(dsl_registry), meta_store_(meta_store) {} 
bool IRToBDI::convertGraph(const IRGraph& ir_graph) { 
     std::cerr << "IRToBDI::convertGraph - STUBBED" << std::endl; 
     // 1. Iterate through IR nodes (topologically?) 
     // 2. For each node, call convertNode 
     // 3. After converting all nodes, connect control flow edges in BDI graph based on IR successors 
     return true; // Placeholder 
} 
bool IRToBDI::convertNode(const IRNode& ir_node) { 
     std::cerr << "IRToBDI::convertNode for IR ID " << ir_node.id << " - STUBBED" << std::endl; 
    bdi::core::graph::NodeID bdi_node_id = 0; // ID of the primary BDI node created 
    // --- Determine BDI operation based on IR opcode --- 
    bdi::core::graph::BDIOperationType bdi_op = bdi::core::graph::BDIOperationType::META_NOP; // Default 
    switch (ir_node.opcode) { 
        case IROpCode::LOAD_CONST: 
            bdi_op = bdi::core::graph::BDIOperationType::META_NOP; // Use NOP with payload 
            bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
            // Convert IR constant value (from operation_data) to BDI payload 
            if (const auto* val_var = std::get_if<BDIValueVariant>(&ir_node.operation_data)) { 
                 builder_.setNodePayload(bdi_node_id, ExecutionContext::variantToPayload(*val_var)); 
            } else return false; // Invalid constant node 
            break; 
        case IROpCode::BINARY_OP: { 
             const auto* op = std::get_if<Operator>(&ir_node.operation_data); 
             if (!op) return false; 
             // Map IR operator string to BDI OpType 
             if (op->representation == "+") bdi_op = bdi::core::graph::BDIOperationType::ARITH_ADD; 
             else if (op->representation == "-") bdi_op = bdi::core::graph::BDIOperationType::ARITH_SUB; 
             // ... other ops ... 
             else return false; // Unknown operator 
             bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
             break; 
        } 
        // ... other IR opcodes -> BDI op types ... 
        case IROpCode::DSL_BLOCK: { 
             // Need DSL name and content from operation_data 
             // Get mapper: auto mapper = dsl_registry_.getDSLMapper(dsl_name); 
             // Call mapper: bdi_node_id = mapper->mapToBDI(dsl_content, builder_, control_node); 
             return false; // Stubbed 
        } 
        default: return false; // Unhandled IR opcode 
    }
    if (bdi_node_id == 0) return false; // Failed to create BDI node 
    // Store mapping 
    ir_to_bdi_node_map_[ir_node.id] = bdi_node_id; 
    // Define output ports based on IR output type 
    if (ir_node.output_type && ir_node.output_type->isResolved()) { 
         // Assuming single output port 0 for simplicity 
         builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType(), ir_node.label + "_out"); 
         // Store mapping for this output value 
         ir_output_to_bdi_port_map_[ir_node.id].push_back({bdi_node_id, 0}); 
    }
    // Connect data inputs 
    for (size_t i = 0; i < ir_node.inputs.size(); ++i) { 
        const auto& input_ref = ir_node.inputs[i]; 
        auto bdi_port_ref_opt = getBDIPortRef(input_ref); // Find BDI source port 
        if (!bdi_port_ref_opt) return false; // Failed to find source 
        builder_.connectData(bdi_port_ref_opt.value().node_id, bdi_port_ref_opt.value().port_index, 
                             bdi_node_id, static_cast<PortIndex>(i)); 
    }
    // Add metadata based on annotations 
    // for (const auto& annot : ir_node.annotations) { ... builder_.setNodeMetadata(...) ... } 
    return true; 
} 
std::optional<bdi::core::graph::PortRef> IRToBDI::getBDIPortRef(const IRValueRef& value_ref) { 
     // Find the BDI port corresponding to the IR node's output that produced this value 
     auto map_it = ir_output_to_bdi_port_map_.find(value_ref.node_id); 
     if (map_it != ir_output_to_bdi_port_map_.end()) { 
         const auto& bdi_ports = map_it->second; 
         if (value_ref.output_index < bdi_ports.size()) { 
             return bdi_ports[value_ref.output_index]; // Found the mapping 
         }
     } 
      std::cerr << "IRToBDI Error: Failed to find BDI output port for IR Node " << value_ref.node_id 
                << " Output " << value_ref.output_index << std::endl; 
     return std::nullopt; // Mapping not found 
} 
} // namespace ir::ir 
