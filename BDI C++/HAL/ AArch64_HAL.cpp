#include "AArch64_HAL.hpp" 
#include <stdexcept>
#include <iostream> 
#include <cstring> 
// --- Assumed Intrinsics/Assembly Helpers --- 
// These would typically come from <arm_acle.h>, <arm_sve.h> or compiler specifics 
// Or need to be implemented with inline asm blocks. 
#define read_sysreg(reg) /* Read system register, e.g., mrs x0, reg */ 0 
#define write_sysreg(reg, val) /* Write system register, e.g., msr reg, x0 */ (void)val 
#define sev() /* asm volatile ("sev" ::: "memory") */ 
#define wfe() /* asm volatile ("wfe" ::: "memory") */ 
#define wfi() /* asm volatile ("wfi" ::: "memory") */ 
#define isb() /* asm volatile ("isb" ::: "memory") */ 
#define dsb(opt) /* asm volatile ("dsb " #opt ::: "memory") */ // e.g., dsb sy 
#define dmb(opt) /* asm volatile ("dmb " #opt ::: "memory") */ // e.g., dmb sy 
#define enable_irq() /* asm volatile ("msr daifclr, #2" ::: "memory", "cc") */ 
#define disable_irq() /* asm volatile ("msr daifset, #2" ::: "memory", "cc") */ 
#define brk(imm) /* asm volatile ("brk #" #imm ::: "memory") */ 
#define tlbi(op, val) /* TLB Invalidation instruction, e.g., tlbi vmalle1is */ (void)val 
// --- Placeholder for GIC addresses (MUST be found via Device Tree/ACPI) --- 
volatile uint32_t* gicd_base = reinterpret_cast<volatile uint32_t*>(0x08000000); // Distributor Base - EXAMPLE ONLY 
volatile uint32_t* gicc_base = reinterpret_cast<volatile uint32_t*>(0x08010000); // CPU Interface Base - EXAMPLE ONLY 
const uint32_t GICC_EOIR = 0x10 / sizeof(uint32_t); // CPU Interface End Of Interrupt Register offset 
namespace bdi::hal { 
uint64_t AArch64_HAL::readSpecialRegister(SpecialRegister reg) { 
uint64_t value = 0; 
switch(reg) { 
case SpecialRegister::StackPointer:  asm volatile ("mov %0, sp" : "=r" (value)); break; 
case SpecialRegister::FramePointer:  asm volatile ("mov %0, x29" : "=r" (value)); break; 
case SpecialRegister::InstructionPointer: asm volatile ("adr %0, ." : "=r" (value)); break; // Address of this instruction 
case SpecialRegister::FlagsRegister: asm volatile ("mrs %0, nzcv" : "=r" (value)); break; 
case SpecialRegister::CoreID:        
asm volatile ("mrs %0, mpidr_el1" : "=r" (value)); value &= 0xFFFFFFFF; break; // Mask to lower 3
 case SpecialRegister::PageTableBase: asm volatile ("mrs %0, ttbr1_el1" : "=r" (value)); break; // Assuming EL1 for OS 
case SpecialRegister::TimerCurrentValue: isb(); asm volatile ("mrs %0, cntpct_el0" : "=r" (value)); break; 
case SpecialRegister::TimerFrequency: asm volatile ("mrs %0, cntfrq_el0" : "=r" (value)); break; 
case SpecialRegister::InterruptMask: asm volatile ("mrs %0, daif" : "=r" (value)); value = (value >> 7) & 1; break; // I bit is bit 7 
default: throw BDIExecutionError("HAL Error: readSpecialRegister: Unhandled AArch64 register"); 
    }
 return value; 
} 
void AArch64_HAL::writeSpecialRegister(SpecialRegister reg, uint64_t value){ 
switch(reg) { 
case SpecialRegister::StackPointer:  asm volatile ("mov sp, %0" :: "r" (value) : "memory"); break; 
case SpecialRegister::FramePointer:  asm volatile ("mov x29, %0" :: "r" (value) : "memory"); break; 
case SpecialRegister::PageTableBase: asm volatile ("msr ttbr1_el1, %0" :: "r" (value)); isb(); break; // ISB recommended after TTBR wr
 case SpecialRegister::InterruptMask: { // Value 1 = disable, 0 = enable 
if (value) disableInterrupts(); else enableInterrupts(); 
break; 
        } 
default: throw BDIExecutionError("HAL Error: writeSpecialRegister: Unhandled/Unwritable AArch64 register"); 
     } 
} 
bool AArch64_HAL::readPhysical(uint64_t pa, std::byte* buf, size_t sz) { 
// Assumes kernel has direct mapping or uses appropriate translation routines 
// Needs cache coherence handling (e.g., invalidate cache line before read if needed) 
// dmb("sy"); // Ensure prior writes visible? Depends on context. 
std::memcpy(buf, reinterpret_cast<void*>(pa), sz); // Requires correct VA mapping for PA 
return true; // Add error checking 
} 
bool AArch64_HAL::writePhysical(uint64_t pa, const std::byte* buf, size_t sz) { 
// Assumes kernel has direct mapping or uses translation routines 
// Needs cache coherence handling (e.g., clean/flush cache line after write) 
std::memcpy(reinterpret_cast<void*>(pa), buf, sz); // Requires correct VA mapping for PA 
    dsb("sy"); // Ensure write completes before subsequent operations 
return true; // Add error checking 
} 
std::optional<uintptr_t> AArch64_HAL::mapPhysicalMemory(uint64_t pa, size_t sz, uint64_t flags) { 
// Requires complex page table manipulation (walk L1/L2/L3 tables, create/update PTEs) 
// PTE flags based on input 'flags' (permissions, cacheability MAIR attributes) 
// After updating PTEs: DSB + TLBI + ISB sequence needed for synchronization. 
std::cerr << "AArch64_HAL Warning: mapPhysicalMemory requires full page table implementation. Returning identity mapping." << std::endl; 
return static_cast<uintptr_t>(pa); // Placeholder: Identity map 
} 
bool AArch64_HAL::unmapMemory(uintptr_t va, size_t sz) { 
// Requires complex page table manipulation (find PTEs for VA range, clear valid bit) 
// DSB + TLBI + ISB sequence needed. 
std::cerr << "AArch64_HAL Warning: unmapMemory requires full page table implementation. Assuming success." << std::endl; 
return true; // Placeholder 
} 
void AArch64_HAL::enableInterrupts() { asm volatile ("msr daifclr, #2" ::: "memory", "cc"); } // Clear I bit in DAIF 
}
void AArch64_HAL::acknowledgeInterrupt(uint32_t vector) { 
    // Write the vector (or Interrupt ID read from GICC_IAR) to the GIC CPU Interface EOI register 
    // Requires MMIO write. GIC version matters (v2/v3/v4). 
    // Example for GICv2/v3 (may need DSB): 
    if (gicc_base) {
        // uint32_t iar_value = gicc_base[GICC_IAR]; // Read Interrupt Acknowledge Register first 
        // gicc_base[GICC_EOIR] = iar_value; // Write back the same value to EOI 
        gicc_base[GICC_EOIR] = vector; // Simpler write if vector known, check GIC spec 
        dsb("sy"); // Ensure visibility 
    } else { 
         throw BDIExecutionError("HAL Error: GIC CPU Interface base not initialized for EOI"); 
    }
 } 
void AArch64_HAL::waitForInterrupt() { 
    // Wait For Interrupt. Enters low-power state until interrupt/event wakes it. 
    // Ensure DSB before WFI is often recommended. 
    dsb("sy"); 
    asm volatile ("wfi" ::: "memory"); 
    isb(); // Synchronize after wake-up 
} 
void AArch64_HAL::systemHalt() { 
    disableInterrupts(); 
    std::cout << "AArch64_HAL: System Halted." << std::endl; 
    while(true) { waitForInterrupt(); } 
} 
void AArch64_HAL::debugBreak() { asm volatile ("brk #0" ::: "memory"); } // Breakpoint instruction 
} // namespace 
