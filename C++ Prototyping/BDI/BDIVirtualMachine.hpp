 #ifndef BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #define BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #include "../core/graph/BDIGraph.hpp"
 #include <memory> // For std::shared_ptr or unique_ptr if VM owns graph
 #include "../verification/ProofVerifier.hpp" // Include ProofVerifier
 #include "../meta/MetadataStore.hpp" // Include MetadataStore
 namespace bdi::runtime {
 // ... imports ...
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::NodeID;
 // Basic Stub for the BDI Virtual Machine / Interpreter
 class BDIVirtualMachine {
 public:
 // Constructor takes MetadataStore AND ProofVerifier
    BDIVirtualMachine(MetadataStore& meta_store,
                      verification::ProofVerifier& proof_verifier, // Add ProofVerifier
                      size_t memory_size = 1024 * 1024);
    BDIVirtualMachine(); // Constructor might take memory manager, scheduler etc.
    // Primary execution entry point
    // Takes graph by reference, doesn't assume ownership here.
    // Returns success/failure or final state info.
    bool execute(BDIGraph& graph, NodeID entry_node_id);
    // Accessors
    ExecutionContext* getExecutionContext();
    MemoryManager* getMemoryManager();
    const ExecutionContext* getExecutionContext() const;
    const MemoryManager* getMemoryManager() const;
    // No public access to proof verifier or metadata store? Or make const accessors?
    // TODO: Add methods for stepping, debugging, state inspection, loading graphs
  private:
    // --- Internal VM State --
    NodeID current_node_id_;
    MetadataStore* metadata_store_; // Store as pointer
    verification::ProofVerifier* proof_verifier_; // Store as pointer
    std::unique_ptr<MemoryManager> memory_manager_;
    std::unique_ptr<ExecutionContext> execution_context_;
    // TODO: Represent register file / execution context / stack
    // TODO: Interface to MemoryManager
    // TODO: Interface to RuntimeScheduler
    // TODO: Interface to TraceGenerator
    // --- Execution Loop Helpers --
    bool fetchDecodeExecuteCycle(BDIGraph& graph);
    bool executeNode(BDINode& node); // Dispatch based on node.operation
    bool executeNode(BDINode& node, BDIGraph& graph); // Pass graph for context
    NodeID determineNextNode(BDINode& node); // Follow control flow
 };
    // Forward declare 
    class BDIOSSchedulerInterface; // Interface for scheduler interaction 
    class BDIOSEventDispatcherInterface; 
    class BDIVirtualMachine { 
        // ... existing members ... 
        BDIOSSchedulerInterface* scheduler_ = nullptr; // Pointer to scheduler logic (could be another graph context) 
        BDIOSEventDispatcherInterface* event_dispatcher_ = nullptr; // Pointer to event logic 
        // VM Internal State Flags 
        bool yield_requested_ = false; 
        bool halt_task_requested_ = false; 
        bool wait_event_requested_ = false; 
        // Add structure to hold info for WAIT/DISPATCH/SEND ops (e.g., event filter, target task) 
 public: 
        // Set scheduler/dispatcher (called during BDIOS init) 
        void setScheduler(BDIOSSchedulerInterface* sched); 
        void setEventDispatcher(BDIOSEventDispatcherInterface* disp); 
        // --- Main Execution Loop Modification --- 
        // Instead of simple execute, maybe run_timeslice or run_until_event? 
        enum class VMExecResult { COMPLETED, YIELDED, HALTED_TASK, WAITING, ERROR }; 
        VMExecResult runSlice(BDIGraph& graph, NodeID entry_or_resume_node_id, uint64_t timeslice_instructions = 1000); 
        // Get current task state for saving context 
        // ExecutionContext& getCurrentContextForSave(); // Needs careful state management 
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
