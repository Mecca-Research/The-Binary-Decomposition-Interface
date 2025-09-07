Kernel Blueprint: Aeon-0

To execute, learn, and verify intelligence as a live, typed computation graph whose nodes map to real instructions, accelerators, and memory tiers to binary.

-----

Layer 0: Core Data Abstractions

This layer defines the fundamental C structs and enums that form the kernel's data model. These are the binary-grounded "types you can code today."

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// --- Binary-Grounded Identifiers ---
typedef uint64_t NodeId;
typedef uint64_t EdgeId;
typedef uint64_t TypeId;
typedef uint64_t RegionId;
typedef uint64_t DeviceId;

// --- Strong Typing with Binary View ---
typedef struct {
    TypeId id;
    uint8_t bit_width;      // 1, 8, 16, 32, 64, 128...
    uint8_t signedness;     // 0=unsigned, 1=signed
    uint8_t fp;             // 0=integer, 1=float
    uint8_t vector_len;     // 1 for scalar, N for SIMD lanes
} BdiType;

// --- Memory Regions (HAM Tiers) ---
typedef enum {
    HAM_CRITICAL, // Hot working set, pinned in fastest memory
    HAM_ACTIVE,   // Near-term use, actively managed
    HAM_DORMANT,  // Cold, compressible data
    HAM_ARCHIVE   // Persistent storage tier
} HamTier;

typedef struct {
    RegionId id;
    HamTier tier;
    size_t capacity_bytes;
    void* base; // Mapped host pointer or device handle
} HamRegion;

// --- Node Opcodes (Minimal Viable Product) ---
typedef enum {
    // Core Ops
    OP_CONST, OP_LOAD, OP_STORE,
    // Arithmetic Ops
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_NEG,
    // AI/ML Ops
    OP_DOT, OP_MATMUL, OP_RELU, OP_SIGMOID,
    // Control Flow
    OP_BRANCH, OP_MERGE, OP_CALL, OP_RET,
    // Learning Hooks
    OP_GRAD, OP_UPDATE,
    // Verification
    OP_PROOF_TAG,
} OpCode;

// --- Graph Node and Edge Structures ---
typedef struct {
    NodeId id;
    OpCode op;
    BdiType out_type;
    NodeId inputs[8];       // Small arity fast path
    uint8_t input_count;
    RegionId region_hint;   // HAM tier hint
    DeviceId device_hint;   // Dispatch hint (CPU/GPU/FPGA)
    uint64_t flags;         // purity, side-effects, determinism
    uint64_t meta_off;      // Offset into metadata arena
} GraphNode;

typedef struct {
    EdgeId id;
    NodeId src, dst;
    uint8_t src_port, dst_port;
    uint64_t props; // Latency, capacity, affinity hints
} GraphEdge;

