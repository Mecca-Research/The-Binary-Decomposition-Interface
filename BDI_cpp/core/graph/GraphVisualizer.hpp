#ifndef CHIMERA_DEVTOOLS_GRAPHVISUALIZER_HPP 
#define CHIMERA_DEVTOOLS_GRAPHVISUALIZER_HPP 
#include "BDIGraph.hpp" 
#include "MetadataStore.hpp" // To potentially display metadata 
#include <string> 
#include <ostream> 
#include <sstream> 
namespace chimera::devtools { 
using namespace bdi::core::graph; 
using namespace bdi::core::types; 
using namespace bdi::meta; 
class GraphVisualizer { 
public: 
// Optionally takes MetadataStore to enrich labels 
explicit GraphVisualizer(const MetadataStore* meta_store = nullptr); 
// Generate DOT language output for the graph 
std::string renderToDOT(const BDIGraph& graph); 
bool renderToDOTFile(const BDIGraph& graph, const std::string& filename); 
private: 
const MetadataStore* metadata_store_; 
// Helper to escape strings for DOT labels 
std::string escapeDOT(const std::string& s); 
// Helper to get node style based on operation type 
std::string getNodeStyle(const BDINode& node);
 // Helper to format payload/metadata for label 
std::string formatNodeLabel(const BDINode& node); 
}; 
} // namespace chimera::devtools 
#endif // CHIMERA_DEVTOOLS_GRAPHVISUALIZER_HPP 
