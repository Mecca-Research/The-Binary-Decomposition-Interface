#ifndef CHIMERA_BACKEND_BDITOLLVMIR_HPP 
#define CHIMERA_BACKEND_BDITOLLVMIR_HPP 
#include "BDIGraph.hpp" 
#include "LLVMContext.h" 
#include "Module.h" 
#include "IRBuilder.h"
#include <memory> 
#include <unordered_map> 
#include <string> 
namespace bdi::core::graph { struct BDINode; } // Forward declare 
namespace chimera::backend { 
using namespace bdi::core::graph; 
using namespace bdi::core::types; 
class BDIToLLVMIR { 
public: 
    BDIToLLVMIR(); // Initializes LLVM components 
    // Convert the entire BDI graph into an LLVM Module 
    std::unique_ptr<llvm::Module> convertGraph(BDIGraph& graph, const std::string& module_id = "bdi_module"); 
private: 
    llvm::LLVMContext llvm_context_; 
    llvm::IRBuilder<> builder_; 
    std::unique_ptr<llvm::Module> module_; 
    // Mappings during conversion 
    std::unordered_map<NodeID, llvm::Value*> bdi_node_to_llvm_value_; // Maps BDI output ports to LLVM Values 
    std::unordered_map<NodeID, llvm::BasicBlock*> bdi_node_to_llvm_block_; // Maps BDI nodes (potential block entries) to LLVM BBs 
    llvm::Function* current_function_ = nullptr; 
    // Conversion helpers 
    llvm::Type* mapBDITypeToLLVM(BDIType bdi_type); 
    llvm::Constant* getLLVMConstant(const BDIValueVariant& value_var); 
    void convertNode(BDINode& node); 
    void setupFunction(BDINode& start_node); // Create LLVM function for BDI entry point 
    void setupBasicBlocks(BDIGraph& graph); // Create basic blocks for control flow nodes 
}; 
} // namespace chimera::backend 
#endif // CHIMERA_BACKEND_BDITOLLVMIR_HPP 
