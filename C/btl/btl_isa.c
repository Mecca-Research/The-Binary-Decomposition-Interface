
// BTL ISA Support Implementation
#include "btl_isa.h"
#include <string.h>

// x86-64 instruction table (one-byte opcodes)
static const BTL_InstructionDescriptor x86_64_instructions[256] = {
    {"ADD", 0x00, BTL_CAT_ARITHMETIC, 2, "Add r/m8 to r8"},
    {"ADD", 0x01, BTL_CAT_ARITHMETIC, 2, "Add r/m32 to r32"},
    {"ADD", 0x02, BTL_CAT_ARITHMETIC, 2, "Add r8 to r/m8"},
    {"ADD", 0x03, BTL_CAT_ARITHMETIC, 2, "Add r32 to r/m32"},
    {"ADD", 0x04, BTL_CAT_ARITHMETIC, 2, "Add imm8 to AL"},
    {"ADD", 0x05, BTL_CAT_ARITHMETIC, 2, "Add imm32 to EAX"},
    {"PUSH", 0x06, BTL_CAT_MEMORY, 1, "Push ES"},
    {"POP", 0x07, BTL_CAT_MEMORY, 1, "Pop ES"},
    {"OR", 0x08, BTL_CAT_LOGIC, 2, "OR r/m8 with r8"},
    {"OR", 0x09, BTL_CAT_LOGIC, 2, "OR r/m32 with r32"},
    {"OR", 0x0A, BTL_CAT_LOGIC, 2, "OR r8 with r/m8"},
    {"OR", 0x0B, BTL_CAT_LOGIC, 2, "OR r32 with r/m32"},
    {"OR", 0x0C, BTL_CAT_LOGIC, 2, "OR imm8 with AL"},
    {"OR", 0x0D, BTL_CAT_LOGIC, 2, "OR imm32 with EAX"},
    {"PUSH", 0x0E, BTL_CAT_MEMORY, 1, "Push CS"},
    {"ESCAPE", 0x0F, BTL_CAT_SYSTEM, 0, "Two-byte opcode prefix"},
    {"ADC", 0x10, BTL_CAT_ARITHMETIC, 2, "Add with carry r/m8 to r8"},
    {"ADC", 0x11, BTL_CAT_ARITHMETIC, 2, "Add with carry r/m32 to r32"},
    {"ADC", 0x12, BTL_CAT_ARITHMETIC, 2, "Add with carry r8 to r/m8"},
    {"ADC", 0x13, BTL_CAT_ARITHMETIC, 2, "Add with carry r32 to r/m32"},
    {"ADC", 0x14, BTL_CAT_ARITHMETIC, 2, "Add with carry imm8 to AL"},
    {"ADC", 0x15, BTL_CAT_ARITHMETIC, 2, "Add with carry imm32 to EAX"},
    {"PUSH", 0x16, BTL_CAT_MEMORY, 1, "Push SS"},
    {"POP", 0x17, BTL_CAT_MEMORY, 1, "Pop SS"},
    {"SBB", 0x18, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow r/m8 from r8"},
    {"SBB", 0x19, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow r/m32 from r32"},
    {"SBB", 0x1A, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow r8 from r/m8"},
    {"SBB", 0x1B, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow r32 from r/m32"},
    {"SBB", 0x1C, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow imm8 from AL"},
    {"SBB", 0x1D, BTL_CAT_ARITHMETIC, 2, "Subtract with borrow imm32 from EAX"},
    {"PUSH", 0x1E, BTL_CAT_MEMORY, 1, "Push DS"},
    {"POP", 0x1F, BTL_CAT_MEMORY, 1, "Pop DS"},
    {"AND", 0x20, BTL_CAT_LOGIC, 2, "AND r/m8 with r8"},
    {"AND", 0x21, BTL_CAT_LOGIC, 2, "AND r/m32 with r32"},
    {"AND", 0x22, BTL_CAT_LOGIC, 2, "AND r8 with r/m8"},
    {"AND", 0x23, BTL_CAT_LOGIC, 2, "AND r32 with r/m32"},
    {"AND", 0x24, BTL_CAT_LOGIC, 2, "AND imm8 with AL"},
    {"AND", 0x25, BTL_CAT_LOGIC, 2, "AND imm32 with EAX"},
    {"ES:", 0x26, BTL_CAT_SYSTEM, 0, "ES segment override prefix"},
    {"DAA", 0x27, BTL_CAT_ARITHMETIC, 0, "Decimal adjust AL after addition"},
    {"SUB", 0x28, BTL_CAT_ARITHMETIC, 2, "Subtract r/m8 from r8"},
    {"SUB", 0x29, BTL_CAT_ARITHMETIC, 2, "Subtract r/m32 from r32"},
    {"SUB", 0x2A, BTL_CAT_ARITHMETIC, 2, "Subtract r8 from r/m8"},
    {"SUB", 0x2B, BTL_CAT_ARITHMETIC, 2, "Subtract r32 from r/m32"},
    {"SUB", 0x2C, BTL_CAT_ARITHMETIC, 2, "Subtract imm8 from AL"},
    {"SUB", 0x2D, BTL_CAT_ARITHMETIC, 2, "Subtract imm32 from EAX"},
    {"CS:", 0x2E, BTL_CAT_SYSTEM, 0, "CS segment override prefix"},
    {"DAS", 0x2F, BTL_CAT_ARITHMETIC, 0, "Decimal adjust AL after subtraction"},
    {"XOR", 0x30, BTL_CAT_LOGIC, 2, "XOR r/m8 with r8"},
    {"XOR", 0x31, BTL_CAT_LOGIC, 2, "XOR r/m32 with r32"},
    {"XOR", 0x32, BTL_CAT_LOGIC, 2, "XOR r8 with r/m8"},
    {"XOR", 0x33, BTL_CAT_LOGIC, 2, "XOR r32 with r/m32"},
    {"XOR", 0x34, BTL_CAT_LOGIC, 2, "XOR imm8 with AL"},
    {"XOR", 0x35, BTL_CAT_LOGIC, 2, "XOR imm32 with EAX"},
    {"SS:", 0x36, BTL_CAT_SYSTEM, 0, "SS segment override prefix"},
    {"AAA", 0x37, BTL_CAT_ARITHMETIC, 0, "ASCII adjust after addition"},
    {"CMP", 0x38, BTL_CAT_ARITHMETIC, 2, "Compare r/m8 with r8"},
    {"CMP", 0x39, BTL_CAT_ARITHMETIC, 2, "Compare r/m32 with r32"},
    {"CMP", 0x3A, BTL_CAT_ARITHMETIC, 2, "Compare r8 with r/m8"},
    {"CMP", 0x3B, BTL_CAT_ARITHMETIC, 2, "Compare r32 with r/m32"},
    {"CMP", 0x3C, BTL_CAT_ARITHMETIC, 2, "Compare imm8 with AL"},
    {"CMP", 0x3D, BTL_CAT_ARITHMETIC, 2, "Compare imm32 with EAX"},
    {"DS:", 0x3E, BTL_CAT_SYSTEM, 0, "DS segment override prefix"},
    {"AAS", 0x3F, BTL_CAT_ARITHMETIC, 0, "ASCII adjust after subtraction"},
    // REX prefixes (0x40-0x4F) in 64-bit mode
    {"REX", 0x40, BTL_CAT_SYSTEM, 0, "REX prefix"},
    {"REX.B", 0x41, BTL_CAT_SYSTEM, 0, "REX prefix with B bit"},
    {"REX.X", 0x42, BTL_CAT_SYSTEM, 0, "REX prefix with X bit"},
    {"REX.XB", 0x43, BTL_CAT_SYSTEM, 0, "REX prefix with X and B bits"},
    {"REX.R", 0x44, BTL_CAT_SYSTEM, 0, "REX prefix with R bit"},
    {"REX.RB", 0x45, BTL_CAT_SYSTEM, 0, "REX prefix with R and B bits"},
    {"REX.RX", 0x46, BTL_CAT_SYSTEM, 0, "REX prefix with R and X bits"},
    {"REX.RXB", 0x47, BTL_CAT_SYSTEM, 0, "REX prefix with R, X, and B bits"},
    {"REX.W", 0x48, BTL_CAT_SYSTEM, 0, "REX prefix with W bit (64-bit operand)"},
    {"REX.WB", 0x49, BTL_CAT_SYSTEM, 0, "REX prefix with W and B bits"},
    {"REX.WX", 0x4A, BTL_CAT_SYSTEM, 0, "REX prefix with W and X bits"},
    {"REX.WXB", 0x4B, BTL_CAT_SYSTEM, 0, "REX prefix with W, X, and B bits"},
    {"REX.WR", 0x4C, BTL_CAT_SYSTEM, 0, "REX prefix with W and R bits"},
    {"REX.WRB", 0x4D, BTL_CAT_SYSTEM, 0, "REX prefix with W, R, and B bits"},
    {"REX.WRX", 0x4E, BTL_CAT_SYSTEM, 0, "REX prefix with W, R, and X bits"},
    {"REX.WRXB", 0x4F, BTL_CAT_SYSTEM, 0, "REX prefix with all bits"},
    {"PUSH", 0x50, BTL_CAT_MEMORY, 1, "Push RAX/EAX"},
    {"PUSH", 0x51, BTL_CAT_MEMORY, 1, "Push RCX/ECX"},
    {"PUSH", 0x52, BTL_CAT_MEMORY, 1, "Push RDX/EDX"},
    {"PUSH", 0x53, BTL_CAT_MEMORY, 1, "Push RBX/EBX"},
    {"PUSH", 0x54, BTL_CAT_MEMORY, 1, "Push RSP/ESP"},
    {"PUSH", 0x55, BTL_CAT_MEMORY, 1, "Push RBP/EBP"},
    {"PUSH", 0x56, BTL_CAT_MEMORY, 1, "Push RSI/ESI"},
    {"PUSH", 0x57, BTL_CAT_MEMORY, 1, "Push RDI/EDI"},
    {"POP", 0x58, BTL_CAT_MEMORY, 1, "Pop RAX/EAX"},
    {"POP", 0x59, BTL_CAT_MEMORY, 1, "Pop RCX/ECX"},
    {"POP", 0x5A, BTL_CAT_MEMORY, 1, "Pop RDX/EDX"},
    {"POP", 0x5B, BTL_CAT_MEMORY, 1, "Pop RBX/EBX"},
    {"POP", 0x5C, BTL_CAT_MEMORY, 1, "Pop RSP/ESP"},
    {"POP", 0x5D, BTL_CAT_MEMORY, 1, "Pop RBP/EBP"},
    {"POP", 0x5E, BTL_CAT_MEMORY, 1, "Pop RSI/ESI"},
    {"POP", 0x5F, BTL_CAT_MEMORY, 1, "Pop RDI/EDI"},
    // Continue with more opcodes...
    {"JO", 0x70, BTL_CAT_CONTROL, 1, "Jump if overflow"},
    {"JNO", 0x71, BTL_CAT_CONTROL, 1, "Jump if not overflow"},
    {"JB", 0x72, BTL_CAT_CONTROL, 1, "Jump if below/carry"},
    {"JAE", 0x73, BTL_CAT_CONTROL, 1, "Jump if above or equal/not carry"},
    {"JE", 0x74, BTL_CAT_CONTROL, 1, "Jump if equal/zero"},
    {"JNE", 0x75, BTL_CAT_CONTROL, 1, "Jump if not equal/not zero"},
    {"JBE", 0x76, BTL_CAT_CONTROL, 1, "Jump if below or equal"},
    {"JA", 0x77, BTL_CAT_CONTROL, 1, "Jump if above"},
    {"JS", 0x78, BTL_CAT_CONTROL, 1, "Jump if sign"},
    {"JNS", 0x79, BTL_CAT_CONTROL, 1, "Jump if not sign"},
    {"JP", 0x7A, BTL_CAT_CONTROL, 1, "Jump if parity/parity even"},
    {"JNP", 0x7B, BTL_CAT_CONTROL, 1, "Jump if not parity/parity odd"},
    {"JL", 0x7C, BTL_CAT_CONTROL, 1, "Jump if less"},
    {"JGE", 0x7D, BTL_CAT_CONTROL, 1, "Jump if greater or equal"},
    {"JLE", 0x7E, BTL_CAT_CONTROL, 1, "Jump if less or equal"},
    {"JG", 0x7F, BTL_CAT_CONTROL, 1, "Jump if greater"},
    {"MOV", 0x88, BTL_CAT_MEMORY, 2, "Move r8 to r/m8"},
    {"MOV", 0x89, BTL_CAT_MEMORY, 2, "Move r32 to r/m32"},
    {"MOV", 0x8A, BTL_CAT_MEMORY, 2, "Move r/m8 to r8"},
    {"MOV", 0x8B, BTL_CAT_MEMORY, 2, "Move r/m32 to r32"},
    {"LEA", 0x8D, BTL_CAT_MEMORY, 2, "Load effective address"},
    {"NOP", 0x90, BTL_CAT_SYSTEM, 0, "No operation"},
    {"XCHG", 0x91, BTL_CAT_MEMORY, 2, "Exchange RAX with RCX"},
    {"XCHG", 0x92, BTL_CAT_MEMORY, 2, "Exchange RAX with RDX"},
    {"XCHG", 0x93, BTL_CAT_MEMORY, 2, "Exchange RAX with RBX"},
    {"XCHG", 0x94, BTL_CAT_MEMORY, 2, "Exchange RAX with RSP"},
    {"XCHG", 0x95, BTL_CAT_MEMORY, 2, "Exchange RAX with RBP"},
    {"XCHG", 0x96, BTL_CAT_MEMORY, 2, "Exchange RAX with RSI"},
    {"XCHG", 0x97, BTL_CAT_MEMORY, 2, "Exchange RAX with RDI"},
    {"MOV", 0xB0, BTL_CAT_MEMORY, 2, "Move imm8 to AL"},
    {"MOV", 0xB1, BTL_CAT_MEMORY, 2, "Move imm8 to CL"},
    {"MOV", 0xB2, BTL_CAT_MEMORY, 2, "Move imm8 to DL"},
    {"MOV", 0xB3, BTL_CAT_MEMORY, 2, "Move imm8 to BL"},
    {"MOV", 0xB4, BTL_CAT_MEMORY, 2, "Move imm8 to AH"},
    {"MOV", 0xB5, BTL_CAT_MEMORY, 2, "Move imm8 to CH"},
    {"MOV", 0xB6, BTL_CAT_MEMORY, 2, "Move imm8 to DH"},
    {"MOV", 0xB7, BTL_CAT_MEMORY, 2, "Move imm8 to BH"},
    {"MOV", 0xB8, BTL_CAT_MEMORY, 2, "Move imm32 to EAX"},
    {"MOV", 0xB9, BTL_CAT_MEMORY, 2, "Move imm32 to ECX"},
    {"MOV", 0xBA, BTL_CAT_MEMORY, 2, "Move imm32 to EDX"},
    {"MOV", 0xBB, BTL_CAT_MEMORY, 2, "Move imm32 to EBX"},
    {"MOV", 0xBC, BTL_CAT_MEMORY, 2, "Move imm32 to ESP"},
    {"MOV", 0xBD, BTL_CAT_MEMORY, 2, "Move imm32 to EBP"},
    {"MOV", 0xBE, BTL_CAT_MEMORY, 2, "Move imm32 to ESI"},
    {"MOV", 0xBF, BTL_CAT_MEMORY, 2, "Move imm32 to EDI"},
    {"RET", 0xC3, BTL_CAT_CONTROL, 0, "Near return"},
    {"CALL", 0xE8, BTL_CAT_CONTROL, 1, "Call near relative"},
    {"JMP", 0xE9, BTL_CAT_CONTROL, 1, "Jump near relative"},
    {"JMP", 0xEB, BTL_CAT_CONTROL, 1, "Jump short relative"},
    {"HLT", 0xF4, BTL_CAT_SYSTEM, 0, "Halt"},
    // Fill remaining with UNKNOWN
};

// x86-64 instruction table is statically initialized
static void init_x86_64_table(void) {
    // Table is already initialized statically
    return;
}

const BTL_InstructionDescriptor* btl_x86_64_get_instruction(uint8_t opcode) {
    init_x86_64_table();
    return &x86_64_instructions[opcode];
}

const char* btl_x86_64_get_mnemonic(uint8_t opcode) {
    return btl_x86_64_get_instruction(opcode)->mnemonic;
}

BTL_InstructionCategory btl_x86_64_get_category(uint8_t opcode) {
    return btl_x86_64_get_instruction(opcode)->category;
}

// ARM64 basic support (simplified)
const BTL_InstructionDescriptor* btl_arm64_get_instruction(uint32_t opcode) {
    static BTL_InstructionDescriptor desc = {"ARM64", 0, BTL_CAT_UNKNOWN, 0, "ARM64 instruction"};
    desc.opcode = opcode;
    return &desc;
}

const char* btl_arm64_get_mnemonic(uint32_t opcode) {
    (void)opcode;
    return "ARM64_INST";
}

BTL_InstructionCategory btl_arm64_get_category(uint32_t opcode) {
    (void)opcode;
    return BTL_CAT_UNKNOWN;
}

// RISC-V basic support (simplified)
const BTL_InstructionDescriptor* btl_riscv_get_instruction(uint32_t opcode) {
    static BTL_InstructionDescriptor desc = {"RISCV", 0, BTL_CAT_UNKNOWN, 0, "RISC-V instruction"};
    desc.opcode = opcode;
    return &desc;
}

const char* btl_riscv_get_mnemonic(uint32_t opcode) {
    (void)opcode;
    return "RISCV_INST";
}

BTL_InstructionCategory btl_riscv_get_category(uint32_t opcode) {
    (void)opcode;
    return BTL_CAT_UNKNOWN;
}

// Architecture detection
BTL_Architecture btl_detect_architecture(void) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
    return BTL_ARCH_X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return BTL_ARCH_ARM64;
#elif defined(__riscv)
    return BTL_ARCH_RISCV;
#else
    return BTL_ARCH_UNKNOWN;
#endif
}

const char* btl_architecture_name(BTL_Architecture arch) {
    switch (arch) {
        case BTL_ARCH_X86_64: return "x86-64";
        case BTL_ARCH_ARM64: return "ARM64";
        case BTL_ARCH_RISCV: return "RISC-V";
        default: return "Unknown";
    }
}
