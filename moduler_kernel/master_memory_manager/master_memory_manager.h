
// ===================================================================
// BDI Master Memory Manager - Phase 2: Advanced Systems & Toolchain
// Complete x86 ISA, interrupts, task switching, SIMD/AVX, atomics
// Multi-rail synthesis: spec → synthesize → prove → bench workflow
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <immintrin.h>
#include "../capgraph/capability.h"
#include "../uabi/uops.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// Advanced x86 Systems - Complete ISA Support
// ===================================================================

// Complete x86 instruction categories
typedef enum {
    MMM_INSTR_DATA_MOVEMENT = 0,    // MOV, MOVZX, MOVSX, LEA, XCHG, etc.
    MMM_INSTR_ARITHMETIC,           // ADD, SUB, MUL, DIV, INC, DEC, etc.
    MMM_INSTR_LOGICAL,              // AND, OR, XOR, NOT, TEST, etc.
    MMM_INSTR_SHIFT_ROTATE,         // SHL, SHR, SAL, SAR, ROL, ROR, etc.
    MMM_INSTR_CONTROL_FLOW,         // JMP, Jcc, CALL, RET, LOOP, etc.
    MMM_INSTR_STRING,               // MOVS, CMPS, SCAS, LODS, STOS, etc.
    MMM_INSTR_STACK,                // PUSH, POP, PUSHAD, POPAD, etc.
    MMM_INSTR_FLAG_CONTROL,         // STC, CLC, STD, CLD, STI, CLI, etc.
    MMM_INSTR_SYSTEM,               // CPUID, RDTSC, INT, IRET, etc.
    MMM_INSTR_SIMD_SSE,             // SSE/SSE2/SSE3/SSSE3/SSE4 instructions
    MMM_INSTR_SIMD_AVX,             // AVX/AVX2 instructions
    MMM_INSTR_SIMD_AVX512,          // AVX-512 instructions
    MMM_INSTR_ATOMIC,               // LOCK prefix, CMPXCHG, XADD, etc.
    MMM_INSTR_MEMORY_FENCE,         // MFENCE, SFENCE, LFENCE
    MMM_INSTR_PRIVILEGED,           // Ring 0 only instructions
    MMM_INSTR_COUNT
} mmm_instruction_category_t;

// x86 addressing modes with complete support
typedef enum {
    MMM_ADDR_IMMEDIATE = 0,         // Immediate operand
    MMM_ADDR_REGISTER,              // Register operand
    MMM_ADDR_DIRECT,                // Direct memory [disp32]
    MMM_ADDR_INDIRECT,              // Indirect [reg]
    MMM_ADDR_INDEXED,               // Indexed [base + index*scale + disp]
    MMM_ADDR_RIP_RELATIVE,          // RIP-relative (x64 only)
    MMM_ADDR_SEGMENT_OFFSET         // Segment:offset (legacy)
} mmm_addressing_mode_t;

