 #ifndef BDI_INTELLIGENCE_METALEARNINGENGINE_HPP
 #define BDI_INTELLIGENCE_METALEARNINGENGINE_HPP
 #include "FeedbackAdapter.hpp" // Uses updates calculated by adapters
 #include "../core/graph/BDIGraph.hpp"
 #include "../runtime/ExecutionContext.hpp" // For applying payload changes
 namespace bdi::intelligence {
 // Applies parameter updates calculated by FeedbackAdapters to the graph/runtime state
 class MetaLearningEngine {
 public:
    // Constructor might take reference to graph, context, etc. if needed directly
    MetaLearningEngine(BDIGraph& target_graph, ExecutionContext& target_context);
    // Apply updates calculated by a specific adapter
    // Returns true if any parameters were successfully updated
    bool applyUpdates(const FeedbackAdapter& adapter);
    // Or apply a specific list of updates
    bool applyUpdates(const std::vector<BasicRewardFeedbackAdapter::ParameterUpdate>& updates);
 private:
    BDIGraph& graph_; // Reference to the graph being modified
    ExecutionContext& context_; // Reference to the context holding runtime values (if needed)
    // Helper to apply a single parameter update
    bool applySingleUpdate(const BasicRewardFeedbackAdapter::ParameterUpdate& update);
 };
 } // namespace bdi::intelligence
 #endif // BDI_INTELLIGENCE_METALEARNINGENGINE_HPP
