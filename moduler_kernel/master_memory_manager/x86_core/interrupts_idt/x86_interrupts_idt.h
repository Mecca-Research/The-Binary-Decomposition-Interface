
/**
 * @file x86_interrupts_idt.h
 * @brief x86 Interrupt Descriptor Table and APIC Management
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete IDT/APIC flow management with trap/interrupt gates, IRET, privilege transitions
 */

#ifndef X86_INTERRUPTS_IDT_H
#define X86_INTERRUPTS_IDT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// IDT Constants
#define IDT_MAX_ENTRIES         256
#define IDT_GATE_TYPE_TASK      0x5
#define IDT_GATE_TYPE_INTERRUPT 0xE
#define IDT_GATE_TYPE_TRAP      0xF

// APIC Constants
#define APIC_BASE_MSR           0x1B
#define APIC_ENABLE_BIT         (1 << 11)
#define APIC_BSP_BIT            (1 << 8)

// Interrupt Vector Ranges
#define EXCEPTION_VECTORS_START 0
#define EXCEPTION_VECTORS_END   31
#define IRQ_VECTORS_START       32
#define IRQ_VECTORS_END         47
#define APIC_VECTORS_START      48
#define APIC_VECTORS_END        255

// Exception Vectors
#define EXCEPTION_DIVIDE_ERROR      0
#define EXCEPTION_DEBUG             1
#define EXCEPTION_NMI               2
#define EXCEPTION_BREAKPOINT        3
#define EXCEPTION_OVERFLOW          4
#define EXCEPTION_BOUND_RANGE       5
#define EXCEPTION_INVALID_OPCODE    6
#define EXCEPTION_DEVICE_NOT_AVAIL  7
#define EXCEPTION_DOUBLE_FAULT      8
#define EXCEPTION_INVALID_TSS       10
#define EXCEPTION_SEGMENT_NOT_PRES  11
#define EXCEPTION_STACK_FAULT       12
#define EXCEPTION_GENERAL_PROTECT   13
#define EXCEPTION_PAGE_FAULT        14
#define EXCEPTION_FPU_ERROR         16
#define EXCEPTION_ALIGNMENT_CHECK   17
#define EXCEPTION_MACHINE_CHECK     18
#define EXCEPTION_SIMD_FP_ERROR     19

// APIC Interrupt Vectors
#define APIC_TIMER_VECTOR           48
#define APIC_THERMAL_VECTOR         49
#define APIC_PERFORMANCE_VECTOR     50
#define APIC_LINT0_VECTOR           51
#define APIC_LINT1_VECTOR           52
#define APIC_ERROR_VECTOR           53
#define APIC_SPURIOUS_VECTOR        255

/**
 * @brief IDT Gate Descriptor Structure
 */
typedef struct {
    uint16_t offset_low;    // Offset bits 0-15
    uint16_t selector;      // Code segment selector
    uint8_t  ist;          // Interrupt Stack Table (x86-64 only)
    uint8_t  type_attr;    // Type and attributes
    uint16_t offset_mid;    // Offset bits 16-31
    uint32_t offset_high;   // Offset bits 32-63 (x86-64 only)
    uint32_t reserved;      // Reserved (x86-64 only)
} __attribute__((packed)) idt_gate_desc_t;

/**
 * @brief IDT Pointer Structure
 */
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uintptr_t base;         // Base address of IDT
} __attribute__((packed)) idt_ptr_t;

/**
 * @brief APIC Register Structure
 */
typedef struct {
    volatile uint32_t reg;
    volatile uint32_t reserved[3];
} apic_reg_t;

/**
 * @brief Local APIC Structure
 */
typedef struct {
    apic_reg_t reserved1[2];        // 0x00-0x10
    apic_reg_t id;                  // 0x20 - Local APIC ID
    apic_reg_t version;             // 0x30 - Local APIC Version
    apic_reg_t reserved2[4];        // 0x40-0x70
    apic_reg_t task_priority;       // 0x80 - Task Priority Register
    apic_reg_t arbitration_priority; // 0x90 - Arbitration Priority Register
    apic_reg_t processor_priority;  // 0xA0 - Processor Priority Register
    apic_reg_t eoi;                 // 0xB0 - End of Interrupt
    apic_reg_t remote_read;         // 0xC0 - Remote Read
    apic_reg_t logical_dest;        // 0xD0 - Logical Destination
    apic_reg_t dest_format;         // 0xE0 - Destination Format
    apic_reg_t spurious_vector;     // 0xF0 - Spurious Interrupt Vector
    apic_reg_t isr[8];              // 0x100-0x170 - In-Service Register
    apic_reg_t tmr[8];              // 0x180-0x1F0 - Trigger Mode Register
    apic_reg_t irr[8];              // 0x200-0x270 - Interrupt Request Register
    apic_reg_t error_status;        // 0x280 - Error Status Register
    apic_reg_t reserved3[6];        // 0x290-0x2E0
    apic_reg_t lvt_cmci;            // 0x2F0 - LVT Corrected Machine Check Interrupt
    apic_reg_t icr_low;             // 0x300 - Interrupt Command Register (Low)
    apic_reg_t icr_high;            // 0x310 - Interrupt Command Register (High)
    apic_reg_t lvt_timer;           // 0x320 - LVT Timer
    apic_reg_t lvt_thermal;         // 0x330 - LVT Thermal Sensor
    apic_reg_t lvt_perf_counter;    // 0x340 - LVT Performance Counter
    apic_reg_t lvt_lint0;           // 0x350 - LVT LINT0
    apic_reg_t lvt_lint1;           // 0x360 - LVT LINT1
    apic_reg_t lvt_error;           // 0x370 - LVT Error
    apic_reg_t timer_initial_count; // 0x380 - Timer Initial Count
    apic_reg_t timer_current_count; // 0x390 - Timer Current Count
    apic_reg_t reserved4[4];        // 0x3A0-0x3D0
    apic_reg_t timer_divide_config; // 0x3E0 - Timer Divide Configuration
    apic_reg_t reserved5;           // 0x3F0
} local_apic_t;

