#include "ExecutionContext.hpp"
#include "TypedPayload.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace bdi::runtime {

// ---------------- Value Storage ----------------
void ExecutionContext::setPortValue(const PortRef& port, BDIValueVariant value) {
    port_values_[port] = std::move(value);
}

void ExecutionContext::setPortValue(NodeID node_id, PortIndex port_idx, BDIValueVariant value) {
    setPortValue({node_id, port_idx}, std::move(value));
}

std::optional<BDIValueVariant> ExecutionContext::getPortValue(const PortRef& port) const {
    auto it = port_values_.find(port);
    if (it != port_values_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<BDIValueVariant> ExecutionContext::getPortValue(NodeID node_id, PortIndex port_idx) const {
    return getPortValue({node_id, port_idx});
}

// --------------- Argument / Return Handling ---------------
void ExecutionContext::setNextArgument(PortIndex arg_index, BDIValueVariant value) {
    next_arguments_[arg_index] = std::move(value);
}

std::optional<BDIValueVariant> ExecutionContext::getCurrentArgument(PortIndex arg_index) {
    if (call_stack_.empty()) {
        return std::nullopt;
    }
    const auto& frame = call_stack_.back();
    auto it = frame.arguments.find(arg_index);
    if (it != frame.arguments.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ExecutionContext::setCurrentReturnValue(BDIValueVariant value) {
    if (call_stack_.empty()) {
        last_return_value_ = std::move(value);
        return;
    }
    call_stack_.back().return_value = std::move(value);
}

std::optional<BDIValueVariant> ExecutionContext::getLastReturnValue() {
    return last_return_value_;
}

// ---------------- Call Stack ----------------
void ExecutionContext::pushCallFrame(NodeID caller_node_id, NodeID return_node_id) {
    CallFrame frame;
    frame.caller_node_id = caller_node_id;
    frame.return_node_id = return_node_id;
    frame.arguments = std::move(next_arguments_);
    next_arguments_.clear();
    last_return_value_ = std::nullopt;
    call_stack_.push_back(std::move(frame));
}

std::optional<ExecutionContext::CallFrame> ExecutionContext::popCallFrame() {
    if (call_stack_.empty()) {
        return std::nullopt;
    }
    CallFrame frame = std::move(call_stack_.back());
    call_stack_.pop_back();
    last_return_value_ = frame.return_value;
    return frame;
}

bool ExecutionContext::isCallStackEmpty() const {
    return call_stack_.empty();
}

// ---------------- Intelligence State ----------------
void ExecutionContext::recordGradient(NodeID param_source_node, BDIValueVariant gradient) {
    parameter_gradients[param_source_node] = std::move(gradient);
}

std::optional<BDIValueVariant> ExecutionContext::getGradient(NodeID param_source_node) const {
    auto it = parameter_gradients.find(param_source_node);
    if (it != parameter_gradients.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ExecutionContext::updateEligibilityTrace(NodeID param_source_node, float decay, float increment) {
    eligibility_traces[param_source_node] = eligibility_traces[param_source_node] * decay + increment;
}

std::optional<float> ExecutionContext::getEligibilityTrace(NodeID param_source_node) const {
    auto it = eligibility_traces.find(param_source_node);
    if (it != eligibility_traces.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ExecutionContext::clearIntelligenceState() {
    parameter_gradients.clear();
    eligibility_traces.clear();
    current_state_features.clear();
}

// ---------------- Service Calls ----------------
void ExecutionContext::pushServiceCall(NodeID caller_id, NodeID resume_id) {
    service_call_stack_.push_back({caller_id, resume_id});
    last_return_value_ = std::nullopt;
}

std::optional<ExecutionContext::ServiceCallReturnState> ExecutionContext::popServiceCall() {
    if (service_call_stack_.empty()) {
        return std::nullopt;
    }
    auto state = service_call_stack_.back();
    service_call_stack_.pop_back();
    return state;
}

// ---------------- Reset ----------------
void ExecutionContext::clear() {
    port_values_.clear();
    call_stack_.clear();
    next_arguments_.clear();
    last_return_value_ = std::nullopt;
    service_call_stack_.clear();
    clearIntelligenceState();
}

// ---------------- Conversion Helpers ----------------
BDIValueVariant ExecutionContext::payloadToVariant(const TypedPayload& payload) {
    using Type = core::types::BDIType;
    try {
        switch (payload.type) {
            case Type::VOID:    return std::monostate{};
            case Type::BOOL:    return payload.getAs<bool>();
            case Type::INT8:    return payload.getAs<int8_t>();
            case Type::UINT8:   return payload.getAs<uint8_t>();
            case Type::INT16:   return payload.getAs<int16_t>();
            case Type::UINT16:  return payload.getAs<uint16_t>();
            case Type::INT32:   return payload.getAs<int32_t>();
            case Type::UINT32:  return payload.getAs<uint32_t>();
            case Type::INT64:   return payload.getAs<int64_t>();
            case Type::UINT64:  return payload.getAs<uint64_t>();
            case Type::FLOAT32: return payload.getAs<float>();
            case Type::FLOAT64: return payload.getAs<double>();
            case Type::POINTER:
            case Type::MEM_REF:
            case Type::FUNC_PTR: return payload.getAs<uintptr_t>();
            default: return std::monostate{};
        }
    } catch (const std::exception& e) {
        std::cerr << "Error converting payload to variant: " << e.what() << std::endl;
        return std::monostate{};
    }
}

TypedPayload ExecutionContext::variantToPayload(const BDIValueVariant& value) {
    TypedPayload result;
    result.type = getBDIType(value);
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (!std::is_same_v<T, std::monostate>) {
            result.data.resize(sizeof(T));
            std::memcpy(result.data.data(), &arg, sizeof(T));
        }
    }, value);
    return result;
}

} // namespace bdi::runtime
