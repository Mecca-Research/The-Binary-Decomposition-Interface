#ifndef BDI_RUNTIME_EXECUTIONCONTEXT_HPP
#define BDI_RUNTIME_EXECUTIONCONTEXT_HPP

#include "BDINode.hpp"       // For PortRef, NodeID
#include "TypedPayload.hpp"  // For TypedPayload
#include "BDIValueVariant.hpp" // Use the new variant type
#include <optional>
#include <unordered_map>
#include <vector>

namespace bdi::runtime {

using bdi::core::graph::NodeID;
using bdi::core::graph::PortRef;
using bdi::core::payload::TypedPayload;

// Hash function for PortRef to use it in unordered_map
struct PortRefHash {
    std::size_t operator()(const PortRef& pr) const noexcept {
        std::size_t h1 = std::hash<NodeID>{}(pr.node_id);
        std::size_t h2 = std::hash<PortIndex>{}(pr.port_index);
        return h1 ^ (h2 << 1);
    }
};

class ExecutionContext {
public:
    ExecutionContext() = default;

    // Value storage
    void setPortValue(const PortRef& port, BDIValueVariant value);
    void setPortValue(NodeID node_id, PortIndex port_idx, BDIValueVariant value);
    std::optional<BDIValueVariant> getPortValue(const PortRef& port) const;
    std::optional<BDIValueVariant> getPortValue(NodeID node_id, PortIndex port_idx) const;

    // Conversion
    static BDIValueVariant payloadToVariant(const TypedPayload& payload);
    static TypedPayload variantToPayload(const BDIValueVariant& value);

    // Argument/return value handling
    void setNextArgument(PortIndex arg_index, BDIValueVariant value);
    std::optional<BDIValueVariant> getCurrentArgument(PortIndex arg_index);
    void setCurrentReturnValue(BDIValueVariant value);
    std::optional<BDIValueVariant> getLastReturnValue();

    // Call stack frame
    struct CallFrame {
        NodeID caller_node_id;
        NodeID return_node_id;
        std::unordered_map<PortIndex, BDIValueVariant> arguments;
        std::optional<BDIValueVariant> return_value = std::nullopt;
    };
    void pushCallFrame(NodeID caller_node_id, NodeID return_node_id);
    std::optional<CallFrame> popCallFrame();
    bool isCallStackEmpty() const;

    // Intelligence state
    void recordGradient(NodeID param_source_node, BDIValueVariant gradient);
    std::optional<BDIValueVariant> getGradient(NodeID param_source_node) const;
    void updateEligibilityTrace(NodeID param_source_node, float decay, float increment);
    std::optional<float> getEligibilityTrace(NodeID param_source_node) const;
    void clearIntelligenceState();

    // Service calls
    struct ServiceCallReturnState {
        NodeID original_caller_node_id; // The OS_SERVICE_CALL node
        NodeID original_resume_node_id; // Where to resume in caller graph
    };
    void pushServiceCall(NodeID caller_id, NodeID resume_id);
    std::optional<ServiceCallReturnState> popServiceCall();

    // Reset all state
    void clear();

private:
    std::unordered_map<PortRef, BDIValueVariant, PortRefHash> port_values_;
    std::vector<CallFrame> call_stack_;
    std::unordered_map<PortIndex, BDIValueVariant> next_arguments_;
    std::optional<BDIValueVariant> last_return_value_;

    // Intelligence state storage
    std::unordered_map<NodeID, BDIValueVariant> parameter_gradients;
    std::unordered_map<NodeID, float> eligibility_traces;
    std::unordered_map<NodeID, BDIValueVariant> current_state_features;

    std::vector<ServiceCallReturnState> service_call_stack_;
};

} // namespace bdi::runtime

#endif // BDI_RUNTIME_EXECUTIONCONTEXT_HPP
