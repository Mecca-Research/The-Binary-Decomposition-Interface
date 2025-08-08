#ifndef BDI_OPTIMIZER_PASSES_COMMONSUBEXPRESSIONELIMINATION_HPP 
#define BDI_OPTIMIZER_PASSES_COMMONSUBEXPRESSIONELIMINATION_HPP 
#include "OptimizationPassBase.hpp" 
#include <unordered_map> 
#include <vector> 
namespace bdi::optimizer { 
// Simple local CSE pass (operates within basic blocks) 
class CommonSubexpressionElimination : public OptimizationPassBase { 
public: 
    CommonSubexpressionElimination() : OptimizationPassBase("CommonSubexpressionElimination") {} 
// Override run or visitGraph to implement CSE logic per block 
bool run(BDIGraph& graph) override; 
private: 
// Hash for identifying potential common subexpressions 
struct ExpressionHash { 
        BDIOperationType op; 
std::vector<NodeID> input_value_nodes; // IDs of nodes providing inputs 
// Include constant payload if relevant? 
// Need a robust hash function over this struct 
size_t operator()(const ExpressionHash& k) const; 
bool operator==(const ExpressionHash& other) const; 
    }; 
// Map expression hash to the NodeID that first computed it in the block 
std::unordered_map<ExpressionHash, NodeID, ExpressionHash> available_expressions_; 
// Requires Basic Block analysis first 

void processBasicBlock(const std::vector<NodeID>& block_nodes, BDIGraph& graph);
};
} // namespace bdi::optimizer 
#endif // BDI_OPTIMIZER_PASSES_COMMONSUBEXPRESSIONELIMINATION_HPP 
