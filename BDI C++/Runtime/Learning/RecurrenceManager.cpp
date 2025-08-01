 #include "RecurrenceManager.hpp"
 #include <iostream>
 namespace bdi::intelligence {
 void RecurrenceManager::storeRecurrentStates(ExecutionContext& current_context, const std::vector<NodeID>& state_producing_node_ids) {
    // It's safer to copy the state rather than clear and refill, 
    // in case loading happens before storing in complex scenarios. 
    decltype(previous_step_states_) next_step_buffer; 
    previous_step_states_.clear(); // Clear previous state history
    // std::cout << "RecurrenceManager: Storing states for nodes: ";
    for (NodeID node_id : state_producing_nodes) {
         // Assuming state is output port 0
         auto state_opt = current_context.getPortValue(node_id, 0);
         if (state_opt) {
             previous_step_states_[node_id] = state_opt.value();
         // std::cout << "RecurrenceManager: Storing states for nodes: "; 
    for (NodeID producer_node_id : state_producing_node_ids) { 
         // Assuming state is output port 0 - could be configurable 
         auto state_opt = current_context.getPortValue(producer_node_id, 0); 
         if (state_opt) {
             next_step_buffer[producer_node_id] = state_opt.value(); 
             // std::cout << node_id << " ";
         } else {
             // std::cerr << "Warning: State not found for recurrent node " << node_id << std::endl;
         }
    }
    // std::cout << std::endl;
    // Atomically swap (or lock and assign) the buffer for the next step 
    previous_step_states_ = std::move(next_step_buffer); 
 }
    // Load states into the context for the upcoming timestep 
 void RecurrenceManager::loadRecurrentStates(ExecutionContext& next_context, const std::map<NodeID, NodeID>& state_mapping) { 
    // std::cout << "RecurrenceManager: Loading states..." << std::endl; 
    for (const auto& pair : state_mapping) { 
        NodeID prev_step_producer_id = pair.first; // ID of node that produced state last step 
        NodeID next_step_consumer_id = pair.second; // ID of node needing state this step (e.g., RECUR_READ) 
        auto it = previous_step_states_.find(prev_step_producer_id); 
        if (it != previous_step_states_.end()) { 
            // Set the *output* port 0 of the *consumer* node in the *next* context 
            // This makes the previous state available as if the consumer node just produced it. 
            next_context.setPortValue(next_step_consumer_id, 0, it->second); 
            // std::cout << "  Loaded state from PrevNode " << prev_step_producer_id 
            //           << " into NextNode " << next_step_consumer_id << " Port 0." << std::endl; 
        } else { 
            std::cerr << "RecurrenceManager Warning: Previous state not found for Node " << prev_step_producer_id << std::endl; 
            // Action: Error? Initialize consumer output to default? Leave unset? Depends on desired semantics. 
            // For now, leave it unset, consumer node might handle missing input. 
        } 
    }
 } 
 void RecurrenceManager::clearHistory() { 
    previous_step_states_.clear(); 
} 
} // namespace bdi::intelligence 
