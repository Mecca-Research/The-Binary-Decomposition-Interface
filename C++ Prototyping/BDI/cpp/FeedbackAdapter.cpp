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
     // STUB: Assume graph nodes involved in the last action sequence have 
     // stored eligibility traces (e.g., as metadata or in context) 
     std::cout << "FeedbackAdapter: Received reward " << reward << ". Calculating updates (STUB)." << std::endl; 
     // --- Replace with actual logic --- 
     // Example: Find nodes annotated with @LearnableParameter 
     // Retrieve their current value from context/memory 
     // Retrieve associated gradients or eligibility traces (needs separate mechanism) 
     // Calculate delta based on reward and traces/gradients 
     // --- End Replace --- 
     // Dummy Update: Increment parameter at Node 50 if reward > 0 
     // Example: Iterate through known parameter nodes (needs a way to identify them) 
     // Iterate through parameters that have gradients or traces stored in context 
     for (const auto& pair : context.parameter_gradients) { // Use gradient map 
              NodeID param_node_id = pair.first; 
     const BDIValueVariant& gradient = pair.second; 
     float eligibility = context.getEligibilityTrace(param_node_id).value_or(1.0f); // Get trace or default to 1 
     // Calculate delta (Example: simple gradient ascent * trace * reward) 
     auto grad_float = vm_ops::convertValue<float>(gradient); // Convert gradient 
     if (grad_float) { 
     float delta_value = learning_rate_ * reward * eligibility * (*grad_float); 
              pending_updates_.push_back({param_node_id, BDIValueVariant{delta_value}}); 
     // std::cout << "  -> Pending update for Node " << param_node_id << ": Delta " << delta_value << std::endl;
     std::vector<NodeID> parameter_nodes = {50, 65, 108}; // Example IDs 
     for (NodeID param_node_id : parameter_nodes) { 
     // Retrieve eligibility trace for this parameter from context (conceptual) 
     // float trace = context.getEligibilityTrace(param_node_id); // Needs context mechanism 
     float trace = (param_node_id == 50) ? 0.8f : 0.3f; // Dummy trace values 
         // Retrieve parameter type (assume FLOAT32) 
         BDIType param_type = BDIType::FLOAT32; // Assume known type 
         // Calculate TD-like delta: learning_rate * reward * trace 
         float delta_value = learning_rate_ * reward * trace; 
         // Create update variant based on parameter type 
         BDIValueVariant delta_variant; 
         if (param_type == BDIType::FLOAT32) delta_variant = BDIValueVariant{delta_value}; 
         // else if (param_type == BDIType::INT32) delta_variant = BDIValueVariant{static_cast<int32_t>(delta_value)}; // Add type handling 
         else continue; // Skip if type not handled 
         pending_updates_.push_back({param_node_id, delta_variant}); 
         //std::cout << "  -> Pending update for Node " << param_node_id << ": Delta " << delta_value << std::endl; 
         }
     } 
     // Clear context state after using it? Optional. 
     // context.clearIntelligenceState(); 
} 
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
     // Assume a different adapter for supervised learning 
   class SupervisedDeltaRuleAdapter : public FeedbackAdapter { 
   public: 
   // Target output port, Actual output port, Input source ports for weights 
       SupervisedDeltaRuleAdapter(PortRef target_port, PortRef actual_port, std::vector<PortRef> input_ports, float learning_rate = 0.01f); 
   void processFeedback(ExecutionContext& context, BDIGraph& graph) override; 
   const std::vector<ParameterUpdate>& getPendingUpdates() const; 
   void clearPendingUpdates(); 
   private: 
       PortRef target_output_port_; 
       PortRef actual_output_port_; 
   std::vector<PortRef> input_source_ports_; // Nodes providing inputs for weights being updated 
   std::vector<NodeID> weight_node_ids_; // NodeIDs of the weights themselves (assume META_CONST nodes) 
   float learning_rate_; 
   std::vector<ParameterUpdate> pending_updates_; 
   }; 
   SupervisedDeltaRuleAdapter::SupervisedDeltaRuleAdapter(PortRef target_port, PortRef actual_port, std::vector<PortRef> input_ports, float learn
       : target_output_port_(target_port), actual_output_port_(actual_port), input_source_ports_(std::move(input_ports)), learning_rate_(learning
    // TODO: Need a way to associate input_ports with corresponding weight_node_ids 
       }
    void SupervisedDeltaRuleAdapter::processFeedback(ExecutionContext& context, BDIGraph& graph) { 
        pending_updates_.clear(); 
   // 1. Get Target and Actual outputs (assume float) 
   auto target_opt = context.getPortValue(target_output_port_); 
   auto actual_opt = context.getPortValue(actual_output_port_); 
   if (!target_opt || !actual_opt) return; // Missing signal 
   auto target_val = vm_ops::convertValue<float>(target_opt.value()); 
   auto actual_val = vm_ops::convertValue<float>(actual_opt.value()); 
   if (!target_val || !actual_val) return; // Type error 
   // 2. Calculate Error 
   float error = target_val.value() - actual_val.value(); 
   // 3. Iterate through inputs/weights and calculate updates (Delta Rule: delta_W = eta * error * input) 
   if (input_source_ports_.size() != weight_node_ids_.size()) { 
   // Error: Mismatch between inputs and weights 
   return; 
        } 
   for(size_t i = 0; i < input_source_ports_.size(); ++i) { 
            NodeID weight_node_id = weight_node_ids_[i]; 
            PortRef input_source_port = input_source_ports_[i]; 
   auto input_val_opt = context.getPortValue(input_source_port); 
   if (!input_val_opt) continue; // Skip if input wasn't available? 
   auto input_float_opt = vm_ops::convertValue<float>(input_val_opt.value()); // Assume float inputs 
   if (!input_float_opt) continue; 
   // Calculate weight delta 
   float weight_delta = learning_rate_ * error * input_float_opt.value(); 
            pending_updates_.push_back({weight_node_id, BDIValueVariant{weight_delta}}); 
        } 
     } 
   // ... getPendingUpdates / clearPendingUpdates ... 
  }
} // namespace bdi::intelligence
