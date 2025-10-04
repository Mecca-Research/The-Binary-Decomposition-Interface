
#ifndef BDI_COMPILER_BACKEND_REGALLOC_REGISTERALLOCATOR_HPP
#define BDI_COMPILER_BACKEND_REGALLOC_REGISTERALLOCATOR_HPP

#include "compiler/ssa/SsaTypes.hpp"
#include "compiler/backend/Architecture.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace bdi::compiler::backend {

using bdi::compiler::ssa::SsaForm;
using bdi::compiler::ssa::SsaVariableID;

/**
 * @brief Interference graph for register allocation
 * 
 * Nodes represent variables, edges represent interference
 * (variables that are live at the same time and cannot share a register)
 */
class InterferenceGraph {
public:
    InterferenceGraph() = default;
    
    void addNode(SsaVariableID var);
    void addEdge(SsaVariableID var1, SsaVariableID var2);
    
    bool hasEdge(SsaVariableID var1, SsaVariableID var2) const;
    const std::unordered_set<SsaVariableID>& getNeighbors(SsaVariableID var) const;
    
    uint32_t getDegree(SsaVariableID var) const;
    const std::unordered_set<SsaVariableID>& getNodes() const { return nodes_; }
    
    void removeNode(SsaVariableID var);
    
private:
    std::unordered_set<SsaVariableID> nodes_;
    std::unordered_map<SsaVariableID, std::unordered_set<SsaVariableID>> adjacency_;
    static const std::unordered_set<SsaVariableID> empty_set_;
};

/**
 * @brief Register allocation result
 */
struct AllocationResult {
    std::unordered_map<SsaVariableID, uint32_t> register_assignment;
    std::unordered_set<SsaVariableID> spilled_variables;
    bool success;
    
    AllocationResult() : success(false) {}
};

/**
 * @brief Register allocator using graph coloring
 * 
 * Implements Chaitin's graph coloring algorithm for register allocation:
 * 1. Build interference graph
 * 2. Simplify graph by removing low-degree nodes
 * 3. Color the graph (assign registers)
 * 4. Handle spills if necessary
 */
class RegisterAllocator {
public:
    explicit RegisterAllocator(const Architecture& arch);
    
    /**
     * @brief Allocate registers for SSA form
     * @param ssa The SSA form to allocate registers for
     * @return Allocation result with register assignments
     */
    AllocationResult allocate(const SsaForm& ssa);
    
    /**
     * @brief Build interference graph from SSA form
     */
    std::unique_ptr<InterferenceGraph> buildInterferenceGraph(const SsaForm& ssa);
    
    /**
     * @brief Color the interference graph
     * @param graph The interference graph
     * @param num_colors Number of available colors (registers)
     * @return Coloring assignment or empty if failed
     */
    std::unordered_map<SsaVariableID, uint32_t> colorGraph(
        InterferenceGraph& graph, uint32_t num_colors);
    
private:
    const Architecture& arch_;
    
    /**
     * @brief Simplify phase: remove nodes with degree < k
     */
    std::vector<SsaVariableID> simplify(InterferenceGraph& graph, uint32_t k);
    
    /**
     * @brief Select phase: assign colors to nodes
     */
    bool select(const std::vector<SsaVariableID>& stack,
               const InterferenceGraph& original_graph,
               std::unordered_map<SsaVariableID, uint32_t>& coloring,
               uint32_t num_colors);
    
    /**
     * @brief Choose a node to spill
     */
    SsaVariableID chooseSpillCandidate(const InterferenceGraph& graph);
    
    /**
     * @brief Calculate spill cost for a variable
     */
    double calculateSpillCost(SsaVariableID var, const SsaForm& ssa);
};

/**
 * @brief Live range analysis for register allocation
 */
class LiveRangeAnalysis {
public:
    struct LiveRange {
        SsaVariableID var;
        uint32_t start;  // First use
        uint32_t end;    // Last use
        
        bool overlaps(const LiveRange& other) const {
            return !(end < other.start || other.end < start);
        }
    };
    
    /**
     * @brief Compute live ranges for all variables
     */
    static std::vector<LiveRange> computeLiveRanges(const SsaForm& ssa);
    
    /**
     * @brief Check if two variables interfere
     */
    static bool interfere(const LiveRange& r1, const LiveRange& r2) {
        return r1.overlaps(r2);
    }
};

} // namespace bdi::compiler::backend

#endif // BDI_COMPILER_BACKEND_REGALLOC_REGISTERALLOCATOR_HPP