// --- The Main Graph Structure ---
typedef struct {
    GraphNode* nodes;
    size_t node_count;
    GraphEdge* edges;
    size_t edge_count;
    uint8_t* meta_arena;    // Arena for proofs, hashes, debug info
    size_t meta_size;
} BdiGraph;
```

-----

Layer 1: Hardware & Memory Interface

This layer defines how the kernel interacts with physical resources.

Device Abstraction (HAL)

The kernel communicates with hardware through a standardized virtual table, allowing for heterogeneous execution.

  * CPU: Baseline device for general-purpose computation, using BCI/BTL for bit-faithful lowering to the target ISA.
  * BPU: "Binary Processing Unit," a disaggregated ALU/FPU for offloading simple scalar arithmetic.
  * GPU: Target for vector and matrix operations like `OP_DOT` and `OP_MATMUL`.
  * FPGA: Target for persistent subgraphs that can be compiled to fixed-latency hardware kernels.
  * DMA/IO: Specialized device for high-speed memory transfers between HAM tiers.

<!-- end list -->

```c
// --- Device Virtual Table ---
typedef struct {
    DeviceId id;
    const char* name;
    // Lowers a BDI node into a device-specific micro-graph or kernel
    int (*lower)(const GraphNode* node, void* out_kernel);
    // Enqueues the kernel for execution on the device
    int (*enqueue)(const void* kernel, const HamRegion* regions);
    // Synchronizes the device execution queue
    int (*sync)(void);
} DeviceVTable;
```

Hierarchical Access Memory (HAM)

The kernel's intelligent memory manager, which treats memory as a tiered, semantic structure.

Policies:
      * CRITICAL: Hot, execution-critical data. Pinned in the fastest available memory (e.g., L1/L2 cache or HBM).
      * ACTIVE: Recently used data. Managed with LRU or entropy-based scoring.
      * DORMANT: Cold data, targeted for symbolic/graphical compression to reduce its memory footprint.
      * ARCHIVE: Persistent tier, mapped to SSD, a key-value store, or a graph database.
Optimization: Memory regions are dynamically promoted or demoted between tiers based on entropy-guided scores and reinforcement learning from access patterns.

<!-- end list -->

```c
// --- HAM Virtual Table ---
typedef struct {
    int (*alloc)(RegionId, size_t, void** out_ptr);
    int (*free)(RegionId, void* ptr);
    // Moves data between HAM tiers
    int (*promote)(RegionId from, RegionId to, void* ptr, size_t n);
    // Applies compression to a DORMANT region
    int (*compress)(RegionId, void* ptr, size_t n);
} HamVTable;
```

-----

Layer 2: Kernel Execution & Scheduling

This layer defines the core runtime loop and the integration of the kernel's learning and verification capabilities.

The Scheduler

A semantic dispatcher that orchestrates the execution of the `BdiGraph`.

1.  Wavefront Selection: Identifies all nodes in the graph whose dependencies are met and adds them to a "ready set."
2.  Type & Proof Gate: Performs a rapid check on each ready node, ensuring its type constraints and proof obligations are met before scheduling.
3.  Semantic Placement: For each validated node, a planner decides the optimal `DeviceId` for execution based on the node's hints, HAM data locality, and current device load.
4.  Lowering & Dispatch: The node is lowered into a device-specific kernel via the `DeviceVTable`, then enqueued for execution.

Learning Hooks

Intelligence is native to the kernel's execution model.

  * `OP_GRAD` nodes are automatically scheduled in a reverse-mode pass after their corresponding forward nodes, propagating gradients.
  * `OP_UPDATE` nodes apply parameter deltas using optimizer policies, triggered by the scheduler after a gradient pass is complete.
  * The AI Trainer's curriculum is a graph generator that emits verified subgraphs for arithmetic, logic, and perception, populating the kernel's standard library.

-----

Layer 3: Verification & Security

This layer defines the mechanisms that ensure the kernel's operations are trustworthy and correct by construction.

Verifiability & Provenance

Every component is traceable to its origin and its correctness specification.

  * Node Metadata: Each `GraphNode` carries metadata including a hash of its definition and a `proof_class`.
  * Merkle Chaining: `OP_PROOF_TAG` nodes link together, forming a Merkle path over a subgraph that provides a single cryptographic hash for verifying the integrity of a complex operation.
  * Secure Mode: When active, the scheduler will refuse to execute any node that lacks the required `proof_class` for its operation.

Binary-Grounded Lowering

The BCI/BTL ensures that the kernel's abstract operations have a bit-faithful representation on the hardware.

  * Binary Recipes: Each `OpCode` has a canonical "Binary Recipe" in the BTL that defines its expected behavior, including flag modifications and overflow conditions.
  * Bit-Accurate Tracing: The PRECOMP layer can use these recipes to generate bit-accurate simulation traces for any node, enabling formal verification and offline debugging.

-----

Layer 4: System Lifecycle & API

This layer defines the kernel's boot process and the interface it exposes to userland for constructing and running graphs.

Kernel Lifecycle

  * Boot: The kernel loads device tables, enumerates HAM regions, and mounts the ARCHIVE tier. It then loads a base standard library graph and system policies.
  * Link: A user program graph is imported. The kernel deduplicates common subgraphs (symbolic compression) and runs a global type and proof pass.
  * Run: The scheduler begins wavefront execution, while HAM performs background migration and telemetry is streamed.
  * Converge: Learning hooks (`OP_GRAD`/`OP_UPDATE`) are cycled until a stop condition is met.
  * Persist: Learned models and updated parameters are saved to the ARCHIVE tier with full provenance.
  * Hot-Swap: The kernel can atomically replace subgraphs at runtime (e.g., updating a GPU kernel or an FPGA bitstream) after verifying the new version's proofs.

Minimal Kernel API

A set of C functions for userland to interact with the Aeon-0 kernel.

```c
// Graph Construction
NodeId aeon_const(BdiGraph* g, BdiType type, const void* bits);
NodeId aeon_op(BdiGraph* g, OpCode op, BdiType out_type, const NodeId* inputs, uint8_t n);
int aeon_attach_meta(BdiGraph* g, NodeId node, const void* meta, size_t n);

// Execution & Learning
typedef struct { double max_ms; bool secure_mode; } RunOpts;
int aeon_run(BdiGraph* g, const RunOpts* opts);
int aeon_step(BdiGraph* g, int steps);

// Persistence
int aeon_save(BdiGraph* g, const char* uri);
int aeon_load(BdiGraph* g, const char* uri);
```