// Complete x86 register set
typedef enum {
    // 8-bit registers
    MMM_REG_AL = 0, MMM_REG_CL, MMM_REG_DL, MMM_REG_BL,
    MMM_REG_AH, MMM_REG_CH, MMM_REG_DH, MMM_REG_BH,
    MMM_REG_R8B, MMM_REG_R9B, MMM_REG_R10B, MMM_REG_R11B,
    MMM_REG_R12B, MMM_REG_R13B, MMM_REG_R14B, MMM_REG_R15B,
    
    // 16-bit registers
    MMM_REG_AX = 16, MMM_REG_CX, MMM_REG_DX, MMM_REG_BX,
    MMM_REG_SP, MMM_REG_BP, MMM_REG_SI, MMM_REG_DI,
    MMM_REG_R8W, MMM_REG_R9W, MMM_REG_R10W, MMM_REG_R11W,
    MMM_REG_R12W, MMM_REG_R13W, MMM_REG_R14W, MMM_REG_R15W,
    
    // 32-bit registers
    MMM_REG_EAX = 32, MMM_REG_ECX, MMM_REG_EDX, MMM_REG_EBX,
    MMM_REG_ESP, MMM_REG_EBP, MMM_REG_ESI, MMM_REG_EDI,
    MMM_REG_R8D, MMM_REG_R9D, MMM_REG_R10D, MMM_REG_R11D,
    MMM_REG_R12D, MMM_REG_R13D, MMM_REG_R14D, MMM_REG_R15D,
    
    // 64-bit registers
    MMM_REG_RAX = 48, MMM_REG_RCX, MMM_REG_RDX, MMM_REG_RBX,
    MMM_REG_RSP, MMM_REG_RBP, MMM_REG_RSI, MMM_REG_RDI,
    MMM_REG_R8, MMM_REG_R9, MMM_REG_R10, MMM_REG_R11,
    MMM_REG_R12, MMM_REG_R13, MMM_REG_R14, MMM_REG_R15,
    
    // Segment registers
    MMM_REG_CS = 64, MMM_REG_DS, MMM_REG_ES, MMM_REG_FS, MMM_REG_GS, MMM_REG_SS,
    
    // Control registers
    MMM_REG_CR0 = 70, MMM_REG_CR1, MMM_REG_CR2, MMM_REG_CR3, MMM_REG_CR4,
    
    // Debug registers
    MMM_REG_DR0 = 75, MMM_REG_DR1, MMM_REG_DR2, MMM_REG_DR3, MMM_REG_DR6, MMM_REG_DR7,
    
    // XMM/YMM/ZMM registers
    MMM_REG_XMM0 = 81, MMM_REG_XMM1, MMM_REG_XMM2, MMM_REG_XMM3,
    MMM_REG_XMM4, MMM_REG_XMM5, MMM_REG_XMM6, MMM_REG_XMM7,
    MMM_REG_XMM8, MMM_REG_XMM9, MMM_REG_XMM10, MMM_REG_XMM11,
    MMM_REG_XMM12, MMM_REG_XMM13, MMM_REG_XMM14, MMM_REG_XMM15,
    
    MMM_REG_COUNT
} mmm_register_t;

// ===================================================================
// Interrupts & IDT/APIC Flow Management
// ===================================================================

// Interrupt descriptor types
typedef enum {
    MMM_IDT_TASK_GATE = 0x5,        // Task gate (32-bit)
    MMM_IDT_INTERRUPT_GATE_16 = 0x6, // 16-bit interrupt gate
    MMM_IDT_TRAP_GATE_16 = 0x7,     // 16-bit trap gate
    MMM_IDT_INTERRUPT_GATE_32 = 0xE, // 32-bit interrupt gate
    MMM_IDT_TRAP_GATE_32 = 0xF      // 32-bit trap gate
} mmm_idt_gate_type_t;

// IDT entry structure
typedef struct __attribute__((packed)) {
    uint16_t offset_low;            // Offset bits 0-15
    uint16_t selector;              // Code segment selector
    uint8_t ist : 3;                // Interrupt Stack Table (x64)
    uint8_t reserved1 : 5;          // Reserved
    uint8_t type : 4;               // Gate type
    uint8_t s : 1;                  // Storage segment (0 for gates)
    uint8_t dpl : 2;                // Descriptor privilege level
    uint8_t p : 1;                  // Present bit
    uint16_t offset_mid;            // Offset bits 16-31
    uint32_t offset_high;           // Offset bits 32-63 (x64 only)
    uint32_t reserved2;             // Reserved (x64 only)
} mmm_idt_entry_t;

// APIC register offsets
typedef enum {
    MMM_APIC_ID = 0x020,            // APIC ID register
    MMM_APIC_VERSION = 0x030,       // APIC version register
    MMM_APIC_TPR = 0x080,           // Task priority register
    MMM_APIC_APR = 0x090,           // Arbitration priority register
    MMM_APIC_PPR = 0x0A0,           // Processor priority register
    MMM_APIC_EOI = 0x0B0,           // End of interrupt register
    MMM_APIC_RRD = 0x0C0,           // Remote read register
    MMM_APIC_LDR = 0x0D0,           // Logical destination register
    MMM_APIC_DFR = 0x0E0,           // Destination format register
    MMM_APIC_SPURIOUS = 0x0F0,      // Spurious interrupt vector register
    MMM_APIC_ISR_BASE = 0x100,      // In-service register base
    MMM_APIC_TMR_BASE = 0x180,      // Trigger mode register base
    MMM_APIC_IRR_BASE = 0x200,      // Interrupt request register base
    MMM_APIC_ESR = 0x280,           // Error status register
    MMM_APIC_ICR_LOW = 0x300,       // Interrupt command register low
    MMM_APIC_ICR_HIGH = 0x310,      // Interrupt command register high
    MMM_APIC_TIMER_LVT = 0x320,     // Timer local vector table
    MMM_APIC_THERMAL_LVT = 0x330,   // Thermal local vector table
    MMM_APIC_PERF_LVT = 0x340,      // Performance counter LVT
    MMM_APIC_LINT0_LVT = 0x350,     // Local interrupt 0 LVT
    MMM_APIC_LINT1_LVT = 0x360,     // Local interrupt 1 LVT
    MMM_APIC_ERROR_LVT = 0x370,     // Error LVT
    MMM_APIC_TIMER_ICR = 0x380,     // Timer initial count register
    MMM_APIC_TIMER_CCR = 0x390,     // Timer current count register
    MMM_APIC_TIMER_DCR = 0x3E0      // Timer divide configuration register
} mmm_apic_register_t;

