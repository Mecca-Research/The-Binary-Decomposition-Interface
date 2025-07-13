 #include "FeedbackAdapter.hpp"
 #include "VMTypeOperations.hpp" // For convertValue
 #include <iostream>
 namespace bdi::intelligence {
 // --- BasicRewardFeedbackAdapter --
BasicRewardFeedbackAdapter::BasicRewardFeedbackAdapter(PortRef reward_signal_port, float learning_rate)
    : reward_port_(reward_signal_port), learning_rate_(learning_rate) {}
 void BasicRewardFeedbackAdapter::processFeedback(ExecutionContext& context, BDIGraph& graph) {
    pending_updates_.clear(); // Clear previous updates
 void BasicRewardFeedbackAdapter::calculateRewardBasedUpdates(ExecutionContext& context, float reward) { 
     // STUB: Simulate finding relevant parameters and calculating simple update 
     std::cout << "FeedbackAdapter: Received reward " << reward << ". Calculating updates (STUB)." << std::endl; 
     // --- Replace with actual logic --- 
     // Example: Find nodes annotated with @LearnableParameter 
     // Retrieve their current value from context/memory 
     // Retrieve associated gradients or eligibility traces (needs separate mechanism) 
     // Calculate delta based on reward and traces/gradients 
     // --- End Replace --- 
     // Dummy Update: Increment parameter at Node 50 if reward > 0 
     if (reward > 0.0f) {
         NodeID target_param_node = 50; // Example target 
         // Assume parameter is FLOAT32 
         auto current_val_opt = context.getPortValue(target_param_node, 0); // Assume param value available on output 0 
         if(current_val_opt) { 
             auto current_f32_opt = vm_ops::convertValue<float>(current_val_opt.value()); 
             if (current_f32_opt) { 
                 float delta_value = learning_rate_ * reward; // Simple update 
                 pending_updates_.push_back({target_param_node, BDIValueVariant{delta_value}}); 
    //std::cout << "  -> Pending update for Node " << target_param_node << ": Add Delta " << delta_value << std::endl; 
    // 1. Get the reward signal
    auto reward_var_opt = context.getPortValue(reward_port_);
    if (!reward_var_opt) {
        // std::cerr << "FeedbackAdapter Warning: Reward signal not found at specified port." << std::endl;
        return; // No reward, no update
    }
    // Convert reward to a float (assuming reward is numeric)
    auto reward_opt = vm_ops::convertValue<float>(reward_var_opt.value());
     if (!reward_opt) {
        std::cerr << "FeedbackAdapter Warning: Reward signal is not convertible to float." << std::endl;
        return;
     }
     float reward = reward_opt.value();
     // 2. Calculate updates based on reward and potentially other context info
     //    (e.g., eligibility traces, gradients stored by other processes)
     calculateRewardBasedUpdates(context, reward);
 }
 const std::vector<BasicRewardFeedbackAdapter::ParameterUpdate>& BasicRewardFeedbackAdapter::getPendingUpdates() const {
    return pending_updates_;
 }
 void BasicRewardFeedbackAdapter::clearPendingUpdates() {
    pending_updates_.clear();
 }
 // STUB: This needs actual learning logic (e.g., Hebbian, gradient ascent, RL algorithms)
 // It needs access to which parameters contributed to the actions leading to this reward.
 void BasicRewardFeedbackAdapter::calculateRewardBasedUpdates(ExecutionContext& context, float reward) {
     std::cout << "FeedbackAdapter: Received reward " << reward << ". Calculating updates (STUB)." << std::endl;
     // --- Placeholder Logic --
     // Assume we somehow know NodeID 50 has a FLOAT32 parameter we want to adjust based on reward
     if (reward > 0.5f) { // Example threshold
         NodeID target_param_node = 50;
         float delta_value = reward * learning_rate_; // Simple gradient ascent like update
         pending_updates_.push_back({target_param_node, BDIValueVariant{delta_value}});
         std::cout << "  -> Pending update for Node " << target_param_node << ": Delta " << delta_value << std::endl;
     }
     // In a real system, this would involve retrieving gradients or eligibility traces
     // stored in the ExecutionContext or MetadataStore by other parts of the computation graph.
 }
 } // namespace bdi::intelligence
