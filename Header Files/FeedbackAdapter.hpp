 #ifndef BDI_INTELLIGENCE_FEEDBACKADAPTER_HPP
 #define BDI_INTELLIGENCE_FEEDBACKADAPTER_HPP
 #include "../runtime/ExecutionContext.hpp"
 #include "../core/graph/BDIGraph.hpp" // Maybe needed for context
 namespace bdi::intelligence {
 using namespace bdi::runtime;
 using namespace bdi::core::graph;
 // Interface/Base class for processing feedback signals and generating updates
 class FeedbackAdapter {
 public:
    virtual ~FeedbackAdapter() = default;
    // Processes feedback available in the context and determines necessary actions
    // (e.g., calculate gradients, determine parameter updates, request graph rewiring)
    // Might return a structure indicating requested changes or directly interact
    // with other intelligence components.
    virtual void processFeedback(ExecutionContext& context, BDIGraph& graph /* Optional graph context */) = 0;
 };
 // Example concrete adapter (could be specialized for different learning types)
 class BasicRewardFeedbackAdapter : public FeedbackAdapter {
 public:
    // Constructor could take parameters like learning rate, reward signal source etc.
    BasicRewardFeedbackAdapter(PortRef reward_signal_port, float learning_rate = 0.01f);
    void processFeedback(ExecutionContext& context, BDIGraph& graph) override;
    // --- Data needed for learning --
    // Store calculated updates (e.g., gradients, deltas) associated with specific parameters (NodeID/PayloadOffset?)
    struct ParameterUpdate {
        NodeID target_node;
        // size_t payload_offset; // If updating part of a payload
        BDIValueVariant delta; // The change to apply (e.g., gradient * learning_rate)
    };
    const std::vector<ParameterUpdate>& getPendingUpdates() const;
    void clearPendingUpdates();
 private:
    PortRef reward_port_; // Where to read the reward signal from context
    float learning_rate_;
    std::vector<ParameterUpdate> pending_updates_;
    // Internal methods to calculate updates based on reward and context history (e.g., eligibility traces)
    void calculateRewardBasedUpdates(ExecutionContext& context, float reward);
 };
 } // namespace bdi::intelligence
 #endif // BDI_INTELLIGENCE_FEEDBACKADAPTER_HPP