// Interrupt context structure
typedef struct {
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    
    // Segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    
    // Control registers
    uint64_t rip, rflags;
    
    // Extended state (SIMD registers)
    uint8_t fxsave_area[512] __attribute__((aligned(16)));
    
    // Interrupt information
    uint32_t vector;
    uint32_t error_code;
    bool has_error_code;
} mmm_interrupt_context_t;

// ===================================================================
// Task Switching vs Software Scheduling
// ===================================================================

// Task state segment (TSS) structure
typedef struct __attribute__((packed)) {
    uint32_t reserved1;
    uint64_t rsp0;                  // Stack pointer for privilege level 0
    uint64_t rsp1;                  // Stack pointer for privilege level 1
    uint64_t rsp2;                  // Stack pointer for privilege level 2
    uint64_t reserved2;
    uint64_t ist1;                  // Interrupt stack table 1
    uint64_t ist2;                  // Interrupt stack table 2
    uint64_t ist3;                  // Interrupt stack table 3
    uint64_t ist4;                  // Interrupt stack table 4
    uint64_t ist5;                  // Interrupt stack table 5
    uint64_t ist6;                  // Interrupt stack table 6
    uint64_t ist7;                  // Interrupt stack table 7
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;            // I/O map base address
} mmm_tss_t;

// Task switching method
typedef enum {
    MMM_TASK_SWITCH_HARDWARE,       // Hardware task switching (legacy)
    MMM_TASK_SWITCH_SOFTWARE        // Software context switching (modern)
} mmm_task_switch_method_t;

// Software task context
typedef struct {
    // CPU state
    mmm_interrupt_context_t cpu_context;
    
    // Memory management
    uint64_t cr3;                   // Page directory base
    
    // Task information
    uint32_t task_id;
    uint32_t priority;
    uint64_t time_slice;
    
    // Stack information
    void *kernel_stack;
    void *user_stack;
    size_t kernel_stack_size;
    size_t user_stack_size;
    
    // Extended state
    bool fpu_used;
    bool avx_used;
    bool avx512_used;
} mmm_task_context_t;

// ===================================================================
// SIMD/AVX Instruction Support and Optimization
// ===================================================================

// SIMD instruction sets
typedef enum {
    MMM_SIMD_NONE = 0,
    MMM_SIMD_SSE = 1 << 0,
    MMM_SIMD_SSE2 = 1 << 1,
    MMM_SIMD_SSE3 = 1 << 2,
    MMM_SIMD_SSSE3 = 1 << 3,
    MMM_SIMD_SSE4_1 = 1 << 4,
    MMM_SIMD_SSE4_2 = 1 << 5,
    MMM_SIMD_AVX = 1 << 6,
    MMM_SIMD_AVX2 = 1 << 7,
    MMM_SIMD_AVX512F = 1 << 8,
    MMM_SIMD_AVX512DQ = 1 << 9,
    MMM_SIMD_AVX512CD = 1 << 10,
    MMM_SIMD_AVX512BW = 1 << 11,
    MMM_SIMD_AVX512VL = 1 << 12
} mmm_simd_features_t;

