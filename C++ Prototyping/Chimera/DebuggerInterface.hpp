#ifndef CHIMERA_DEVTOOLS_DEBUGGERINTERFACE_HPP 
#define CHIMERA_DEVTOOLS_DEBUGGERINTERFACE_HPP 
#include "../runtime/BDIVirtualMachine.hpp" // Needs VM access 
#include "../core/graph/BDIGraph.hpp" 
#include <set> 
#include <string> 
#include <functional> 
namespace chimera::devtools { 
using namespace bdi::core::graph; 
using namespace bdi::runtime; 
// Interface for controlling and inspecting VM execution 
class DebuggerInterface {
 public: 
    // Attach to a VM instance (VM needs hooks) 
    explicit DebuggerInterface(BDIVirtualMachine& vm_instance); 
    // --- Control --- 
    void run();      // Run continuously until breakpoint or halt 
    void stepNode(); // Execute one BDI node 
    void stepInstruction(); // (Advanced) Execute one underlying machine instruction if possible? 
    void pause();    // Signal VM to pause (requires VM cooperation) 
    void setBreakpoint(NodeID node_id); 
    void removeBreakpoint(NodeID node_id); 
    void clearBreakpoints(); 
    // --- Inspection --- 
    NodeID getCurrentNode() const; 
    std::optional<BDIValueVariant> getNodeOutput(NodeID node_id, PortIndex port_idx); 
    // std::optional<VariableInfo> lookupVariable(const std::string& name, int scope_level); // Needs symbol table access 
    bool readMemory(uintptr_t address, size_t size_bytes, std::vector<std::byte>& out_buffer); 
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
