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
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
