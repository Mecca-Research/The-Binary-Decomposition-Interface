#include "IRToBDI.hpp"
#include "OperationTypes.hpp" 
#include "ExecutionContext.hpp" // For payload conversions 
#include <iostream> 
#include <stdexcept>
#include <vector> 
#include <map> // For variable address mapping 
namespace ir::ir { 
// Need class member to track variable address sources 
// std::unordered_map<std::string, bdi::core::graph::NodeID> variable_address_bdi_nodes_; 
// Need function entry point map 
// std::unordered_map<std::string, bdi::core::graph::NodeID> function_entry_bdi_nodes_; 
// --- IRToBDI --- 
IRToBDI::IRToBDI(bdi::frontend::api::GraphBuilder& builder, 
                       frontend::dsl::DSLRegistry& dsl_registry, 
                       bdi::meta::MetadataStore& meta_store) 
    : builder_(builder), dsl_registry_(dsl_registry), meta_store_(meta_store) {} 
bool IRToBDI::convertGraph(const IRGraph& ir_graph) { 
     // ... clear maps ... 
     ir_to_bdi_node_map_.clear(); 
     ir_output_to_bdi_port_map_.clear(); 
     current_bdi_cfg_node_ = 0; // Track last BDI control flow node 
     variable_address_nodes_.clear(); // Track BDI nodes holding variable addresses 
// Simple iterative approach for demonstration - WILL FAIL ON COMPLEX FLOW 
// --- Must process nodes in an order respecting dependencies (Topological or iterative) --- 
// Perform a traversal (e.g., DFS from entry) to ensure nodes are created before use 
std::vector<IRNodeId> processing_order; 
std::map<IRNodeId, const IRNode*> node_map; // Need quick lookup 
for(const auto& pair : ir_graph.nodes_) { // Use nodes_ direct member access 
         node_map[pair.first] = pair.second.get(); 
         processing_order.push_back(pair.first); // Naive order 
std::set<IRNodeId> visited;
std::function<void(IRNodeId)> dfs_visit = 
         [&](IRNodeId current_id) { 
if (visited.count(current_id)) return; 
const IRNode* node = ir_graph.getNode(current_id); 
if (!node) return; // Should not happen 
         visited.insert(current_id); 
// Visit dependencies first (simplistic - ignores complex CFG for now) 
for (const auto& input_ref : node->inputs) { 
             dfs_visit(input_ref.node_id); 
         }
 // Visit control flow successors? Depends on desired order. Post-order usually better. 
for (const auto& succ_id : node->control_successors) { 
             dfs_visit(succ_id); 
         }
         processing_order.push_back(current_id); // Add in post-order 
     }; 
if (auto entry_id = ir_graph.getEntryNode()) { 
         dfs_visit(*entry_id); 
// Ensure all nodes were reached? Might need multiple entry points or graph analysis. 
// Add remaining nodes not visited (error case or multiple components?) 
for (const auto& pair : ir_graph.nodes_) { // Use nodes_ member 
if (visited.find(pair.first) == visited.end()) { 
// Process unreachable nodes? Or error? Let's process them naively. 
                  dfs_visit(pair.first);
         }
 std::reverse(processing_order.begin(), processing_order.end()); // Reverse for processing order (approx topo sort) 
} 
else { /* Error */ return false; } 
         } 
// TODO: Replace processing_order with actual topological sort result 
// --- Convert Nodes --- 
for(IRNodeId ir_id : processing_order) { /* ... convertNode call ... */ }
const IRNode* ir_node_ptr = node_map[ir_id]; 
if (!ir_node_ptr) { 
throw BDIExecutionError("Internal Error: IR node missing during conversion"); 
         }
 if (!convertNode(*ir_node_ptr)) { 
std::cerr << "IRToBDI Error: Failed to convert IR Node " << ir_id << std::endl; 
return false; 
         }
// --- Convert Nodes in determined order --- 
for(IRNodeId ir_id : processing_order) { 
const IRNode* ir_node_ptr = ir_graph.getNode(ir_id); 
if (!ir_node_ptr || !convertNode(*ir_node_ptr)) { /* Error */ return false; }  
if (!ir_node_ptr || !ir_to_bdi_node_map_.count(ir_id)) continue; 
     }
} 
// --- Connect Control Flow --- 
// Iterate again now that all nodes are mapped 
for(IRNodeId ir_id : processing_order) { 
const IRNode* ir_node_ptr = ir_graph.getNode(ir_id); 
if (!ir_node_ptr || !ir_to_bdi_node_map_.count(ir_id)) continue; 
NodeID bdi_source_node = ir_to_bdi_node_map_[ir_id];   
const IRNode* ir_node_ptr = node_map[ir_id]; 
NodeID bdi_source_node = ir_to_bdi_node_map_[ir_id];
BDINode* bdi_source_node_ptr = builder_.getGraph().getNodeMutable(bdi_source_node); // Need mutable graph access   
// Connect based on specific IR opcode logic 
switch(ir_node_ptr->opcode) { 
case IROpCode::JUMP: { 
if (const auto* target_id = std::get_if<IRNodeId>(&ir_node_ptr->operation_data)) { 
if (ir_to_bdi_node_map_.count(*target_id)) { 
builder_.connectControl(bdi_source_node, ir_to_bdi_node_map_[*target_id]); 
} 
else { /* Error: Target not converted */ return false; } 
} 
else { /* Error: Jump target missing */ return false; } 
break; 
             } 
case IROpCode::BRANCH_COND: { 
                  auto* bdi_branch_node = builder_.getGraph().getNodeMutable(bdi_source_node); 
                  if (const auto* targets = std::get_if<std::pair<IRNodeId, IRNodeId>>(&ir_node_ptr->operation_data)) { 
                      if (ir_to_bdi_node_map_.count(targets->first) && ir_to_bdi_node_map_.count(targets->second)) { 
                         bdi_branch_node->control_outputs = { ir_to_bdi_node_map_.at(targets->first), ir_to_bdi_node_map_.at(targets->se
                      } else return false; // Target not mapped 
                  } else return false; // Branch target data missing 
                  break; 
             } 
             // CALL, RETURN, EXIT don't have standard outgoing edges connected here 
             case IROpCode::CALL: case IROpCode::RETURN: case IROpCode::RETURN_VALUE: case IROpCode::EXIT: break; 
             default: { // Default sequential flow 
                 if (!ir_node_ptr->control_successors.empty()) { 
                     IRNodeId ir_target = ir_node_ptr->control_successors[0]; 
                     if (ir_to_bdi_node_map_.count(ir_target)) { 
                          builder_.connectControl(bdi_source_node, ir_to_bdi_node_map_.at(ir_target)); 
                     } // Else: Successor might be terminator, handled implicitly 
                 } 
// Targets now stored explicitly in BDI node's control_outputs by convertNode 
// No connection needed here, connection happens *from* predecessors *to* this branch node. 
                  break; 
             } 
             case IROpCode::CALL: // Standard sequential flow assumed after call returns 
             case IROpCode::RETURN: // Terminator 
             case IROpCode::EXIT: // Terminator 
                  // No standard outgoing control flow edge needed here 
                  break; 
             default: { // Default sequential flow 
                 // Connect to the BDI node corresponding to the *first* IR control successor 
                 if (!ir_node_ptr->control_successors.empty()) { 
                     IRNodeId ir_target = ir_node_ptr->control_successors[0]; 
                     if (ir_to_bdi_node_map_.count(ir_target)) { 
                          builder_.connectControl(bdi_source_node, ir_to_bdi_node_map_[ir_target]); 
                     } else { /* Error: Successor not converted */ return false; } 
                 }            // Else: No successor, likely graph exit path 
// Get target IR ID from operation_data (assuming stored there) 
// IRNodeId ir_target = std::get<IRNodeId>(ir_node_ptr->operation_data); 
// NodeID bdi_target = ir_to_bdi_node_map_[ir_target]; 
// builder_.connectControl(bdi_source_node, bdi_target); 
// Get true/false target IR IDs from operation_data 
// auto targets = std::get<std::pair<IRNodeId, IRNodeId>>(ir_node_ptr->operation_data); 
// NodeID bdi_true_target = ir_to_bdi_node_map_[targets.first]; 
// NodeID bdi_false_target = ir_to_bdi_node_map_[targets.second]; 
// auto* bdi_branch_node = builder_.getGraph().getNodeMutable(bdi_source_node); 
// bdi_branch_node->control_outputs = {bdi_true_target, bdi_false_target}; // Set explicitly 
                  break; // Needs IR node definition update 
             } 
             // Handle CALL/RETURN sequence linking? More complex. 
             default: { 
             // Default: Connect to BDI node mapped from the *first* successor in IR CFG (if any) 
             // if (!ir_node_ptr->control_successors.empty()) { 
                  //     NodeID bdi_target = ir_to_bdi_node_map_[ir_node_ptr->control_successors[0]]; 
                  //     builder_.connectControl(bdi_source_node, bdi_target); 
                  // } 
                  break; // Needs explicit successors in IRNode 
             } 
         }
     } 
     return true; 
} 
bool IRToBDI::convertNode(const IRNode& ir_node) { 
     if (ir_to_bdi_node_map_.count(ir_node.id)) return true; // Already processed 
     NodeID bdi_node_id = 0; 
     BDIOperationType bdi_op = BDIOperationType::META_NOP; 
     bool connect_standard_control_flow = true; // Assume standard sequential flow unless overridden 
     // --- Map IR Opcode to BDI Op --- 
     switch (ir_node.opcode) { 
        case IROpCode::ENTRY: bdi_op = BDIOperationType::META_START; break; 
        // If it's a function entry, store its BDI ID 
        // if (/* is function entry? check context */ true) { 
        //     function_entry_bdi_nodes_[/* function name */] = bdi_node_id; 
        // } 
        case IROpCode::EXIT: bdi_op = BDIOperationType::META_END; break; 
        case IROpCode::PARAM: // Represents function parameter input, Load parameter value (passed via context or registers?) 
            // For now, map to NOP; value loaded by caller/START node. 
            bdi_op = BDIOperationType::META_NOP; // Value loaded by META_START, PARAM node itself might be optimized out later 
            // Or map to a specific LOAD_PARAM BDI op if exists 
            // Store mapping from IR PARAM node ID to itself (as value source) 
            // This requires ir_output_to_bdi_port_map_ to handle self-references maybe? 
            break; 
        case IROpCode::RETURN_VALUE: 
            bdi_op = BDIOperationType::CTRL_RETURN; break; 
        case IROpCode::LOAD_CONST: 
            bdi_op = BDIOperationType::META_CONST; break; 
        case IROpCode::LOAD_SYMBOL: { 
             const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); 
             if (!symbol) return false; 
             auto addr_it = variable_address_nodes_.find(symbol->name); 
             if (addr_it == variable_address_nodes_.end()) { 
                 throw BDIExecutionError("Compiler Error: Address for variable '" + symbol->name + "' not found during LOAD."); 
         } 
             // Connect standard sequential control flow *from previous node* 
             if (connect_standard_control_flow && current_bdi_cfg_node_ != 0) { 
             builder_.connectControl(current_bdi_cfg_node_, bdi_node_id); 
         }
             // Update current control flow position *unless* it's a non-sequential jump/branch/return 
             if (bdi_op != BDIOperationType::CTRL_JUMP && bdi_op != BDIOperationType::CTRL_BRANCH_COND && 
             bdi_op != BDIOperationType::CTRL_RETURN && bdi_op != BDIOperationType::META_END) { 
             current_bdi_cfg_node_ = bdi_node_id; 
         } else { 
             // After a terminator/branch, the logical "current" node is less clear, 
             // subsequent nodes should connect from the branch targets or jump sources. 
             // Setting to 0 signifies the end of a basic block. 
             current_bdi_cfg_node_ = 0; 
     } 
             // Handle specific control flow targets for JUMP/BRANCH 
             if (ir_node.opcode == IROpCode::BRANCH_COND) { 
             auto* bdi_branch_node = builder_.getGraph().getNodeMutable(bdi_node_id); 
             if (const auto* targets = std::get_if<std::pair<IRNodeId, IRNodeId>>(&ir_node.operation_data)) { 
             // Map IR target IDs to BDI target IDs - CAUTION: Targets might not be converted yet! 
             // This is why control flow connection needs to happen *after* all nodes are mapped. 
             // Store IR target IDs temporarily? Or rely on later pass. 
             // bdi_branch_node->control_outputs = { ir_to_bdi_node_map_[targets->first], ir_to_bdi_node_map_[targets->second] }; 
         } else { return false; /* Invalid branch node data */ } 
     }
             bdi_op = BDIOperationType::MEM_LOAD; 
             // Need to create a node providing the address first 
             NodeID addr_const_node = builder_.addNode(BDIOperationType::META_CONST); // Use const node for address 
             // builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); // Store address in payload 
             // TODO: How to get actual address? Need runtime allocation info or stack offset. For now, store BDI ID of ALLOC node. 
             builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); // Store ALLOC node ID 
             builder_.defineDataOutput(addr_const_node, 0, BDIType::NODE_ID); // Output is NodeID for address lookup later 
             // The actual LOAD node will take input from addr_const_node 
             bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
             builder_.connectData(addr_const_node, 0, bdi_node_id, 0); // Connect address source 
             // Control flow: Need addr const first 
             if (current_bdi_cfg_node_ != 0) builder_.connectControl(current_bdi_cfg_node_, addr_const_node); 
             builder_.connectControl(addr_const_node, bdi_node_id); 
             connect_standard_control_flow = false; // Manual control flow handled 
             break; 
        } 
        case IROpCode::ALLOC_MEM: { 
             bdi_op = BDIOperationType::MEM_ALLOC; 
             bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
             ir_to_bdi_node_map_[ir_node.id] = bdi_node_id; // Map early for address storage 
             // Store this BDI node ID as the address source for the variable 
             // if (const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data)) { 
             //     variable_address_bdi_nodes_[symbol->name] = bdi_node_id; 
             // } else { /* Alloc needs symbol association */ return false; } 
             // Connect size input (assuming input 0) 
             // Set size input? Assumes size is determined and passed as input IR node 
             if (ir_node.inputs.size() >= 1) { /* ... connect data input 0 ... */ } 
                 auto bdi_port_ref_opt = getBDIPortRef(ir_node.inputs[0]); 
                 // Define address output (Port 0) 
                 if (!bdi_port_ref_opt) return false; 
                 builder_.connectData(bdi_port_ref_opt.value().node_id, bdi_port_ref_opt.value().port_index, bdi_node_id, 0); 
             } else { /* Error: Alloc needs size */ return false; } 
             // Define output (pointer/address) 
             if (ir_node.output_type) builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType(), BDIType::POINTER); // Alloc returns pointer 
             connect_standard_control_flow = true; // Alloc is sequential op 
             // Store mapping from variable name (if available in op_data) to this alloc node's ID 
             // if (const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data)) { 
             //     variable_address_nodes_[symbol->name] = bdi_node_id; // Store BDI ID of node producing address 
             // } 
             connect_standard_control_flow = true; // Alloc is sequential op 
             break; // Exit early as node created 
       } 
        case IROpCode::LOAD_SYMBOL: { 
            const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); 
            if (!symbol) return false; 
            // auto addr_bdi_node_id_it = variable_address_bdi_nodes_.find(symbol->name); 
            // if (addr_bdi_node_id_it == variable_address_bdi_nodes_.end()) throw BDIExecutionError("Undef symbol load: " + symbol->name); 
            // NodeID bdi_addr_node_id = addr_bdi_node_id_it->second; 
            bdi_op = BDIOperationType::MEM_LOAD; 
            bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
            // Connect address input (Input 0) from the stored address node ID 
            // builder_.connectData(bdi_addr_node_id, 0, bdi_node_id, 0); 
            // Define output port with correct type from IR node 
            if (ir_node.output_type) builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType()); 
             break;
        } 
         case IROpCode::STORE_MEM: { // Assignment 
             bdi_op = BDIOperationType::MEM_STORE; 
             bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
             if (ir_node.inputs.size() != 2) return false; // Expect Addr, Value Ref 
             // Input 0: Address (Lookup symbol address from variable allocation) 
             const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); // Assume symbol stored here 
             if (!symbol) return false; 
             auto addr_it = variable_address_nodes_.find(symbol->name); 
             if (addr_it == variable_address_nodes_.end()) throw BDIExecutionError("Compiler Error: Address for variable '" + symbol->name + "
             //NodeID bdi_addr_node_id = addr_bdi_node_id_it->second; 
             builder_.connectData(bdi_addr_node_id, 0, bdi_node_id, 0);   
             // Create const node for address ID 
             NodeID addr_const_node = builder_.addNode(BDIOperationType::META_CONST); 
             builder_.setNodePayload(addr_const_node, TypedPayload::createFrom(addr_it->second)); 
             builder_.defineDataOutput(addr_const_node, 0, BDIType::NODE_ID); 
              if (current_bdi_cfg_node_ != 0) builder_.connectControl(current_bdi_cfg_node_, addr_const_node); // Control flow for addr const 
             // Input 1: Value 
             auto bdi_value_port_ref_opt = getBDIPortRef(ir_node.inputs[1]); 
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
            case IROpCode::BINARY_OP: { /* ... Map operator to BDI OpType ... */ } break; 
            case IROpCode::UNARY_OP: { /* ... Map operator to BDI OpType ... */ } break; 
            case IROpCode::JUMP: bdi_op = BDIOperationType::CTRL_JUMP; connect_standard_control_flow = false; break; // Control connected late
            case IROpCode::BRANCH_COND: bdi_op = BDIOperationType::CTRL_BRANCH_COND; connect_standard_control_flow = false; break; // Control 
            case IROpCode::CALL: bdi_op = BDIOperationType::CTRL_CALL;
            bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
             // Define output port for return value 
             if (ir_node.output_type && ir_node.output_type->getBaseBDIType() != BDIType::VOID) { 
             builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType()); 
             // Store mapping for call node output 
             if(ir_output_to_bdi_port_map_[ir_node.id].empty()) ir_output_to_bdi_port_map_[ir_node.id].resize(1); 
             ir_output_to_bdi_port_map_[ir_node.id][0] = {bdi_node_id, 0};
        } 
             // Connect arguments (Inputs to IR CALL become Inputs to BDI CALL) 
             for (size_t i = 0; i < ir_node.inputs.size(); ++i) { /* ... Connect using getBDIPortRef ... */ } 
             // Control flow connection handled later based on target/return site 
            connect_standard_control_flow = false; 
            default: throw BDIExecutionError("Unhandled IR Opcode during BDI conversion"); 
            break; // Args connected, control connected later 
        } 
            // Create node if not already created by specific logic above 
            if (bdi_node_id == 0) { 
            bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
        } 
            ir_to_bdi_node_map_[
            ir_node.id] = bdi_node_id; // Store mapping 
            // Define Output Port (if not already done) 
            if (ir_node.output_type && ir_node.output_type->isResolved() && ir_node.output_type->getBaseBDIType() != BDIType::VOID) { 
            auto* bdi_node = builder_.getGraph().getNodeMutable(bdi_node_id); 
            if (bdi_node && bdi_node->data_outputs.empty()) { // Avoid redefining if already done 
            builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType()); 
            // Ensure mapping exists 
            if(ir_output_to_bdi_port_map_[
            ir_node.id].empty()) ir_output_to_bdi_port_map_[
            ir_output_to_bdi_port_map_[
            ir_node.id][0] = {bdi_node_id, 0}; 
        } 
  }  
            ir_node.id].resize(1); 
            // Connect Data Inputs (if not handled specifically above) 
            if (bdi_op != BDIOperationType::MEM_LOAD && bdi_op != BDIOperationType::MEM_STORE) { // Avoid re-connecting 
            for (size_t i = 0; i < ir_node.inputs.size(); ++i) { /* ... Connect using getBDIPortRef ... */ } 
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
            case IROpCode::RETURN_VALUE: { 
            bdi_op = BDIOperationType::CTRL_RETURN; 
            bdi_node_id = builder_.addNode(bdi_op, ir_node.label); 
            // Connect return value (Input 0 to IR RETURN -> Input 0 to BDI RETURN) 
            if (!ir_node.inputs.empty()) { 
                  auto bdi_ret_val_ref_opt = getBDIPortRef(ir_node.inputs[0]); 
                  if (!bdi_ret_val_ref_opt) return false; 
                  builder_.connectData(bdi_ret_val_ref_opt.value().node_id, bdi_ret_val_ref_opt.value().port_index, bdi_node_id, 0); 
            } 
            connect_standard_control_flow = true; // Connect predecessor to RETURN 
            break; 
        }
          case IROpCode::DSL_BLOCK: { 
             // 1. Extract DSL name and the specific DSL AST node pointer 
             std::string dsl_name = ir_node.label; // Assuming label holds DSL name 
             const IDSLSpecificASTNode* dsl_ast_node = nullptr; 
             if (const auto* node_ptr_any = std::get_if<std::shared_ptr<IDSLSpecificASTNode>>(&ir_node.operation_data)) { 
                 dsl_ast_node = node_ptr_any->get(); 
             } else { 
                  throw BDIExecutionError("Invalid operation_data for DSL_BLOCK IR node " + std::to_string(ir_node.id)); 
             } 
             if (!dsl_ast_node) throw BDIExecutionError("Null DSL AST node pointer in DSL_BLOCK"); 
             // 2. Get Mapper from Registry 
             auto mapper = dsl_registry_.getDSLMapper(dsl_name); 
             if (!mapper) { 
                  throw BDIExecutionError("No registered DSL mapper found for DSL '" + dsl_name + "'"); 
             } 
             // 3. Verify Mapper handles this node type (Optional but recommended) 
             if (!mapper->handlesNodeType(dsl_ast_node->getNodeTypeName())) { 
                  throw BDIExecutionError("Registered mapper for DSL '" + dsl_name + "' does not handle node type '" + dsl_ast_node->getNodeTy
             } 
             // 4. Call Mapper's mapToBDI function 
             // Need to track current BDI control flow node correctly here 
             bdi_node_id = mapper->mapToBDI(dsl_ast_node, builder_, current_bdi_cfg_node_); 
             if (bdi_node_id == 0) { 
                 throw BDIExecutionError("DSL Mapper for '" + dsl_name + "' failed for IR node " + std::to_string(ir_node.id)); 
             } 
             // Mapper updates current_bdi_cfg_node_ internally 
             connect_standard_control_flow = false; // Mapper handled flow for its generated nodes 
             break; 
        }
             // ... getBDIPortRef ... 
             // ... Create node if needed, map IDs, define output ports, connect data inputs ... 
             // ... Handle standard control flow connection ... 
             return true; 
        }
  } // namespace ir::ir 
