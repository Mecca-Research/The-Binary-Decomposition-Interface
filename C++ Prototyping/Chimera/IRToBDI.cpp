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
// Add BDI operations for stack if not present 
// BDIOperationType::STACK_PUSH, STACK_POP, STACK_ALLOC, STACK_SET_FP etc. 
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
    // Need to manage BDI stack pointer / frame concept during conversion 
    struct BDIConversionContext { 
    bdi::frontend::api::GraphBuilder& builder; 
    bdi::meta::MetadataStore& meta_store; 
    std::unordered_map<IRNodeId, NodeID>& ir_to_bdi_node_map; 
    std::unordered_map<IRNodeId, std::vector<PortRef>>& ir_output_to_bdi_port_map; 
    // Map variable name/scope -> BDI node ID holding stack address 
    // ... builder, meta_store, node maps ... 
    // Map {scope_level, name} -> BDI NodeID holding stack address 
    std::map<std::pair<int, std::string>, NodeID> variable_address_bdi_nodes; // Key: {scope_level, name} 
    NodeID frame_pointer_bdi_node = 0; // BDI Node representing CURRENT Frame Pointer Value for this function 
    NodeID stack_pointer_bdi_node = 0; // BDI Node representing CURRENT Stack Pointer Value for this function 
    NodeID current_bdi_cfg_node = 0; 
    // Stack management 
    NodeID frame_pointer_node = 0; // BDI node representing frame pointer (if used) 
    size_t current_stack_offset = 0; 
}; 
    // --- Helper to emit code loading SP or FP --- 
    NodeID ChiIRToBDI::getSpecialRegister(BDISpecialRegister reg, BDIConversionContext& ctx) { 
    // Check if already loaded in this context? Cache result? 
    // For now, create a new read node each time 
    NodeID read_node = ctx.builder.addNode(BDIOperationType::REG_READ); 
     ctx.builder.setNodePayload(read_node, TypedPayload::createFrom(static_cast<uint8_t>(reg))); // Payload identifies register 
     ctx.builder.defineDataOutput(read_node, 0, BDIType::POINTER); // SP/FP are addresses 
    if (ctx.current_bdi_cfg_node != 0) ctx.builder.connectControl(ctx.current_bdi_cfg_node, read_node); 
     ctx.current_bdi_cfg_node = read_node; 
    return read_node; 
} 
    // --- Helper to emit code storing to SP or FP --- 
    void ChiIRToBDI::setSpecialRegister(BDISpecialRegister reg, NodeID value_source_node, PortIndex value_source_port, BDIConversionContext& ctx) 
    NodeID write_node = ctx.builder.addNode(BDIOperationType::REG_WRITE); 
     ctx.builder.setNodePayload(write_node, TypedPayload::createFrom(static_cast<uint8_t>(reg))); // Payload identifies register 
     ctx.builder.connectData(value_source_node, value_source_port, write_node, 0); // Connect value to write 
    if (ctx.current_bdi_cfg_node != 0) ctx.builder.connectControl(ctx.current_bdi_cfg_node, write_node); 
     ctx.current_bdi_cfg_node = write_node; 
    // Update the context's tracking variable for SP or FP if necessary 
    if (reg == BDISpecialRegister::STACK_POINTER) ctx.stack_pointer_bdi_node = write_node; // Output of write? Or value source? Value source
    if (reg == BDISpecialRegister::FRAME_POINTER) ctx.frame_pointer_bdi_node = write_node; // Need output from REG_WRITE? No, just track val
    // Let's assume REG_WRITE doesn't produce output, just updates internal state. 
    // Track the *value node* that was written. 
    if (reg == BDISpecialRegister::STACK_POINTER) ctx.stack_pointer_bdi_node = value_source_node; 
    if (reg == BDISpecialRegister::FRAME_POINTER) ctx.frame_pointer_bdi_node = value_source_node; 
} 
    // --- Helper to emit Add/Sub SP/FP with constant offset --- 
    NodeID ChiIRToBDI::adjustPointer(NodeID ptr_source_node, int64_t offset_val, BDIConversionContext& ctx) { 
    NodeID offset_const = ctx.builder.addNode(BDIOperationType::META_CONST); 
    // Use INT64 for offsets for flexibility? Or POINTER type? Use POINTER (uintptr_t) 
     ctx.builder.setNodePayload(offset_const, TypedPayload::createFrom(static_cast<uintptr_t>(offset_val))); 
     ctx.builder.defineDataOutput(offset_const, 0, BDIType::POINTER); // Offset treated as pointer-sized int 
    BDIOperationType op = (offset_val >= 0) ? BDIOperationType::ARITH_ADD : BDIOperationType::ARITH_SUB; 
    NodeID add_sub_node = ctx.builder.addNode(op); 
    // Need absolute value for SUB offset constant if using SUB op 
    if (offset_val < 0) { 
     ctx.builder.setNodePayload(offset_const, TypedPayload::createFrom(static_cast<uintptr_t>(-offset_val))); 
} 
     ctx.builder.connectData(ptr_source_node, 0, add_sub_node, 0); // Input 0: Base Pointer 
     ctx.builder.connectData(offset_const, 0, add_sub_node, 1); // Input 1: Offset 
     ctx.builder.defineDataOutput(add_sub_node, 0, BDIType::POINTER); // Output: New Address 
    // Control Flow 
    if (ctx.current_bdi_cfg_node != 0) ctx.builder.connectControl(ctx.current_bdi_cfg_node, offset_const); 
     ctx.builder.connectControl(offset_const, add_sub_node); 
     ctx.current_bdi_cfg_node = add_sub_node; 
    return add_sub_node; // Return node producing the new address 
} 
     // Modify convertNode and convertGraph to pass BDIConversionContext   
