#ifndef CHIMERA_DEVTOOLS_DEBUGGERINTERFACE_HPP 
#define CHIMERA_DEVTOOLS_DEBUGGERINTERFACE_HPP 
#include "BDIVirtualMachine.hpp" // Needs VM access 
#include "BDIGraph.hpp" 
#include <set> 
#include <string> 
#include <functional> 
#include <iostream> 
#include <thread> // Potentially for waiting 
namespace chimera::devtools { 
using namespace bdi::core::graph; 
using namespace bdi::runtime; 
// Interface for controlling and inspecting VM execution 
class DebuggerInterface {
 public: 
    // Attach to a VM instance (VM needs hooks) 
    explicit DebuggerInterface(BDIVirtualMachine& vm_instance); 
    // TODO: Register callbacks with the VM 
    // vm_.attachDebugger(this); 
    // --- Control --- 
    void DebuggerInterface::run() { 
    current_state_ = State::RUNNING; 
    step_requested_ = false; 
    std::cout << "[Debugger] Running..." << std::endl; 
    // signalVMResume(); // Signal VM thread if separate 
    // In simple single-thread model, run loop might be external or integrate with VM loop 
} 
void DebuggerInterface::stepNode() { 
     if (current_state_ == State::PAUSED) { 
        current_state_ = State::RUNNING; // Temporarily run for one step 
        step_requested_ = true; 
        std::cout << "[Debugger] Stepping one node..." << std::endl; 
        // signalVMResume(); 
     } else { 
        std::cout << "[Debugger] Cannot step: VM not paused." << std::endl; 
     } 
} 
void stepInstruction(); // (Advanced) Execute one underlying machine instruction if possible? 
void DebuggerInterface::pause() { 
    if (current_state_ == State::RUNNING) { 
         std::cout << "[Debugger] Requesting pause..." << std::endl; 
         // Signal VM to pause (e.g., set an atomic flag VM checks) 
         // vm_.requestPause(); 
         // waitForVMPause(); // Wait for VM to signal it has paused 
         current_state_ = State::PAUSED; // Assume pause succeeded for now 
    }
 } 
    void pause();    // Signal VM to pause (requires VM cooperation) 
    void DebuggerInterface::setBreakpoint(NodeID node_id) { 
    std::cout << "[Debugger] Setting breakpoint at Node " << node_id << std::endl; 
    breakpoints_.insert(node_id); 
} 
void DebuggerInterface::removeBreakpoint(NodeID node_id) { breakpoints_.erase(node_id); } 
void DebuggerInterface::clearBreakpoints() { breakpoints_.clear(); } 
    void removeBreakpoint(NodeID node_id); 
    void clearBreakpoints(); 
    // --- Inspection --- 
    NodeID getCurrentNode() const; 
    std::optional<BDIValueVariant> getNodeOutput(NodeID node_id, PortIndex port_idx); 
    // std::optional<VariableInfo> lookupVariable(const std::string& name, int scope_level); // Needs symbol table access 
    bool readMemory(uintptr_t address, size_t size_bytes, std::vector<std::byte>& out_buffer); 
    NodeID DebuggerInterface::getCurrentNode() const { 
    // Need getter from VM: return vm_.getCurrentNodeId(); 
    return 0; // Placeholder 
} 
    std::optional<BDIValueVariant> DebuggerInterface::getNodeOutput(NodeID node_id, PortIndex port_idx) { 
     // Need getter from VM: return vm_.getExecutionContext()->getPortValue(node_id, port_idx); 
     return std::nullopt; // Placeholder
 } 
 bool DebuggerInterface::readMemory(uintptr_t address, size_t size_bytes, std::vector<std::byte>& out_buffer) { 
     out_buffer.resize(size_bytes); 
     // Need getter from VM: return vm_.getMemoryManager()->readMemory(address, out_buffer.data(), size_bytes); 
      return false; // Placeholder 
 }
    // Get call stack info 
    // Get metadata for node 
    // --- Callbacks (VM needs to call these) --- 
    // Called by VM *before* executing a node 
    virtual void onPreNodeExecute(NodeID node_id); 
    // Called by VM *after* executing a node 
    virtual void onPostNodeExecute(NodeID node_id, bool success); 
    // Called when VM hits a breakpoint 
    virtual void onBreakpointHit(NodeID node_id); 
    // Called when VM halts 
    virtual void onHalt(); 
// --- Callbacks Implementation --- 
void DebuggerInterface::onPreNodeExecute(NodeID node_id) { 
// Check breakpoints BEFORE execution 
if (breakpoints_.count(node_id)) { 
        onBreakpointHit(node_id); 
// wait_for_resume_signal(); // Block VM thread until debugger says continue 
    }
 if (step_requested_) { 
// If step was requested, pause immediately *before* executing this node 
         current_state_ = State::PAUSED; 
         step_requested_ = false; 
std::cout << "[Debugger] Paused before Node " << node_id << " (Step)" << std::endl; 
// wait_for_resume_signal(); // Wait for user action (run/step) 
     } 
} 
void DebuggerInterface::onPostNodeExecute(NodeID node_id, bool success) { 
// Optionally log execution or update UI 
} 
void DebuggerInterface::onBreakpointHit(NodeID node_id) { 
    current_state_ = State::PAUSED; 
std::cout << "[Debugger] Breakpoint hit at Node " << node_id << "! Paused." << std::endl; 
// wait_for_resume_signal(); // Wait for user action (run/step/inspect) 
} 
void DebuggerInterface::onHalt() { 
     current_state_ = State::HALTED; 
std::cout << "[Debugger] VM Halted." << std::endl;
 } 
private: 
    BDIVirtualMachine& vm_; // Reference to the VM being controlled 
    std::set<NodeID> breakpoints_; 
    enum class State { RUNNING, PAUSED, HALTED } current_state_ = State::PAUSED; 
    bool step_requested_ = false; 
    // Internal communication/synchronization with VM thread if async 
    void signalVMResume(); 
    void waitForVMPause(); // Requires VM signaling mechanism 
}; 
} // namespace chimera::devtools 
#endif // CHIMERA_DEVTOOLS_DEBUGGERINTERFACE_HPP 
