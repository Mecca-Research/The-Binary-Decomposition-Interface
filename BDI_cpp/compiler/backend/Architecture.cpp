
#include "Architecture.hpp"
#include <stdexcept>
#include <memory>

namespace bdi::compiler::backend {

// X86_64Architecture implementation
X86_64Architecture::X86_64Architecture() {
    // Initialize x86-64 registers
    registers_ = {
        RegisterInfo(0, "rax", false, true),
        RegisterInfo(1, "rcx", false, true),
        RegisterInfo(2, "rdx", false, true),
        RegisterInfo(3, "rbx", true, true),
        RegisterInfo(4, "rsp", true, false),  // Stack pointer - not allocatable
        RegisterInfo(5, "rbp", true, true),
        RegisterInfo(6, "rsi", false, true),
        RegisterInfo(7, "rdi", false, true),
        RegisterInfo(8, "r8", false, true),
        RegisterInfo(9, "r9", false, true),
        RegisterInfo(10, "r10", false, true),
        RegisterInfo(11, "r11", false, true),
        RegisterInfo(12, "r12", true, true),
        RegisterInfo(13, "r13", true, true),
        RegisterInfo(14, "r14", true, true),
        RegisterInfo(15, "r15", true, true),
    };
}

std::vector<uint32_t> X86_64Architecture::getArgumentRegisters() const {
    // System V AMD64 ABI calling convention
    return {7, 6, 2, 1, 8, 9}; // rdi, rsi, rdx, rcx, r8, r9
}

std::vector<uint32_t> X86_64Architecture::getReturnRegisters() const {
    return {0, 2}; // rax, rdx
}

std::vector<uint32_t> X86_64Architecture::getCallerSavedRegisters() const {
    return {0, 1, 2, 6, 7, 8, 9, 10, 11}; // rax, rcx, rdx, rsi, rdi, r8-r11
}

std::vector<uint32_t> X86_64Architecture::getCalleeSavedRegisters() const {
    return {3, 5, 12, 13, 14, 15}; // rbx, rbp, r12-r15
}

// ARM64Architecture implementation
ARM64Architecture::ARM64Architecture() {
    // Initialize ARM64 registers (x0-x30)
    for (uint32_t i = 0; i < 31; ++i) {
        bool is_callee_saved = (i >= 19 && i <= 28);
        bool is_allocatable = (i != 18 && i != 29 && i != 30); // x18, x29(fp), x30(lr) not allocatable
        registers_.emplace_back(i, "x" + std::to_string(i), is_callee_saved, is_allocatable);
    }
}

std::vector<uint32_t> ARM64Architecture::getArgumentRegisters() const {
    // ARM64 calling convention
    return {0, 1, 2, 3, 4, 5, 6, 7}; // x0-x7
}

std::vector<uint32_t> ARM64Architecture::getReturnRegisters() const {
    return {0, 1}; // x0, x1
}

std::vector<uint32_t> ARM64Architecture::getCallerSavedRegisters() const {
    std::vector<uint32_t> regs;
    for (uint32_t i = 0; i <= 18; ++i) {
        regs.push_back(i);
    }
    return regs; // x0-x18
}

std::vector<uint32_t> ARM64Architecture::getCalleeSavedRegisters() const {
    std::vector<uint32_t> regs;
    for (uint32_t i = 19; i <= 28; ++i) {
        regs.push_back(i);
    }
    return regs; // x19-x28
}

// ArchitectureFactory implementation
std::unique_ptr<Architecture> ArchitectureFactory::create(ArchType type) {
    switch (type) {
        case ArchType::X86_64:
            return std::make_unique<X86_64Architecture>();
        case ArchType::ARM64:
            return std::make_unique<ARM64Architecture>();
        default:
            throw std::runtime_error("Unsupported architecture type");
    }
}

std::unique_ptr<Architecture> ArchitectureFactory::createNative() {
#if defined(__x86_64__) || defined(_M_X64)
    return create(ArchType::X86_64);
#elif defined(__aarch64__) || defined(_M_ARM64)
    return create(ArchType::ARM64);
#else
    return create(ArchType::X86_64); // Default to x86-64
#endif
}

} // namespace bdi::compiler::backend
