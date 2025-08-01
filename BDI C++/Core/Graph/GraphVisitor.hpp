 #ifndef BDI_OPTIMIZER_GRAPHVISITOR_HPP
 #define BDI_OPTIMIZER_GRAPHVISITOR_HPP
 #include "../core/graph/BDIGraph.hpp"
 #include "../core/graph/BDINode.hpp"
 namespace bdi::optimizer {
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::BDINode;
 // Abstract base class for graph visitors (used by optimizers, analyzers, etc.)
 class GraphVisitor {
 public:
    virtual ~GraphVisitor() = default;
    // Entry point for visiting a graph
    virtual void visitGraph(BDIGraph& graph) {
        // Default implementation iterates over all nodes
        for (auto it = graph.begin(); it != graph.end(); ++it) {
            // Need mutable access to the node, getting pointer from unique_ptr
             if (it->second) { // Check if unique_ptr is valid
                visitNode(*(it->second));
            }
        }
    }
    // Method called for each node
    // Can be overridden by specific visitors
    virtual void visitNode(BDINode& node) {
        // Default implementation does nothing
        (void)node; // Mark as unused
    }
 };
 } // namespace bdi::optimizer
 #endif // BDI_OPTIMIZER_GRAPHVISITOR_HPP
