#include "BDIToLLVMIR.hpp" 
#include "BDINode.hpp" 
#include "BDIValueVariant.hpp" // For constants 
#include "Verifier.h" // To verify generated module 
#include "raw_ostream.h" // For printing errors 
#include <iostream> 
#include <vector> 
#include <set> // For visited nodes 
namespace chimera::backend { 
BDIToLLVMIR::BDIToLLVMIR() : builder_(llvm_context_) { 
     // Module will be created per graph conversion 
} 
llvm::Type* BDIToLLVMIR::mapBDITypeToLLVM(BDIType bdi_type) { 
    switch (bdi_type) { 
        case BDIType::VOID:    return llvm::Type::getVoidTy(llvm_context_); 
        case BDIType::BOOL:    return llvm::Type::getInt1Ty(llvm_context_); 
        case BDIType::INT8:    return llvm::Type::getInt8Ty(llvm_context_); 
        case BDIType::UINT8:   return llvm::Type::getInt8Ty(llvm_context_); // LLVM often doesn't distinguish signedness in core types 
        case BDIType::INT16:   return llvm::Type::getInt16Ty(llvm_context_); 
        case BDIType::UINT16:  return llvm::Type::getInt16Ty(llvm_context_); 
        case BDIType::INT32:   return llvm::Type::getInt32Ty(llvm_context_); 
        case BDIType::UINT32:  return llvm::Type::getInt32Ty(llvm_context_); 
        case BDIType::INT64:   return llvm::Type::getInt64Ty(llvm_context_); 
        case BDIType::UINT64:  return llvm::Type::getInt64Ty(llvm_context_); 
        case BDIType::FLOAT16: return llvm::Type::getHalfTy(llvm_context_); 
        case BDIType::FLOAT32: return llvm::Type::getFloatTy(llvm_context_); 
        case BDIType::FLOAT64: return llvm::Type::getDoubleTy(llvm_context_); 
        case BDIType::POINTER: // Fallthrough 
        case BDIType::MEM_REF: // Fallthrough 
        case BDIType::FUNC_PTR: // Use opaque pointer or pointer to function type 
            // For simplicity, use generic byte pointer (i8*) 
            return llvm::Type::getInt8PtrTy(llvm_context_); 
        default: 
            llvm::errs() << "BDIToLLVMIR Error: Cannot map BDI type " << static_cast<int>(bdi_type) << " to LLVM type.\n"; 
            return nullptr; 
    }
 } 
llvm::Constant* BDIToLLVMIR::getLLVMConstant(const BDIValueVariant& value_var) { 
    llvm::Type* llvm_type = mapBDITypeToLLVM(bdi::runtime::getBDIType(value_var)); 
    if (!llvm_type) return nullptr; 
    return std::visit([&](auto&& arg) -> llvm::Constant* { 
        using T = std::decay_t<decltype(arg)>; 
        if constexpr (std::is_same_v<T, std::monostate>) { return nullptr; /* No constant for void? */ } 
        else if constexpr (std::is_same_v<T, bool>)      { return builder_.getInt1(arg); } 
        else if constexpr (std::is_integral_v<T>)        { return llvm::ConstantInt::get(llvm_type, static_cast<uint64_t>(arg), std::is_signed
        else if constexpr (std::is_same_v<T, float>)     { return llvm::ConstantFP::get(llvm_context_, llvm::APFloat(arg)); } 
        else if constexpr (std::is_same_v<T, double>)    { return llvm::ConstantFP::get(llvm_context_, llvm::APFloat(arg)); } 
        else if constexpr (std::is_same_v<T, uintptr_t>) { return llvm::ConstantInt::get(llvm_type, arg); } // Treat pointer const as int for 
        else { return nullptr; } 
    }, value_var); 
} 
void BDIToLLVMIR::setupBasicBlocks(BDIGraph& graph) { 
    // Create entry block for the function 
    BDINode* start_node = graph.getNodeMutable(1); // Assume start node ID 1? Need reliable way. 
    if (!start_node) throw std::runtime_error("Cannot find graph entry node for LLVM setup"); 
    llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(llvm_context_, "entry", current_function_); 
    bdi_node_to_llvm_block_[start_node->id] = entry_bb; 
    // Iterate and create blocks for targets of jumps/branches and function entry points 
    // Requires graph analysis to identify block headers 
    // For now, just the entry block 
} 
void BDIToLLVMIR::setupFunction(BDINode& start_node) { 
    // Define function type (e.g., void func()) - needs proper signature based on graph later 
    llvm::FunctionType* func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_context_), false); // Placeholder 
    current_function_ = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "bdi_func", module_.get()); 
    // TODO: Set argument names, types based on PARAM nodes 
} 
void BDIToLLVMIR::convertNode(BDINode& node) { 
     // Ensure we are inserting into a valid basic block 
     if (!builder_.GetInsertBlock()) { 
        // Find or create the block for this node 
         auto bb_it = bdi_node_to_llvm_block_.find(node.id); 
         if (bb_it != bdi_node_to_llvm_block_.end()) { 
             builder_.SetInsertPoint(bb_it->second); 
         } else { 
              // This node doesn't start a block - needs predecessor's block context 
              // Requires CFG traversal. For now, assume entry block. 
              builder_.SetInsertPoint(bdi_node_to_llvm_block_[1]); // Fallback to entry 
              // llvm::errs() << "Warning: Could not find specific basic block for BDI Node " << node.id << "\n"; 
         }
     } 
    // Get LLVM Values for inputs 
    std::vector<llvm::Value*> llvm_inputs; 
    for (const auto& port_ref : node.data_inputs) { 
        auto val_it = bdi_node_to_llvm_value_.find(port_ref.node_id); // Find source node output value 
        if (val_it == bdi_node_to_llvm_value_.end()) { 
            llvm::errs() << "BDIToLLVMIR Error: LLVM Value not found for input BDI Node " << port_ref.node_id << "\n"; 
            return; // Error 
        } 
        llvm_inputs.push_back(val_it->second); 
        // TODO: Handle multiple output ports if PortIndex > 0 
    }
    llvm::Value* result_val = nullptr; 
    // Generate LLVM instruction based on BDI OpType 
    switch (node.operation) { 
        case BDIOperationType::META_START: /* Handled by setupFunction/setupBasicBlocks */ break; 
        case BDIOperationType::META_END: builder_.CreateRetVoid(); break; // Simple void return 
        case BDIOperationType::META_CONST: { 
             auto val_var = bdi::runtime::ExecutionContext::payloadToVariant(node.payload); 
             result_val = getLLVMConstant(val_var); 
             break; 
        } 
        case BDIOperationType::ARITH_ADD: { 
             if (llvm_inputs.size() != 2) return; // Error 
             if (llvm_inputs[0]->getType()->isIntegerTy()) result_val = builder_.CreateAdd(llvm_inputs[0], llvm_inputs[1], "addtmp"); 
             else if (llvm_inputs[0]->getType()->isFloatingPointTy()) result_val = builder_.CreateFAdd(llvm_inputs[0], llvm_inputs[1], "faddtm
             else { /* Error */ } 
             break; 
        } 
         case BDIOperationType::MEM_LOAD: { 
              if (llvm_inputs.size() != 1) return; // Need address input 
              llvm::Type* load_type = mapBDITypeToLLVM(node.getOutputType(0)); 
              if (!load_type) return; 
              llvm::Value* ptr_input = llvm_inputs[0]; 
              // Ensure pointer type matches load type (may need bitcast) 
              // llvm::Value* ptr = builder_.CreateBitCast(ptr_input, load_type->getPointerTo(), "loadptr"); 
              // result_val = builder_.CreateLoad(load_type, ptr, "loadtmp"); 
               result_val = builder_.CreateLoad(load_type, ptr_input, "loadtmp"); // Simpler for now 
               break; 
         }
         case BDIOperationType::MEM_STORE: { 
              if (llvm_inputs.size() != 2) return; // Need address and value 
              llvm::Value* ptr_input = llvm_inputs[0]; 
              llvm::Value* val_input = llvm_inputs[1]; 
              // Ensure pointer type matches value type (may need bitcast) 
              // builder_.CreateStore(val_input, ptr); 
              builder_.CreateStore(val_input, ptr_input); // Simpler for now 
              break;
         }
         case BDIOperationType::CTRL_JUMP: { 
              // Node ID target needs mapping to LLVM BasicBlock* 
              // llvm::BasicBlock* target_bb = bdi_node_to_llvm_block_.at( /* target NodeID */ ); 
              // builder_.CreateBr(target_bb); 
              break;
         }
          case BDIOperationType::CTRL_BRANCH_COND: { 
               // llvm::Value* condition = llvm_inputs[0]; // Assume input 0 is bool 
               // llvm::BasicBlock* true_bb = bdi_node_to_llvm_block_.at( /* true target NodeID */ ); 
               // llvm::BasicBlock* false_bb = bdi_node_to_llvm_block_.at( /* false target NodeID */ ); 
               // builder_.CreateCondBr(condition, true_bb, false_bb); 
               break; 
          } 
        // ... Implement ALL other BDI -> LLVM mappings ... 
        default: 
             llvm::errs() << "BDIToLLVMIR Warning: Unhandled BDI OpType: " << static_cast<int>(node.operation) << "\n"; 
             break; 
    }
    // Store mapping from BDI node output to LLVM value 
    if (result_val && !node.data_outputs.empty()) { 
         bdi_node_to_llvm_value_[node.id] = result_val; // Assume output port 0 maps to this value 
    }
    // If node was a block terminator, clear the builder's insert point 
    if (llvm::isa<llvm::TerminatorInst>(result_val) || node.operation == BDIOperationType::META_END || node.operation == BDIOperationType::CT
    // builder_.ClearInsertionPoint(); // Might not be needed if SetInsertPoint is called at start of next node's block 
    } 
    std::unique_ptr<llvm::Module> BDIToLLVMIR::convertGraph(BDIGraph& graph, const std::string& module_id) { 
    module_ = std::make_unique<llvm::Module>(module_id, llvm_context_); 
    bdi_node_to_llvm_value_.clear(); 
    bdi_node_to_llvm_block_.clear(); 
    current_function_ = nullptr; 
    // --- Requires proper CFG analysis and block identification --- 
    // 1. Find entry node(s) - Assuming one entry for now 
    // 2. Identify basic block headers (targets of jumps/branches, function entry) 
    // 3. Create llvm::BasicBlock for each header 
    // 4. Create llvm::Function for graph entry 
    // 5. Iterate through nodes in suitable order (e.g., per-block RPO) 
    // 6. Call convertNode for each node, setting builder insert point correctly 
    // --- Simplified Stub --- 
    BDINode* entry_node = graph.getNodeMutable(1); // Find entry node reliably 
    if (!entry_node) return nullptr; 
    setupFunction(*entry_node); 
    setupBasicBlocks(graph); // Creates entry block for now 
    // Iterate all nodes (incorrect order, just for stub) 
    for(auto& pair : graph) {
     if (pair.second) { 
            convertNode(*(pair.second)); 
             }
         } 
    // --- End Simplified Stub --- 
    // Verify the generated module 
    if (llvm::verifyFunction(*current_function_, &llvm::errs())) { 
            llvm::errs() << "LLVM Function Verification Failed!\n"; 
    // module_->print(llvm::errs(), nullptr); // Print faulty IR 
    return nullptr; // Or return module with errors? 
    } 
    if (llvm::verifyModule(*module_, &llvm::errs())) { 
            llvm::errs() << "LLVM Module Verification Failed!\n"; 
    // module_->print(llvm::errs(), nullptr); 
    return nullptr; 
    } 
    return std::move(module_);
  } 
} // namespace chimera::backend 
