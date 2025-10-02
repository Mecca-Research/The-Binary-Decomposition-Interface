// ===================================================================
// DESC: Defines the core data structures for the BDI Graph, including
//       nodes, edges, types, and the main graph container.
//       This is the central abstraction of the Aeon-0 kernel.
// ===================================================================
#ifndef AEON_GRAPH_H
#define AEON_GRAPH_H

#include "c23_compat.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// --- Binary-grounded identifiers ---
typedef uint64_t NodeId;
typedef uint64_t EdgeId;
typedef uint64_t TypeId;
typedef uint64_t RegionId;
typedef uint64_t DeviceId;

// --- Strong typing + binary view ---
typedef struct {
    TypeId id;
    uint8_t bit_width;      // e.g., 1, 8, 16, 32, 64, 128
    uint8_t signedness;     // 0 = unsigned, 1 = signed
    uint8_t fp;             // 0 = integer, 1 = float
    uint8_t vector_len;     // 1 for scalar, N for SIMD lanes
} BdiType;

// --- Node opcodes (M0 Viable Product) ---
typedef enum {
    // Core Ops
    OP_CONST, OP_LOAD, OP_STORE,
    // Arithmetic Ops
    OP_ADD, OP_MUL,
    // AI/ML Ops
    OP_RELU, OP_MATMUL,
    // Control Flow
    OP_RET,
    OP_GRAD,      // Propagate gradients
    OP_UPDATE,    // Apply a parameter update
    // === FPGA Synthesis Opcodes ===
    OP_SUBGRAPH_BEGIN, // Marks the start of a subgraph for synthesis
    OP_SUBGRAPH_END,   // Marks the end of a synthesizable subgraph
} OpCode;

// --- Verifiable Metadata and Proof Classes ---
// Defines the class of proof required for a node to be considered secure.
typedef enum {
    PROOF_CLASS_NONE   = 0,
    PROOF_CLASS_SAFETY = 1 << 0, // e.g., memory safety, no UB
    PROOF_CLASS_BOUNDS = 1 << 1, // e.g., array bounds checks
} ProofClass;

// The metadata attached to a node, stored in the meta_arena.
typedef struct {
    uint8_t hash[32];      // SHA-256/Blake3 hash of (op, types, inputs)
    uint32_t proof_class;  // Bitmask of required ProofClass flags.
    uint32_t origin;       // Source of the node (e.g., USER, TRAINER).
} NodeMeta;

// --- Node Flags ---
#define NODE_FLAG_SYNTHESIZE (1 << 0) // Hint that this node is part of an FPGA subgraph

// --- Node + edges ---
typedef struct {
    NodeId id;
    OpCode op;
    BdiType out_type;
    NodeId inputs[8];       // Small arity fast path
    uint8_t input_count;
    RegionId region_hint;   // HAM hint
    DeviceId device_hint;   // Dispatch hint (CPU/GPU/FPGA)
    uint64_t flags;         // Used for hints like NODE_FLAG_SYNTHESIZE
    uint64_t meta_off;      // Offset into metadata arena
} GraphNode;

typedef struct {
    EdgeId id;
    NodeId src, dst;
    uint8_t src_port, dst_port;
    uint64_t props; // Latency/capacity/affinity hints
} GraphEdge;

// --- The Main Graph Structure ---
typedef struct {
    GraphNode* nodes;
    size_t node_count;
    size_t node_capacity;
    GraphEdge* edges;
    size_t edge_count;
    size_t edge_capacity;
    uint8_t* meta_arena;
    size_t meta_size;
    size_t meta_capacity;
} BdiGraph;

// --- Structure for defining a parameter update ---
typedef struct {
    float lr; // Learning rate
} OptimizerParams; // Simplified for now

typedef struct {
    NodeId param_node;      // The node holding the parameter to be updated.
    NodeId grad_node;       // The node providing the gradient for the update.
    OptimizerParams opt;
} UpdateSpec;

// --- Graph API ---
NODISCARD BdiGraph* aeon_graph_create();
void aeon_graph_free(BdiGraph* g);
NodeId aeon_graph_add_node(BdiGraph* g, GraphNode node);
// Binds an update rule to the graph
NODISCARD int aeon_bind_update(BdiGraph* g, const UpdateSpec* spec);
// Attaches metadata to a node and stores it in the meta_arena.
NODISCARD int aeon_attach_meta(BdiGraph* g, NodeId node_id, const NodeMeta* meta);


// ===================================================================
// C23 ENHANCEMENTS - Phase 1
// ===================================================================

// C23 compile-time constants
constexpr NodeId INVALID_NODE_ID = 0;
constexpr EdgeId INVALID_EDGE_ID = 0;
constexpr TypeId INVALID_TYPE_ID = 0;
constexpr RegionId INVALID_REGION_ID = 0;
constexpr DeviceId INVALID_DEVICE_ID = 0;

// Graph capacity constants
constexpr size_t MAX_GRAPH_NODES = 1048576;  // 1M nodes
constexpr size_t MAX_GRAPH_EDGES = 4194304;  // 4M edges
constexpr size_t DEFAULT_GRAPH_CAPACITY = 1024;

// Alignment and sizing constants
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t NODE_ALIGNMENT = 64;

// C23 static assertions for structure layout
_Static_assert(sizeof(NodeId) == 8, "NodeId must be 64-bit");
_Static_assert(sizeof(EdgeId) == 8, "EdgeId must be 64-bit");
_Static_assert(sizeof(TypeId) == 8, "TypeId must be 64-bit");
_Static_assert(sizeof(BdiType) <= 8, "BdiType should fit in 64 bits");

#endif // AEON_GRAPH_H
