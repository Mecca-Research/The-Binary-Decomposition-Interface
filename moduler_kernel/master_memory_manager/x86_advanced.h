
// ===================================================================
// BDI Master Memory Manager - Advanced x86 Systems
// Complete x86 ISA support, SIMD/AVX optimizations, atomics
// ===================================================================

#pragma once

#include "master_memory_manager.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// Complete x86 Instruction Set Architecture
// ===================================================================

// x86 instruction prefixes
typedef enum {
    MMM_PREFIX_NONE = 0,
    MMM_PREFIX_LOCK = 0xF0,         // LOCK prefix for atomic operations
    MMM_PREFIX_REPNE = 0xF2,        // REPNE/REPNZ prefix
    MMM_PREFIX_REP = 0xF3,          // REP/REPE/REPZ prefix
    MMM_PREFIX_CS = 0x2E,           // CS segment override
    MMM_PREFIX_SS = 0x36,           // SS segment override
    MMM_PREFIX_DS = 0x3E,           // DS segment override
    MMM_PREFIX_ES = 0x26,           // ES segment override
    MMM_PREFIX_FS = 0x64,           // FS segment override
    MMM_PREFIX_GS = 0x65,           // GS segment override
    MMM_PREFIX_OPERAND_SIZE = 0x66, // Operand size override
    MMM_PREFIX_ADDRESS_SIZE = 0x67  // Address size override
} mmm_x86_prefix_t;

// ModR/M byte structure
typedef struct {
    uint8_t rm : 3;                 // R/M field
    uint8_t reg : 3;                // Reg field
    uint8_t mod : 2;                // Mod field
} mmm_modrm_t;

// SIB byte structure
typedef struct {
    uint8_t base : 3;               // Base field
    uint8_t index : 3;              // Index field
    uint8_t scale : 2;              // Scale field
} mmm_sib_t;

// Complete x86 instruction structure
typedef struct {
    // Prefixes
    uint8_t prefixes[4];            // Up to 4 prefixes
    uint8_t prefix_count;
    
    // Opcode
    uint8_t opcode[3];              // Up to 3-byte opcode
    uint8_t opcode_length;
    
    // ModR/M and SIB
    mmm_modrm_t modrm;
    bool has_modrm;
    mmm_sib_t sib;
    bool has_sib;
    
    // Displacement and immediate
    int32_t displacement;
    uint8_t displacement_size;
    uint64_t immediate;
    uint8_t immediate_size;
    
    // Instruction properties
    mmm_instruction_category_t category;
    mmm_addressing_mode_t addressing;
    uint8_t total_length;
} mmm_x86_instruction_t;

// ===================================================================
// Advanced SIMD/AVX Operations
// ===================================================================

// SIMD operation types
typedef enum {
    MMM_SIMD_OP_LOAD = 0,           // Load operations
    MMM_SIMD_OP_STORE,              // Store operations
    MMM_SIMD_OP_ARITHMETIC,         // Arithmetic operations
    MMM_SIMD_OP_LOGICAL,            // Logical operations
    MMM_SIMD_OP_COMPARE,            // Comparison operations
    MMM_SIMD_OP_SHUFFLE,            // Shuffle/permute operations
    MMM_SIMD_OP_CONVERT,            // Type conversion operations
    MMM_SIMD_OP_BLEND               // Blend operations
} mmm_simd_operation_t;

// SIMD data layout
typedef enum {
    MMM_SIMD_LAYOUT_PACKED = 0,     // Packed layout (no gaps)
    MMM_SIMD_LAYOUT_SCALAR,         // Scalar layout (single element)
    MMM_SIMD_LAYOUT_BROADCAST       // Broadcast layout (replicated element)
} mmm_simd_layout_t;

// Advanced SIMD context
typedef struct {
    // Available instruction sets
    mmm_simd_features_t features;
    
    // Register state
    mmm_simd_register_t registers[32]; // XMM0-XMM31 (AVX-512)
    uint32_t register_mask;         // Mask of used registers
    
    // Control state
    uint32_t mxcsr;                 // SIMD control and status register
    bool ftz;                       // Flush-to-zero mode
    bool daz;                       // Denormals-are-zero mode
    
    // Optimization state
    mmm_simd_hints_t hints;
    uint32_t preferred_width;       // Preferred vector width
    bool unaligned_penalty;         // Unaligned access penalty
} mmm_simd_context_t;

// ===================================================================
// Atomic Operations and Memory Ordering
// ===================================================================

// Extended atomic operations
typedef enum {
    MMM_ATOMIC_BIT_TEST_SET = 0,    // Bit test and set
    MMM_ATOMIC_BIT_TEST_RESET,      // Bit test and reset
    MMM_ATOMIC_BIT_TEST_COMPLEMENT, // Bit test and complement
    MMM_ATOMIC_DOUBLE_CAS,          // Double-width compare-and-swap
    MMM_ATOMIC_FETCH_MIN,           // Fetch minimum
    MMM_ATOMIC_FETCH_MAX            // Fetch maximum
} mmm_atomic_extended_op_t;

// Memory barrier types
typedef enum {
    MMM_BARRIER_COMPILER = 0,       // Compiler barrier only
    MMM_BARRIER_CPU,                // CPU memory barrier
    MMM_BARRIER_FULL                // Full system barrier
} mmm_barrier_type_t;

// Lock-free data structure support
typedef struct {
    volatile uint64_t value;        // Atomic value
    uint32_t version;               // Version counter for ABA prevention
} mmm_atomic_versioned_t;

// ===================================================================
// Advanced Interrupt Handling
// ===================================================================

