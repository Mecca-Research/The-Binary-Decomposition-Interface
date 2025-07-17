 #ifndef BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #define BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #include "BDINode.hpp"       // For PortRef, NodeID
 #include "TypedPayload.hpp" // For TypedPayload
 #include "BDIValueVariant.hpp" // Use the new variant type
 #include <unordered_map>
 #include <optional>
 #include <vector> // For call stack
 namespace bdi::runtime {
 using bdi::core::graph::NodeID;
 using bdi::core::graph::PortRef;
 using bdi::core::payload::TypedPayload;
 // Hash function for PortRef to use it in unordered_map
 struct PortRefHash {
    std::size_t operator()(const PortRef& pr) const noexcept {
        // Simple hash combination - consider better hashing for performance
        std::size_t h1 = std::hash<NodeID>{}(pr.node_id);
        std::size_t h2 = std::hash<PortIndex>{}(pr.port_index);
        return h1 ^ (h2 << 1); // Combine hashes
    }
 };
 class ExecutionContext {
    // --- State for Intelligence --- 
    // Conceptual: Store gradients or traces per parameter source node 
    std::unordered_map<NodeID, BDIValueVariant> parameter_gradients; // NodeID producing param -> Gradient Value 
    std::unordered_map<NodeID, float> eligibility_traces; // NodeID producing param -> Trace value 
    // Add methods to set/get/clear gradients and traces 
 public:
    ExecutionContext() = default;
    // Store the output value for a specific port
    void setPortValue(const PortRef& port, BDIValueVariant value); // Takes variant
    void setPortValue(NodeID node_id, PortIndex port_idx, BDIValueVariant value);
    // Retrieve the value variant for a specific port
    std::optional<BDIValueVariant> getPortValue(const PortRef& port) const; // Returns variant
    std::optional<BDIValueVariant> getPortValue(NodeID node_id, PortIndex port_idx) const;
    // --- Conversion --
    // Convert from TypedPayload (binary) to BDIValueVariant (runtime value)
    // Returns std::monostate in variant on error
    static BDIValueVariant payloadToVariant(const TypedPayload& payload);
    // Convert from BDIValueVariant (runtime value) back to TypedPayload (binary)
    // Returns TypedPayload with UNKNOWN type on error
    static TypedPayload variantToPayload(const BDIValueVariant& value);
    // --- Argument/Return Value Handling --
    // Set argument for the *next* call frame
    void setNextArgument(PortIndex arg_index, BDIValueVariant value);
    // Retrieve argument from the *current* call frame
    std::optional<BDIValueVariant> getCurrentArgument(PortIndex arg_index);
    // Set the return value for the *current* call frame
    void setCurrentReturnValue(BDIValueVariant value);
    // Retrieve the return value from the *previous* call frame (after RETURN)
    std::optional<BDIValueVariant> getLastReturnValue();
    // --- Call Stack --
    void pushCall(NodeID return_node_id);
    std::optional<NodeID> popCall();
    void recordGradient(NodeID param_source_node, BDIValueVariant gradient); 
    std::optional<BDIValueVariant> getGradient(NodeID param_source_node) const; 
    void updateEligibilityTrace(NodeID param_source_node, float decay, float increment); 
    std::optional<float> getEligibilityTrace(NodeID param_source_node) const; 
    void clearIntelligenceState(); // Clear gradients/traces 
    bool isCallStackEmpty() const;
    void clear();
}; 
 private:
    std::unordered_map<PortRef, BDIValueVariant, PortRefHash> port_values_; // Stores variants
    std::vector<NodeID> call_stack_; // Replaced by vector of CallFrames
    std::unordered_map<PortIndex, BDIValueVariant> next_arguments_; // Staging area for next call
    std::optional<BDIValueVariant> last_return_value_; // Value returned by the last RETURN
 };
    // --- Call Stack Frame --
    struct CallFrame {
        NodeID caller_node_id; // << ADDED: ID of the CTRL_CALL node
        NodeID return_node_id;
        std::unordered_map<PortIndex, BDIValueVariant> arguments;
        std::optional<BDIValueVariant> return_value = std::nullopt;
    };
    void pushCallFrame(NodeID caller_node_id, NodeID return_node_id); // << MODIFIED
    std::optional<CallFrame> popCallFrame(); // Returns the whole frame
    bool isCallStackEmpty() const;
    void clear(); // Also clears args/return value state 
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_EXECUTIONCONTEXT_HPP
