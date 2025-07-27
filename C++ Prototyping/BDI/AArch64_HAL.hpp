#ifndef BDI_HAL_AARCH64_HAL_HPP 
#define BDI_HAL_AARCH64_HAL_HPP 
#include "../HardwareAbstractionLayer.hpp" 
namespace bdi::hal { 
class AArch64_HAL : public HardwareAbstractionLayer { 
public: 
    AArch64_HAL() = default; 
virtual ~AArch64_HAL() override = default; 
// Delete copy/move 
    AArch64_HAL(const AArch64_HAL&) = delete; /*...*/ 
uint64_t readSpecialRegister(SpecialRegister reg) override; 
void writeSpecialRegister(SpecialRegister reg, uint64_t value) override; 
// ... Implementations for read/writePhysical, map/unmap, interrupts, halt, etc. ... 
bool readPhysical(uint64_t pa, std::byte* buf, size_t sz) override; 
bool writePhysical(uint64_t pa, const std::byte* buf, size_t sz) override; 
std::optional<uintptr_t> mapPhysicalMemory(uint64_t pa, size_t sz, uint64_t flags) override; 
bool unmapMemory(uintptr_t va, size_t sz) override; 
void enableInterrupts() override; 
void disableInterrupts() override; 
void acknowledgeInterrupt(uint32_t vector) override; // GIC interaction 
void waitForInterrupt() override; 
void systemHalt() override; 
void debugBreak() override; 
}; 
} // namespace bdi::hal 
#endif // BDI_HAL_AARCH64_HAL_HPP
