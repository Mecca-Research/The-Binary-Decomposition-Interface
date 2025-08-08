 #include "MetaLearningEngine.hpp"
 #include "VMTypeOperations.hpp" // For operations on variants
 #include <iostream>
 namespace bdi::intelligence {
 MetaLearningEngine::MetaLearningEngine(BDIGraph& target_graph, ExecutionContext& target_context)
    : graph_(target_graph), context_(target_context) {}
 bool MetaLearningEngine::applyUpdates(const FeedbackAdapter& adapter) {
    // Assuming adapter is BasicRewardFeedbackAdapter for now
    const auto* reward_adapter = dynamic_cast<const BasicRewardFeedbackAdapter*>(&adapter);
    if (!reward_adapter) {
         std::cerr << "MetaLearningEngine Error: Unsupported FeedbackAdapter type." << std::endl;
         return false;
    }
    return applyUpdates(reward_adapter->getPendingUpdates());
 }
 bool MetaLearningEngine::applyUpdates(const std::vector<BasicRewardFeedbackAdapter::ParameterUpdate>& updates) {
    bool applied_any = false;
    std::cout << "MetaLearningEngine: Applying " << updates.size() << " updates..." << std::endl;
    for (const auto& update : updates) {
        if (applySingleUpdate(update)) {
            applied_any = true;
        }
    }
    return applied_any;
 }
 bool MetaLearningEngine::applySingleUpdate(const BasicRewardFeedbackAdapter::ParameterUpdate& update) {
    BDINode* target_node = graph_.getNodeMutable(update.target_node);
    if (!target_node) { /* ... Error ... */ return false; } 
         std::cerr << "  Error: Target node " << update.target_node << " for update not found." << std::endl;
        return false;
    }
    // Assume the update delta applies to the node's payload (parameter)
    // This requires knowing the structure/type of the payload.
    // Simple case: Payload holds a single value matching the delta type.
    // Assume update applies to payload, could also apply to context value? 
    // Assume simple case: Payload IS the parameter value. 
    BDIValueVariant current_payload_val = ExecutionContext::payloadToVariant(target_node->payload);
    BDIValueVariant delta_val = update.delta;
    // Perform the update (e.g., add the delta)
    // Needs type compatibility checks and promotion logic!
    // Perform update operation (e.g., Add delta) 
    BDIValueVariant updated_val; 
    try { 
    BDIValueVariant updated_val = vm_ops::performAddition(current_payload_val, delta_val); // Use VM ops for consistency
    catch (const BDIExecutionError& e) {
    if (std::holds_alternative<std::monostate>(updated_val)) {
         std::cerr << "  Error: Failed to calculate updated value for Node " << update.target_node << "." << std::endl;
        return false;
    }
    if (std::holds_alternative<std::monostate>(updated_val)) { /* ... Error ... */ return false; } 
    // Convert back to payload and update the node
    TypedPayload new_payload = ExecutionContext::variantToPayload(updated_val);
    if (new_payload.type == BDIType::UNKNOWN) { /* ... Error ... */ return false; } 
        std::cerr << "  Error: Failed to convert updated value back to payload for Node " << update.target_node << "." << std::endl;
        return false;
    }
    // std::cout << "  Applying update to Node " << update.target_node << ": New payload type " << core::types::bdiTypeToString(new_payload.type) <<std::endl;
    target_node->payload = std::move(new_payload);
    return true;
    // TODO: If parameters are in memory, need to generate STORE operation instead! 
 }
 } // namespace bdi::intelligence