// SIMD data types
typedef union {
    // SSE types
    __m128 m128_f32;                // 4x float32
    __m128d m128_f64;               // 2x float64
    __m128i m128_i;                 // 128-bit integer
    
    // AVX types
    __m256 m256_f32;                // 8x float32
    __m256d m256_f64;               // 4x float64
    __m256i m256_i;                 // 256-bit integer
    
    // AVX-512 types
    __m512 m512_f32;                // 16x float32
    __m512d m512_f64;               // 8x float64
    __m512i m512_i;                 // 512-bit integer
    
    // Raw bytes
    uint8_t bytes[64];
} mmm_simd_register_t;

// SIMD optimization hints
typedef struct {
    bool prefer_avx512;             // Prefer AVX-512 when available
    bool prefer_fma;                // Prefer fused multiply-add
    bool allow_unaligned;           // Allow unaligned memory access
    uint32_t vector_width;          // Preferred vector width in bits
    uint32_t cache_line_size;       // Target cache line size
} mmm_simd_hints_t;

// ===================================================================
// Atomic Operations and Memory Fences
// ===================================================================

// Memory ordering constraints
typedef enum {
    MMM_MEMORY_ORDER_RELAXED = 0,   // No ordering constraints
    MMM_MEMORY_ORDER_CONSUME,       // Consume ordering (deprecated)
    MMM_MEMORY_ORDER_ACQUIRE,       // Acquire ordering
    MMM_MEMORY_ORDER_RELEASE,       // Release ordering
    MMM_MEMORY_ORDER_ACQ_REL,       // Acquire-release ordering
    MMM_MEMORY_ORDER_SEQ_CST        // Sequential consistency
} mmm_memory_order_t;

// Atomic operation types
typedef enum {
    MMM_ATOMIC_LOAD = 0,
    MMM_ATOMIC_STORE,
    MMM_ATOMIC_EXCHANGE,
    MMM_ATOMIC_COMPARE_EXCHANGE,
    MMM_ATOMIC_FETCH_ADD,
    MMM_ATOMIC_FETCH_SUB,
    MMM_ATOMIC_FETCH_AND,
    MMM_ATOMIC_FETCH_OR,
    MMM_ATOMIC_FETCH_XOR
} mmm_atomic_op_t;

// Memory fence types
typedef enum {
    MMM_FENCE_LOAD = 0,             // Load fence (LFENCE)
    MMM_FENCE_STORE,                // Store fence (SFENCE)
    MMM_FENCE_FULL                  // Full fence (MFENCE)
} mmm_fence_type_t;

// ===================================================================
// DMA & PCIe Queue Management
// ===================================================================

// DMA transfer types
typedef enum {
    MMM_DMA_MEMORY_TO_MEMORY = 0,
    MMM_DMA_MEMORY_TO_DEVICE,
    MMM_DMA_DEVICE_TO_MEMORY,
    MMM_DMA_DEVICE_TO_DEVICE
} mmm_dma_transfer_type_t;

// DMA descriptor
typedef struct {
    uint64_t src_addr;              // Source address
    uint64_t dst_addr;              // Destination address
    uint32_t length;                // Transfer length in bytes
    uint32_t flags;                 // Transfer flags
    struct mmm_dma_descriptor *next; // Next descriptor (for chaining)
} mmm_dma_descriptor_t;

// PCIe queue entry
typedef struct {
    uint64_t address;               // Memory address
    uint32_t length;                // Data length
    uint16_t flags;                 // Queue flags
    uint16_t tag;                   // Transaction tag
} mmm_pcie_queue_entry_t;

// ===================================================================
// Multi-Rail Synthesis Toolchain
// ===================================================================

// BDI Dot/Filament graph constraints
typedef struct {
    // Performance constraints
    uint32_t max_latency_cycles;    // Maximum latency in CPU cycles
    uint32_t min_uops_per_cycle;    // Minimum micro-ops per cycle
    uint32_t max_code_size_bytes;   // Maximum code size in bytes
    
    // Instruction constraints
    uint64_t permitted_instructions; // Bitmask of allowed instruction categories
    bool allow_memory_ops;          // Allow memory operations
    bool allow_branches;            // Allow branch instructions
    bool allow_simd;                // Allow SIMD instructions
    
    // Memory layout constraints
    uint32_t stack_alignment;       // Required stack alignment
    uint32_t data_alignment;        // Required data alignment
    bool prefer_registers;          // Prefer register allocation
    
    // Calling convention
    uint32_t calling_convention;    // Calling convention ID
    uint32_t max_parameters;        // Maximum function parameters
    bool preserve_registers;        // Preserve non-volatile registers
} mmm_synthesis_constraints_t;

