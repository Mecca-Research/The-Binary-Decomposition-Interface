 #include "RecurrenceManager.hpp"
 #include <iostream>
 namespace bdi::intelligence {
 void RecurrenceManager::storeRecurrentStates(ExecutionContext& current_context, const std::vector<NodeID>& state_producing_nodes) {
    previous_step_states_.clear(); // Clear previous state history
    // std::cout << "RecurrenceManager: Storing states for nodes: ";
    for (NodeID node_id : state_producing_nodes) {
         // Assuming state is output port 0
         auto state_opt = current_context.getPortValue(node_id, 0);
         if (state_opt) {
             previous_step_states_[node_id] = state_opt.value();
             // std::cout << node_id << " ";
         } else {
             // std::cerr << "Warning: State not found for recurrent node " << node_id << std::endl;
         }
    }
    // std::cout << std::endl;
 }
 void RecurrenceManager::loadRecurrentStates(ExecutionContext& next_context, const std::map<NodeID, NodeID>& state_mapping) {
    // std::cout << "RecurrenceManager: Loading states..." << std::endl;
    for (const auto& pair : state_mapping) {
        NodeID prev_step_node_id = pair.first;
        NodeID next_step_node_id = pair.second; // This is the node that needs the state as input
        auto it = previous_step_states_.find(prev_step_node_id);
        if (it != previous_step_states_.end()) {
            // Set the *output* of the corresponding node in the *next* context
            // This makes the state available as if computed in the previous step.
            // Convention: Set output port 0 of the destination node.
            next_context.setPortValue(next_step_node_id, 0, it->second);
             // std::cout << "  Loaded state from Node " << prev_step_node_id << " into Node " << next_step_node_id << " Port 0." << std::endl;
        } else {
            // std::cerr << "Warning: Previous state not found for recurrent node " << prev_step_node_id << std::endl;
            // How to handle missing state? Initialize to default? Error?
        }
    }
 }
 void RecurrenceManager::clearHistory() {
    previous_step_states_.clear();
 }
 } // namespace bdi::intelligence
