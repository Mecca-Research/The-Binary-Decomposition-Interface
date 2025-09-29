
/**
 * @file x86_registers.c
 * @brief x86 Register Management Implementation
 * 
 * Implementation of comprehensive x86 register management system
 * providing register allocation, context switching, and state management.
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "x86_registers.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool g_x86_registers_initialized = false;
static x86_reg_alloc_entry_t g_reg_alloc_table32[X86_REG_COUNT_32];
static x86_reg_alloc_entry_t g_reg_alloc_table64[X86_REG_COUNT_64];
static uint64_t g_allocation_counter = 0;

// =============================================================================
// REGISTER NAME TABLES
// =============================================================================

static const char *reg_names_32bit[] = {
    [X86_REG_EAX] = "EAX",
    [X86_REG_EBX] = "EBX", 
    [X86_REG_ECX] = "ECX",
    [X86_REG_EDX] = "EDX",
    [X86_REG_ESI] = "ESI",
    [X86_REG_EDI] = "EDI",
    [X86_REG_ESP] = "ESP",
    [X86_REG_EBP] = "EBP"
};

static const char *reg_names_64bit[] = {
    [X86_REG_RAX] = "RAX",
    [X86_REG_RBX] = "RBX",
    [X86_REG_RCX] = "RCX", 
    [X86_REG_RDX] = "RDX",
    [X86_REG_RSI] = "RSI",
    [X86_REG_RDI] = "RDI",
    [X86_REG_RSP] = "RSP",
    [X86_REG_RBP] = "RBP",
    [X86_REG_R8]  = "R8",
    [X86_REG_R9]  = "R9",
    [X86_REG_R10] = "R10",
    [X86_REG_R11] = "R11",
    [X86_REG_R12] = "R12",
    [X86_REG_R13] = "R13",
    [X86_REG_R14] = "R14",
    [X86_REG_R15] = "R15"
};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static void x86_init_register_table(void);
static bool x86_is_register_reserved(int reg_id);
static uint64_t x86_get_timestamp(void);

// =============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =============================================================================

int x86_registers_initialize(void)
{
    if (g_x86_registers_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize register allocation tables
    x86_init_register_table();
    
    // Reset allocation counter
    g_allocation_counter = 0;
    
    g_x86_registers_initialized = true;
    return 0;
}

int x86_registers_shutdown(void)
{
    if (!g_x86_registers_initialized) {
        return -1; // Not initialized
    }
    
    // Clear all allocations
    memset(g_reg_alloc_table32, 0, sizeof(g_reg_alloc_table32));
    memset(g_reg_alloc_table64, 0, sizeof(g_reg_alloc_table64));
    
    g_x86_registers_initialized = false;
    return 0;
}

int x86_save_context32(x86_context32_t *context)
{
    if (!g_x86_registers_initialized || context == NULL) {
        return -1;
    }
    
    // Note: In a real implementation, this would use inline assembly
    // to save the actual register values. This is a placeholder.
    memset(context, 0, sizeof(x86_context32_t));
    
    // Placeholder implementation - would use inline assembly like:
    // __asm__ volatile ("movl %%eax, %0" : "=m" (context->eax));
    // __asm__ volatile ("movl %%ebx, %0" : "=m" (context->ebx));
    // ... etc for all registers
    
    return 0;
}

int x86_restore_context32(const x86_context32_t *context)
{
    if (!g_x86_registers_initialized || context == NULL) {
        return -1;
    }
    
    // Note: In a real implementation, this would use inline assembly
    // to restore the actual register values. This is a placeholder.
    
    // Placeholder implementation - would use inline assembly like:
    // __asm__ volatile ("movl %0, %%eax" : : "m" (context->eax));
    // __asm__ volatile ("movl %0, %%ebx" : : "m" (context->ebx));
    // ... etc for all registers
    
    return 0;
}

int x86_save_context64(x86_context64_t *context)
{
    if (!g_x86_registers_initialized || context == NULL) {
        return -1;
    }
    
    // Note: In a real implementation, this would use inline assembly
    // to save the actual register values. This is a placeholder.
    memset(context, 0, sizeof(x86_context64_t));
    
    return 0;
}

int x86_restore_context64(const x86_context64_t *context)
{
    if (!g_x86_registers_initialized || context == NULL) {
        return -1;
    }
    
    // Note: In a real implementation, this would use inline assembly
    // to restore the actual register values. This is a placeholder.
    
    return 0;
}

int x86_allocate_register(int reg_type, uint32_t owner_id, const char *description)
{
    if (!g_x86_registers_initialized) {
        return -1;
    }
    
    x86_reg_alloc_entry_t *table;
    int table_size;
    
    // Select appropriate table based on register type
    if (reg_type == 32) {
        table = g_reg_alloc_table32;
        table_size = X86_REG_COUNT_32;
    } else if (reg_type == 64) {
        table = g_reg_alloc_table64;
        table_size = X86_REG_COUNT_64;
    } else {
        return -1; // Invalid register type
    }
    
    // Find first available register
    for (int i = 0; i < table_size; i++) {
        if (table[i].status == X86_REG_FREE && !x86_is_register_reserved(i)) {
            // Allocate the register
            table[i].status = X86_REG_ALLOCATED;
            table[i].owner_id = owner_id;
            table[i].timestamp = x86_get_timestamp();
            table[i].description = description;
            
            return i; // Return register ID
        }
    }
    
    return -1; // No available registers
}

int x86_free_register(int reg_id, uint32_t owner_id)
{
    if (!g_x86_registers_initialized) {
        return -1;
    }
    
    // Check 32-bit table first
    if (reg_id >= 0 && reg_id < X86_REG_COUNT_32) {
        if (g_reg_alloc_table32[reg_id].status == X86_REG_ALLOCATED &&
            g_reg_alloc_table32[reg_id].owner_id == owner_id) {
            
            // Free the register
            g_reg_alloc_table32[reg_id].status = X86_REG_FREE;
            g_reg_alloc_table32[reg_id].owner_id = 0;
            g_reg_alloc_table32[reg_id].timestamp = 0;
            g_reg_alloc_table32[reg_id].description = NULL;
            
            return 0;
        }
    }
    
    // Check 64-bit table
    if (reg_id >= 0 && reg_id < X86_REG_COUNT_64) {
        if (g_reg_alloc_table64[reg_id].status == X86_REG_ALLOCATED &&
            g_reg_alloc_table64[reg_id].owner_id == owner_id) {
            
            // Free the register
            g_reg_alloc_table64[reg_id].status = X86_REG_FREE;
            g_reg_alloc_table64[reg_id].owner_id = 0;
            g_reg_alloc_table64[reg_id].timestamp = 0;
            g_reg_alloc_table64[reg_id].description = NULL;
            
            return 0;
        }
    }
    
    return -1; // Invalid register ID or owner mismatch
}

const x86_reg_alloc_entry_t *x86_get_register_status(int reg_id)
{
    if (!g_x86_registers_initialized) {
        return NULL;
    }
    
    // Check 32-bit table first
    if (reg_id >= 0 && reg_id < X86_REG_COUNT_32) {
        return &g_reg_alloc_table32[reg_id];
    }
    
    // Check 64-bit table
    if (reg_id >= 0 && reg_id < X86_REG_COUNT_64) {
        return &g_reg_alloc_table64[reg_id];
    }
    
    return NULL;
}

bool x86_is_register_volatile(int reg_id)
{
    // Based on x86 calling conventions:
    // Volatile (caller-saved): EAX, ECX, EDX, R8-R11
    // Non-volatile (callee-saved): EBX, ESI, EDI, EBP, R12-R15
    // Reserved: ESP, RSP
    
    switch (reg_id) {
        case X86_REG_EAX:
        case X86_REG_ECX: 
        case X86_REG_EDX:
        case X86_REG_R8:
        case X86_REG_R9:
        case X86_REG_R10:
        case X86_REG_R11:
            return true;
            
        default:
            return false;
    }
}

const char *x86_get_register_name(int reg_id, bool is_64bit)
{
    if (is_64bit) {
        if (reg_id >= 0 && reg_id < X86_REG_COUNT_64) {
            return reg_names_64bit[reg_id];
        }
    } else {
        if (reg_id >= 0 && reg_id < X86_REG_COUNT_32) {
            return reg_names_32bit[reg_id];
        }
    }
    
    return NULL;
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static void x86_init_register_table(void)
{
    // Initialize 32-bit register table
    for (int i = 0; i < X86_REG_COUNT_32; i++) {
        g_reg_alloc_table32[i].status = X86_REG_FREE;
        g_reg_alloc_table32[i].owner_id = 0;
        g_reg_alloc_table32[i].timestamp = 0;
        g_reg_alloc_table32[i].description = NULL;
        
        // Mark reserved registers
        if (x86_is_register_reserved(i)) {
            g_reg_alloc_table32[i].status = X86_REG_RESERVED;
        }
        
        // Mark volatile registers
        if (x86_is_register_volatile(i)) {
            g_reg_alloc_table32[i].status = X86_REG_VOLATILE;
        }
    }
    
    // Initialize 64-bit register table
    for (int i = 0; i < X86_REG_COUNT_64; i++) {
        g_reg_alloc_table64[i].status = X86_REG_FREE;
        g_reg_alloc_table64[i].owner_id = 0;
        g_reg_alloc_table64[i].timestamp = 0;
        g_reg_alloc_table64[i].description = NULL;
        
        // Mark reserved registers
        if (x86_is_register_reserved(i)) {
            g_reg_alloc_table64[i].status = X86_REG_RESERVED;
        }
        
        // Mark volatile registers
        if (x86_is_register_volatile(i)) {
            g_reg_alloc_table64[i].status = X86_REG_VOLATILE;
        }
    }
}

static bool x86_is_register_reserved(int reg_id)
{
    // ESP/RSP and EBP/RBP are typically reserved for stack management
    return (reg_id == X86_REG_ESP || reg_id == X86_REG_EBP ||
            reg_id == X86_REG_RSP || reg_id == X86_REG_RBP);
}

static uint64_t x86_get_timestamp(void)
{
    return ++g_allocation_counter;
}
