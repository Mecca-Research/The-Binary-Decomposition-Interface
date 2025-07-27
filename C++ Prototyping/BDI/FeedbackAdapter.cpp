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
// Assume BDI graph nodes exist for: 
// - Input activations (source nodes) 
// - Weights (META_CONST nodes, ID stored) 
// - Weighted sum (e.g., LINALG_DOT or sequence of MUL/ADD) 
// - Activation function (e.g., SIGMOID, RELU ops) 
// - Output activation (final node of layer) 
// - Target output value (from context) 
// - Downstream error signal (delta) (from context) 
class BackpropFeedbackAdapter : public FeedbackAdapter { 
// ... constructor stores relevant node IDs/references ... 
    NodeID output_node_id; 
    NodeID target_node_id; // Node providing target value 
    NodeID downstream_delta_node_id; // Node providing error signal from next layer 
std::vector<NodeID> weight_node_ids; 
std::vector<NodeID> input_node_ids; // Corresponding input activations 
float learning_rate; 
void processFeedback(ExecutionContext& context, BDIGraph& graph) override { 
        pending_updates_.clear(); 
// 1. Get Target, Actual Output, Downstream Delta (assume FLOAT32) 
auto target = vm_ops::convertValue<float>(context.getPortValue(target_node_id, 0).value_or(BDIValueVariant{0.0f})); 
auto actual = vm_ops::convertValue<float>(context.getPortValue(output_node_id, 0).value_or(BDIValueVariant{0.0f})); 
auto downstream_delta = vm_ops::convertValue<float>(context.getPortValue(downstream_delta_node_id, 0).value_or(BDIValueVariant{0.0f}))
 if (!target || !actual /* || !downstream_delta needed? */) { /* Error */ return; } 
// 2. Calculate Output Error (delta) for this layer 
// Depends on activation function derivative. Assume sigmoid derivative = output * (1 - output) 
float output_derivative = actual.value() * (1.0f - actual.value()); 
// If output layer: error = (target - actual) * output_derivative 
// If hidden layer: error = downstream_delta_sum * output_derivative (need weighted sum of deltas from next layer) 
float layer_delta = (target.value() - actual.value()) * output_derivative; // Simplified output layer case 
// 3. Calculate Weight Updates (delta_W = eta * layer_delta * input_activation) 
if (input_node_ids.size() != weight_node_ids.size()) { /* Error */ return; } 
for (size_t i = 0; i < weight_node_ids.size(); ++i) { 
auto input_activation_opt = context.getPortValue(input_node_ids[i], 0); 
if (!input_activation_opt) continue; 
auto input_act = vm_ops::convertValue<float>(input_activation_opt.value()); 
if (!input_act) continue; 
float weight_delta = learning_rate_ * layer_delta * input_act.value(); 
            pending_updates_.push_back({weight_node_ids[i], BDIValueVariant{weight_delta}}); 
// std::cout << "  BP Update for Weight Node " << weight_node_ids[i] << ": Delta " << weight_delta << std::endl; 
        } 
 // Assume config provides NodeIDs for: 
// output_activation_node, target_node, downstream_delta_node (if hidden), 
// activation_derivative_node (node calculating derivative of activation func), 
// weight_nodes (vector<NodeID>), input_activation_nodes (vector<NodeID>) 
// Also error_signal_output_nodes (map<NodeID input_act_node, NodeID output_for_its_error>) 
void BackpropFeedbackAdapter::processFeedback(ExecutionContext& context, BDIGraph& graph) { 
         pending_updates_.clear(); 
         context.clearGradients(); // Clear gradients from previous step 
try { 
// 1. Get required values (assume float) 
auto actual = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.output_activation_node, 0).value()); 
auto activation_deriv = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.activation_derivative_node, 0).value()); 
float layer_error_signal; // This layer's delta * activation_derivative 
// 2. Calculate Error Signal (Delta * Derivative) for this layer 
if (config_.is_output_layer) { 
auto target = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.target_node, 0).value()); 
float error = target - actual; // Simple error for now 
                 layer_error_signal = error * activation_deriv; 
             } 
