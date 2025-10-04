
#ifndef BDI_COMPILER_BACKEND_CODEGEN_CODEEMITTER_HPP
#define BDI_COMPILER_BACKEND_CODEGEN_CODEEMITTER_HPP

#include "compiler/backend/codegen/InstructionSelection.hpp"
#include "compiler/backend/regalloc/RegisterAllocator.hpp"
#include "compiler/backend/Architecture.hpp"
#include <vector>
#include <cstdint>
#include <string>

namespace bdi::compiler::backend {

/**
 * @brief Code buffer for emitted machine code
 */
class CodeBuffer {
public:
    CodeBuffer(size_t initial_capacity = 4096);
    ~CodeBuffer();
    
    void emit8(uint8_t byte);
    void emit16(uint16_t word);
    void emit32(uint32_t dword);
    void emit64(uint64_t qword);
    
    void emitBytes(const uint8_t* data, size_t size);
    
    const uint8_t* getCode() const { return code_; }
    size_t getSize() const { return size_; }
    size_t getCapacity() const { return capacity_; }
    
    void clear();
    void reserve(size_t new_capacity);
    
private:
    uint8_t* code_;
    size_t size_;
    size_t capacity_;
    
    void ensureCapacity(size_t additional);
};

/**
 * @brief Code emitter for generating machine code
 * 
 * Takes machine instructions and register allocation,
 * and emits actual binary machine code.
 */
class CodeEmitter {
public:
    explicit CodeEmitter(const Architecture& arch);
    
    /**
     * @brief Emit machine code for instructions
     * @param instructions Machine instructions to emit
     * @param allocation Register allocation result
     * @return Code buffer with emitted machine code
     */
    std::unique_ptr<CodeBuffer> emit(
        const std::vector<MachineInstruction>& instructions,
        const AllocationResult& allocation);
    
    /**
     * @brief Emit a single instruction
     */
    void emitInstruction(const MachineInstruction& instr,
                        CodeBuffer& buffer);
    
    /**
     * @brief Generate assembly text (for debugging)
     */
    std::string generateAssembly(
        const std::vector<MachineInstruction>& instructions,
        const AllocationResult& allocation);
    
private:
    const Architecture& arch_;
    
    // Architecture-specific emission
    void emitX86_64(const MachineInstruction& instr, CodeBuffer& buffer);
    void emitARM64(const MachineInstruction& instr, CodeBuffer& buffer);
};

/**
 * @brief Complete backend code generator
 * 
 * Orchestrates instruction selection, register allocation, and code emission
 */
class CodeGenerator {
public:
    explicit CodeGenerator(const Architecture& arch);
    
    /**
     * @brief Generate machine code from SSA form
     * @param ssa SSA form to compile
     * @param graph Original BDI graph
     * @return Code buffer with machine code
     */
    std::unique_ptr<CodeBuffer> generate(const SsaForm& ssa, const BDIGraph& graph);
    
    /**
     * @brief Get generated assembly for debugging
     */
    std::string getAssembly() const { return assembly_; }
    
private:
    const Architecture& arch_;
    std::string assembly_;
};

} // namespace bdi::compiler::backend

#endif // BDI_COMPILER_BACKEND_CODEGEN_CODEEMITTER_HPP
