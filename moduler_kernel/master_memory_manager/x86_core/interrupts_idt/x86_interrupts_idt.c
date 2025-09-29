
/**
 * @file x86_interrupts_idt.c
 * @brief x86 Interrupt Descriptor Table and APIC Management Implementation
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete IDT/APIC flow management with trap/interrupt gates, IRET, privilege transitions
 */

#include "x86_interrupts_idt.h"
#include <string.h>
#include <stdio.h>

// Global IDT and APIC state
static idt_gate_desc_t idt_table[IDT_MAX_ENTRIES] __attribute__((aligned(16)));
static idt_ptr_t idt_ptr;
static interrupt_handler_t interrupt_handlers[IDT_MAX_ENTRIES];
static exception_handler_t exception_handlers[IDT_MAX_ENTRIES];
static local_apic_t* local_apic = NULL;
static bool apic_enabled = false;

// Exception names for debugging
static const char* exception_names[] = {
    "Divide Error",
    "Debug Exception",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

/**
 * @brief Initialize the Interrupt Descriptor Table
 */
int x86_idt_init(void) {
    // Clear the IDT table
    memset(idt_table, 0, sizeof(idt_table));
    memset(interrupt_handlers, 0, sizeof(interrupt_handlers));
    memset(exception_handlers, 0, sizeof(exception_handlers));
    
    // Set up IDT pointer
    idt_ptr.limit = sizeof(idt_table) - 1;
    idt_ptr.base = (uintptr_t)idt_table;
    
    // Set up exception handlers (vectors 0-31)
    x86_idt_set_gate(0, (uintptr_t)x86_interrupt_stub_0, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(1, (uintptr_t)x86_interrupt_stub_1, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(2, (uintptr_t)x86_interrupt_stub_2, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(3, (uintptr_t)x86_interrupt_stub_3, 0x08, IDT_GATE_TYPE_TRAP, 3);
    x86_idt_set_gate(4, (uintptr_t)x86_interrupt_stub_4, 0x08, IDT_GATE_TYPE_TRAP, 3);
    x86_idt_set_gate(5, (uintptr_t)x86_interrupt_stub_5, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(6, (uintptr_t)x86_interrupt_stub_6, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(7, (uintptr_t)x86_interrupt_stub_7, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(8, (uintptr_t)x86_interrupt_stub_8, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(10, (uintptr_t)x86_interrupt_stub_10, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(11, (uintptr_t)x86_interrupt_stub_11, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(12, (uintptr_t)x86_interrupt_stub_12, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(13, (uintptr_t)x86_interrupt_stub_13, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(14, (uintptr_t)x86_interrupt_stub_14, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(16, (uintptr_t)x86_interrupt_stub_16, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(17, (uintptr_t)x86_interrupt_stub_17, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(18, (uintptr_t)x86_interrupt_stub_18, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(19, (uintptr_t)x86_interrupt_stub_19, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    
    // Set up IRQ handlers (vectors 32-47)
    for (int i = IRQ_VECTORS_START; i <= IRQ_VECTORS_END; i++) {
        x86_idt_set_gate(i, (uintptr_t)x86_interrupt_stub_32, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    }
    
    // Set up APIC vectors (vectors 48-255)
    x86_idt_set_gate(APIC_TIMER_VECTOR, (uintptr_t)x86_interrupt_stub_48, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    x86_idt_set_gate(APIC_SPURIOUS_VECTOR, (uintptr_t)x86_interrupt_stub_255, 0x08, IDT_GATE_TYPE_INTERRUPT, 0);
    
    return 0;
}

/**
 * @brief Set an IDT gate descriptor
 */
int x86_idt_set_gate(uint8_t vector, uintptr_t handler, uint16_t selector, 
                     uint8_t type, uint8_t dpl) {
    if (vector >= IDT_MAX_ENTRIES) {
        return -1;
    }
    
    idt_gate_desc_t* gate = &idt_table[vector];
    
    // Set offset
    gate->offset_low = handler & 0xFFFF;
    gate->offset_mid = (handler >> 16) & 0xFFFF;
    gate->offset_high = (handler >> 32) & 0xFFFFFFFF;
    
    // Set selector
    gate->selector = selector;
    
    // Set IST (Interrupt Stack Table) - 0 for now
    gate->ist = 0;
    
    // Set type and attributes
    gate->type_attr = 0x80 | (dpl << 5) | type; // Present bit | DPL | Type
    
    // Reserved field
    gate->reserved = 0;
    
    return 0;
}

/**
 * @brief Load the IDT
 */
int x86_idt_load(void) {
    x86_idt_flush_asm(&idt_ptr);
    return 0;
}

/**
 * @brief Flush the IDT (alias for load)
 */
void x86_idt_flush(void) {
    x86_idt_load();
}

/**
 * @brief Initialize the Local APIC
 */
int x86_apic_init(void) {
    uint64_t apic_base_msr;
    
    // Read APIC base MSR
    __asm__ volatile("rdmsr" : "=A"(apic_base_msr) : "c"(APIC_BASE_MSR));
    
    // Check if APIC is enabled
    if (!(apic_base_msr & APIC_ENABLE_BIT)) {
        // Enable APIC
        apic_base_msr |= APIC_ENABLE_BIT;
        __asm__ volatile("wrmsr" : : "A"(apic_base_msr), "c"(APIC_BASE_MSR));
    }
    
    // Map APIC registers (assuming identity mapping for now)
    local_apic = (local_apic_t*)(apic_base_msr & 0xFFFFF000);
    
    // Enable APIC in spurious interrupt vector register
    local_apic->spurious_vector.reg = APIC_SPURIOUS_VECTOR | 0x100;
    
    // Set task priority to 0 (accept all interrupts)
    local_apic->task_priority.reg = 0;
    
    apic_enabled = true;
    return 0;
}

/**
 * @brief Check if APIC is enabled
 */
bool x86_apic_is_enabled(void) {
    return apic_enabled;
}

/**
 * @brief Get Local APIC ID
 */
uint32_t x86_apic_get_id(void) {
    if (!apic_enabled || !local_apic) {
        return 0;
    }
    return (local_apic->id.reg >> 24) & 0xFF;
}

/**
 * @brief Enable APIC
 */
void x86_apic_enable(void) {
    if (local_apic) {
        local_apic->spurious_vector.reg |= 0x100;
        apic_enabled = true;
    }
}

/**
 * @brief Disable APIC
 */
void x86_apic_disable(void) {
    if (local_apic) {
        local_apic->spurious_vector.reg &= ~0x100;
        apic_enabled = false;
    }
}

/**
 * @brief Send End of Interrupt signal
 */
void x86_apic_eoi(void) {
    if (local_apic) {
        local_apic->eoi.reg = 0;
    }
}

/**
 * @brief Set up APIC timer
 */
int x86_apic_set_timer(uint32_t initial_count, uint8_t vector, bool periodic) {
    if (!local_apic) {
        return -1;
    }
    
    // Set timer vector and mode
    uint32_t lvt_timer = vector;
    if (periodic) {
        lvt_timer |= 0x20000; // Periodic mode
    }
    local_apic->lvt_timer.reg = lvt_timer;
    
    // Set divide configuration (divide by 1)
    local_apic->timer_divide_config.reg = 0x0B;
    
    // Set initial count
    local_apic->timer_initial_count.reg = initial_count;
    
    return 0;
}

/**
 * @brief Send Inter-Processor Interrupt
 */
void x86_apic_send_ipi(uint32_t dest_apic_id, uint8_t vector) {
    if (!local_apic) {
        return;
    }
    
    // Set destination
    local_apic->icr_high.reg = (dest_apic_id << 24);
    
    // Send IPI
    local_apic->icr_low.reg = vector | 0x4000; // Fixed delivery mode
}

/**
 * @brief Register interrupt handler
 */
int x86_register_interrupt_handler(uint8_t vector, interrupt_handler_t handler) {
    if (vector >= IDT_MAX_ENTRIES) {
        return -1;
    }
    
    interrupt_handlers[vector] = handler;
    return 0;
}

/**
 * @brief Register exception handler
 */
int x86_register_exception_handler(uint8_t vector, exception_handler_t handler) {
    if (vector >= IDT_MAX_ENTRIES) {
        return -1;
    }
    
    exception_handlers[vector] = handler;
    return 0;
}

/**
 * @brief Unregister interrupt handler
 */
void x86_unregister_interrupt_handler(uint8_t vector) {
    if (vector < IDT_MAX_ENTRIES) {
        interrupt_handlers[vector] = NULL;
    }
}

/**
 * @brief Unregister exception handler
 */
void x86_unregister_exception_handler(uint8_t vector) {
    if (vector < IDT_MAX_ENTRIES) {
        exception_handlers[vector] = NULL;
    }
}

/**
 * @brief Set up privilege level transition
 */
int x86_setup_privilege_transition(uint8_t vector, uint16_t kernel_cs, 
                                   uintptr_t kernel_stack) {
    if (vector >= IDT_MAX_ENTRIES) {
        return -1;
    }
    
    // Update IDT gate with kernel code segment
    idt_table[vector].selector = kernel_cs;
    
    // Note: Stack switching is handled by TSS in hardware
    // This function sets up the gate for privilege transitions
    
    return 0;
}

/**
 * @brief Check if privilege transition occurred
 */
bool x86_is_privilege_transition(uint16_t old_cs, uint16_t new_cs) {
    return (old_cs & 3) != (new_cs & 3);
}

/**
 * @brief Disable interrupts
 */
void x86_disable_interrupts(void) {
    __asm__ volatile("cli");
}

/**
 * @brief Enable interrupts
 */
void x86_enable_interrupts(void) {
    __asm__ volatile("sti");
}

/**
 * @brief Check if interrupts are enabled
 */
bool x86_are_interrupts_enabled(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags));
    return (flags & 0x200) != 0;
}

/**
 * @brief Save and disable interrupts
 */
uint64_t x86_save_and_disable_interrupts(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags));
    return flags;
}

/**
 * @brief Restore interrupt state
 */
void x86_restore_interrupts(uint64_t flags) {
    __asm__ volatile("pushq %0; popfq" : : "r"(flags));
}

/**
 * @brief Prepare IRET frame
 */
void x86_prepare_iret_frame(interrupt_context_t* context, uintptr_t return_addr,
                           uint16_t return_cs, uint64_t return_flags) {
    context->rip = return_addr;
    context->cs = return_cs;
    context->rflags = return_flags;
}

/**
 * @brief Validate IRET frame
 */
int x86_validate_iret_frame(interrupt_context_t* context) {
    // Check code segment selector
    if ((context->cs & 0xFFFC) == 0) {
        return -1; // Invalid selector
    }
    
    // Check flags register
    if (context->rflags & 0xFFC08028) {
        return -1; // Reserved bits set
    }
    
    return 0;
}

/**
 * @brief Set up LVT entry
 */
int x86_apic_setup_lvt_entry(uint32_t lvt_offset, uint8_t vector, 
                             uint8_t delivery_mode, bool masked) {
    if (!local_apic) {
        return -1;
    }
    
    volatile uint32_t* lvt_reg = (volatile uint32_t*)((uintptr_t)local_apic + lvt_offset);
    uint32_t value = vector | (delivery_mode << 8);
    
    if (masked) {
        value |= 0x10000; // Mask bit
    }
    
    *lvt_reg = value;
    return 0;
}

/**
 * @brief Set task priority
 */
void x86_apic_set_task_priority(uint8_t priority) {
    if (local_apic) {
        local_apic->task_priority.reg = priority & 0xFF;
    }
}

/**
 * @brief Get task priority
 */
uint8_t x86_apic_get_task_priority(void) {
    if (local_apic) {
        return local_apic->task_priority.reg & 0xFF;
    }
    return 0;
}

/**
 * @brief Read error status
 */
uint32_t x86_apic_read_error_status(void) {
    if (local_apic) {
        return local_apic->error_status.reg;
    }
    return 0;
}

/**
 * @brief Clear error status
 */
void x86_apic_clear_error_status(void) {
    if (local_apic) {
        local_apic->error_status.reg = 0;
    }
}

/**
 * @brief Dump IDT table for debugging
 */
void x86_idt_dump_table(void) {
    printf("IDT Table Dump:\n");
    printf("Base: 0x%lx, Limit: %d\n", idt_ptr.base, idt_ptr.limit);
    
    for (int i = 0; i < 32; i++) {
        idt_gate_desc_t* gate = &idt_table[i];
        if (gate->type_attr & 0x80) { // Present bit
            uintptr_t offset = gate->offset_low | 
                              ((uintptr_t)gate->offset_mid << 16) |
                              ((uintptr_t)gate->offset_high << 32);
            printf("Vector %d: Handler=0x%lx, Selector=0x%x, Type=0x%x\n",
                   i, offset, gate->selector, gate->type_attr & 0x0F);
        }
    }
}

/**
 * @brief Dump APIC registers for debugging
 */
void x86_apic_dump_registers(void) {
    if (!local_apic) {
        printf("APIC not initialized\n");
        return;
    }
    
    printf("APIC Register Dump:\n");
    printf("ID: 0x%x\n", local_apic->id.reg);
    printf("Version: 0x%x\n", local_apic->version.reg);
    printf("Task Priority: 0x%x\n", local_apic->task_priority.reg);
    printf("Spurious Vector: 0x%x\n", local_apic->spurious_vector.reg);
    printf("Error Status: 0x%x\n", local_apic->error_status.reg);
}

/**
 * @brief Get exception name
 */
const char* x86_get_exception_name(uint8_t vector) {
    if (vector < sizeof(exception_names) / sizeof(exception_names[0])) {
        return exception_names[vector];
    }
    return "Unknown Exception";
}

/**
 * @brief Print interrupt context for debugging
 */
void x86_print_interrupt_context(interrupt_context_t* context) {
    printf("Interrupt Context:\n");
    printf("Vector: %d (%s)\n", context->vector, x86_get_exception_name(context->vector));
    printf("RIP: 0x%lx, CS: 0x%x, RFLAGS: 0x%lx\n", 
           context->rip, context->cs, context->rflags);
    printf("RAX: 0x%lx, RBX: 0x%lx, RCX: 0x%lx, RDX: 0x%lx\n",
           context->rax, context->rbx, context->rcx, context->rdx);
    printf("RSI: 0x%lx, RDI: 0x%lx, RBP: 0x%lx, RSP: 0x%lx\n",
           context->rsi, context->rdi, context->rbp, context->rsp);
    
    if (context->privilege_change) {
        printf("Privilege transition: CS 0x%x -> 0x%x\n", 
               context->old_cs, context->cs);
    }
}

/**
 * @brief Common interrupt handler (called from assembly stubs)
 */
void x86_common_interrupt_handler(interrupt_context_t* context) {
    uint8_t vector = context->vector;
    
    // Handle exceptions (0-31)
    if (vector <= EXCEPTION_VECTORS_END) {
        if (exception_handlers[vector]) {
            exception_handlers[vector](context, context->error_code);
        } else {
            printf("Unhandled exception %d: %s\n", vector, x86_get_exception_name(vector));
            x86_print_interrupt_context(context);
        }
    }
    // Handle interrupts (32+)
    else {
        if (interrupt_handlers[vector]) {
            interrupt_handlers[vector](context);
        }
        
        // Send EOI for APIC interrupts
        if (vector >= APIC_VECTORS_START && apic_enabled) {
            x86_apic_eoi();
        }
    }
}
