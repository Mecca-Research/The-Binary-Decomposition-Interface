 #include "DeadCodeElimination.hpp"
 #include <iostream>
 #include <vector>
 #include <algorithm>
 namespace bdi::optimizer {
 bool DeadCodeElimination::hasSideEffects(const BDINode& node) {
    // Define operations considered to have side effects
    switch (node.operation) {
        case BDIOperationType::MEM_STORE:
        case BDIOperationType::MEM_FREE:
        case BDIOperationType::IO_WRITE_PORT:
        case BDIOperationType::IO_PRINT:
        case BDIOperationType::META_END: // End node is essential
        case BDIOperationType::CTRL_RETURN: // Return is essential
        case BDIOperationType::SYNC_MUTEX_LOCK: // Synchronization ops
        case BDIOperationType::SYNC_MUTEX_UNLOCK:
        case BDIOperationType::SYNC_ATOMIC_RMW:
        case BDIOperationType::COMM_CHANNEL_SEND: // Communication ops
            return true;
        // MEM_ALLOC might be considered dead if the returned pointer isn't used,
        // but conservatively mark it live for now, assuming memory state matters.
        case BDIOperationType::MEM_ALLOC:
             return true; // Conservative assumption
        default:
            return false;
    }
 }
 void DeadCodeElimination::markLiveNodes(NodeID current_node_id) {
    // Use depth-first search backwards from essential nodes
    if (live_nodes_.count(current_node_id)) {
        return; // Already visited and marked live
    }
     auto node_opt = current_graph_->getNode(current_node_id);
     if (!node_opt) {
         return; // Node doesn't exist (shouldn't happen in valid graph)
     }
     const BDINode& node = node_opt.value();
    live_nodes_.insert(current_node_id);
    //std::cout << "    Marking live: " << current_node_id << std::endl;
    // Mark data dependency predecessors as live
    for (const auto& input_ref : node.data_inputs) {
        markLiveNodes(input_ref.node_id);
    }
    // Mark control dependency predecessors as live
    // For structured control flow, this might need refinement (e.g., only mark
    // the predecessor that can actually lead here based on analysis).
    // Simple approach: mark all control inputs.
    for (const NodeID& pred_id : node.control_inputs) {
         markLiveNodes(pred_id);
    }
 }
 bool DeadCodeElimination::run(BDIGraph& graph) {
    current_graph_ = &graph;
    live_nodes_.clear();
    // 1. Find essentially live nodes (those with side effects or end nodes)
    std::vector<NodeID> root_live_nodes;
    for (const auto& pair : graph) {
         if (pair.second && hasSideEffects(*(pair.second))) {
             root_live_nodes.push_back(pair.first);
         }
    }
     if (root_live_nodes.empty()) {
          std::cout << "    DCE Warning: No essential live nodes found (e.g., META_END). Graph might be empty or invalid." << std::endl;
          // Maybe find the designated START node if no END node?
          // return false; // No changes if nothing is live
     }
    // 2. Mark all reachable nodes backwards from the roots
    for (NodeID root_id : root_live_nodes) {
        markLiveNodes(root_id);
    }
    // 3. Collect dead nodes
    std::vector<NodeID> dead_nodes;
    for (const auto& pair : graph) {
        if (!live_nodes_.count(pair.first)) {
            dead_nodes.push_back(pair.first);
        }
    }
    // 4. Remove dead nodes (carefully, consider dependencies within dead nodes)
    if (!dead_nodes.empty()) {
        std::cout << "    DCE Found dead nodes: ";
        for(NodeID id : dead_nodes) std::cout << id << " ";
        std::cout << std::endl;
        for (NodeID dead_id : dead_nodes) {
            // Removal logic needs to handle edges cleanly. BDIGraph::removeNode should do this.
            if (graph.removeNode(dead_id)) {
                 markGraphModified();
                 //std::cout << "      Removed dead node: " << dead_id << std::endl;
            } else {
                 std::cerr << "      DCE Error: Failed to remove dead node " << dead_id << std::endl;
            }
        }
    } else {
         std::cout << "    DCE: No dead nodes found." << std::endl;
    }
    current_graph_ = nullptr;
    return wasGraphModified();
 }
 } // namespace bdi::optimizer
