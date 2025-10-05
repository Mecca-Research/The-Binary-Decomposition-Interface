
// BTL Register Allocator - Linear scan register allocation
/**
 * @file btl_regalloc.h
 * @brief Btl Regalloc API
 * @details This file provides the btl regalloc functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BTL_REGALLOC_H
#define BTL_REGALLOC_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Register types
typedef enum {
    BTL_REG_GENERAL,
    BTL_REG_FLOAT,
    BTL_REG_VECTOR,
    BTL_REG_SPECIAL
} BTL_RegisterType;

// Register descriptor
typedef struct {
    uint8_t id;
    BTL_RegisterType type;
    bool allocated;
    uint32_t live_start;
    uint32_t live_end;
    const char *name;
} BTL_Register;

// Live interval for a variable
typedef struct {
    uint32_t var_id;
    uint32_t start;
    uint32_t end;
    int assigned_register;
    bool spilled;
    uint32_t spill_location;
} BTL_LiveInterval;

// Register allocator context
typedef struct BTL_RegAllocator BTL_RegAllocator;

// Create and destroy allocator
BTL_RegAllocator* btl_regalloc_create(size_t num_registers, BTL_RegisterType type);
void btl_regalloc_destroy(BTL_RegAllocator *allocator);

// Register operations
NODISCARD int btl_regalloc_acquire(BTL_RegAllocator *allocator, uint32_t var_id, 
                                    uint32_t start, uint32_t end);
void btl_regalloc_release(BTL_RegAllocator *allocator, int reg_id);
NODISCARD bool btl_regalloc_is_available(BTL_RegAllocator *allocator, int reg_id);

// Live interval management
void btl_regalloc_add_interval(BTL_RegAllocator *allocator, uint32_t var_id,
                                uint32_t start, uint32_t end);
NODISCARD const BTL_LiveInterval* btl_regalloc_get_interval(BTL_RegAllocator *allocator,
                                                              uint32_t var_id);

// Linear scan allocation
bool btl_regalloc_linear_scan(BTL_RegAllocator *allocator);

// Spilling
NODISCARD bool btl_regalloc_needs_spill(BTL_RegAllocator *allocator);
uint32_t btl_regalloc_get_spill_count(BTL_RegAllocator *allocator);

// Statistics
size_t btl_regalloc_get_allocated_count(BTL_RegAllocator *allocator);
size_t btl_regalloc_get_free_count(BTL_RegAllocator *allocator);

// Register naming (architecture-specific)
const char* btl_regalloc_get_register_name(int reg_id, BTL_RegisterType type);

#endif // BTL_REGALLOC_H
