
#ifndef BDI_COMPILER_BACKEND_ARCHITECTURE_HPP
#define BDI_COMPILER_BACKEND_ARCHITECTURE_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace bdi::compiler::backend {

/**
 * @brief Supported target architectures
 */
enum class ArchType {
    X86_64,
    ARM64,
    RISCV64,
    GENERIC
};

/**
 * @brief Register information
 */
struct RegisterInfo {
    uint32_t id;
    std::string name;
    bool is_callee_saved;
    bool is_allocatable;
    
    RegisterInfo(uint32_t id_ = 0, std::string name_ = "", 
                 bool callee_saved = false, bool allocatable = true)
        : id(id_), name(std::move(name_)), 
          is_callee_saved(callee_saved), is_allocatable(allocatable) {}
};

/**
 * @brief Architecture abstraction layer
 * 
 * Provides target-specific information for code generation
 */
class Architecture {
public:
    virtual ~Architecture() = default;
    
    virtual ArchType getType() const = 0;
    virtual std::string getName() const = 0;
    
    virtual uint32_t getPointerSize() const = 0;
    virtual uint32_t getRegisterCount() const = 0;
    virtual const std::vector<RegisterInfo>& getRegisters() const = 0;
    
    virtual uint32_t getStackAlignment() const = 0;
    virtual bool isLittleEndian() const = 0;
    
    // Calling convention
    virtual std::vector<uint32_t> getArgumentRegisters() const = 0;
    virtual std::vector<uint32_t> getReturnRegisters() const = 0;
    virtual std::vector<uint32_t> getCallerSavedRegisters() const = 0;
    virtual std::vector<uint32_t> getCalleeSavedRegisters() const = 0;
};

/**
 * @brief x86-64 architecture
 */
class X86_64Architecture : public Architecture {
public:
    X86_64Architecture();
    
    ArchType getType() const override { return ArchType::X86_64; }
    std::string getName() const override { return "x86-64"; }
    
    uint32_t getPointerSize() const override { return 8; }
    uint32_t getRegisterCount() const override { return 16; }
    const std::vector<RegisterInfo>& getRegisters() const override { return registers_; }
    
    uint32_t getStackAlignment() const override { return 16; }
    bool isLittleEndian() const override { return true; }
    
    std::vector<uint32_t> getArgumentRegisters() const override;
    std::vector<uint32_t> getReturnRegisters() const override;
    std::vector<uint32_t> getCallerSavedRegisters() const override;
    std::vector<uint32_t> getCalleeSavedRegisters() const override;
    
private:
    std::vector<RegisterInfo> registers_;
};

/**
 * @brief ARM64 architecture
 */
class ARM64Architecture : public Architecture {
public:
    ARM64Architecture();
    
    ArchType getType() const override { return ArchType::ARM64; }
    std::string getName() const override { return "ARM64"; }
    
    uint32_t getPointerSize() const override { return 8; }
    uint32_t getRegisterCount() const override { return 31; }
    const std::vector<RegisterInfo>& getRegisters() const override { return registers_; }
    
    uint32_t getStackAlignment() const override { return 16; }
    bool isLittleEndian() const override { return true; }
    
    std::vector<uint32_t> getArgumentRegisters() const override;
    std::vector<uint32_t> getReturnRegisters() const override;
    std::vector<uint32_t> getCallerSavedRegisters() const override;
    std::vector<uint32_t> getCalleeSavedRegisters() const override;
    
private:
    std::vector<RegisterInfo> registers_;
};

/**
 * @brief Factory for creating architecture instances
 */
class ArchitectureFactory {
public:
    static std::unique_ptr<Architecture> create(ArchType type);
    static std::unique_ptr<Architecture> createNative();
};

} // namespace bdi::compiler::backend

#endif // BDI_COMPILER_BACKEND_ARCHITECTURE_HPP
