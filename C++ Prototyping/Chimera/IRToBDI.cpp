#include "ChiIRToBDI.hpp"
#include "OperationTypes.hpp" 
#include "ExecutionContext.hpp" // For payload conversions 
#include <iostream> 
#include <stdexcept>
#include <vector> 
#include <map> // For variable address mapping 
namespace chimera::ir { 
// --- ChiIRToBDI --- 
ChiIRToBDI::ChiIRToBDI(bdi::frontend::api::GraphBuilder& builder, 
                       frontend::dsl::DSLRegistry& dsl_registry, 
                       bdi::meta::MetadataStore& meta_store) 
    : builder_(builder), dsl_registry_(dsl_registry), meta_store_(meta_store) {} 
bool ChiIRToBDI::convertGraph(const ChiIRGraph& chiir_graph) { 
     chiir_to_bdi_node_map_.clear(); 
     chiir_output_to_bdi_port_map_.clear(); 
     current_bdi_cfg_node_ = 0; // Track last BDI control flow node 
     variable_address_nodes_.clear(); // Track BDI nodes holding variable addresses 
// --- Topological Sort or DFS Traversal Required --- 
// Simple iterative approach for demonstration - WILL FAIL ON COMPLEX FLOW 
std::vector<ChiIRNodeId> processing_order; 
std::map<ChiIRNodeId, const ChiIRNode*> node_map; // Need quick lookup 
for(const auto& pair : chiir_graph.nodes_) { // Use nodes_ direct member access 
         node_map[pair.first] = pair.second.get(); 
         processing_order.push_back(pair.first); // Naive order 
     } 
// TODO: Replace processing_order with actual topological sort result 
// --- Convert Nodes --- 
for(ChiIRNodeId chiir_id : processing_order) { 
const ChiIRNode* chiir_node_ptr = node_map[chiir_id]; 
if (!chiir_node_ptr) { 
throw BDIExecutionError("Internal Error: ChiIR node missing during conversion"); 
         }
 if (!convertNode(*chiir_node_ptr)) { 
std::cerr << "ChiIRToBDI Error: Failed to convert ChiIR Node " << chiir_id << std::endl; 
return false; 
         }
     } 
// --- Connect Control Flow --- 
// Iterate again now that all nodes are mapped 
for(ChiIRNodeId chiir_id : processing_order) { 
const ChiIRNode* chiir_node_ptr = node_map[chiir_id]; 
         NodeID bdi_source_node = chiir_to_bdi_node_map_[chiir_id]; 
// Connect based on specific ChiIR opcode logic 
switch(chiir_node_ptr->opcode) { 
case ChiIROpCode::JUMP: { 
// Get target ChiIR ID from operation_data (assuming stored there) 
// ChiIRNodeId chiir_target = std::get<ChiIRNodeId>(chiir_node_ptr->operation_data); 
// NodeID bdi_target = chiir_to_bdi_node_map_[chiir_target]; 
// builder_.connectControl(bdi_source_node, bdi_target); 
break; // Needs ChiIR node definition update 
             } 
case ChiIROpCode::BRANCH_COND: { 
// Get true/false target ChiIR IDs from operation_data 
// auto targets = std::get<std::pair<ChiIRNodeId, ChiIRNodeId>>(chiir_node_ptr->operation_data); 
// NodeID bdi_true_target = chiir_to_bdi_node_map_[targets.first]; 
// NodeID bdi_false_target = chiir_to_bdi_node_map_[targets.second]; 
// auto* bdi_branch_node = builder_.getGraph().getNodeMutable(bdi_source_node); 
// bdi_branch_node->control_outputs = {bdi_true_target, bdi_false_target}; // Set explicitly 
                  break; // Needs ChiIR node definition update 
             } 
             // Handle CALL/RETURN sequence linking? More complex. 
             default: { 
             // Default: Connect to BDI node mapped from the *first* successor in ChiIR CFG (if any) 
             // if (!chiir_node_ptr->control_successors.empty()) { 
                  //     NodeID bdi_target = chiir_to_bdi_node_map_[chiir_node_ptr->control_successors[0]]; 
                  //     builder_.connectControl(bdi_source_node, bdi_target); 
                  // } 
                  break; // Needs explicit successors in ChiIRNode 
             } 
         }
     } 
     return true; 
} 
bool ChiIRToBDI::convertNode(const ChiIRNode& chiir_node) { 
     if (chiir_to_bdi_node_map_.count(chiir_node.id)) return true; // Already processed 
     NodeID bdi_node_id = 0; 
     BDIOperationType bdi_op = BDIOperationType::META_NOP; 
     bool connect_standard_control_flow = true; // Assume standard sequential flow unless overridden 
     // --- Map ChiIR Opcode to BDI Op --- 
     switch (chiir_node.opcode) { 
        case ChiIROpCode::ENTRY: bdi_op = BDIOperationType::META_START; break; 
        case ChiIROpCode::EXIT: bdi_op = BDIOperationType::META_END; break; 
        case ChiIROpCode::PARAM: // Represents function parameter input 
            bdi_op = BDIOperationType::META_NOP; // Value loaded by META_START, PARAM node itself might be optimized out later 
            // Or map to a specific LOAD_PARAM BDI op if exists 
            break; 
        case ChiIROpCode::RETURN_VALUE: 
            bdi_op = BDIOperationType::CTRL_RETURN; break; 
        case ChiIROpCode::LOAD_CONST: 
            bdi_op = BDIOperationType::META_CONST; break; 
        case ChiIROpCode::LOAD_SYMBOL: { 
             const auto* symbol = std::get_if<Symbol>(&chiir_node.operation_data); 
             if (!symbol) return false; 
             auto addr_it = variable_address_nodes_.find(symbol->name); 
             if (addr_it == variable_address_nodes_.end()) { 
                 throw BDIExecutionError("Compiler Error: Address for variable '" + symbol->name + "' not found during LOAD."); 
             } 
             bdi_op = BDIOperationType::MEM_LOAD; 
             // Need to create a node providing the address first 
             NodeID addr_const_node = builder_.addNode(BDIOperationType::META_CONST); // Use const node for address 
             // builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); // Store address in payload 
             // TODO: How to get actual address? Need runtime allocation info or stack offset. For now, store BDI ID of ALLOC node. 
             builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); // Store ALLOC node ID 
             builder_.defineDataOutput(addr_const_node, 0, BDIType::NODE_ID); // Output is NodeID for address lookup later 
             // The actual LOAD node will take input from addr_const_node 
             bdi_node_id = builder_.addNode(bdi_op, chiir_node.label); 
             builder_.connectData(addr_const_node, 0, bdi_node_id, 0); // Connect address source 
             // Control flow: Need addr const first 
             if (current_bdi_cfg_node_ != 0) builder_.connectControl(current_bdi_cfg_node_, addr_const_node); 
             builder_.connectControl(addr_const_node, bdi_node_id); 
             connect_standard_control_flow = false; // Manual control flow handled 
             break; 
        } 
        case ChiIROpCode::ALLOC_MEM: { 
             bdi_op = BDIOperationType::MEM_ALLOC; 
             bdi_node_id = builder_.addNode(bdi_op, chiir_node.label); 
             // Set size input? Assumes size is determined and passed as input ChiIR node 
             if (chiir_node.inputs.size() >= 1) { 
                 auto bdi_port_ref_opt = getBDIPortRef(chiir_node.inputs[0]); 
                 if (!bdi_port_ref_opt) return false; 
                 builder_.connectData(bdi_port_ref_opt.value().node_id, bdi_port_ref_opt.value().port_index, bdi_node_id, 0); 
             } else { /* Error: Alloc needs size */ return false; } 
             // Define output (pointer/address) 
             if (chiir_node.output_type) builder_.defineDataOutput(bdi_node_id, 0, chiir_node.output_type->getBaseBDIType()); 
             // Store mapping from variable name (if available in op_data) to this alloc node's ID 
             // if (const auto* symbol = std::get_if<Symbol>(&chiir_node.operation_data)) { 
             //     variable_address_nodes_[symbol->name] = bdi_node_id; // Store BDI ID of node producing address 
             // } 
             break; 
        } 
         case ChiIROpCode::STORE_MEM: { // Assignment 
             bdi_op = BDIOperationType::MEM_STORE; 
             bdi_node_id = builder_.addNode(bdi_op, chiir_node.label); 
             if (chiir_node.inputs.size() != 2) return false; // Expect Addr, Value 
             // Input 0: Address (Lookup symbol address) 
             const auto* symbol = std::get_if<Symbol>(&chiir_node.operation_data); // Assume symbol stored here 
             if (!symbol) return false; 
             auto addr_it = variable_address_nodes_.find(symbol->name); 
             if (addr_it == variable_address_nodes_.end()) throw BDIExecutionError("Compiler Error: Address for variable '" + symbol->name + "
             // Create const node for address ID 
             NodeID addr_const_node = builder_.addNode(BDIOperationType::META_CONST); 
             builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); 
             builder_.defineDataOutput(addr_const_node, 0, BDIType::NODE_ID); 
              if (current_bdi_cfg_node_ != 0) builder_.connectControl(current_bdi_cfg_node_, addr_const_node); // Control flow for addr const 
             // Input 1: Value 
             auto bdi_value_port_ref_opt = getBDIPortRef(chiir_node.inputs[1]); 
if (!bdi_value_port_ref_opt) return false; 
// Connect inputs to STORE node 
             builder_.connectData(addr_const_node, 0, bdi_node_id, 0); // Address input 
             builder_.connectData(bdi_value_port_ref_opt.value().node_id, bdi_value_port_ref_opt.value().port_index, bdi_node_id, 1); // Value
// Control flow: Address & Value must be computed first 
              builder_.connectControl(addr_const_node, bdi_node_id); 
// Need control flow from value source too! Requires better CFG tracking. 
// builder_.connectControl(bdi_value_port_ref_opt.value().node_id, bdi_node_id); // Simple assumption 
              connect_standard_control_flow = false; 
break;
         }
case ChiIROpCode::BINARY_OP: { /* ... Map operator to BDI OpType ... */ } break; 
case ChiIROpCode::UNARY_OP: { /* ... Map operator to BDI OpType ... */ } break; 
case ChiIROpCode::JUMP: bdi_op = BDIOperationType::CTRL_JUMP; connect_standard_control_flow = false; break; // Control connected late
case ChiIROpCode::BRANCH_COND: bdi_op = BDIOperationType::CTRL_BRANCH_COND; connect_standard_control_flow = false; break; // Control 
case ChiIROpCode::CALL: bdi_op = BDIOperationType::CTRL_CALL; break; // Args connected, control connected later 
case ChiIROpCode::DSL_BLOCK: { /* ... Dispatch to DSLMapper ... */ return false; /* Stubbed */ } 
default: throw BDIExecutionError("Unhandled ChiIR Opcode during BDI conversion"); 
     } 
// Create node if not already created by specific logic above 
if (bdi_node_id == 0) { 
         bdi_node_id = builder_.addNode(bdi_op, chiir_node.label); 
     } 
     chiir_to_bdi_node_map_[
 chiir_node.id] = bdi_node_id; // Store mapping 
// Define Output Port (if not already done) 
if (chiir_node.output_type && chiir_node.output_type->isResolved() && chiir_node.output_type->getBaseBDIType() != BDIType::VOID) { 
auto* bdi_node = builder_.getGraph().getNodeMutable(bdi_node_id); 
if (bdi_node && bdi_node->data_outputs.empty()) { // Avoid redefining if already done 
             builder_.defineDataOutput(bdi_node_id, 0, chiir_node.output_type->getBaseBDIType()); 
// Ensure mapping exists 
if(chiir_output_to_bdi_port_map_[
 chiir_node.id].empty()) chiir_output_to_bdi_port_map_[
             chiir_output_to_bdi_port_map_[
 chiir_node.id][0] = {bdi_node_id, 0}; 
          } 
      } 
chiir_node.id].resize(1); 
// Connect Data Inputs (if not handled specifically above) 
if (bdi_op != BDIOperationType::MEM_LOAD && bdi_op != BDIOperationType::MEM_STORE) { // Avoid re-connecting 
for (size_t i = 0; i < chiir_node.inputs.size(); ++i) { /* ... Connect using getBDIPortRef ... */ } 
     } 
// Connect standard sequential control flow 
if (connect_standard_control_flow && current_bdi_cfg_node_ != 0) { 
         builder_.connectControl(current_bdi_cfg_node_, bdi_node_id); 
     } 
if (bdi_op != BDIOperationType::CTRL_JUMP && bdi_op != BDIOperationType::CTRL_BRANCH_COND && bdi_op != BDIOperationType::CTRL_RETURN && b
        current_bdi_cfg_node_ = bdi_node_id; // Update position unless it's a flow terminator 
     } 
return true; 
} 
// ... getBDIPortRef ... 
} // namespace ir::ir 
