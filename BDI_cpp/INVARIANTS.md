# BDI Graph & Runtime Invariants (Draft)

## Graph/Node/Port
- **NodeID uniqueness:** Every node has a unique stable identifier.
- **Port typing:** Each port has a declared type; all inbound edges must type-check.
- **Arity & direction:** Ports have direction (in/out) and fixed arity per Node kind.
- **Acyclic constraints (if any):** Define where cycles are permitted (e.g., feedback loops with delay).

## Payload & Typed Values
- **TypedPayload:** Encodes compile-time tag + runtime value domain.
- **Conversions:** Only allowed via explicit adapters; no implicit narrowing in graph mutators.
- **Const-meta:** Metadata with `META_CONST` must be immutable across transforms.

## Region/ISA Mapping
- **RegionMapping:** Subgraphs are assigned to regions that imply execution backends.
- **Isolation:** Data leaving a region is serialized or passed through defined ABI boundaries.
- **Proof tags:** Transformations that cross regions must propagate or re-derive proof tags.

## ExecutionContext
- **Ownership:** `ExecutionContext` owns transient buffers; graph owns topology and static metadata.
- **Determinism:** Given the same graph state and inputs, execution is deterministic unless a region is marked non-deterministic (documented).
- **PortRef hashing:** Hash function combines `NodeID` and `PortIndex`; collisions must be improbable.
- **Error policy:** All runtime errors return status codes (or exceptions)—policy must be consistent.

## Serialization & Ledger
- **Stable format:** Versioned serialization with schema hash.
- **Proof-carrying execution:** Ledger stores trace + proof tag; any replay must re-verify hash chain.

## Testing
- **Golden traces:** Hello-graph → deterministic output & trace.
- **Property tests:** Graph rewrites preserve type and semantics.
- **Fuzzing entry-points:** Deserialization, graph mutators, and region boundary calls.
