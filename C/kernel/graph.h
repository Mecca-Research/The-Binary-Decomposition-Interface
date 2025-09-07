// ===================================================================
// DESC: Defines the core data structures for the BDI Graph, including
//       nodes, edges, types, and the main graph container.
//       This is the central abstraction of the Aion-0 kernel.
// ===================================================================
#ifndef AION_GRAPH_H
#define AION_GRAPH_H

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
} OpCode;

// --- Node + edges ---
typedef struct {
    NodeId id;
    OpCode op;
    BdiType out_type;
    NodeId inputs[8];       // Small arity fast path
    uint8_t input_count;
    RegionId region_hint;   // HAM hint
    DeviceId device_hint;   // Dispatch hint (CPU/GPU/FPGA)
    uint64_t flags;         // Purity, side-effects, determinism
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

// --- Graph API ---
BdiGraph* aion_graph_create();
void aion_graph_free(BdiGraph* g);
NodeId aion_graph_add_node(BdiGraph* g, GraphNode node);

#endif // AION_GRAPH_H


// ===================================================================
// FILE: kernel/ham.h
// DESC: Defines the structures and interface for Hierarchical
//       Access Memory (HAM). M0 includes two tiers: CRITICAL & ACTIVE.
// ===================================================================
#ifndef AION_HAM_H
#define AION_HAM_H

#include "graph.h" // For RegionId

// --- Memory regions (HAM tiers) for M0 ---
typedef enum {
    HAM_CRITICAL, // Hot working set, pinned in fastest memory
    HAM_ACTIVE    // Near-term use, general purpose
} HamTier;

typedef struct {
    RegionId id;
    HamTier tier;
    size_t capacity_bytes;
    void* base; // Mapped host pointer
} HamRegion;

// --- HAM Virtual Table (Interface) ---
typedef struct {
    // Allocates a new region of a specific tier and size.
    int (*alloc)(RegionId* out_id, HamTier tier, size_t size_bytes, void** out_ptr);
    // Frees a previously allocated region.
    int (*free)(RegionId id);
} HamVTable;

#endif // AION_HAM_H


// ===================================================================
// FILE: kernel/device.h
// DESC: Defines the abstraction for an execution device (e.g., CPU).
//       M0 includes the CPU device.
// ===================================================================
#ifndef AION_DEVICE_H
#define AION_DEVICE_H

#include "graph.h"
#include "ham.h"

// --- Device Virtual Table (Interface) ---
typedef struct {
    DeviceId id;
    const char* name;
    // Lowers a BDI node into a device-specific, executable kernel.
    // For a CPU, this might be a pointer to a function or a micro-op sequence.
    int (*lower)(const GraphNode* node, void* out_kernel);
    // Enqueues the kernel for execution on the device.
    int (*enqueue)(const void* kernel, const HamRegion** regions, size_t num_regions);
    // Blocks until all enqueued kernels on the device are complete.
    int (*sync)(void);
} DeviceVTable;

#endif // AION_DEVICE_H
