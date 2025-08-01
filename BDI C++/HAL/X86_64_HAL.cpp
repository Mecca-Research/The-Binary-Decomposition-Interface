#include "X86_64_HAL.hpp"
#include <stdexcept> // For errors 
#include <iostream>  // For stubs 
#include <cstring>   // For memcpy 
#include <cpuid.h> // For __get_cpuid_max, __get_cpuid (GCC/Clang intrinsic) 
// Potentially include <immintrin.h> or <x86intrin.h> for other intrinsics if needed 
// --- VERY IMPORTANT --- 
// This requires inline assembly or platform-specific APIs (like intrinsics) 
// which cannot be fully represented portably here. Stubs are used. 
// Use compiler intrinsics or specific assembly blocks for real implementation. 
// Example using GCC/Clang style inline asm syntax (conceptual) 
// --- Placeholder for APIC MMIO Base --- 
// This MUST be determined dynamically during boot via ACPI tables (MADT) 
volatile uint32_t* local_apic_base = reinterpret_cast<volatile uint32_t*>(0xFEE00000); // Common default, but NOT reliable! 
const uint32_t LAPIC_EOI_REGISTER_OFFSET = 0xB0 / sizeof(uint32_t); // Offset for End-Of-Interrupt register 
namespace bdi::hal { 
// --- Register Access --- 
// readSpecialRegister(SP): Use `mov rax, rsp` 
// writeSpecialRegister(SP): Use `mov rsp, rax` (use input value) 
// readSpecialRegister(FP): Use `mov rax, rbp` 
// writeSpecialRegister(FP): Use `mov rbp, rax` 
// readSpecialRegister(Flags): Use `pushfq; pop rax` 
// writeSpecialRegister(Flags): Use `push rax; popfq` 
// readSpecialRegister(IP): Use `lea rax, [rip + 0];` (relative to next instruction) or call/pop pattern. 
// readSpecialRegister(CR3): Use `mov rax, cr3` 
// writeSpecialRegister(CR3): Use `mov cr3, rax` 
// readSpecialRegister(CoreID): Use `cpuid` (Leaf 0x1 or others) or `rdmsr IA32_TSC_AUX` / `rdpid`. Requires CPUID setup/checking. 
// readSpecialRegister(TimerCurrentValue): Use `rdtsc` or `rdtscp`. Needs calibration. 
// --- Memory Access --- 
// read/writePhysical: Use direct memory moves (`mov [phys_addr], reg`). Assumes sufficient privilege and appropriate memory mapping (e.g.
// mapPhysicalMemory: For initial BDIOS, likely rely on identity mapping set by bootloader. Return `physical_address`. A Phase 2 BDIOS wou
// unmapMemory: Update page table entries and invoke TLB flush (`invlpg` or `mov cr3, rax` again). 
// --- Interrupts --- 
// enableInterrupts: Use `sti` instruction. 
// disableInterrupts: Use `cli` instruction. 
// acknowledgeInterrupt: Requires MMIO write to the Local APIC's EOI register (address found via ACPI/MP tables). Example: `mov dword ptr 
// waitForInterrupt: Use `hlt` instruction. 
// --- System --- 
// systemHalt: Use `cli; hlt` loop. 
// debugBreak: Use `int3` instruction. 
}     
uint64_t X86_64_HAL::readSpecialRegister(SpecialRegister reg) { 
    uint64_t value = 0; 
    switch(reg) { 
        case SpecialRegister::StackPointer: 
             asm volatile ("mov %%rsp, %0" : "=r" (value)); // Read RSP 
             value = 0xDEADBEEFSP000000; // Placeholder 
            break; 
        case SpecialRegister::FramePointer: 
             asm volatile ("mov %%rbp, %0" : "=r" (value)); // Read RBP 
             value = 0xDEADBEEFFP000000; // Placeholder 
            break; 
        case SpecialRegister::InstructionPointer: 
             // Getting precise RIP requires a trick. Call instruction pushes return address. 
             // Or use LEA relative to RIP (but gives address of data, not code being executed) 
             // This gives address *after* the call. 
            asm volatile ( 
                "call 1f;" 
                "1: pop %0;" 
                : "=r"(value) 
                : // no inputs 
                : // no clobbers (technically pushes/pops stack) 
            ); 
             // A simpler but less precise way for general location: 
             // asm volatile ("lea .(%0), %1" : "=r" (value) : "r" (value)); // Doesn't work reliably 
             // Need specific mechanism, e.g., call + pop or LEA relative RIP 
             value = 0xFFFFFFFFDEADBEEF; // Placeholder 
            break; 
        case SpecialRegister::FlagsRegister: 
             asm volatile ("pushfq; popq %0" : "=r" (value)); // Read RFLAGS 
             value = 0x202; // Placeholder 
            break; 
        case SpecialRegister::CoreID: 
             // Use CPUID instruction or MSR read (e.g., IA32_TSC_AUX) - complex 
             // Or use newer features like leaf 0xB or MSRs if available & preferred 
             // Or use rdtscp's auxiliary field if kernel configured it (IA32_TSC_AUX MSR) 
             // Simple CPUID Leaf 1 approach: 
             uint32_t eax, ebx, ecx, edx; 
             if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) { 
                 value = (ebx >> 24) & 0xFF; // Initial Local APIC ID 
             } else {
             value = 0; // Placeholder for core 0,  Fallback or error 
                       throw BDIExecutionError("HAL Error: Failed to execute CPUID leaf 1"); 
             } 
            break; 
        }
        case SpecialRegister::PageTableBase: // Read CR3 
             asm volatile ("mov %%cr3, %0" : "=r" (value));
             value = 0x1000; // Placeholder 
             break; 
        case SpecialRegister::TimerCurrentValue: // Read time stamp counter, Needs calibration. 
        // Ensure TSC is available and reliable (check CPUID features, constant_tsc flag) 
             asm volatile ( 
                 "rdtsc;"        // Reads TSC into EDX:EAX 
                 "shl $32, %%rdx;" // Shift high bits 
                 "or %%rdx, %%rax;" // Combine into RAX 
                 : "=a"(value)   // Output to RAX ('a') 
                 : // No inputs 
                 : "%rdx"        // Clobbers RDX 
             );
            break; 
        case SpecialRegister::TimerFrequency: // From CPUID or calibration, Read from MSR or calibrate? Hard to get reliably. 
             // Needs calibration loop against known timer or CPUID leaf 0x15/0x16 if supported. 
             // Return placeholder - BDIOS Init might need to calibrate and store this. 
             value = 3000000000; // Example 3 GHz (placeholder) 
             std::cerr << "X86_64_HAL Warning: TimerFrequency is a placeholder value." << std::endl; 
            break; 
        case SpecialRegister::InterruptMask: // Read APIC or PIC registers? Complex. Read RFLAGS IF bit
            value = readSpecialRegister(SpecialRegister::FlagsRegister); 
            value = (value & 0x200) ? 1 : 0; // Check IF bit (bit 9) - 1=Enabled, 0=Disabled 
            break; 
        default: 
             std::cerr << "X86_64_HAL Warning: readSpecialRegister for unimplemented/unknown register " << static_cast<int>(reg) << std::endl;
             value = 0; // Return 0 for unimplemented 
             break; 
    }
             throw BDIExecutionError("HAL Error: readSpecialRegister for unknown/unimplemented register: " + std::to_string(static_cast<int>(r
    return value; 
} 
void X86_64_HAL::writeSpecialRegister(SpecialRegister reg, uint64_t value) { 
     switch(reg) { 
        case SpecialRegister::StackPointer: 
             asm volatile ("mov %0, %%rsp" :: "r" (value) : "memory"); // Write RSP 
             std::cout << "X86_64_HAL Stub: Write RSP = " << value << std::endl; 
            break; 
        case SpecialRegister::FramePointer: 
            asm volatile ("mov %0, %%rbp" :: "r" (value) : "memory"); // Write RBP 
             std::cout << "X86_64_HAL Stub: Write RBP = " << value << std::endl; 
            break; 
        case SpecialRegister::PageTableBase: 
            asm volatile ("mov %0, %%cr3" :: "r" (value) : "memory"); // Write CR3 
            std::cout << "X86_64_HAL Stub: Write CR3 = " << value << std::endl; 
            break; 
        case SpecialRegister::FlagsRegister: // RFLAGS (use with caution!) 
             asm volatile ("pushq %0; popfq" :: "r" (value) : "memory", "cc"); 
             break; 
        case SpecialRegister::InterruptMask: // Only controls IF bit in RFLAGS 
             if (value) enableInterrupts(); else disableInterrupts(); 
             break; 
        // Cannot write to CoreID, IP, TimerValue, TimerFrequency directly 
        default: 
             std::cerr << "X86_64_HAL Warning: writeSpecialRegister for unimplemented/unknown register " << static_cast<int>(reg) << std::endl
             throw BDIExecutionError("HAL Error: writeSpecialRegister for unknown/unwritable register: " + std::to_string(static_cast<int>(re
             break;                  
    }
 } 
// --- Memory Access --- 
// Assumes kernel-level access where virtual addresses map directly to physical 
// or BDIOS manages its own identity map / simple page table. 
bool X86_64_HAL::readPhysical(uint64_t physical_address, std::byte* buffer, size_t size_bytes) { 
    // Assumes identity mapping or direct physical access (e.g., in early boot or ring 0) 
    // Very unsafe without proper MMU setup & checks! 
    // TODO: Add checks: Is address valid? Mapped? Readable? 
    try { 
        std::memcpy(buffer, reinterpret_cast<void*>(physical_address), size_bytes); 
        return true;
    } catch (const std::exception& e) { // Catch potential segmentation faults (won't work reliably for HW faults) 
         std::cerr << "HAL Error: Exception during readPhysical from 0x" << std::hex << physical_address << ": " << e.what() << std::endl; 
        return false; 
    }
 }  
bool X86_64_HAL::writePhysical(uint64_t physical_address, const std::byte* buffer, size_t size_bytes) { 
    // TODO: Add checks: Is address valid? Mapped? Writable? 
     try { 
        std::memcpy(reinterpret_cast<void*>(physical_address), buffer, size_bytes); 
        return true;
    } catch (const std::exception& e) { 
         std::cerr << "HAL Error: Exception during writePhysical to 0x" << std::hex << physical_address << ": " << e.what() << std::endl; 
        return false; 
    }
 } 
// --- Memory Mapping (Simplified - Assumes Identity Mapping) --- 
std::optional<uintptr_t> X86_64_HAL::mapPhysicalMemory(uint64_t physical_address, size_t size_bytes, uint64_t flags) { 
    // In a minimal BDIOS running without complex MMU setup, we might assume 
    // physical addresses are directly usable as virtual addresses. 
    // A real implementation *must* interact with page tables (CR3). 
     std::cout << "X86_64_HAL Info: mapPhysicalMemory (Assuming Identity Map): PA=0x" << std::hex << physical_address << std::dec << " Size=" 
    // TODO: Store mapping info, check for overlaps if not identity mapped. 
    return static_cast<uintptr_t>(physical_address); 
} 
bool X86_64_HAL::unmapMemory(uintptr_t virtual_address, size_t size_bytes) { 
    // If using page tables, update PTEs to invalid and flush TLB (`invlpg`). 
    // If identity mapped, this might be a no-op or just bookkeeping. 
    std::cout << "X86_64_HAL Info: unmapMemory (Assuming Identity Map): VA=0x" << std::hex << virtual_address << std::dec << " Size=" << size_
     // TODO: Remove mapping info. 
    return true; 
} 
// --- Interrupt Control --- 
void X86_64_HAL::enableInterrupts() { 
    asm volatile ("sti" ::: "memory", "cc"); 
} 
void X86_64_HAL::disableInterrupts() { 
    asm volatile ("cli" ::: "memory", "cc"); 
} 
void X86_64_HAL::acknowledgeInterrupt(uint32_t vector) { 
    // Write 0 to the EOI register of the Local APIC. 
    // This requires knowing the MMIO base address of the Local APIC. 
    if (local_apic_base) { 
         *(local_apic_base + LAPIC_EOI_REGISTER_OFFSET) = 0; 
    } else { 
        // This is critical - need APIC base from ACPI/firmware! 
        std::cerr << "X86_64_HAL Error: Local APIC base address not initialized. Cannot send EOI." << std::endl; 
        // Or use legacy PIC EOI if APIC not present/enabled? 
    }
 } 
// --- System Control --- 
void X86_64_HAL::waitForInterrupt() { 
    // Halt processor until next interrupt arrives 
    asm volatile ("hlt" ::: "memory"); 
} 
void X86_64_HAL::systemHalt() { 
    disableInterrupts(); // Disable interrupts first 
    while(true) { 
        waitForInterrupt(); // Halt in a loop 
    }
 } 
void X86_64_HAL::debugBreak() { 
asm volatile ("int3"); // Trigger breakpoint exception 
std::cout << "X86_64_HAL Stub: debugBreak (int3)" << std::endl; 
} 
NodeID generateAllocatorGraph(GraphBuilder& builder, MetadataStore& meta_store, NodeID free_list_head_addr_id, NodeID lock_addr_id) { 
using Op = BDIOperationType; using Type = BDIType; 
auto add = [&](auto... args){ return builder.addNode(args...); }; // Shorthand 
auto link = [&](auto... args){ builder.connectData(args...); }; 
auto ctl = [&](auto... args){ builder.connectControl(args...); }; 
auto payload = [&](auto... args){ builder.setNodePayload(args...); }; 
auto output = [&](auto... args){ builder.defineDataOutput(args...); }; 
auto constant = [&](auto value, NodeID& cfg){ // Helper for constants 
         NodeID id = add(Op::META_CONST); 
         payload(id, TypedPayload::createFrom(value)); 
         output(id, 0, MapCppTypeToBdiType<decltype(value)>::value); 
if(cfg != 0) ctl(cfg, id); cfg = id; return id; 
    }; 
auto load = [&](NodeID addr_node, BDIType type, NodeID& cfg){ /*...*/ return 0; }; // Helper for LOAD 
auto store = [&](NodeID addr_node, NodeID val_node, NodeID& cfg){ /*...*/ }; // Helper for STORE 
auto ptr_add = [&](NodeID base, uint64_t offset, NodeID& cfg){ /*...*/ return 0; }; // Helper for Addr+Offset 
auto compare = [&](Op cmp_op, NodeID lhs, NodeID rhs, NodeID& cfg){ /*...*/ return 0; }; 
auto branch = [&](NodeID cond, NodeID& cfg){ /*...*/ return 0; }; // Returns BRANCH node 
auto jump = [&](NodeID& cfg){ /*...*/ return 0; }; // Returns JUMP node (needs target set later) 
auto set_jump_target = [&](NodeID jump_node, NodeID target_node){ /*...*/ }; 
auto set_branch_targets = [&](NodeID branch_node, NodeID true_node, NodeID false_node){ /*...*/ }; 
// --- Entry & Get Inputs --- 
    NodeID entry = add(Op::META_START, "AllocatorEntry"); 
    NodeID op_code_val = add(Op::FUNC_ARG, "OpCode"); // Conceptual op for getting arg 0 
    output(op_code_val, 0, Type::UINT32); 
    NodeID param1_val = add(Op::FUNC_ARG, "Param1"); // Arg 1 (size/addr) 
    output(param1_val, 0, Type::UINT64); 
    NodeID current_cfg = entry; // Start control flow 
    ctl(entry, op_code_val); current_cfg = op_code_val; 
    ctl(current_cfg, param1_val); current_cfg = param1_val; 
// --- Acquire Lock --- (Stubbed - needs SYNC primitive) 
// current_cfg = add(Op::SYNC_MUTEX_LOCK, ...) 
// --- Dispatch --- 
    NodeID const_alloc_op = constant(static_cast<uint32_t>(AllocatorOp::ALLOC), current_cfg); 
    NodeID is_alloc = compare(Op::CMP_EQ, op_code_val, const_alloc_op, current_cfg); 
    NodeID dispatch_branch = addBranch(is_alloc, current_cfg); 
// --- Allocate Path --- 
    NodeID alloc_entry_label = add(Op::META_NOP, "AllocPath"); 
    ctl(dispatch_branch, alloc_entry_label); // True path of branch 
    current_cfg = alloc_entry_label; 
    NodeID requested_size = param1_val; // Size is Param1 
    NodeID head_ptr_addr = free_list_head_addr_id; // Assume passed in or constant 
    NodeID current_block_ptr_addr = add(Op::REG_ALLOC); // Allocate temp storage for loop variable (current block ptr) 
    output(current_block_ptr_addr, 0, Type::POINTER); 
    NodeID head_ptr_val = load(head_ptr_addr, Type::POINTER, current_cfg); // Load head ptr value 
    store(current_block_ptr_addr, head_ptr_val, current_cfg); // current_block_ptr = head 
    NodeID loop_header = add(Op::META_NOP, "AllocLoopHeader"); 
    NodeID loop_entry_jump = addJump(current_cfg); set_jump_target(loop_entry_jump, loop_header); 
    current_cfg = loop_header; // Start of loop logic 
    NodeID current_block_ptr = load(current_block_ptr_addr, Type::POINTER, current_cfg); // Load current ptr value 
    NodeID is_null = compare(Op::CMP_EQ, current_block_ptr, constant(uintptr_t{0}, current_cfg), current_cfg); 
    NodeID branch_is_null = addBranch(is_null, current_cfg); // Branch if end of list 
// If NOT null (False path of branch_is_null) 
    NodeID not_null_path = addNop(builder, "CheckSize", current_cfg); 
    ctl(branch_is_null, not_null_path); // Connect false path 
    current_cfg = not_null_path; 
    NodeID block_size_addr = ptr_add(current_block_ptr, 0 /*offsetof size*/, current_cfg); // Addr = current + offset(size) 
    NodeID block_size = load(block_size_addr, Type::UINT64, current_cfg); // Load block size 
    NodeID size_ok = compare(Op::CMP_GE, block_size, requested_size, current_cfg); // block_size >= requested_size 
    NodeID branch_size = addBranch(size_ok, current_cfg); 
// If size NOT ok (False path of branch_size) 
    NodeID size_not_ok_path = addNop(builder,"NextBlock", current_cfg); 
    ctl(branch_size, size_not_ok_path); 
    current_cfg = size_not_ok_path; 
    NodeID next_ptr_addr = ptr_add(current_block_ptr, 8 /*offsetof next*/, current_cfg); // Addr = current + offset(next) 
    NodeID next_ptr = load(next_ptr_addr, Type::POINTER, current_cfg); // Load next ptr value 
    store(current_block_ptr_addr, next_ptr, current_cfg); // Update loop variable: current_block_ptr = next_ptr 
    NodeID loop_back_jump = addJump(current_cfg); set_jump_target(loop_back_jump, loop_header); // Jump back to header 
// If size IS ok (True path of branch_size) 
    NodeID size_ok_path = addNop(builder,"BlockFound", current_cfg); 
    ctl(branch_size, size_ok_path); 
    current_cfg = size_ok_path; 
// --- Complex logic to split/remove block from free list via STOREs --- 
    NodeID found_addr = current_block_ptr; // Address to return 
    NodeID alloc_success = constant(true, current_cfg); 
    NodeID jump_to_exit_alloc = addJump(current_cfg); // Jump to common exit 
// If end of list reached (True path of branch_is_null) 
    NodeID null_path = addNop(builder, "OutOfMemory", current_cfg); 
    ctl(branch_is_null, null_path); 
    current_cfg = null_path; 
    NodeID fail_addr = constant(uintptr_t{0}, current_cfg); 
    NodeID alloc_fail = constant(false, current_cfg); 
    NodeID jump_to_exit_oom = addJump(current_cfg); 
// --- FREE Path --- 
    NodeID free_entry_label = addNop(builder, "FreePath"); 
    ctl(dispatch_branch, free_entry_label); // False path of dispatch branch 
    current_cfg = free_entry_label; 
    NodeID addr_to_free = param1_val; // Address is Param1 
// --- Complex logic for list insertion & merging using LOAD/STORE/CMP/ADD/SUB --- 
    NodeID free_success = constant(true, current_cfg); 
    NodeID jump_to_exit_free = addJump(current_cfg); 
// --- Exit Path --- 
    NodeID exit_label = add(Op::META_NOP, "AllocatorExit"); 
    set_jump_target(jump_to_exit_alloc, exit_label); 
    set_jump_target(jump_to_exit_oom, exit_label); 
    set_jump_target(jump_to_exit_free, exit_label); 
    current_cfg = exit_label; // Code converges here 
// --- PHI Nodes needed here to select result based on path --- 
// Conceptual: Select Address result 
// NodeID final_addr = addPhi(builder, { {size_ok_path, found_addr}, {null_path, fail_addr}, {free_entry_label, constant(0)} }, curre
 // Conceptual: Select Success result 
// NodeID final_success = addPhi(builder, { {size_ok_path, alloc_success}, {null_path, alloc_fail}, {free_entry_label, free_success} 
    NodeID final_addr = addConstU64(builder, 0xABC0, current_cfg); // Placeholder PHI result 
    NodeID final_success = addConstBool(builder, true, current_cfg); // Placeholder PHI result 
// --- Release Lock --- 
// current_cfg = addMutexUnlock(...) 
// --- Return --- 
    NodeID ret = add(Op::CTRL_RETURN); 
    link(final_addr, 0, ret, 0);    
// Connect address result 
    link(final_success, 0, ret, 1); // Connect success flag result (Requires multi-value return or struct) 
    ctl(current_cfg, ret); 
return entry; // Return entry point of service graph 
} 
// Implement similar conceptual generation for Scheduler graph
} // namespace bdi::hal 
