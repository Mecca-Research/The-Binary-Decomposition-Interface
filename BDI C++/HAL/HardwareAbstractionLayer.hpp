#ifndef BDI_HAL_HARDWAREABSTRACTIONLAYER_HPP 
#define BDI_HAL_HARDWAREABSTRACTIONLAYER_HPP 
#include "HardwareInfo.hpp" // Include info structures 
#include <cstdint> 
#include <cstddef> // For size_t, std::byte 
#include <optional> 
namespace bdi::hal { 
// Define types for special registers accessible via HAL 
// Corresponds somewhat to BDISpecialRegister, but at HAL level 
enum class SpecialRegister { 
// Core Registers 
    StackPointer,       
// Machine SP 
    FramePointer,       
// Machine FP/BP 
    InstructionPointer, // Machine IP/PC 
    FlagsRegister,      
// Machine FLAGS/PSR 
// System Control / Info 
    CoreID,             
// Current logical/physical CPU core ID 
    TimerCurrentValue,  // Read current value of a system timer 
    TimerFrequency,     
// Get frequency of the system timer 
    InterruptMask,      
// Read/Write Interrupt Mask Register (IMR) 
// MMU/Paging (if BDIOS manages paging, highly complex) 
    PageTableBase,      
// e.g., CR3 on x86 
// Add others as needed by target architecture & SYS ops 
}; 
/** 
 * @brief Abstract interface for low-level hardware access needed by BDIVM SYS_* ops. 
 * This layer hides specific hardware details (MMIO addresses, instruction sequences). 
 * An instance specific to the target architecture (x86_64, AArch64, RISC-V) must be provided to the BDIVM. 
 */ 
class HardwareAbstractionLayer { 
public: 
virtual ~HardwareAbstractionLayer() = default; 
/** @brief Reads a special purpose hardware register. */ 
virtual uint64_t readSpecialRegister(SpecialRegister reg) = 0; 
/** @brief Writes to a special purpose hardware register. */ 
virtual void writeSpecialRegister(SpecialRegister reg, uint64_t value) = 0; 
/** @brief Reads from physical memory or Memory-Mapped IO (MMIO). */ 
virtual bool readPhysical(uint64_t physical_address, std::byte* buffer, size_t size_bytes) = 0; 
/** @brief Writes to physical memory or Memory-Mapped IO (MMIO). */ 
virtual bool writePhysical(uint64_t physical_address, const std::byte* buffer, size_t size_bytes) = 0; 
/** 
     * @brief Maps a physical memory range into the virtual address space (if applicable). 
     * BDIOS might manage its own simple address space on top of physical, or use HW MMU. 
     * This function might interact with an MMU or simply return the physical address 
     * if running in identity-mapped mode initially. Needs careful design. 
* Returns the *virtual* address BDI should use. 
     */ 
virtual std::optional<uintptr_t> mapPhysicalMemory(uint64_t physical_address, size_t size_bytes, uint64_t flags /* e.g., cacheable, writab
 /** @brief Unmaps a previously mapped region. */ 
virtual bool unmapMemory(uintptr_t virtual_address, size_t size_bytes) = 0; 
/** @brief Enables hardware interrupts. */ 
virtual void enableInterrupts() = 0; 
/** @brief Disables hardware interrupts. */ 
virtual void disableInterrupts() = 0; 
/** @brief Acknowledges a specific hardware interrupt vector. */ 
virtual void acknowledgeInterrupt(uint32_t vector) = 0; 
/** @brief Halts the current CPU core until an interrupt occurs. */ 
virtual void waitForInterrupt() = 0; 
/** @brief Halts the entire system. */ 
virtual void systemHalt() = 0; 
/** @brief Causes a debug break (e.g., int3). */ 
virtual void debugBreak() = 0; 
// Add cache control functions (flush, invalidate) if needed 
// Add TLB flush functions if managing paging 
}; 
} // namespace bdi::hal 
#endif // BDI_HAL_HARDWAREABSTRACTIONLAYER_HPP 
