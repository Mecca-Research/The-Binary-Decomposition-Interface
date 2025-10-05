// Boolean Translation Layer (BTL)
/**
 * @file chimera_btl.h
 * @brief Chimera Btl API
 * @details This file provides the chimera btl functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef CHIMERA_BTL_H
#define CHIMERA_BTL_H


#include "c23_compat.h"
#include <stdint.h>


const char* get_isa_mnemonic(uint8_t opcode);
const char* decode_flag_state(uint8_t flags);



// Compile-time invariants
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(void*) >= 4, "BTL requires at least 32-bit pointers");
_Static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
#endif

#endif // CHIMERA_BTL_H
