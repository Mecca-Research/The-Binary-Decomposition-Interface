
#include "CodeEmitter.hpp"
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace bdi::compiler::backend {

// CodeBuffer implementation
CodeBuffer::CodeBuffer(size_t initial_capacity)
    : code_(nullptr), size_(0), capacity_(initial_capacity) {
    code_ = new uint8_t[capacity_];
}

CodeBuffer::~CodeBuffer() {
    delete[] code_;
}

void CodeBuffer::emit8(uint8_t byte) {
    ensureCapacity(1);
    code_[size_++] = byte;
}

void CodeBuffer::emit16(uint16_t word) {
    ensureCapacity(2);
    std::memcpy(code_ + size_, &word, 2);
    size_ += 2;
}

void CodeBuffer::emit32(uint32_t dword) {
    ensureCapacity(4);
    std::memcpy(code_ + size_, &dword, 4);
    size_ += 4;
}

void CodeBuffer::emit64(uint64_t qword) {
    ensureCapacity(8);
    std::memcpy(code_ + size_, &qword, 8);
    size_ += 8;
}

void CodeBuffer::emitBytes(const uint8_t* data, size_t size) {
    ensureCapacity(size);
    std::memcpy(code_ + size_, data, size);
    size_ += size;
}

void CodeBuffer::clear() {
    size_ = 0;
}

void CodeBuffer::reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
        uint8_t* new_code = new uint8_t[new_capacity];
        std::memcpy(new_code, code_, size_);
        delete[] code_;
        code_ = new_code;
        capacity_ = new_capacity;
    }
}

void CodeBuffer::ensureCapacity(size_t additional) {
    if (size_ + additional > capacity_) {
        reserve(capacity_ * 2 + additional);
    }
}

// CodeEmitter implementation
CodeEmitter::CodeEmitter(const Architecture& arch)
    : arch_(arch) {}

std::unique_ptr<CodeBuffer> CodeEmitter::emit(
    const std::vector<MachineInstruction>& instructions,
    const AllocationResult& allocation) {
    
    auto buffer = std::make_unique<CodeBuffer>();
    
    for (const auto& instr : instructions) {
        emitInstruction(instr, *buffer);
    }
    
    return buffer;
}

void CodeEmitter::emitInstruction(const MachineInstruction& instr,
                                  CodeBuffer& buffer) {
    if (arch_.getType() == ArchType::X86_64) {
        emitX86_64(instr, buffer);
    } else if (arch_.getType() == ArchType::ARM64) {
        emitARM64(instr, buffer);
    }
}

void CodeEmitter::emitX86_64(const MachineInstruction& instr, CodeBuffer& buffer) {
    // Simplified x86-64 encoding
    // Real implementation would use proper instruction encoding
    
    if (instr.opcode == "add") {
        // ADD r/m64, r64 (REX.W + 01 /r)
        buffer.emit8(0x48); // REX.W prefix
        buffer.emit8(0x01); // ADD opcode
        // ModR/M byte would go here
        buffer.emit8(0xC0); // Placeholder
    } else if (instr.opcode == "sub") {
        buffer.emit8(0x48); // REX.W prefix
        buffer.emit8(0x29); // SUB opcode
        buffer.emit8(0xC0); // Placeholder
    } else if (instr.opcode == "mov") {
        buffer.emit8(0x48); // REX.W prefix
        buffer.emit8(0x89); // MOV opcode
        buffer.emit8(0xC0); // Placeholder
    } else if (instr.opcode == "ret") {
        buffer.emit8(0xC3); // RET
    }
    // Add more instruction encodings as needed
}

void CodeEmitter::emitARM64(const MachineInstruction& instr, CodeBuffer& buffer) {
    // Simplified ARM64 encoding
    // Real implementation would use proper instruction encoding
    
    if (instr.opcode == "add") {
        // ADD (immediate): sf=1, op=0, S=0, shift=00, imm12, Rn, Rd
        uint32_t encoding = 0x91000000; // Base encoding for ADD
        buffer.emit32(encoding);
    } else if (instr.opcode == "sub") {
        uint32_t encoding = 0xD1000000; // Base encoding for SUB
        buffer.emit32(encoding);
    } else if (instr.opcode == "mov") {
        uint32_t encoding = 0xAA0003E0; // MOV (register)
        buffer.emit32(encoding);
    } else if (instr.opcode == "ret") {
        buffer.emit32(0xD65F03C0); // RET
    }
    // Add more instruction encodings as needed
}

std::string CodeEmitter::generateAssembly(
    const std::vector<MachineInstruction>& instructions,
    const AllocationResult& allocation) {
    
    std::ostringstream os;
    
    os << "; Generated assembly for " << arch_.getName() << "\n";
    os << "; Register allocation:\n";
    for (const auto& [var, reg] : allocation.register_assignment) {
        os << ";   var_" << var << " -> r" << reg << "\n";
    }
    
    if (!allocation.spilled_variables.empty()) {
        os << "; Spilled variables: ";
        for (SsaVariableID var : allocation.spilled_variables) {
            os << "var_" << var << " ";
        }
        os << "\n";
    }
    
    os << "\n";
    
    for (const auto& instr : instructions) {
        os << "  " << instr.toString() << "\n";
    }
    
    return os.str();
}

// CodeGenerator implementation
CodeGenerator::CodeGenerator(const Architecture& arch)
    : arch_(arch) {}

std::unique_ptr<CodeBuffer> CodeGenerator::generate(
    const SsaForm& ssa, const BDIGraph& graph) {
    
    // Phase 1: Instruction selection
    InstructionSelector selector(arch_);
    auto instructions = selector.selectInstructions(ssa, graph);
    
    // Phase 2: Register allocation
    RegisterAllocator allocator(arch_);
    auto allocation = allocator.allocate(ssa);
    
    // Phase 3: Code emission
    CodeEmitter emitter(arch_);
    auto code = emitter.emit(instructions, allocation);
    
    // Generate assembly for debugging
    assembly_ = emitter.generateAssembly(instructions, allocation);
    
    return code;
}

} // namespace bdi::compiler::backend
