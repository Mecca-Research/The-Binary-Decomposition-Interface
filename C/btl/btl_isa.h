
// BTL ISA Support - Multi-architecture instruction set support
#ifndef BTL_ISA_H
#define BTL_ISA_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>

// Supported architectures
typedef enum {
    BTL_ARCH_X86_64,
    BTL_ARCH_ARM64,
    BTL_ARCH_RISCV,
    BTL_ARCH_UNKNOWN
} BTL_Architecture;

// Instruction categories
typedef enum {
    BTL_CAT_ARITHMETIC,
    BTL_CAT_LOGIC,
    BTL_CAT_SHIFT,
    BTL_CAT_MEMORY,
    BTL_CAT_CONTROL,
    BTL_CAT_SIMD,
    BTL_CAT_SYSTEM,
    BTL_CAT_UNKNOWN
} BTL_InstructionCategory;

// Instruction descriptor
typedef struct {
    const char *mnemonic;
    uint32_t opcode;
    BTL_InstructionCategory category;
    uint8_t operand_count;
    const char *description;
} BTL_InstructionDescriptor;

// x86-64 ISA support
const BTL_InstructionDescriptor* btl_x86_64_get_instruction(uint8_t opcode);
const char* btl_x86_64_get_mnemonic(uint8_t opcode);
BTL_InstructionCategory btl_x86_64_get_category(uint8_t opcode);

// ARM64 ISA support
const BTL_InstructionDescriptor* btl_arm64_get_instruction(uint32_t opcode);
const char* btl_arm64_get_mnemonic(uint32_t opcode);
BTL_InstructionCategory btl_arm64_get_category(uint32_t opcode);

// RISC-V ISA support
const BTL_InstructionDescriptor* btl_riscv_get_instruction(uint32_t opcode);
const char* btl_riscv_get_mnemonic(uint32_t opcode);
BTL_InstructionCategory btl_riscv_get_category(uint32_t opcode);

// Architecture detection
BTL_Architecture btl_detect_architecture(void);
const char* btl_architecture_name(BTL_Architecture arch);

#endif // BTL_ISA_H
