
#include "RegisterAllocator.hpp"
#include <algorithm>
#include <queue>
#include <limits>

namespace bdi::compiler::backend {

// Static member initialization
const std::unordered_set<SsaVariableID> InterferenceGraph::empty_set_;

// InterferenceGraph implementation
void InterferenceGraph::addNode(SsaVariableID var) {
    nodes_.insert(var);
    if (adjacency_.find(var) == adjacency_.end()) {
        adjacency_[var] = std::unordered_set<SsaVariableID>();
    }
}

void InterferenceGraph::addEdge(SsaVariableID var1, SsaVariableID var2) {
    if (var1 == var2) return;
    
    addNode(var1);
    addNode(var2);
    
    adjacency_[var1].insert(var2);
    adjacency_[var2].insert(var1);
}

bool InterferenceGraph::hasEdge(SsaVariableID var1, SsaVariableID var2) const {
    auto it = adjacency_.find(var1);
    if (it == adjacency_.end()) return false;
    return it->second.find(var2) != it->second.end();
}

const std::unordered_set<SsaVariableID>& 
InterferenceGraph::getNeighbors(SsaVariableID var) const {
    auto it = adjacency_.find(var);
    if (it != adjacency_.end()) {
        return it->second;
    }
    return empty_set_;
}

uint32_t InterferenceGraph::getDegree(SsaVariableID var) const {
    return getNeighbors(var).size();
}

void InterferenceGraph::removeNode(SsaVariableID var) {
    nodes_.erase(var);
    
    // Remove from all adjacency lists
    for (auto& [node, neighbors] : adjacency_) {
        neighbors.erase(var);
    }
    
    adjacency_.erase(var);
}

// RegisterAllocator implementation
RegisterAllocator::RegisterAllocator(const Architecture& arch)
    : arch_(arch) {}

AllocationResult RegisterAllocator::allocate(const SsaForm& ssa) {
    AllocationResult result;
    
    // Build interference graph
    auto graph = buildInterferenceGraph(ssa);
    
    // Get number of available registers
    uint32_t num_registers = 0;
    for (const auto& reg : arch_.getRegisters()) {
        if (reg.is_allocatable) {
            num_registers++;
        }
    }
    
    // Try to color the graph
    result.register_assignment = colorGraph(*graph, num_registers);
    
    if (result.register_assignment.size() == ssa.getVariables().size()) {
        result.success = true;
    } else {
        // Some variables couldn't be colored - they need to be spilled
        for (const auto& [var_id, var] : ssa.getVariables()) {
            if (result.register_assignment.find(var_id) == 
                result.register_assignment.end()) {
                result.spilled_variables.insert(var_id);
            }
        }
        result.success = false;
    }
    
    return result;
}

std::unique_ptr<InterferenceGraph> 
RegisterAllocator::buildInterferenceGraph(const SsaForm& ssa) {
    auto graph = std::make_unique<InterferenceGraph>();
    
    // Add all variables as nodes
    for (const auto& [var_id, var] : ssa.getVariables()) {
        graph->addNode(var_id);
    }
    
    // Compute live ranges
    auto live_ranges = LiveRangeAnalysis::computeLiveRanges(ssa);
    
    // Add edges for interfering variables
    for (size_t i = 0; i < live_ranges.size(); ++i) {
        for (size_t j = i + 1; j < live_ranges.size(); ++j) {
            if (LiveRangeAnalysis::interfere(live_ranges[i], live_ranges[j])) {
                graph->addEdge(live_ranges[i].var, live_ranges[j].var);
            }
        }
    }
    
    return graph;
}

std::unordered_map<SsaVariableID, uint32_t> 
RegisterAllocator::colorGraph(InterferenceGraph& graph, uint32_t num_colors) {
    std::unordered_map<SsaVariableID, uint32_t> coloring;
    
    // Save original graph before simplify destroys it
    InterferenceGraph original_graph = graph;
    
    // Simplify phase
    auto stack = simplify(graph, num_colors);
    
    // Select phase
    bool success = select(stack, original_graph, coloring, num_colors);
    
    if (!success) {
        // Coloring failed - some nodes need to be spilled
        // Return partial coloring
    }
    
    return coloring;
}

std::vector<SsaVariableID> 
RegisterAllocator::simplify(InterferenceGraph& graph, uint32_t k) {
    std::vector<SsaVariableID> stack;
    
    bool progress = true;
    while (progress && !graph.getNodes().empty()) {
        progress = false;
        
        // Find a node with degree < k
        for (SsaVariableID node : graph.getNodes()) {
            if (graph.getDegree(node) < k) {
                stack.push_back(node);
                graph.removeNode(node);
                progress = true;
                break;
            }
        }
        
        // If no low-degree node found, pick a spill candidate
        if (!progress && !graph.getNodes().empty()) {
            SsaVariableID spill = chooseSpillCandidate(graph);
            stack.push_back(spill);
            graph.removeNode(spill);
            progress = true;
        }
    }
    
    return stack;
}

bool RegisterAllocator::select(
    const std::vector<SsaVariableID>& stack,
    const InterferenceGraph& original_graph,
    std::unordered_map<SsaVariableID, uint32_t>& coloring,
    uint32_t num_colors) {
    
    // Process stack in reverse order
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        SsaVariableID var = *it;
        
        // Find available colors
        std::unordered_set<uint32_t> used_colors;
        for (SsaVariableID neighbor : original_graph.getNeighbors(var)) {
            auto color_it = coloring.find(neighbor);
            if (color_it != coloring.end()) {
                used_colors.insert(color_it->second);
            }
        }
        
        // Assign first available color
        uint32_t color = 0;
        for (color = 0; color < num_colors; ++color) {
            if (used_colors.find(color) == used_colors.end()) {
                coloring[var] = color;
                break;
            }
        }
        
        // If no color available, this variable needs to be spilled
        if (color >= num_colors) {
            return false;
        }
    }
    
    return true;
}

SsaVariableID RegisterAllocator::chooseSpillCandidate(
    const InterferenceGraph& graph) {
    
    // Simple heuristic: choose node with highest degree
    SsaVariableID best = 0;
    uint32_t max_degree = 0;
    
    for (SsaVariableID node : graph.getNodes()) {
        uint32_t degree = graph.getDegree(node);
        if (degree > max_degree) {
            max_degree = degree;
            best = node;
        }
    }
    
    return best;
}

double RegisterAllocator::calculateSpillCost(SsaVariableID var, const SsaForm& ssa) {
    // Simplified spill cost calculation
    // In real implementation, would consider:
    // - Number of uses
    // - Loop nesting depth
    // - Interference degree
    
    return 1.0; // Placeholder
}

// LiveRangeAnalysis implementation
std::vector<LiveRangeAnalysis::LiveRange> 
LiveRangeAnalysis::computeLiveRanges(const SsaForm& ssa) {
    std::vector<LiveRange> ranges;
    
    // Simplified live range computation
    // In real implementation, would analyze actual usage
    uint32_t position = 0;
    for (const auto& [var_id, var] : ssa.getVariables()) {
        LiveRange range;
        range.var = var_id;
        range.start = position;
        range.end = position + 10; // Simplified
        ranges.push_back(range);
        position += 5;
    }
    
    return ranges;
}

} // namespace bdi::compiler::backend