bool IRToBDI::convertNode(const IRNode& ir_node, BDIConversionContext& ctx) { // Pass context
     if (ir_to_bdi_node_map_.count(ir_node.id)) return true; // Already processed 
     // ... Check map ... 
     NodeID bdi_node_id = 0; 
     BDIOperationType bdi_op = BDIOperationType::META_NOP; 
     bool connect_standard_control_flow = true; // Assume standard sequential flow unless overridden 
     // --- Map IR Opcode to BDI Op --- 
     // --- Update current register state nodes --- 
     // (This assumes we always need the latest SP/FP before using them. Could optimize.) 
     // ctx.stack_pointer_bdi_node = getSpecialRegister(BDISpecialRegister::STACK_POINTER, ctx); 
     // ctx.frame_pointer_bdi_node = getSpecialRegister(BDISpecialRegister::FRAME_POINTER, ctx); 
     switch (ir_node.opcode) { 
        case IROpCode::ENTRY: // Function Prologue 
        bdi_op = BDIOperationType::META_START; // Map ENTRY to START 
        bdi_node_id = ctx.builder.addNode(bdi_op, chiir_node.label); 
        ctx.current_bdi_cfg_node = bdi_node_id; // Start control flow here 
        // Store this as the function entry BDI node? 
        // --- Prologue --- 
        // 1. Get current FP & SP (conceptually, before modification) 
        ctx.frame_pointer_bdi_node = getSpecialRegister(BDISpecialRegister::FRAME_POINTER, ctx); 
        ctx.stack_pointer_bdi_node = getSpecialRegister(BDISpecialRegister::STACK_POINTER, ctx);  
        // 2. Push Old FP onto stack: STORE_MEM(SP - PtrSize, FP) 
        // Assume PtrSize is 8 bytes (64-bit) 
        NodeID push_addr_node = adjustPointer(initial_sp, -8, ctx); // Calculate SP-8 
        NodeID push_node = ctx.builder.addNode(BDIOperationType::MEM_STORE);   
        ctx.builder.connectData(push_addr_node, 0, push_node, 0); // Address 
        ctx.builder.connectData(ctx.frame_pointer_bdi_node, 0, push_fp, 1); // Value Input (Old FP)
        ctx.current_bdi_cfg_node = push_fp; // Update CFG 
        // 3. Set New FP: FP = SP (Use initial_sp, *before* frame allocation) 
        setSpecialRegister(BDISpecialRegister::FRAME_POINTER, initial_sp, 0, ctx); 
        ctx.frame_pointer_bdi_node = initial_sp; // Track the node holding the NEW FP value 
        // 4. Allocate Frame: SP = SP - FrameSize 
        // size_t frame_size = ... // Get frame size calculated by TypeChecker, stored somewhere? 
        size_t frame_size = 128; // Placeholder 
        NodeID new_sp_node = adjustPointer(initial_sp, -((int64_t)frame_size + 8), ctx); // Adjust for pushed FP too 
        setSpecialRegister(BDISpecialRegister::STACK_POINTER, new_sp_node, 0, ctx); 
        setSpecialRegister(BDISpecialRegister::FRAME_POINTER, ctx.stack_pointer_bdi_node, 0, ctx); 
        // ctx.frame_pointer_bdi_node remains the node ID holding the *new* FP value (which was the old SP value node) 
        // 4. Allocate Frame: SP = SP - FrameSize 
        // size_t frame_size = ... // Get from ChiIR Function Info or context 
        size_t frame_size = 128; // Placeholder 
        NodeID new_sp = adjustPointer(ctx.stack_pointer_bdi_node, -(int64_t)frame_size, ctx); 
        setSpecialRegister(BDISpecialRegister::STACK_POINTER, new_sp, 0, ctx); 
        ctx.stack_pointer_bdi_node = new_sp_node; // Track node holding NEW SP value 
        connect_standard_control_flow = false; // Prologue handles its own flow 
        break; 
     } 
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
        case IROpCode::RETURN_VALUE:{ // Function Epilogue before Return 
             bdi_op = BDIOperationType::CTRL_RETURN; 
             // --- Epilogue --- 
             // --- Generate BDI Epilogue (before the RETURN BDI node) --- 
             // 1. (Optional) Store return value (ChiIR input 0) to dedicated return register or stack slot? Assume value node is ready. 
             NodeID return_value_source_node = 0; 
             PortIndex return_value_source_port = 0; 
             if (!chiir_node.inputs.empty()) { 
             auto bdi_ret_val_ref_opt = getBDIPortRef(chiir_node.inputs[0], ctx); 
             if (!bdi_ret_val_ref_opt) return false; 
             return_value_source_node = bdi_ret_val_ref_opt.value().node_id; 
             return_value_source_port = bdi_ret_val_ref_opt.value().port_index; 
            } 
             // 1. Deallocate Frame: SP = FP 
             NodeID current_fp = ctx.frame_pointer_bdi_node; // Get current FP value node 
             setSpecialRegister(BDISpecialRegister::STACK_POINTER, current_fp, 0, ctx); 
             ctx.stack_pointer_bdi_node = current_fp; // SP is now same as FP 
             // 2. Pop Old FP into FP register: FP = LOAD_MEM(SP) ; SP = SP + PtrSize 
              NodeID pop_addr_node = current_fp; // Address to load from is current FP (which is now SP) 
              NodeID old_fp_load_node = ctx.builder.addNode(BDIOperationType::MEM_LOAD); 
              ctx.builder.connectData(pop_addr_node, 0, old_fp_load_node, 0); // Address input 
              ctx.builder.defineDataOutput(old_fp_load_node, 0, BDIType::POINTER); // Output is old FP address 
              if (ctx.current_bdi_cfg_node != 0) ctx.builder.connectControl(ctx.current_bdi_cfg_node, old_fp_load_node); 
              ctx.current_bdi_cfg_node = old_fp_load_node; 
              setSpecialRegister(BDISpecialRegister::FRAME_POINTER, old_fp_load_node, 0, ctx); // Restore old FP 
              ctx.frame_pointer_bdi_node = old_fp_load_node; 
             // Adjust SP 
              NodeID final_sp_node = adjustPointer(current_fp, 8, ctx); // SP = Current FP + 8 
              setSpecialRegister(BDISpecialRegister::STACK_POINTER, final_sp_node, 0, ctx); 
              ctx.stack_pointer_bdi_node = final_sp_node; 
              // 3. Create the actual BDI RETURN node 
              bdi_node_id = ctx.builder.addNode(bdi_op, chiir_node.label); 
              // Connect return value if exists (ChiIR input 0 -> BDI input 0) 
              if (!chiir_node.inputs.empty()) { 
                  auto bdi_ret_val_ref_opt = getBDIPortRef(chiir_node.inputs[0], ctx); 
                  if (!bdi_ret_val_ref_opt) return false; 
                  ctx.builder.connectData(bdi_ret_val_ref_opt.value().node_id, bdi_ret_val_ref_opt.value().port_index, bdi_node_id, 0); 
            } 
             ctx.builder.connectControl(ctx.current_bdi_cfg_node, bdi_node_id); // Connect epilogue to RETURN 
             connect_standard_control_flow = false; // Epilogue handles flow 
             // 4. Create BDI RETURN node 
             bdi_op = BDIOperationType::CTRL_RETURN; 
             bdi_node_id = ctx.builder.addNode(bdi_op, chiir_node.label); 
             // Connect the calculated return value (if any) to BDI RETURN Input 0 
             if (return_value_source_node != 0) { 
             ctx.builder.connectData(return_value_source_node, return_value_source_port, bdi_node_id, 0); 
            } 
             ctx.builder.connectControl(ctx.current_bdi_cfg_node, bdi_node_id); // Connect epilogue to return 
             connect_standard_control_flow = false; // Return is terminator 
             break; 
        }  
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
        case IROpCode::ALLOC_MEM: { // Stack allocation address calculation, Generates address calc node  
             // Get size/alignment/type info from ir_node->operation_data 
             // size_t alloc_size = ...; size_t alloc_align = ...; 
             // Calculate stack offset (this requires a frame concept) 
             // size_t offset = ctx.allocateStackSpace(alloc_size, alloc_align); // Need context method 
             // Create BDI node representing the address computation (e.g., ADD FramePtr, Offset) 
             // NodeID fp_node = ctx.frame_pointer_node; // Get frame pointer node 
             // NodeID offset_const_node = ctx.builder.addNode(BDIOperationType::META_CONST); // Const node for offset 
             // ctx.builder.setNodePayload(offset_const_node, TypedPayload::createFrom(static_cast<uintptr_t>(offset))); 
             // ctx.builder.defineDataOutput(offset_const_node, 0, BDIType::POINTER); // Treat offset as pointer size 
             // bdi_op = BDIOperationType::ARITH_ADD; // FP + Offset 
             // bdi_node_id = ctx.builder.addNode(bdi_op, "stack_addr_" + ir_node.label); 
             // ctx.builder.connectData(fp_node, 0, bdi_node_id, 0); 
             // ctx.builder.connectData(offset_const_node, 0, bdi_node_id, 1); 
             // ctx.builder.defineDataOutput(bdi_node_id, 0, BDIType::POINTER); // Result is address 
             // Store this address node ID associated with the variable symbol 
             // if (const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data)) { 
             // int scope = ... ; // Need scope info associated with IR node 
             // ctx.variable_address_bdi_nodes[{scope, symbol->name}] = bdi_node_id; 
             // }
             bdi_op = BDIOperationType::MEM_ALLOC; // Or use direct Alloc if VM handles stack implicitly 
             bdi_node_id = ctx.builder_.addNode(bdi_op, ir_node.label); 
             // Connect size input... Define address output... Store mapping... 
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
                // Already handled stack offset calc conceptually in ASTToChiIR 
                // This ChiIR node now just generates the BDI node representing the calculated address 
                const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); // Assuming symbol stored for name, Need scope/offset info  
                // const auto* alloc_info = std::get_if<AllocInfo>(&chiir_node.operation_data); // Use proper struct 
                if (!symbol /* || !alloc_info */) return false;
                // size_t offset = context.lookupSymbolInfo(symbol->name)->stack_offset; // Need context access 
                size_t offset = 16; // Placeholder offset, Get correct offset from SymbolInfo / context  
                NodeID addr_node_id = generateBDIAddressCalc(offset, ctx); // Generate FP + Offset, This *is* the BDI node for this IR alloc node  
                bdi_node_id = addr_node_id; // The result of this node IS the address 
                // Store mapping for symbol to this BDI address node ID 
                // ctx.variable_address_bdi_nodes[{scope, symbol->name}] = bdi_node_id; 
                connect_standard_control_flow = false; // Address calc handles flow   
                // int scope = ... // Need scope info from IR node annotation or context 
                // size_t offset = ... // Need offset calculated by TypeChecker/ASTToIR 
                // Create BDI nodes for: FP + Offset 
                NodeID offset_const_node = ctx.builder.addNode(BDIOperationType::META_CONST); 
                // ctx.builder.setNodePayload(offset_const_node, TypedPayload::createFrom(static_cast<uintptr_t>(offset))); 
                ctx.builder.defineDataOutput(offset_const_node, 0, BDIType::POINTER); // Treat offset as pointer size 
                bdi_op = BDIOperationType::ARITH_ADD; // FP + Offset 
                bdi_node_id = ctx.builder.addNode(bdi_op, "addr_" + symbol->name); 
                ctx.builder.connectData(ctx.frame_pointer_bdi_node, 0, bdi_node_id, 0); // Connect Frame Ptr 
                ctx.builder.connectData(offset_const_node, 0, bdi_node_id, 1); // Connect Offset Const 
                ctx.builder.defineDataOutput(bdi_node_id, 0, BDIType::POINTER); // Result is address 
                // Store mapping 
                // ctx.variable_address_bdi_nodes[{scope, symbol->name}] = bdi_nod
             }
             // Define output (pointer/address) 
             if (ir_node.output_type) builder_.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType(), BDIType::POINTER); // Alloc returns pointer 
             ctx.variable_address_bdi_nodes[{/*scope*/ 0, symbol->name}] = bdi_node_id; // Store address node ID 
             connect_standard_control_flow = false; // Address calc handles flow 
             // Store mapping from variable name (if available in op_data) to this alloc node's ID 
             // if (const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data)) { 
             //     variable_address_nodes_[symbol->name] = bdi_node_id; // Store BDI ID of node producing address 
             // } 
             connect_standard_control_flow = true; // Alloc is sequential op 
             break; // Exit early as node created 
       } 
                    // --- Map Stack Ops --- 
         case ChiIROpCode::STACK_PUSH: 
              bdi_op = BDIOperationType::MEM_STORE; // Need SP based store 
              // Calculate address SP - sizeof(value) 
              // Connect value input chiir_node.inputs[0] -> BDI STORE input 1 
              // Adjust SP: Generate ADD SP, -Size node AFTER store? Complex. 
              // Needs dedicated STACK_PUSH BDI op or ABI knowledge. 
              std::cerr << "STACK_PUSH ChiIR->BDI Not fully implemented." << std::endl; 
              break;
          case ChiIROpCode::STACK_SET_FP: 
              // bdi_op = BDIOperationType::REG_MOV; // If BDI has register moves: MOV FP, SP 
              std::cerr << "STACK_SET_FP ChiIR->BDI Not fully implemented." << std::endl; 
              break;
          case ChiIROpCode::STACK_ALLOC: 
              // bdi_op = BDIOperationType::ARITH_SUB; // SP = SP - frame_size 
              // Get frame_size from chiir_node.operation_data 
              std::cerr << "STACK_ALLOC ChiIR->BDI Not fully implemented." << std::endl; 
              break;
          case ChiIROpCode::STACK_DEALLOC: // SP = FP 
              // bdi_op = BDIOperationType::REG_MOV; // MOV SP, FP 
              std::cerr << "STACK_DEALLOC ChiIR->BDI Not fully implemented." << std::endl; 
              break;
          case ChiIROpCode::STACK_POP: 
               bdi_op = BDIOperationType::MEM_LOAD; // Load old FP from stack relative to SP 
               // Adjust SP: Generate ADD SP, Size node AFTER load? 
               std::cerr << "STACK_POP ChiIR->BDI Not fully implemented." << std::endl; 
               break; 
               // --- Address Calculation for Stack Variables --- 
               // LOAD_SYMBOL / STORE_MEM for stack vars need to use the generated address calc nodes 
        case IROpCode::LOAD_SYMBOL: { 
               // ... get SymbolInfo ... 
               // if (symbol_info->location == SymbolInfo::Location::STACK) { 
               // ChiIRNodeId addr_calc_node_id = generateBDIAddressCalc(symbol_info->stack_offset, ctx); 
               // bdi_op = BDIOperationType::MEM_LOAD; 
               // bdi_node_id = ctx.builder.addNode(bdi_op); 
               // ctx.builder.connectData(addr_calc_node_id, 0, bdi_node_id, 0); // Address input 
               // ... define output ... 
               // } else { /* handle SSA */ } 
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
        case IROpCode::LOAD_MEM: { // Load from variable address 
            const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); // Address source handled here 
            if (!symbol) return false; 
            // int scope = ...; // Get scope 
            // auto addr_bdi_node_id = ctx.variable_address_bdi_nodes.at({scope, symbol->name}); // Get BDI Addr node 
            // auto addr_node_it = ctx.variable_address_bdi_nodes.find({scope, symbol->name}); 
            // if (addr_node_it == ctx.variable_address_bdi_nodes.end()) throw BDIExecutionError("Address node not found for LOAD"); 
            // NodeID bdi_addr_node_id = addr_node_it->second; 
            // ... get SymbolInfo ... 
            // if (symbol_info->location == SymbolInfo::Location::STACK) { 
            // ChiIRNodeId addr_calc_node_id = generateBDIAddressCalc(symbol_info->stack_offset, ctx); 
            // bdi_op = BDIOperationType::MEM_STORE; 
            // bdi_node_id = ctx.builder.addNode(bdi_op); 
            // ctx.builder.connectData(addr_calc_node_id, 0, bdi_node_id, 0); // Address input 
            // Connect value input (chiir_node.inputs[0]) -> BDI STORE input 1 
            // ... 
            NodeID bdi_addr_node_id = 123; // Placeholder 
            bdi_op = BDIOperationType::MEM_LOAD; 
            bdi_node_id = ctx.builder.addNode(bdi_op, ir_node.label); 
            ctx.builder.connectData(bdi_addr_node_id, 0, bdi_node_id, 0); // Connect address source  // Address source 
            if (ir_node.output_type) ctx.builder.defineDataOutput(bdi_node_id, 0, ir_node.output_type->getBaseBDIType()); 
            break; 
       }
         case IROpCode::STORE_MEM: { // Assignment, Store to variable address  
             const auto* symbol = std::get_if<Symbol>(&ir_node.operation_data); 
             if (!symbol || ir_node.inputs.size() < 1) return false; // Need value input 
             // int scope = ...; 
             // Lookup the BDI node ID that produces the address for this symbol 
             // auto addr_node_it = ctx.variable_address_bdi_nodes.find({scope, symbol->name}); 
             // if (addr_node_it == ctx.variable_address_bdi_nodes.end()) throw BDIExecutionError("Address node not found for STORE"); 
             // NodeID bdi_addr_node_id = addr_node_it->second;
             // auto addr_bdi_node_id = 
             ctx.variable_address_bdi_nodes.at({scope, symbol->name}); 
             NodeID bdi_addr_node_id = 123; // Placeholder 
             bdi_op = BDIOperationType::MEM_STORE; 
             if (chiir_node.opcode == ChiIROpCode::LOAD_SYMBOL) { 
                 bdi_op = BDIOperationType::MEM_LOAD; 
                 bdi_node_id = ctx.builder.addNode(bdi_op, chiir_node.label); 
                 ctx.builder.connectData(bdi_addr_node_id, 0, bdi_node_id, 0); // Connect address 
                 // Connect value input (ChiIR input 0 -> BDI input 1) 
                 // Define output port... 
             } else { // STORE_MEM 
                 bdi_op = BDIOperationType::MEM_STORE; 
                 bdi_node_id = ctx.builder.addNode(bdi_op, chiir_node.label); 
                 ctx.builder.connectData(bdi_addr_node_id, 0, bdi_node_id, 0); // Connect address 
                 // Connect value input (ChiIR input 0 -> BDI input 1) 
                 auto bdi_value_ref = getBDIPortRef(chiir_node.inputs[0], ctx); 
                 if (!bdi_value_ref) return false; 
                 ctx.builder.connectData(bdi_value_ref->node_id, bdi_value_ref->port_index, bdi_node_id, 1); 
             }
             bdi_node_id = ctx.builder_.addNode(bdi_op, ir_node.label); 
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
             // Connect address source (Input 0) 
             // ctx.builder.connectData(bdi_addr_node_id, 0, bdi_node_id, 0); 
             // Connect value source (Input 1, comes from IR input 0) 
             auto bdi_value_port_ref_opt = getBDIPortRef(ir_node.inputs[1]); 
             if (!bdi_value_port_ref_opt) return false; 
             ctx.builder.connectData(bdi_value_port_ref_opt.value().node_id, bdi_value_port_ref_opt.value().port_index, bdi_node_id, 1); 
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
             // ... map IDs, define outputs, connect data inputs (using context), connect control flow ... 
             return true; 
        }
             // Update getBDIPortRef signature 
             std::optional<bdi::core::graph::PortRef> IRToBDI::getBDIPortRef(const IRValueRef& value_ref, const BDIConversionContext& ctx) { 
             // Use ctx.ir_output_to_bdi_port_map_ 
             // ... 
             return std::nullopt; // Placeholder
        } 
             // ... map IDs, define outputs, connect data, connect control flow ... 
             return true;
    }
            // Implement generateBDIAddressCalc using adjustPointer helper and ctx.frame_pointer_bdi_node 
            NodeID ChiIRToBDI::generateBDIAddressCalc(size_t offset, BDIConversionContext& ctx, NodeID base_addr_node_id /* = 0 */) { 
            NodeID base_node = (base_addr_node_id != 0) ? base_addr_node_id : ctx.frame_pointer_bdi_node; 
            if (base_node == 0) throw BDIExecutionError("Base address node (e.g., Frame Pointer) not available for address calculation"); 
            // Offset is relative to FP (positive for args, negative for locals typically) 
            return adjustPointer(base_node, static_cast<int64_t>(offset), ctx); // Use helper 
 } // namespace chimera::ir 