// Extended interrupt types
typedef enum {
    MMM_INT_HARDWARE = 0,           // Hardware interrupt
    MMM_INT_SOFTWARE,               // Software interrupt (INT instruction)
    MMM_INT_EXCEPTION,              // CPU exception
    MMM_INT_NMI,                    // Non-maskable interrupt
    MMM_INT_SMI                     // System management interrupt
} mmm_interrupt_type_t;

// Interrupt priority levels
typedef enum {
    MMM_INT_PRIORITY_LOW = 0,       // Low priority (15-8)
    MMM_INT_PRIORITY_NORMAL,        // Normal priority (7-4)
    MMM_INT_PRIORITY_HIGH,          // High priority (3-1)
    MMM_INT_PRIORITY_CRITICAL       // Critical priority (0, NMI)
} mmm_interrupt_priority_t;

// Extended interrupt context
typedef struct {
    mmm_interrupt_context_t base;   // Base interrupt context
    
    // Extended state
    mmm_simd_context_t simd_state;  // SIMD register state
    uint64_t debug_registers[8];    // Debug registers DR0-DR7
    uint64_t control_registers[8];  // Control registers CR0-CR7
    
    // Interrupt information
    mmm_interrupt_type_t int_type;
    mmm_interrupt_priority_t priority;
    uint64_t timestamp;             // Interrupt timestamp
    uint32_t cpu_id;                // CPU that handled interrupt
} mmm_extended_interrupt_context_t;

// ===================================================================
// Performance Monitoring and Profiling
// ===================================================================

// Performance counter types
typedef enum {
    MMM_PERF_INSTRUCTIONS = 0,      // Instructions retired
    MMM_PERF_CYCLES,                // CPU cycles
    MMM_PERF_CACHE_REFERENCES,      // Cache references
    MMM_PERF_CACHE_MISSES,          // Cache misses
    MMM_PERF_BRANCH_INSTRUCTIONS,   // Branch instructions
    MMM_PERF_BRANCH_MISSES,         // Branch mispredictions
    MMM_PERF_TLB_REFERENCES,        // TLB references
    MMM_PERF_TLB_MISSES,            // TLB misses
    MMM_PERF_MEMORY_LOADS,          // Memory load operations
    MMM_PERF_MEMORY_STORES          // Memory store operations
} mmm_performance_counter_t;

// Performance monitoring unit (PMU) state
typedef struct {
    uint64_t counters[16];          // Hardware performance counters
    uint64_t counter_configs[16];   // Counter configuration
    bool counters_enabled[16];      // Counter enable flags
    uint64_t global_ctrl;           // Global control register
    uint64_t fixed_counters[3];     // Fixed-function counters
} mmm_pmu_state_t;

// ===================================================================
// Function Declarations
// ===================================================================

// Advanced x86 instruction support
bool mmm_decode_x86_instruction(const void *bytes, size_t length, 
                               mmm_x86_instruction_t *instruction);
bool mmm_encode_x86_instruction(const mmm_x86_instruction_t *instruction,
                               void *output, size_t *output_length);
bool mmm_validate_x86_instruction(const mmm_x86_instruction_t *instruction);

// SIMD/AVX operations
bool mmm_simd_initialize(mmm_simd_context_t *ctx, mmm_simd_features_t features);
bool mmm_simd_execute_operation(mmm_simd_context_t *ctx,
                               mmm_simd_operation_t operation,
                               const mmm_simd_register_t *src1,
                               const mmm_simd_register_t *src2,
                               mmm_simd_register_t *dst);
bool mmm_simd_optimize_sequence(mmm_simd_context_t *ctx,
                               const mmm_x86_instruction_t *instructions,
                               size_t count,
                               mmm_x86_instruction_t **optimized,
                               size_t *optimized_count);

// Extended atomic operations
bool mmm_atomic_extended_operation(mmm_atomic_extended_op_t operation,
                                  void *address,
                                  void *value,
                                  void *expected,
                                  mmm_memory_order_t order);
void mmm_memory_barrier(mmm_barrier_type_t type);
bool mmm_atomic_versioned_cas(mmm_atomic_versioned_t *target,
                             uint64_t expected_value,
                             uint32_t expected_version,
                             uint64_t new_value);

// Advanced interrupt handling
bool mmm_install_extended_interrupt_handler(mmm_master_memory_manager_t *mmm,
                                           uint8_t vector,
                                           void (*handler)(mmm_extended_interrupt_context_t *ctx),
                                           mmm_interrupt_type_t type,
                                           mmm_interrupt_priority_t priority);
bool mmm_setup_interrupt_stack_table(mmm_master_memory_manager_t *mmm,
                                     uint8_t ist_index,
                                     void *stack,
                                     size_t stack_size);

// Performance monitoring
bool mmm_pmu_initialize(mmm_pmu_state_t *pmu);
bool mmm_pmu_configure_counter(mmm_pmu_state_t *pmu,
                              uint8_t counter_index,
                              mmm_performance_counter_t counter_type,
                              uint64_t config);
bool mmm_pmu_start_counting(mmm_pmu_state_t *pmu);
bool mmm_pmu_stop_counting(mmm_pmu_state_t *pmu);
uint64_t mmm_pmu_read_counter(mmm_pmu_state_t *pmu, uint8_t counter_index);

// Utility functions
const char* mmm_instruction_category_name(mmm_instruction_category_t category);
const char* mmm_simd_feature_name(mmm_simd_features_t feature);
const char* mmm_atomic_operation_name(mmm_atomic_op_t operation);

#ifdef __cplusplus
}
#endif

#endif // X86_ADVANCED_H
