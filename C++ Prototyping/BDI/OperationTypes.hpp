 #ifndef BDI_CORE_GRAPH_OPERATIONTYPES_HPP
 #define BDI_CORE_GRAPH_OPERATIONTYPES_HPP
 #include <cstdint>
 namespace bdi::core::graph {
 // Enum defining the operations a BDINode can perform.
 // Needs to be comprehensive. Naming convention: DOMAIN_ACTION_TYPEVARIANT
 enum class BDIOperationType : uint16_t {
    // Meta Operations
    META_NOP,           // No operation
    META_START,         // Graph entry point
    META_END,           // Graph exit point / return
    META_COMMENT,       // Holds descriptive text (in payload/metadata)
    META_ASSERT,        // Assert a condition (input bool must be true)
    META_VERIFY_PROOF,  // Verify associated ProofTag
    META_CONST,         // Represents a compile-time constant value (in payload)
    // Memory Operations
    MEM_ALLOC,          // Allocate memory region (size/type in payload/input)
    MEM_FREE,           // Free memory region (ref input)
    MEM_LOAD,           // Load data from memory address (ref input -> output data)
    MEM_STORE,          // Store data to memory address (ref input, data input)
    MEM_COPY,           // Copy memory block (src ref, dest ref, size)
    MEM_SET,            // Set memory block to value (ref, value, size)
    // Arithmetic Operations (Type specified by inputs/node config)
    ARITH_ADD,
    ARITH_SUB,
    ARITH_MUL,
    ARITH_DIV,
    ARITH_MOD,          // Modulo / Remainder
    ARITH_NEG,          // Negation
    ARITH_ABS,          // Absolute Value
    ARITH_INC,          // Increment
    ARITH_DEC,          // Decrement
    // Specialized (Fused Multiply-Add etc.)
    ARITH_FMA,
    // Bitwise Operations
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    BIT_SHL,            // Shift Left
    BIT_SHR,            // Shift Right (Logical or Arithmetic? Often distinct ops needed)
    BIT_ASHR,           // Arithmetic Shift Right
    BIT_ROL,            // Rotate Left
    BIT_ROR,            // Rotate Right
    BIT_POPCOUNT,       // Population Count (count set bits)
    BIT_LZCNT,          // Leading Zero Count
    BIT_TZCNT,          // Trailing Zero Count
    // Logical Operations
    LOGIC_AND,          // Boolean AND
    LOGIC_OR,           // Boolean OR
    LOGIC_XOR,          // Boolean XOR
    LOGIC_NOT,          // Boolean NOT
    // Comparison Operations (Result is typically BOOL)
    CMP_EQ,             // Equal
    CMP_NE,             // Not Equal
    CMP_LT,             // Less Than
    CMP_LE,             // Less Than or Equal
    CMP_GT,             // Greater Than
    CMP_GE,             // Greater Than or Equal
    // Control Flow Operations
    CTRL_JUMP,          // Unconditional jump (target node ID)
    CTRL_BRANCH_COND,   // Conditional branch (condition input, true target, false target)
    CTRL_CALL,          // Call subgraph/function (target node ID / func ptr input)
    CTRL_RETURN,        // Return from subgraph/function (optional return value input)
    CTRL_SWITCH,        // Multi-way branch (value input, map<value, target>)
    // Type Conversion Operations
    CONV_TRUNC,         // Truncation (e.g., float -> int)
    CONV_EXTEND_SIGN,   // Sign extension (e.g., int16 -> int32)
    CONV_EXTEND_ZERO,   // Zero extension (e.g., uint16 -> uint32)
    CONV_FLOAT_TO_INT,
    CONV_INT_TO_FLOAT,
    CONV_BITCAST,       // Reinterpret bits as different type of same size
    // Input/Output Operations
    IO_READ_PORT,
    IO_WRITE_PORT,
    IO_PRINT,           // Debug print
    // --- System / OS Primitives --- 
    SYS_REG_READ,       
    // Read Special HW Register. Payload: BDISpecialRegister enum val. Output: u64 value. 
    SYS_REG_WRITE,      
    SYS_MEM_MAP,        
    SYS_MEM_UNMAP,      
    SYS_MEM_PROPS,      
    SYS_CONTEXT_SAVE,   
    // Write Special HW Register. Payload: BDISpecialRegister enum val. Input 0: u64 value. 
    // Map physical memory. Input 0: phys_addr(u64), Input 1: size(u64), Input 2: flags(u64). Output 0: virt_addr(ptr) or 
    // Unmap memory. Input 0: virt_addr(ptr), Input 1: size(u64). Output 0: bool success. (Privileged) 
    // Set memory region properties. Input 0: RegionID/Addr?, Input 1: props(u64). Output 0: bool success. (Privileged) 
    // Save current ExecutionContext state. Output 0: ContextHandle/Ptr. (Internal VM Use) 
    SYS_CONTEXT_RESTORE,// Restore ExecutionContext state. Input 0: ContextHandle/Ptr. (Internal VM Use) 
    SYS_DISPATCH,       
    // Request scheduler dispatch. Input 0: Target Task Entry NodeID (u64), Input 1: Target Context ID/Handle (u64/ptr). (
    SYS_YIELD,          
    SYS_HALT_TASK,      
    SYS_WAIT_EVENT,     
    SYS_SEND_EVENT,     
    // Yield CPU to scheduler. 
    // Terminate current task/graph execution. 
    // Pause task awaiting event. Input 0: Event type filter? Timeout? 
    // Send event to dispatcher. Input 0: Target Queue/Task ID (u64), Input 1: Event Payload (Any type? MemRef?). 
    SYS_INTERRUPT_ENABLE, // Enable HW interrupts. (Privileged) 
    SYS_INTERRUPT_DISABLE,// Disable HW interrupts. (Privileged) 
    SYS_ACK_INTERRUPT,  // Acknowledge HW interrupt. Input 0: vector(u32). (Privileged) 
    SYS_DEBUG_BREAK,    
    // Trigger debug break. 
    SYS_HALT_SYSTEM,    
    OS_SERVICE_CALL,    
    // Halt entire machine. (Privileged) 
    // Call a registered BDIOS Service Graph. Payload: ServiceID. Inputs: Args for service. Output 0: Result from service.
    // --- End System / OS Primitives ---
    // Concurrency / Parallelism
    CONCURRENCY_SPAWN,  // Start new execution thread/task for subgraph
    CONCURRENCY_JOIN,   // Wait for spawned task to complete
    SYNC_MUTEX_LOCK,
    SYNC_MUTEX_UNLOCK,
    SYNC_ATOMIC_RMW,    // Atomic Read-Modify-Write
    COMM_CHANNEL_SEND,
    COMM_CHANNEL_RECV,
    // DSL / High-Level Placeholders (to be decomposed)
    DSL_RESOLVE,        // Placeholder for complex DSL operation needing decomposition
    DSL_LAMBDA_CREATE,
    DSL_LAMBDA_APPLY,
    // Intelligence Engine Primitives (Examples)
    LEARN_UPDATE_PARAM, // Apply delta to a parameter in memory
    FEEDBACK_CALC_ERROR,
    RECUR_PROPAGATE_STATE,
    // Vector / SIMD Operations (Often need size/mask variants)
    VEC_ADD,
    VEC_MUL,
    VEC_LOAD_PACKED,
    VEC_STORE_PACKED,
    VEC_SHUFFLE,
    // Need a way to identify special registers if using REG_READ/WRITE 
    // Intelligence Ops  
    LEARN_GET_GRADIENT,  // Output: Gradient value for specified Param Source ID 
    LEARN_APPLY_DELTA,   // Input 0: Param Ref (NodeID?), Input 1: Delta Value. Modifies param. 
    RECUR_READ_STATE,    // Input 0: Prev Step Producer NodeID. Output: State value. 
    RECUR_WRITE_STATE,   // Input 0: Value to store for next step (associated with this node's ID) 
    // ...
 };
 enum class BDISpecialRegister : uint8_t { 
    STACK_POINTER, 
    FRAME_POINTER, 
    INSTRUCTION_POINTER, 
    // Add others if needed (flags, etc.) 
    }; 
    REG_READ,       
    REG_WRITE,      
    STACK_PUSH,     
    STACK_POP,      
    // Read from special register (e.g., SP, FP, Instruction Ptr) 
    // Write to special register 
    // Pushes value onto stack (updates SP implicitly) - Optional High-Level Op 
    // Pops value from stack (updates SP implicitly) - Optional High-Level Op 
    // Alternatively, use lower-level ops: 
    // MEM_STORE relative to REG_READ(SP) 
    // ARITH_ADD/SUB to update REG_READ(SP) 
    // MEM_LOAD relative to updated SP 
    // ...
    // Placeholders for other domains
    GRAPH_TRAVERSE,
    LINALG_MATMUL,
    SIGNAL_FFT,
    OPERATION_TYPE_COUNT // Sentinel value
    };
 // Define BDISpecialRegister enum (as in HAL, maybe shared header?) 
 enum class BDISpecialRegister : uint8_t { /* ... values like STACK_POINTER, FRAME_POINTER ... */ }; 
 } // namespace bdi::core::graph
 #endif // BDI_CORE_GRAPH_OPERATIONTYPES_HPP
