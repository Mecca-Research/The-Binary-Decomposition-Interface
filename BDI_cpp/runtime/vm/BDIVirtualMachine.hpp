 #ifndef BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #define BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #include "BDIGraph.hpp"
 #include <memory> // For std::shared_ptr or unique_ptr if VM owns graph
 #include "ProofVerifier.hpp" // Include ProofVerifier
 #include "MetadataStore.hpp" // Include MetadataStore
 namespace bdi::runtime {
 // ... imports ...
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::NodeID;
 class BDIVirtualMachine {
    bool fetchDecodeExecuteCycle(BDIGraph& graph);
    bool executeNode(BDINode& node); // Dispatch based on node.operation
    bool executeNode(BDINode& node, BDIGraph& graph); // Pass graph for context
    NodeID determineNextNode(BDINode& node); // Follow control flow
    // ... existing members ... 
    // ExecutionContext needs Task ID 
    // std::unordered_map<uint64_t /*TaskID*/, std::unique_ptr<ExecutionContext>> task_contexts_; 
    uint64_t current_task_id_ = 0; 
    ExecutionContext* current_context_ = nullptr; // Points into task_contexts_ map 
    BDIOSSchedulerInterface* scheduler_ = nullptr; // Pointer to scheduler logic (could be another graph context) 
    BDIOSEventDispatcherInterface* event_dispatcher_ = nullptr; // Pointer to event logic 
    // VM Internal State Flags 
    bool yield_requested_ = false; 
    bool halt_task_requested_ = false; 
    bool wait_event_requested_ = false; 
    // Add structure to hold info for WAIT/DISPATCH/SEND ops (e.g., event filter, target task) 
    DebuggerInterface* debugger_ = nullptr; // Attached debugger 
    std::atomic<bool> pause_requested_ = false; 
    std::atomic<bool> step_requested_ = false; 
    std::mutex vm_mutex_; // For state synchronization with debugger 
    std::condition_variable vm_cv_; 
    enum class VMState { RUNNING, PAUSED, HALTED } vm_state_ = VMState::PAUSED; 
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
    // Set scheduler/dispatcher (called during BDIOS init) 
    void setScheduler(BDIOSSchedulerInterface* sched); 
    void setEventDispatcher(BDIOSEventDispatcherInterface* disp); 
    // --- Main Execution Loop Modification --- 
    // Instead of simple execute, maybe run_timeslice or run_until_event? 
    enum class VMExecResult { COMPLETED, YIELDED, HALTED_TASK, WAITING, ERROR }; 
    VMExecResult runSlice(BDIGraph& graph, NodeID entry_or_resume_node_id, uint64_t timeslice_instructions = 1000); 
    // Get current task state for saving context 
    // ExecutionContext& getCurrentContextForSave(); // Needs careful state management 
    // ... Constructor takes HAL, MetaStore, Verifier ... 
    // void setScheduler(BDIOSSchedulerInterface* sched); // Set external scheduler logic 
    // Main execution loop runs the scheduler 
    void runOS(); 
    // Internal function to execute a single task's timeslice 
    VMExecResult runTaskSlice(uint64_t task_id, uint64_t timeslice_instructions);
    //
    void attachDebugger(DebuggerInterface* dbg); 
    void detachDebugger(); 
    void requestPause(); 
    void requestResume(bool single_step = false); 
    VMState getState() const; // Thread-safe getter maybe needed 
    NodeID getCurrentNodeId() const; // Getter for debugger 
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
    // Internal context switch logic 
    bool saveCurrentContext(uint64_t task_id); 
    bool restoreContext(uint64_t task_id); 
    void switchToScheduler(); // Changes current_node_id_ etc. to run scheduler graph 
    void signalPaused(); 
    void waitForResume();
    // ... executeNode, determineNextNode modified to check flags/call hooks ... 
 };
    // Forward declare 
    class BDIOSEventDispatcherInterface; 
    // --- Task Control Block (Conceptual - Managed by Scheduler Graph) --- 
    struct TaskControlBlock { 
        uint64_t task_id; 
        NodeID resume_node_id; // Where to restart execution 
        // Pointer/Handle to ExecutionContext state 
        // Priority, Status (Ready, Running, Waiting, Halted), Wait Condition 
        // Memory Region List? Capabilities? 
    }; 
    // --- Scheduler Interface (Implemented by Scheduler Graph Logic) --- 
    class BDIOSSchedulerInterface { // Interface for scheduler interaction 
    public: 
        virtual ~BDIOSSchedulerInterface() = default; 
        // Called by VM after yield/halt/wait or timeslice end 
        virtual std::optional<TaskControlBlock> getNextTaskToRun() = 0; 
        // Called by VM when task state changes (e.g., becomes WAITING) 
        virtual void updateTaskState(uint64_t task_id, /* New State */ int state, /* Wait condition? */ uint64_t condition) = 0; 
        // Called by OS_SERVICE_CALL for AddTask etc. 
        virtual BDIValueVariant handleServiceCall(/* Args */) = 0; 
    }; 
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