else { 
// Hidden layer: Sum weighted deltas from downstream layer 
auto downstream_delta_sum = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.downstream_delta_sum_node, 0).val
                 layer_error_signal = downstream_delta_sum * activation_deriv; 
             } 
// 3. Calculate Weight Updates & Gradients for Previous Layer 
if (config_.input_activation_nodes.size() != config_.weight_node_ids.size()) throw BDIExecutionError("Input/Weight mismatch"); 
for (size_t i = 0; i < config_.weight_node_ids.size(); ++i) { 
                 NodeID weight_node_id = config_.weight_node_ids[i]; 
                 NodeID input_act_node_id = config_.input_activation_nodes[i]; 
auto input_activation_val = context.getPortValue(input_act_node_id, 0); 
if (!input_activation_val) throw BDIExecutionError("Missing input activation for backprop"); 
auto input_act = vm_ops::convertValueOrThrow<float>(input_activation_val.value()); 
// Calculate Weight Delta: eta * error_signal * input_activation 
float weight_delta = config_.learning_rate * layer_error_signal * input_act; 
                 pending_updates_.push_back({weight_node_id, BDIValueVariant{weight_delta}}); // Add delta (negative for descent) 
// Calculate Error Signal for Previous Layer (Propagate Backwards) 
if (!config_.is_input_layer) { 
auto weight_val_var = context.getPortValue(weight_node_id, 0); // Read current weight value 
if (!weight_val_var) throw BDIExecutionError("Cannot read weight for backprop"); 
auto weight_val = vm_ops::convertValueOrThrow<float>(weight_val_var.value()); 
                     BDIValueVariant error_for_prev_layer = BDIValueVariant{layer_error_signal * weight_val}; 
// Store this gradient/error signal associated with the *input activation* node 
// This allows the previous layer's adapter to sum incoming signals 
                     context.recordGradient(input_act_node_id, error_for_prev_layer); // Use gradient storage for backprop signal 
                 } 
             } 
       } catch (const std::exception& e) { /* ... error ... */ } 
    }