// Multi-rail synthesis rails
typedef enum {
    MMM_RAIL_ASM_DSL = 0,          // Rail-1: Constrained ASM DSL
    MMM_RAIL_C_REFERENCE,          // Rail-2: C reference implementation
    MMM_RAIL_PROOF_STUBS,          // Rail-3: Proof stubs
    MMM_RAIL_COUNT
} mmm_synthesis_rail_t;

// ASM DSL token types
typedef enum {
    MMM_TOKEN_OPCODE = 0,          // Instruction opcode
    MMM_TOKEN_REGISTER,            // Register operand
    MMM_TOKEN_IMMEDIATE,           // Immediate operand
    MMM_TOKEN_MEMORY,              // Memory operand
    MMM_TOKEN_LABEL,               // Jump/call label
    MMM_TOKEN_DIRECTIVE            // Assembler directive
} mmm_asm_token_type_t;

// ASM DSL token
typedef struct {
    mmm_asm_token_type_t type;
    char text[64];
    uint64_t value;
    uint32_t line_number;
} mmm_asm_token_t;

// Proof stub types
typedef enum {
    MMM_PROOF_MEMORY_REGION = 0,   // Memory region specification
    MMM_PROOF_ALIAS_ANALYSIS,      // Pointer aliasing constraints
    MMM_PROOF_CLOBBER_LIST,        // Register/memory clobber list
    MMM_PROOF_PRIVILEGE_CHECK,     // Privilege level requirements
    MMM_PROOF_BDI_CHK             // BDI check mapping
} mmm_proof_stub_type_t;

// ===================================================================
// Hard Validation System
// ===================================================================

// Validation test types
typedef enum {
    MMM_VALIDATION_EQUIVALENCE = 0, // Behavioral equivalence testing
    MMM_VALIDATION_SAFETY,          // Memory safety checks
    MMM_VALIDATION_PERFORMANCE,     // Performance validation
    MMM_VALIDATION_CORRECTNESS      // Functional correctness
} mmm_validation_type_t;

// Validation result
typedef struct {
    mmm_validation_type_t type;
    bool passed;
    char error_message[256];
    uint64_t execution_cycles;
    uint32_t memory_usage_bytes;
    double performance_score;
} mmm_validation_result_t;

// ===================================================================
// Auto-Rewrite Loop System
// ===================================================================

// Failure types for auto-rewrite
typedef enum {
    MMM_FAILURE_PERFORMANCE = 0,   // Performance target not met
    MMM_FAILURE_SAFETY,            // Safety violation detected
    MMM_FAILURE_CORRECTNESS,       // Functional incorrectness
    MMM_FAILURE_RESOURCE           // Resource constraint violation
} mmm_failure_type_t;

// Rewrite strategy
typedef enum {
    MMM_REWRITE_INSTRUCTION_SELECTION = 0, // Try different instructions
    MMM_REWRITE_REGISTER_ALLOCATION,       // Different register allocation
    MMM_REWRITE_MEMORY_LAYOUT,             // Different memory layout
    MMM_REWRITE_SIMD_VECTORIZATION,        // SIMD vectorization changes
    MMM_REWRITE_LOOP_UNROLLING,            // Loop unrolling adjustments
    MMM_REWRITE_CALLING_CONVENTION         // Calling convention changes
} mmm_rewrite_strategy_t;

// ===================================================================
// Master Memory Manager Core Structure
// ===================================================================

typedef struct {
    // Hardware capabilities
    bdi_cpu_caps_t cpu_caps;
    mmm_simd_features_t simd_features;
    
    // Interrupt management
    mmm_idt_entry_t *idt_table;
    size_t idt_size;
    void *apic_base;
    
    // Task management
    mmm_task_switch_method_t task_method;
    mmm_task_context_t *current_task;
    mmm_tss_t *tss;
    
    // Memory management
    void *page_directory;
    void *tlb_cache;
    
    // DMA management
    mmm_dma_descriptor_t *dma_descriptors;
    size_t dma_descriptor_count;
    
    // Synthesis toolchain
    mmm_synthesis_constraints_t constraints;
    mmm_validation_result_t last_validation;
    
    // Performance counters
    uint64_t instruction_count;
    uint64_t cycle_count;
    uint64_t cache_misses;
    uint64_t tlb_misses;
} mmm_master_memory_manager_t;