/**
 * @brief Interrupt Context Structure
 */
typedef struct {
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    
    // Segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    
    // Control registers
    uint64_t rip;
    uint64_t rflags;
    
    // Error code (if applicable)
    uint64_t error_code;
    
    // Interrupt vector
    uint32_t vector;
    
    // Privilege level transition info
    bool privilege_change;
    uint16_t old_cs;
    uint64_t old_rsp;
} interrupt_context_t;

/**
 * @brief Interrupt Handler Function Type
 */
typedef void (*interrupt_handler_t)(interrupt_context_t* context);

/**
 * @brief Exception Handler Function Type
 */
typedef void (*exception_handler_t)(interrupt_context_t* context, uint64_t error_code);

// Core IDT Management Functions
int x86_idt_init(void);
int x86_idt_set_gate(uint8_t vector, uintptr_t handler, uint16_t selector, 
                     uint8_t type, uint8_t dpl);
int x86_idt_load(void);
void x86_idt_flush(void);

// APIC Management Functions
int x86_apic_init(void);
bool x86_apic_is_enabled(void);
uint32_t x86_apic_get_id(void);
void x86_apic_enable(void);
void x86_apic_disable(void);
void x86_apic_eoi(void);
int x86_apic_set_timer(uint32_t initial_count, uint8_t vector, bool periodic);
void x86_apic_send_ipi(uint32_t dest_apic_id, uint8_t vector);

// Interrupt Handler Registration
int x86_register_interrupt_handler(uint8_t vector, interrupt_handler_t handler);
int x86_register_exception_handler(uint8_t vector, exception_handler_t handler);
void x86_unregister_interrupt_handler(uint8_t vector);
void x86_unregister_exception_handler(uint8_t vector);

// Privilege Level Management
int x86_setup_privilege_transition(uint8_t vector, uint16_t kernel_cs, 
                                   uintptr_t kernel_stack);
bool x86_is_privilege_transition(uint16_t old_cs, uint16_t new_cs);

// Interrupt Control
void x86_disable_interrupts(void);
void x86_enable_interrupts(void);
bool x86_are_interrupts_enabled(void);
uint64_t x86_save_and_disable_interrupts(void);
void x86_restore_interrupts(uint64_t flags);

// IRET Support
void x86_prepare_iret_frame(interrupt_context_t* context, uintptr_t return_addr,
                           uint16_t return_cs, uint64_t return_flags);
int x86_validate_iret_frame(interrupt_context_t* context);

// Advanced APIC Features
int x86_apic_setup_lvt_entry(uint32_t lvt_offset, uint8_t vector, 
                             uint8_t delivery_mode, bool masked);
void x86_apic_set_task_priority(uint8_t priority);
uint8_t x86_apic_get_task_priority(void);
uint32_t x86_apic_read_error_status(void);
void x86_apic_clear_error_status(void);

// Debugging and Diagnostics
void x86_idt_dump_table(void);
void x86_apic_dump_registers(void);
const char* x86_get_exception_name(uint8_t vector);
void x86_print_interrupt_context(interrupt_context_t* context);

// Assembly stubs (implemented in assembly)
extern void x86_idt_flush_asm(idt_ptr_t* idt_ptr);
extern void x86_interrupt_stub_0(void);
extern void x86_interrupt_stub_1(void);
extern void x86_interrupt_stub_2(void);
extern void x86_interrupt_stub_3(void);
extern void x86_interrupt_stub_4(void);
extern void x86_interrupt_stub_5(void);
extern void x86_interrupt_stub_6(void);
extern void x86_interrupt_stub_7(void);
extern void x86_interrupt_stub_8(void);
extern void x86_interrupt_stub_10(void);
extern void x86_interrupt_stub_11(void);
extern void x86_interrupt_stub_12(void);
extern void x86_interrupt_stub_13(void);
extern void x86_interrupt_stub_14(void);
extern void x86_interrupt_stub_16(void);
extern void x86_interrupt_stub_17(void);
extern void x86_interrupt_stub_18(void);
extern void x86_interrupt_stub_19(void);

// Generic interrupt stubs for IRQs and APIC vectors
extern void x86_interrupt_stub_32(void);
extern void x86_interrupt_stub_33(void);
extern void x86_interrupt_stub_48(void);
extern void x86_interrupt_stub_255(void);

#ifdef __cplusplus
}
#endif

#endif // X86_INTERRUPTS_IDT_H
