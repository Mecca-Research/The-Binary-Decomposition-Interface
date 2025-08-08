 #ifndef BDI_VERIFICATION_PROOFVERIFIER_HPP
 #define BDI_VERIFICATION_PROOFVERIFIER_HPP
 #include "MetadataStore.hpp" // For ProofTag definition
 #include "BDIGraph.hpp" // May need graph context
 namespace bdi::verification {
 using bdi::meta::ProofTag;
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::NodeID;
 // Interface for external or internal proof verification systems
 class ProofVerifier {
 public:
    virtual ~ProofVerifier() = default;
    // Verify the proof associated with a specific node
    // Returns true if verification succeeds or is not applicable, false on failure
    virtual bool verify(const ProofTag& proof_tag,
                        const BDIGraph& graph_context, // Context for verification
                        NodeID verifying_node_id) = 0; // Node triggering verification
 };
 // Simple stub implementation that always succeeds
 class StubProofVerifier : public ProofVerifier {
 public:
     bool verify(const ProofTag& proof_tag,
                 const BDIGraph& graph_context,
                 NodeID verifying_node_id) override {
        // STUB: Always return true for now
        if (proof_tag.system != ProofTag::ProofSystem::NONE) {
             // std::cout << "  (StubProofVerifier: Verifying proof for node " << verifying_node_id << ")" << std::endl;
        }
        return true;
     }
 };
 } // namespace bdi::verification
 #endif // BDI_VERIFICATION_PROOFVERIFIER_HPP
