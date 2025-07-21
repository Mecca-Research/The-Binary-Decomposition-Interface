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
    // ... (Get input llvm::Value*s as before) ... 
    llvm::Value* result_val = nullptr; 
    // Generate LLVM instruction based on BDI OpType 
    switch (node.operation) { 
    // ... Meta Start/End, Const ... 
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
             else if (llvm_inputs[0]->getType()->isFloatingPointTy()) result_val = builder_.CreateFAdd(llvm_inputs[0], llvm_inputs[1], 
             } else { /* Error */ } 
             break; 
             // ... Implement MUL, DIV (SDiv/UDiv/FDiv), REM (SRem/URem/FRem), NEG (Sub 0, X / FNeg) ... 
             // Bitwise (Use integer instructions) 
             case BDIOperationType::BIT_AND: if(llvm_inputs.size()==2) result_val = builder_.CreateAnd(llvm_inputs[0], llvm_inputs[1], "andtmp"); b
             case BDIOperationType::BIT_OR: if(llvm_inputs.size()==2) result_val = builder_.CreateOr(llvm_inputs[0], llvm_inputs[1], "ortmp"); brea
             case BDIOperationType::BIT_XOR: if(llvm_inputs.size()==2) result_val = builder_.CreateXor(llvm_inputs[0], llvm_inputs[1], "xortmp"); b
             case BDIOperationType::BIT_NOT: if(llvm_inputs.size()==1) result_val = builder_.CreateNot(llvm_inputs[0], "nottmp"); break; 
             case BDIOperationType::BIT_SHL: if(llvm_inputs.size()==2) result_val = builder_.CreateShl(llvm_inputs[0], llvm_inputs[1], "shltmp"); b
             case BDIOperationType::BIT_SHR: if(llvm_inputs.size()==2) result_val = builder_.CreateLShr(llvm_inputs[0], llvm_inputs[1], "shrtmp"); 
             case BDIOperationType::BIT_ASHR: if(llvm_inputs.size()==2) result_val = builder_.CreateAShr(llvm_inputs[0], llvm_inputs[1], "ashrtmp")
             // Comparison 
             case BDIOperationType::CMP_EQ: if(llvm_inputs.size()==2) result_val = builder_.CreateICmpEQ(llvm_inputs[0], llvm_inputs[1], "eqtmp"); 
             case BDIOperationType::CMP_NE: if(llvm_inputs.size()==2) result_val = builder_.CreateICmpNE(llvm_inputs[0], llvm_inputs[1], "netmp"); 
             case BDIOperationType::CMP_LT: if(llvm_inputs.size()==2) result_val = builder_.CreateICmpSLT(llvm_inputs[0], llvm_inputs[1], "lttmp");
             // ... Implement other comparisons (SLE, ULE, OLE, SGT, UGT, OGT, SGE, UGE, OGE) ...  
        } 
         // Memory (Using GEP for address calculation is crucial here) 
         case BDIOperationType::MEM_LOAD: { 
             if (llvm_inputs.size() != 1) return; // Address must be provided 
             llvm::Value* ptr_input = llvm_inputs[0]; 
             llvm::Type* load_type = mapBDITypeToLLVM(node.getOutputType(0)); 
             if (!load_type || !ptr_input->getType()->isPointerTy()) return; // Invalid type or ptr 
             // Ensure pointer points to the correct type, GEP often needed before load 
             // llvm::Value* typed_ptr = builder_.CreateBitOrPointerCast(ptr_input, load_type->getPointerTo()); 
             result_val = builder_.CreateLoad(load_type, ptr_input, "loadtmp"); // Assume ptr is already correct type for now 
             break; 
        } 
         case BDIOperationType::MEM_STORE: { 
              if (llvm_inputs.size() != 2) return; // Need address and value 
              llvm::Value* ptr_input = llvm_inputs[0]; // Address 
              llvm::Value* val_input = llvm_inputs[1]; // Value 
              if (!ptr_input->getType()->isPointerTy()) return; 
              // Ensure pointer type matches value type 
               // llvm::Value* typed_ptr = builder_.CreateBitOrPointerCast(ptr_input, val_input->getType()->getPointerTo()); 
               builder_.CreateStore(val_input, ptr_input); // Assume ptr is correct type 
               break; 
         }
         case BDIOperationType::MEM_ALLOC: { // Translate to stack allocation (alloca) 
             // Get size from input or node data? Assume input 0 is size (e.g., uint64) 
             if (llvm_inputs.empty()) return; 
             llvm::Value* size_val = llvm_inputs[0]; 
             llvm::Type* alloc_type = mapBDITypeToLLVM(node.getOutputType(0)); // Type being allocated? Or just return i8*? Let's assume outpu
             if (!alloc_type || !size_val->getType()->isIntegerTy()) return; 
             // Alloca typically allocates specific types, not raw bytes easily from dynamic size 
             // Usually: builder.CreateAlloca(llvm_type, size_array_val_opt, name); 
             // For dynamic size, might need malloc call or different approach 
             llvm::errs() << "BDIToLLVMIR Warning: MEM_ALLOC to Alloca translation is complex/stubbed.\n"; 
             // Placeholder: allocate i8 array of size (treat as void*) 
             llvm::Type* byte_type = builder_.getInt8Ty(); 
             result_val = builder_.CreateAlloca(byte_type, size_val, "allocatmp"); 
             break; 
         }
        // Control Flow (Terminators - linkage done separately) 
        case BDIOperationType::CTRL_JUMP: /* CreateBr - linking done later */ break; 
        case BDIOperationType::CTRL_BRANCH_COND: /* CreateCondBr - linking done later */ break; 
        case BDIOperationType::CTRL_RETURN: { 
             if (!llvm_inputs.empty()) builder_.CreateRet(llvm_inputs[0]); // Return value 
             else builder_.CreateRetVoid(); 
             break; 
        } 
        case BDIOperationType::CTRL_CALL: { 
             // Need function signature lookup based on target 
             // llvm::Function* target_func = ...; 
             // std::vector<llvm::Value*> llvm_args = llvm_inputs; // Get args 
             // result_val = builder_.CreateCall(target_func, llvm_args, "calltmp"); 
              llvm::errs() << "BDIToLLVMIR Warning: CALL translation stubbed.\n"; 
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
            // ... Implement other ops (Conversions -> Trunc, SExt, ZExt, SIToFP, FPToSI etc.) ... 
            default: /* Warning */ 
             llvm::errs() << "BDIToLLVMIR Warning: Unhandled BDI OpType: " << static_cast<int>(node.operation) << "\n"; 
             break; 
    }
    // ... Store result_val in map ... 
    // ... Handle terminators ... 
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
    // 1. Identify Basic Block Headers (Function Entry, Targets of Jumps/Branches) 
    // Requires CFG analysis on BDI graph 
    std::set<NodeID> block_headers;
    // Find entry node(s)... add to headers 
    // Find successors of BRANCH_COND and JUMP nodes... add to headers 
    // ...
    // 2. Create llvm::Function and entry BasicBlock 
    // ... setupFunction ... 
    // 3. Create llvm::BasicBlock for each identified header 
    // for (NodeID header_id : block_headers) { 
    //     llvm::BasicBlock* bb = llvm::BasicBlock::Create(llvm_context_, "bb_" + std::to_string(header_id), current_function_); 
    //     bdi_node_to_llvm_block_[header_id] = bb; 
    // } 
    // 4. Iterate through nodes (in RPO within each block ideally) 
    std::vector<NodeID> processing_order; // Get RPO order 
    std::set<NodeID> visited; 
    // ... Perform traversal (DFS/RPO) starting from entry ... 
    for (NodeID node_id : processing_order) { 
        BDINode* node = graph.getNodeMutable(node_id); 
        if (!node) continue; 
        // Set insert point to the correct block 
        // if (bdi_node_to_llvm_block_.count(node_id)) { 
        //     builder_.SetInsertPoint(bdi_node_to_llvm_block_.at(node_id)); 
        // } else if (!builder_.GetInsertBlock()) { /* Error or handle hanging nodes */ } 
        // Convert node instructions 
        convertNode(*node); 
        // Handle terminators (JUMP, BRANCH, RETURN) by creating corresponding LLVM terminators 
        // Connect blocks using CreateBr, CreateCondBr 
    }
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
