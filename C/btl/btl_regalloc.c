
// BTL Register Allocator Implementation
#include "btl_regalloc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_INTERVALS 1024

struct BTL_RegAllocator {
    BTL_Register *registers;
    size_t num_registers;
    BTL_RegisterType type;
    
    BTL_LiveInterval *intervals;
    size_t num_intervals;
    size_t intervals_capacity;
    
    uint32_t spill_count;
    uint32_t next_spill_location;
};

// x86-64 register names
static const char* x86_64_general_regs[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

static const char* x86_64_float_regs[] = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
};

BTL_RegAllocator* btl_regalloc_create(size_t num_registers, BTL_RegisterType type) {
    BTL_RegAllocator *allocator = malloc(sizeof(BTL_RegAllocator));
    if (!allocator) return NULL;
    
    allocator->registers = calloc(num_registers, sizeof(BTL_Register));
    if (!allocator->registers) {
        free(allocator);
        return NULL;
    }
    
    allocator->num_registers = num_registers;
    allocator->type = type;
    
    // Initialize registers
    for (size_t i = 0; i < num_registers; i++) {
        allocator->registers[i].id = i;
        allocator->registers[i].type = type;
        allocator->registers[i].allocated = false;
        allocator->registers[i].live_start = 0;
        allocator->registers[i].live_end = 0;
        allocator->registers[i].name = btl_regalloc_get_register_name(i, type);
    }
    
    // Initialize intervals
    allocator->intervals_capacity = MAX_INTERVALS;
    allocator->intervals = calloc(MAX_INTERVALS, sizeof(BTL_LiveInterval));
    if (!allocator->intervals) {
        free(allocator->registers);
        free(allocator);
        return NULL;
    }
    allocator->num_intervals = 0;
    
    allocator->spill_count = 0;
    allocator->next_spill_location = 0;
    
    return allocator;
}

void btl_regalloc_destroy(BTL_RegAllocator *allocator) {
    if (!allocator) return;
    free(allocator->registers);
    free(allocator->intervals);
    free(allocator);
}

int btl_regalloc_acquire(BTL_RegAllocator *allocator, uint32_t var_id,
                         uint32_t start, uint32_t end) {
    if (!allocator) return -1;
    
    // Find a free register
    for (size_t i = 0; i < allocator->num_registers; i++) {
        if (!allocator->registers[i].allocated) {
            allocator->registers[i].allocated = true;
            allocator->registers[i].live_start = start;
            allocator->registers[i].live_end = end;
            
            // NOTE: Do NOT add interval here during linear scan.
            // Intervals should already exist before the scan begins.
            // Adding intervals during scan causes the loop to process
            // newly appended entries, leading to infinite growth.
            
            return (int)i;
        }
    }
    
    return -1; // No free register
}

void btl_regalloc_release(BTL_RegAllocator *allocator, int reg_id) {
    if (!allocator || reg_id < 0 || (size_t)reg_id >= allocator->num_registers) return;
    
    allocator->registers[reg_id].allocated = false;
    allocator->registers[reg_id].live_start = 0;
    allocator->registers[reg_id].live_end = 0;
}

bool btl_regalloc_is_available(BTL_RegAllocator *allocator, int reg_id) {
    if (!allocator || reg_id < 0 || (size_t)reg_id >= allocator->num_registers) return false;
    return !allocator->registers[reg_id].allocated;
}

void btl_regalloc_add_interval(BTL_RegAllocator *allocator, uint32_t var_id,
                                uint32_t start, uint32_t end) {
    if (!allocator || allocator->num_intervals >= allocator->intervals_capacity) return;
    
    BTL_LiveInterval *interval = &allocator->intervals[allocator->num_intervals++];
    interval->var_id = var_id;
    interval->start = start;
    interval->end = end;
    interval->assigned_register = -1;
    interval->spilled = false;
    interval->spill_location = 0;
}

const BTL_LiveInterval* btl_regalloc_get_interval(BTL_RegAllocator *allocator,
                                                    uint32_t var_id) {
    if (!allocator) return NULL;
    
    for (size_t i = 0; i < allocator->num_intervals; i++) {
        if (allocator->intervals[i].var_id == var_id) {
            return &allocator->intervals[i];
        }
    }
    
    return NULL;
}

// Comparison function for sorting intervals by start point
static int compare_intervals(const void *a, const void *b) {
    const BTL_LiveInterval *ia = (const BTL_LiveInterval*)a;
    const BTL_LiveInterval *ib = (const BTL_LiveInterval*)b;
    return (int)ia->start - (int)ib->start;
}

// Linear scan register allocation algorithm
bool btl_regalloc_linear_scan(BTL_RegAllocator *allocator) {
    if (!allocator || allocator->num_intervals == 0) return false;
    
    // Sort intervals by start point
    qsort(allocator->intervals, allocator->num_intervals, 
          sizeof(BTL_LiveInterval), compare_intervals);
    
    // Active list (intervals currently using registers)
    BTL_LiveInterval *active[256];
    size_t active_count = 0;
    
    // Process each interval
    for (size_t i = 0; i < allocator->num_intervals; i++) {
        BTL_LiveInterval *current = &allocator->intervals[i];
        
        // Expire old intervals
        for (size_t j = 0; j < active_count; ) {
            if (active[j]->end <= current->start) {
                // Release register
                if (active[j]->assigned_register >= 0) {
                    btl_regalloc_release(allocator, active[j]->assigned_register);
                }
                // Remove from active list
                active[j] = active[--active_count];
            } else {
                j++;
            }
        }
        
        // Try to allocate a register
        int reg = btl_regalloc_acquire(allocator, current->var_id, 
                                        current->start, current->end);
        
        if (reg >= 0) {
            current->assigned_register = reg;
            active[active_count++] = current;
        } else {
            // Spill: no free register
            current->spilled = true;
            current->spill_location = allocator->next_spill_location++;
            allocator->spill_count++;
        }
    }
    
    return true;
}

bool btl_regalloc_needs_spill(BTL_RegAllocator *allocator) {
    return allocator && allocator->spill_count > 0;
}

uint32_t btl_regalloc_get_spill_count(BTL_RegAllocator *allocator) {
    return allocator ? allocator->spill_count : 0;
}

size_t btl_regalloc_get_allocated_count(BTL_RegAllocator *allocator) {
    if (!allocator) return 0;
    
    size_t count = 0;
    for (size_t i = 0; i < allocator->num_registers; i++) {
        if (allocator->registers[i].allocated) count++;
    }
    return count;
}

size_t btl_regalloc_get_free_count(BTL_RegAllocator *allocator) {
    if (!allocator) return 0;
    return allocator->num_registers - btl_regalloc_get_allocated_count(allocator);
}

const char* btl_regalloc_get_register_name(int reg_id, BTL_RegisterType type) {
    if (reg_id < 0) return "INVALID";
    
    switch (type) {
        case BTL_REG_GENERAL:
            if (reg_id < 16) return x86_64_general_regs[reg_id];
            break;
        case BTL_REG_FLOAT:
        case BTL_REG_VECTOR:
            if (reg_id < 16) return x86_64_float_regs[reg_id];
            break;
        default:
            break;
    }
    
    return "UNKNOWN";
}
