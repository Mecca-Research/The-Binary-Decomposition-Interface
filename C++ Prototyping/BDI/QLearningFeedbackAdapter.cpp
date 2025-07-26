#include "FeedbackAdapter.hpp" // Assuming QLearningFeedbackAdapter defined in header 
#include "VMTypeOperations.hpp" 
#include <cmath> // for std::fabs 
#include <limits> 
namespace bdi::intelligence { 
// --- QLearningFeedbackAdapter --- 
// Assume constructor takes config for port IDs, alpha, gamma 
// struct QLearningConfig { PortRef state_port, action_port, reward_port, next_state_port; float alpha, gamma; }; 
void QLearningFeedbackAdapter::processFeedback(ExecutionContext& context, BDIGraph& graph) { 
     pending_updates_.clear(); 
     try { 
         // 1. Get S, A, R, S' from context 
         auto current_state_var = context.getPortValue(config_.state_port).value_or(BDIValueVariant{}); 
         auto action_var = context.getPortValue(config_.action_taken_port).value_or(BDIValueVariant{}); 
         auto reward_val = vm_ops::convertValueOrThrow<float>(context.getPortValue(config_.reward_port).value()); 
         auto next_state_var = context.getPortValue(config_.next_state_port).value_or(BDIValueVariant{}); 
         // TODO: Convert state variants (current_state_var, next_state_var) into hashable keys 
         //       Convert action variant (action_var) into an ActionID (e.g., int) 
         uint64_t current_state_hash = stateToHash(current_state_var); 
         int32_t action_id = actionToInt(action_var); 
         uint64_t next_state_hash = stateToHash(next_state_var); 
         // 2. Find max Q(S', a') 
         float max_next_q = -std::numeric_limits<float>::infinity(); 
         // for (int32_t next_action_id : getAllPossibleActions(next_state_hash)) { 
         //     NodeID q_node_next = findQValueNodeID(next_state_hash, next_action_id); 
         //     auto q_val_next_opt = context.getPortValue(q_node_next, 0).value_or(BDIValueVariant{0.0f}); 
         //     max_next_q = std::max(max_next_q, vm_ops::convertValueOrThrow<float>(q_val_next_opt)); 
         // } 
          max_next_q = 0.0f; // Placeholder lookup 
         // 3. Get current Q(S, A) 
         NodeID current_q_node_id = findQValueNodeID(current_state_hash, action_id); 
         auto current_q_val_var = context.getPortValue(current_q_node_id, 0).value_or(BDIValueVariant{0.0f}); 
         float current_q = vm_ops::convertValueOrThrow<float>(current_q_val_var); 
         // 4. Calculate Update Delta: alpha * (R + gamma * max_next_q - current_q) 
         float td_error = reward_val + config_.gamma * max_next_q - current_q; 
         float delta_q = config_.alpha * td_error; 
         // 5. Generate update (Add delta to current Q-value) 
         pending_updates_.push_back({current_q_node_id, BDIValueVariant{delta_q}}); 
     } catch (const std::exception& e) { 
         std::cerr << "QLearningFeedbackAdapter Error: " << e.what() << std::endl; 
     } 
} 
// --- Need helper implementations --- 
// uint64_t QLearningFeedbackAdapter::stateToHash(const BDIValueVariant& state) { /* ... */ return 0; } 
// int32_t QLearningFeedbackAdapter::actionToInt(const BDIValueVariant& action) { /* ... */ return 0; } 
// NodeID QLearningFeedbackAdapter::findQValueNodeID(uint64_t state_hash, int32_t action_id) { /* ... Requires Q-table representation knowledg
 // float QLearningFeedbackAdapter::findMaxQValueForState(uint64_t state_hash) { /* ... */ return 0.0f; } 
} // namespace 
