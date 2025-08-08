#ifndef BDI_HAL_X86_64_HAL_HPP 
#define BDI_HAL_X86_64_HAL_HPP 
#include "HardwareAbstractionLayer.hpp" 
namespace bdi::hal { 
class X86_64_HAL : public HardwareAbstractionLayer { 
public: 
    X86_64_HAL() = default; 
virtual ~X86_64_HAL() override = default;
 // Delete copy/move constructors/assignment operators 
    X86_64_HAL(const X86_64_HAL&) = delete; 
    X86_64_HAL& operator=(const X86_64_HAL&) = delete; 
    X86_64_HAL(X86_64_HAL&&) = delete; 
    X86_64_HAL& operator=(X86_64_HAL&&) = delete; 
uint64_t readSpecialRegister(SpecialRegister reg) override; 
void writeSpecialRegister(SpecialRegister reg, uint64_t value) override; 
bool readPhysical(uint64_t physical_address, std::byte* buffer, size_t size_bytes) override; 
bool writePhysical(uint64_t physical_address, const std::byte* buffer, size_t size_bytes) override; 
std::optional<uintptr_t> mapPhysicalMemory(uint64_t physical_address, size_t size_bytes, uint64_t flags) override; 
bool unmapMemory(uintptr_t virtual_address, size_t size_bytes) override; 
void enableInterrupts() override; 
void disableInterrupts() override; 
void acknowledgeInterrupt(uint32_t vector) override; // Requires APIC interaction 
void waitForInterrupt() override; 
void systemHalt() override; 
void debugBreak() override; 
}; 
} // namespace bdi::hal 
#endif // BDI_HAL_X86_64_HAL_HPP
