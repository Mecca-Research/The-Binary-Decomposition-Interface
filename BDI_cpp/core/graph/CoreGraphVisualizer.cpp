#include "GraphVisualizer.hpp"
#include "ExecutionContext.hpp" // For payload->variant->string conversion 
#include <fstream> 
#include <iomanip> // For escape sequences 
namespace chimera::devtools { 
GraphVisualizer::GraphVisualizer(const MetadataStore* meta_store) 
    : metadata_store_(meta_store) {} 
std::string GraphVisualizer::escapeDOT(const std::string& s) { 
    std::ostringstream oss; 
    for (char c : s) { 
        switch (c) {
            case '"':  oss << "\\\""; break; 
            case '\\': oss << "\\\\"; break; 
            case '{':  oss << "\\{"; break; 
            case '}':  oss << "\\}"; break; 
            case '<':  oss << "\\<"; break; 
            case '>':  oss << "\\>"; break; 
            case '|':  oss << "\\|"; break; 
            case '\n': oss << "\\n"; break; 
            case '\t': oss << "\\t"; break; 
            default:
                if (std::isprint(static_cast<unsigned char>(c))) { 
                    oss << c; 
                } else { 
                    // Output non-printable as hex escape 
                    oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c)); 
                } 
        } 
    }
    return oss.str(); 
} 
std::string GraphVisualizer::getNodeStyle(const BDINode& node) { 
    std::string shape = "record"; // Default shape for structured label 
    std::string color = "black"; 
    std::string fillcolor = "lightgrey";
    std::string style = "filled"; 
    switch (node.operation) { 
        case BDIOperationType::META_START: fillcolor = "palegreen"; break; 
        case BDIOperationType::META_END: 
        case BDIOperationType::CTRL_RETURN: fillcolor = "lightcoral"; break; 
        case BDIOperationType::META_CONST: fillcolor = "lightblue"; break; 
        case BDIOperationType::MEM_LOAD: fillcolor = "lightgoldenrod"; break; 
        case BDIOperationType::MEM_STORE: fillcolor = "lightsalmon"; break; 
        case BDIOperationType::CTRL_BRANCH_COND: shape = "diamond"; fillcolor = "yellow"; break; 
        case BDIOperationType::CTRL_CALL: shape = "box"; style="filled,rounded"; fillcolor = "mediumpurple1"; break; 
        // Add more styling based on operation categories 
        default: break; 
    }
    return "[shape=" + shape + ", style=" + style + ", fillcolor=" + fillcolor + ", color=" + color + "]"; 
} 
std::string GraphVisualizer::formatNodeLabel(const BDINode& node) { 
     std::ostringstream label; 
     // Using DOT record shape: { Port0 | { Main Content } | Port1 } 
     // Simplified: { OpType | ID | Payload | Meta } 
     label << "{"; 
     // Row 1: Operation Type 
     label << "Op: " << static_cast<int>(node.operation); // Use int for now, map to string later 
     label << " | ";
     // Row 2: ID 
     label << "ID: " << node.id; 
     // Row 3: Payload (if present) 
     if (!node.payload.data.empty()) { 
         label << " | "; 
         label << "Payload: Type=" << bdiTypeToString(node.payload.type); 
         // Try to convert payload to string representation 
          try { 
              BDIValueVariant val = ExecutionContext::payloadToVariant(node.payload); // Static call 
              label << " Val="; 
              std::visit([&](auto&& arg){ label << arg; }, val); // Requires operator<< for types in variant 
          } catch(...) { label << " (raw data)"; } // Fallback 
     } 
     // Row 4: Metadata (if present and store provided) 
     if (metadata_store_ && node.metadata_handle != 0) { 
          const MetadataVariant* meta = metadata_store_->getMetadata(node.metadata_handle); 
          if (meta) { 
              label << " | "; 
              label << "Meta: "; 
              std::visit([&](auto&& arg){ 
                  using T = std::decay_t<decltype(arg)>; 
                  if constexpr (std::is_same_v<T, SemanticTag>) { label << "Sem(\"" << escapeDOT(arg.description) << "\")"; } 
                  else if constexpr (std::is_same_v<T, ProofTag>) { label << "Proof(" << static_cast<int>(arg.system) << ")"; } 
                  else if constexpr (std::is_same_v<T, HardwareHints>) { label << "HWHints(...)"; } 
                  else if constexpr (!std::is_same_v<T, std::monostate>) { label << "(Other)";} 
              }, *meta); 
           }
     }
     label << "}"; 
return escapeDOT(label.str()); 
} 
std::string GraphVisualizer::renderToDOT(const BDIGraph& graph) {
 std::ostringstream dot; 
    dot << "digraph BDI_Graph_" << escapeDOT(graph.getName()) << " {\n"; 
    dot << "  rankdir=TB;\n"; // Top-to-bottom layout 
    dot << "  node [shape=record, fontname=\"Helvetica\", fontsize=10];\n"; 
    dot << "  edge [fontname=\"Helvetica\", fontsize=9];\n\n"; 
// Define Nodes 
for (const auto& pair : graph) { 
if (!pair.second) continue;
 const BDINode& node = *(pair.second); 
        dot << "  N" << 
node.id << " " << getNodeStyle(node) << " [label=\"" << formatNodeLabel(node) << "\"];\n"; 
    }
    dot << "\n"; 
// Define Edges 
for (const auto& pair : graph) { 
if (!pair.second) continue; 
const BDINode& node = *(pair.second); 
// Data Edges 
for (size_t i = 0; i < node.data_inputs.size(); ++i) { 
const PortRef& ref = node.data_inputs[i]; 
             dot << 
"  N" << ref.node_id << " -> N" << 
                 << 
node.id 
" [style=dashed, color=blue, taillabel=\" P:" << ref.port_index << "\", headlabel=\"In:" << i << "\"];\n"; 
         }
 // Control Edges 
for (NodeID target_id : node.control_outputs) { 
// Check if target exists before drawing edge 
if (graph.getNode(target_id)) { 
                  dot << 
"  N" << 
node.id << " -> N" << target_id << " [style=solid, color=black];\n"; 
              } 
         }
 // Special handling for BRANCH_COND targets? Add True/False labels? 
if (node.operation == BDIOperationType::CTRL_BRANCH_COND && node.control_outputs.size() >= 2) { 
if(graph.getNode(node.control_outputs[0])) dot << "  N" << 
node.id << " -> N" << node.control_outputs[0] << " [style=solid, color
 if(graph.getNode(node.control_outputs[1])) dot << "  N" << 
         }
    }
    dot << "}\n"; 
return dot.str(); 
} 
node.id << " -> N" << node.control_outputs[1] << " [style=solid, color
 bool GraphVisualizer::renderToDOTFile(const BDIGraph& graph, const std::string& filename) { 
std::ofstream ofs(filename); 
if (!ofs) {
 std::cerr << "GraphVisualizer Error: Cannot open file " << filename << " for writing." << std::endl; 
return false; 
    }
    ofs << renderToDOT(graph); 
return ofs.good(); 
} 
} // namespace chimera::devtools 
