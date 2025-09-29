
/**
 * @file x86_registers.h
 * @brief x86 Register Management Interface
 * 
 * Provides comprehensive x86 register management including:
 * - 32-bit and 64-bit register mapping
 * - Register allocation and context switching
 * - Special purpose register handling
 * - Register state preservation
 * 
 * Based on technical foundation from Assembly Language for x86 Processors 7th Edition
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef X86_REGISTERS_H
#define X86_REGISTERS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// REGISTER DEFINITIONS
// =============================================================================

/**
 * @brief x86 32-bit general purpose registers
 */
typedef enum {
    X86_REG_EAX = 0,    ///< Accumulator register
    X86_REG_EBX,        ///< Base register
    X86_REG_ECX,        ///< Counter register
    X86_REG_EDX,        ///< Data register
    X86_REG_ESI,        ///< Source index register
    X86_REG_EDI,        ///< Destination index register
    X86_REG_ESP,        ///< Stack pointer register
    X86_REG_EBP,        ///< Base pointer register
    X86_REG_COUNT_32    ///< Total count of 32-bit registers
} x86_reg32_t;

/**
 * @brief x86 64-bit general purpose registers
 */
typedef enum {
    X86_REG_RAX = 0,    ///< 64-bit accumulator register
    X86_REG_RBX,        ///< 64-bit base register
    X86_REG_RCX,        ///< 64-bit counter register
    X86_REG_RDX,        ///< 64-bit data register
    X86_REG_RSI,        ///< 64-bit source index register
    X86_REG_RDI,        ///< 64-bit destination index register
    X86_REG_RSP,        ///< 64-bit stack pointer register
    X86_REG_RBP,        ///< 64-bit base pointer register
    X86_REG_R8,         ///< Additional 64-bit register R8
    X86_REG_R9,         ///< Additional 64-bit register R9
    X86_REG_R10,        ///< Additional 64-bit register R10
    X86_REG_R11,        ///< Additional 64-bit register R11
    X86_REG_R12,        ///< Additional 64-bit register R12
    X86_REG_R13,        ///< Additional 64-bit register R13
    X86_REG_R14,        ///< Additional 64-bit register R14
    X86_REG_R15,        ///< Additional 64-bit register R15
    X86_REG_COUNT_64    ///< Total count of 64-bit registers
} x86_reg64_t;

/**
 * @brief x86 segment registers
 */
typedef enum {
    X86_SEG_CS = 0,     ///< Code segment
    X86_SEG_DS,         ///< Data segment
    X86_SEG_ES,         ///< Extra segment
    X86_SEG_FS,         ///< Additional segment F
    X86_SEG_GS,         ///< Additional segment G
    X86_SEG_SS,         ///< Stack segment
    X86_SEG_COUNT       ///< Total count of segment registers
} x86_seg_reg_t;

/**
 * @brief EFLAGS register bits
 */
typedef enum {
    X86_FLAG_CF = 0,    ///< Carry Flag
    X86_FLAG_PF = 2,    ///< Parity Flag
    X86_FLAG_AF = 4,    ///< Auxiliary Carry Flag
    X86_FLAG_ZF = 6,    ///< Zero Flag
    X86_FLAG_SF = 7,    ///< Sign Flag
    X86_FLAG_TF = 8,    ///< Trap Flag
    X86_FLAG_IF = 9,    ///< Interrupt Enable Flag
    X86_FLAG_DF = 10,   ///< Direction Flag
    X86_FLAG_OF = 11,   ///< Overflow Flag
    X86_FLAG_IOPL = 12, ///< I/O Privilege Level (2 bits)
    X86_FLAG_NT = 14,   ///< Nested Task Flag
    X86_FLAG_RF = 16,   ///< Resume Flag
    X86_FLAG_VM = 17,   ///< Virtual 8086 Mode Flag
    X86_FLAG_AC = 18,   ///< Alignment Check Flag
    X86_FLAG_VIF = 19,  ///< Virtual Interrupt Flag
    X86_FLAG_VIP = 20,  ///< Virtual Interrupt Pending Flag
    X86_FLAG_ID = 21    ///< ID Flag
} x86_eflags_bit_t;

/**
 * @brief Register context structure for 32-bit mode
 */
typedef struct {
    uint32_t eax, ebx, ecx, edx;    ///< General purpose registers
    uint32_t esi, edi, esp, ebp;    ///< Index and pointer registers
    uint32_t eip;                   ///< Instruction pointer
    uint32_t eflags;                ///< Flags register
    uint16_t cs, ds, es, fs, gs, ss; ///< Segment registers
} x86_context32_t;