// 4. (Optional) Calculate and store gradient w.r.t inputs for previous layer 
// gradient_for_prev_layer = layer_delta * weight; Store this in context for prev layer adapter. 
// Assume a GradientDescentAdapter 
class GradientDescentAdapter : public FeedbackAdapter { 
public: 
    GradientDescentAdapter(float learning_rate = 0.01f) : learning_rate_(learning_rate) {} 
void processFeedback(ExecutionContext& context, BDIGraph& graph) override { 
        pending_updates_.clear(); 
std::cout << "FeedbackAdapter: Processing gradients..." << std::endl; 
// Iterate through all recorded gradients in the context 
for (const auto& grad_pair : context.parameter_gradients) { // Use actual gradient map 
            NodeID param_node_id = grad_pair.first; 
const BDIValueVariant& gradient = grad_pair.second; 
// Calculate delta (eta * gradient) -> Note: Gradient Descent SUBTRACTS 
// Need to negate the gradient or subtract the delta. Let's calculate positive delta first. 
            BDIValueVariant scaled_gradient = vm_ops::performMultiplication(gradient, BDIValueVariant{learning_rate_}); 
// Negate for descent step 
            BDIValueVariant delta_variant = vm_ops::performNegation(scaled_gradient); 
if (!std::holds_alternative<std::monostate>(delta_variant)) { 
                pending_updates_.push_back({param_node_id, delta_variant}); 
// std::cout << "  -> Pending GD update for Node " << param_node_id << std::endl; 
            } 
else {
 std::cerr << "  FeedbackAdapter Warning: Failed to calculate delta for gradient of Node " << param_node_id << std::endl; 
            } 
        } 
    }
// Clear gradients after processing? Optional, depends on usage. 
// context.clearIntelligenceState(); // Or just clear gradients 
// ... get/clear updates ... 
class QLearningFeedbackAdapter : public FeedbackAdapter { 
public: 
    // Configurable params: learning rate (alpha), discount factor (gamma) 
    // Needs info about: State representation nodes, Action taken node, Reward node 
    QLearningFeedbackAdapter(float alpha, float gamma, /* config telling where state/action/reward are */); 
    void QLearningFeedbackAdapter::processFeedback(ExecutionContext& context, BDIGraph& graph) { 
    pending_updates_.clear(); 
    try { 
        // 1. Get S, A, R, S' (Assuming these NodeIDs/Ports are configured) 
        //    Requires a mechanism to identify the nodes holding these values in the context. 
        //    e.g., reading from specific output ports set by the BDI graph execution. 
        auto current_state_repr = context.getPortValue(config_.current_state_port).value_or(BDIValueVariant{}); 
        auto action_taken_repr = context.getPortValue(config_.action_taken_port).value_or(BDIValueVariant{}); 
        auto reward_val = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.reward_port).value()); 
        auto next_state_repr = context.getPortValue(config_.next_state_port).value_or(BDIValueVariant{}); 
        // 2. Find max Q(S', a') for all possible next actions a' 
        float max_next_q = 0.0f; 
        // This requires iterating possible actions, finding/querying their Q-value nodes (or a function) 
        // for next_state_repr, and finding the max. Highly dependent on Q-value representation. 
        // max_next_q = lookupMaxQValue(next_state_repr, graph, context); // Placeholder lookup 
        // auto action_taken = getActionTaken(context); 
        // 3. Get current Q(S, A) 
        // NodeID current_q_node_id = findQValueNodeID(current_state_repr, action_taken_repr); // Lookup mechanism needed 
        NodeID current_q_node_id = 1000; // Placeholder ID 
        auto current_q_val_var = context.getPortValue(current_q_node_id, 0).value_or(BDIValueVariant{0.0f}); // Default Q=0 
        float current_q = vm_ops::convertValueOrThrow<float>(current_q_val_var); 
        // auto reward_val = getReward(context); 
        // 4. Get next state S' features (from context after next step simulation?) 
        // Calculate Update Delta: alpha * (R + gamma * max_next_q - current_q) 
        float td_error = reward_val + gamma_ * max_next_q - current_q; 
        float delta_q = alpha_ * td_error; 
        // auto next_state_features = getNextStateFeatures(context);  
        // 5. Find max Q(S', a') over all possible next actions a' 
        // Generate update 
        pending_updates_.push_back({current_q_node_id, BDIValueVariant{delta_q}}); 
        } catch (const std::exception& e) { 
           std::cerr << "QLearningFeedbackAdapter Error: " << e.what() << std::endl; 
       }
    }
        // float max_next_q = findMaxQValueForState(next_state_features); // Needs access to Q-value table/network 
        // 6. Get current Q(S, A) 
        // NodeID current_q_node_id = findQValueNode(current_state_features, action_taken); // Find node storing Q(S,A) 
        // auto current_q_opt = context.getPortValue(current_q_node_id, 0); 
        // auto current_q = vm_ops::convertValue<float>(current_q_opt.value()); 
        // 7. Calculate TD Error: R + gamma * max_next_q - current_q 
        // float td_error = reward_val + gamma_ * max_next_q - current_q.value(); 
        // 8. Calculate Update Delta: alpha * td_error 
        // float delta_q = alpha_ * td_error; 
        // 9. Add pending update for Q(S,A) node 
        // pending_updates_.push_back({current_q_node_id, BDIValueVariant{delta_q}}); 
        std::cout << "QLearningFeedbackAdapter::processFeedback - STUBBED" << std::endl; 
    }
    // ... other methods ... 
private: 
    float alpha_; // Learning rate 
    float gamma_; // Discount factor 
    // ... configuration ... 
    std::vector<ParameterUpdate> pending_updates_; 
}; 
private: 
float learning_rate_; 
std::vector<ParameterUpdate> pending_updates_; 
};
} // namespace bdi::intelligence
