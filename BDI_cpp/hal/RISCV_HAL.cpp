#include "RISCV_HAL.hpp" 
#include <stdexcept>
#include <iostream> 
#include <cstring> 
// --- Assumed CSR definitions (from RISC-V specs) --- 
#define MSTATUS_MIE 0x00000008 // Machine Mode Interrupt Enable bit 
#define SSTATUS_SIE 0x00000002 // Supervisor Mode Interrupt Enable bit 
// --- Placeholder for PLIC MMIO Base (MUST be found via Device Tree) --- 
volatile uint32_t* plic_base = reinterpret_cast<volatile uint32_t*>(0x0C000000); // EXAMPLE ONLY 
const uint32_t PLIC_CLAIM_COMPLETE_OFFSET = 0x200004 / sizeof(uint32_t); // Context 0 Claim/Complete offset - EXAMPLE 
namespace bdi::hal { 
uint64_t RISCV_HAL::readSpecialRegister(SpecialRegister reg) { 
    uint64_t value = 0; 
    // Use csrr instruction. Need to handle RV32 vs RV64 appropriately for register size. 
    switch(reg) { 
        case SpecialRegister::StackPointer:  asm volatile ("mv %0, sp" : "=r" (value)); break; 
        case SpecialRegister::FramePointer:  asm volatile ("mv %0, fp" : "=r" (value)); break; // fp is s0 (x8) 
        case SpecialRegister::InstructionPointer: asm volatile ("auipc %0, 0; addi %0, %0, 0" : "=r" (value)); break; // Gets PC (needs adjust
        case SpecialRegister::FlagsRegister: return 0; // RISC-V has no general flags register like x86/ARM 
        case SpecialRegister::CoreID:        asm volatile ("csrr %0, mhartid" : "=r" (value)); break; // Hardware thread ID 
        case SpecialRegister::PageTableBase: asm volatile ("csrr %0, satp" : "=r" (value)); break; // Supervisor Address Translation and Prote
        case SpecialRegister::TimerCurrentValue: asm volatile ("csrr %0, time" : "=r" (value)); break; // M-mode timer (S-mode uses SBI call u
        case SpecialRegister::TimerFrequency: value = 10000000; /* Get from device tree */ break; // Platform specific 
        case SpecialRegister::InterruptMask: // Read sie (S-Mode) or mie (M-Mode) 
             asm volatile ("csrr %0, sie" : "=r" (value)); // Assume S-Mode
             value = (value & SSTATUS_SIE) ? 1 : 0; // 1=Enabled, 0=Disabled 
             break; 
        default: throw BDIExecutionError("HAL Error: readSpecialRegister: Unhandled RISC-V register"); 
    }
    return value; 
} 
void RISCV_HAL::writeSpecialRegister(SpecialRegister reg, uint64_t value){ 
     switch(reg) { 
        case SpecialRegister::StackPointer:  asm volatile ("mv sp, %0" :: "r" (value) : "memory"); break; 
        case SpecialRegister::FramePointer:  asm volatile ("mv fp, %0" :: "r" (value) : "memory"); break; 
        case SpecialRegister::PageTableBase: asm volatile ("csrw satp, %0" :: "r" (value)); asm volatile("sfence.vma"); break; // Flush TLB af
        case SpecialRegister::InterruptMask: { // Value 1 = enable, 0 = disable (SIE bit) 
             if (value) asm volatile ("csrrs zero, sstatus, %0" :: "i" (SSTATUS_SIE)); // Set SIE bit 
             else asm volatile ("csrrc zero, sstatus, %0" :: "i" (SSTATUS_SIE)); // Clear SIE bit 
             break; 
        } 
        default: throw BDIExecutionError("HAL Error: writeSpecialRegister: Unhandled/Unwritable RISC-V register"); 
     } 
} 
bool RISCV_HAL::readPhysical(uint64_t pa, std::byte* buf, size_t sz) { /* memcpy based on kernel VA mapping PA */ return true; } 
bool RISCV_HAL::writePhysical(uint64_t pa, const std::byte* buf, size_t sz) { /* memcpy based on kernel VA mapping PA */ return true; } 
std::optional<uintptr_t> RISCV_HAL::mapPhysicalMemory(uint64_t pa, size_t sz, uint64_t flags) { /* Update Sv39/Sv48 page tables, sfence.vma */
 bool RISCV_HAL::unmapMemory(uintptr_t va, size_t sz) { /* Update page tables, sfence.vma */ return true; } 
void RISCV_HAL::enableInterrupts() { asm volatile ("csrrs zero, sstatus, %0" :: "i" (SSTATUS_SIE) : "memory"); } // Set SIE bit in sstatus 
void RISCV_HAL::disableInterrupts() { asm volatile ("csrrc zero, sstatus, %0" :: "i" (SSTATUS_SIE) : "memory"); } // Clear SIE bit in sstatus 
void RISCV_HAL::acknowledgeInterrupt(uint32_t vector /* IRQ ID for PLIC */) { 
    // Requires MMIO write to the PLIC Claim/Complete register for the current context (hart) 
    if (plic_base) {
        // 1. Claim the interrupt (read from claim/complete register) 
// uint32_t claimed_irq = plic_base[PLIC_CLAIM_COMPLETE_OFFSET]; 
// if (claimed_irq == vector) { // Verify it's the one expected? Optional. 
// 2. Complete the interrupt (write the claimed IRQ ID back) 
// plic_base[PLIC_CLAIM_COMPLETE_OFFSET] = claimed_irq; 
        plic_base[PLIC_CLAIM_COMPLETE_OFFSET] = vector; // Write IRQ ID back 
// } else { /* Error handling */ } 
else { 
throw BDIExecutionError("HAL Error: PLIC base address not initialized for EOI"); 
    }
 } 
void RISCV_HAL::waitForInterrupt() { 
// Wait For Interrupt instruction 
asm volatile ("wfi" ::: "memory"); 
} 
void RISCV_HAL::systemHalt() {
    disableInterrupts(); 
std::cout << "RISC-V_HAL: System Halted." << std::endl; 
// Use SBI shutdown call if available, otherwise loop WFI 
// Example SBI call (conceptual assembly) 
// li a7, 8 // SBI SRST Extension ID 
// li a6, 0 // Type: Shutdown 
// ecall 
while(true) { waitForInterrupt(); } 
} 
void RISCV_HAL::debugBreak() { asm volatile ("ebreak" ::: "memory"); } // Breakpoint instruction 
} // namespace bdi::hal 