/**
 * @brief Register context structure for 64-bit mode
 */
typedef struct {
    uint64_t rax, rbx, rcx, rdx;    ///< General purpose registers
    uint64_t rsi, rdi, rsp, rbp;    ///< Index and pointer registers
    uint64_t r8, r9, r10, r11;      ///< Additional registers
    uint64_t r12, r13, r14, r15;    ///< Additional registers
    uint64_t rip;                   ///< Instruction pointer
    uint64_t rflags;                ///< Flags register
    uint16_t cs, ds, es, fs, gs, ss; ///< Segment registers
} x86_context64_t;

/**
 * @brief Register allocation status
 */
typedef enum {
    X86_REG_FREE = 0,       ///< Register is available
    X86_REG_ALLOCATED,      ///< Register is allocated
    X86_REG_RESERVED,       ///< Register is reserved (ESP, EBP)
    X86_REG_VOLATILE,       ///< Register is volatile (caller-saved)
    X86_REG_NON_VOLATILE    ///< Register is non-volatile (callee-saved)
} x86_reg_status_t;

/**
 * @brief Register allocation table entry
 */
typedef struct {
    x86_reg_status_t status;    ///< Current allocation status
    uint32_t owner_id;          ///< ID of the owner (function, thread, etc.)
    uint64_t timestamp;         ///< Allocation timestamp
    const char *description;    ///< Description of usage
} x86_reg_alloc_entry_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize x86 register management system
 * @return Status code
 */
int x86_registers_initialize(void);

/**
 * @brief Shutdown x86 register management system
 * @return Status code
 */
int x86_registers_shutdown(void);

/**
 * @brief Save current register context (32-bit)
 * @param context Pointer to context structure to fill
 * @return Status code
 */
int x86_save_context32(x86_context32_t *context);

/**
 * @brief Restore register context (32-bit)
 * @param context Pointer to context structure to restore
 * @return Status code
 */
int x86_restore_context32(const x86_context32_t *context);

/**
 * @brief Save current register context (64-bit)
 * @param context Pointer to context structure to fill
 * @return Status code
 */
int x86_save_context64(x86_context64_t *context);

/**
 * @brief Restore register context (64-bit)
 * @param context Pointer to context structure to restore
 * @return Status code
 */
int x86_restore_context64(const x86_context64_t *context);

/**
 * @brief Allocate a general purpose register
 * @param reg_type Preferred register type (32-bit or 64-bit)
 * @param owner_id Owner identifier
 * @param description Usage description
 * @return Allocated register ID or negative error code
 */
int x86_allocate_register(int reg_type, uint32_t owner_id, const char *description);

/**
 * @brief Free an allocated register
 * @param reg_id Register ID to free
 * @param owner_id Owner identifier (must match allocation)
 * @return Status code
 */
int x86_free_register(int reg_id, uint32_t owner_id);

/**
 * @brief Get register allocation status
 * @param reg_id Register ID
 * @return Pointer to allocation entry or NULL if invalid
 */
const x86_reg_alloc_entry_t *x86_get_register_status(int reg_id);

/**
 * @brief Check if register is volatile (caller-saved)
 * @param reg_id Register ID
 * @return true if volatile, false otherwise
 */
bool x86_is_register_volatile(int reg_id);

/**
 * @brief Get register name string
 * @param reg_id Register ID
 * @param is_64bit true for 64-bit names, false for 32-bit
 * @return Register name string or NULL if invalid
 */
const char *x86_get_register_name(int reg_id, bool is_64bit);

/**
 * @brief Set EFLAGS bit
 * @param bit Flag bit to set
 * @return Status code
 */
static inline int x86_set_flag(x86_eflags_bit_t bit)
{
    // Implementation would use inline assembly
    // This is a placeholder for the interface
    (void)bit;
    return 0;
}

/**
 * @brief Clear EFLAGS bit
 * @param bit Flag bit to clear
 * @return Status code
 */
static inline int x86_clear_flag(x86_eflags_bit_t bit)
{
    // Implementation would use inline assembly
    // This is a placeholder for the interface
    (void)bit;
    return 0;
}

/**
 * @brief Test EFLAGS bit
 * @param bit Flag bit to test
 * @return true if set, false if clear
 */
static inline bool x86_test_flag(x86_eflags_bit_t bit)
{
    // Implementation would use inline assembly
    // This is a placeholder for the interface
    (void)bit;
    return false;
}

#ifdef __cplusplus
}
#endif

#endif // X86_REGISTERS_H
