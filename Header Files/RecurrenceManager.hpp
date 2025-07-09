 #ifndef BDI_INTELLIGENCE_RECURRENCEMANAGER_HPP
 #define BDI_INTELLIGENCE_RECURRENCEMANAGER_HPP
 #include "../runtime/ExecutionContext.hpp"
 #include "../core/graph/BDIGraph.hpp" // For node IDs
 #include <vector>
 #include <map>
 namespace bdi::intelligence {
 // Manages state propagation across time steps for recurrent connections
 // This is conceptual - requires specific BDI operations like RECUR_READ_STATE, RECUR_WRITE_STATE
 class RecurrenceManager {
 public:
    RecurrenceManager() = default;
    // Called at the end of a time step to store state for the next step
    void storeRecurrentStates(ExecutionContext& current_context, const std::vector<NodeID>& state_producing_nodes);
    // Called at the beginning of a time step to load state from the previous step
    void loadRecurrentStates(ExecutionContext& next_context, const std::map<NodeID, NodeID>& state_mapping); // Map: Prev Step Node -> Next Step
 Node
    void clearHistory();
 private:
    // Store state values from the previous time step, keyed by the NodeID that produced them
    std::unordered_map<NodeID, BDIValueVariant> previous_step_states_;
 };
 } // namespace bdi::intelligence
 #endif // BDI_INTELLIGENCE_RECURRENCEMANAGER_HPP