// ===================================================================
// Core API Functions
// ===================================================================

// Initialization and cleanup
mmm_master_memory_manager_t* mmm_create(const bdi_cpu_caps_t *caps);
void mmm_destroy(mmm_master_memory_manager_t *mmm);
bool mmm_initialize(mmm_master_memory_manager_t *mmm);
void mmm_shutdown(mmm_master_memory_manager_t *mmm);

// x86 ISA support
bool mmm_execute_instruction(mmm_master_memory_manager_t *mmm, 
                            mmm_instruction_category_t category,
                            const void *instruction_bytes,
                            size_t instruction_length);
bool mmm_decode_instruction(const void *instruction_bytes,
                           size_t length,
                           mmm_instruction_category_t *category,
                           mmm_addressing_mode_t *addressing);

// Interrupt management
bool mmm_setup_idt(mmm_master_memory_manager_t *mmm, size_t entries);
bool mmm_install_interrupt_handler(mmm_master_memory_manager_t *mmm,
                                  uint8_t vector,
                                  void (*handler)(mmm_interrupt_context_t *ctx),
                                  mmm_idt_gate_type_t gate_type,
                                  uint8_t privilege_level);
void mmm_send_eoi(mmm_master_memory_manager_t *mmm);

// Task switching
bool mmm_setup_task_switching(mmm_master_memory_manager_t *mmm,
                             mmm_task_switch_method_t method);
bool mmm_create_task(mmm_master_memory_manager_t *mmm,
                    mmm_task_context_t *task,
                    void (*entry_point)(void),
                    void *stack,
                    size_t stack_size);
bool mmm_switch_task(mmm_master_memory_manager_t *mmm,
                    mmm_task_context_t *new_task);

// SIMD/AVX support
bool mmm_detect_simd_features(mmm_master_memory_manager_t *mmm);
bool mmm_optimize_simd_code(mmm_master_memory_manager_t *mmm,
                           const void *input_code,
                           size_t input_size,
                           void **output_code,
                           size_t *output_size,
                           const mmm_simd_hints_t *hints);

// Atomic operations
bool mmm_atomic_operation(mmm_master_memory_manager_t *mmm,
                         mmm_atomic_op_t operation,
                         void *address,
                         void *value,
                         void *expected,
                         mmm_memory_order_t order);
void mmm_memory_fence(mmm_master_memory_manager_t *mmm, mmm_fence_type_t type);

// DMA management
bool mmm_setup_dma(mmm_master_memory_manager_t *mmm, size_t descriptor_count);
bool mmm_start_dma_transfer(mmm_master_memory_manager_t *mmm,
                           const mmm_dma_descriptor_t *descriptor);
bool mmm_wait_dma_completion(mmm_master_memory_manager_t *mmm, uint32_t timeout_ms);

// Multi-rail synthesis
bool mmm_parse_bdi_spec(mmm_master_memory_manager_t *mmm,
                       const char *spec_text,
                       mmm_synthesis_constraints_t *constraints);
bool mmm_synthesize_multi_rail(mmm_master_memory_manager_t *mmm,
                              const mmm_synthesis_constraints_t *constraints,
                              void **asm_output,
                              void **c_output,
                              void **proof_output);

// Validation system
bool mmm_validate_implementation(mmm_master_memory_manager_t *mmm,
                               const void *asm_code,
                               const void *c_code,
                               const void *proof_stubs,
                               mmm_validation_result_t *result);

// Auto-rewrite loop
bool mmm_auto_rewrite(mmm_master_memory_manager_t *mmm,
                     mmm_failure_type_t failure_type,
                     mmm_rewrite_strategy_t strategy,
                     void **improved_code,
                     size_t *improved_size);

// Performance monitoring
void mmm_reset_performance_counters(mmm_master_memory_manager_t *mmm);
void mmm_get_performance_stats(mmm_master_memory_manager_t *mmm,
                              uint64_t *instructions,
                              uint64_t *cycles,
                              uint64_t *cache_misses,
                              uint64_t *tlb_misses);

#ifdef __cplusplus
}
#endif
