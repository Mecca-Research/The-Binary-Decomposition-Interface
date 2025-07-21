 #ifndef BDI_OPTIMIZER_PASSES_DEADCODEELIMINATION_HPP
 #define BDI_OPTIMIZER_PASSES_DEADCODEELIMINATION_HPP
 #include "../OptimizationPassBase.hpp"
 #include <set>
 namespace bdi::optimizer {
 class DeadCodeElimination : public OptimizationPassBase {
 public:
    DeadCodeElimination() : OptimizationPassBase("DeadCodeElimination") {}
    // Override run to perform analysis and removal
    bool run(BDIGraph& graph) override;
 private:
    std::set<NodeID> live_nodes_; // Nodes identified as live
    BDIGraph* current_graph_ = nullptr;
    // Mark nodes reachable backwards from essential nodes
    void markLiveNodes(NodeID current_node_id);
    // Check if a node has side effects (memory write, IO, volatile op, etc.)
    bool hasSideEffects(const BDINode& node);
 };
 } // namespace bdi::optimizer
 #endif // BDI_OPTIMIZER_PASSES_DEADCODEELIMINATION_HPP
