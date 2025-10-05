// Binary Counting Interface (BCI)
/**
 * @file chimera_bci.h
 * @brief Chimera Bci API
 * @details This file provides the chimera bci functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef CHIMERA_BCI_H
#define CHIMERA_BCI_H


#include "c23_compat.h"
#include <stdint.h>
#include <stdio.h>


// Print binary representation of an integer
void print_binary(uint32_t value);


// Print binary representation of a float (IEEE-754)
void print_binary_float(float f);



// Compile-time invariants
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(void*) >= 4, "BCI requires at least 32-bit pointers");
_Static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
#endif

#endif // CHIMERA_BCI_H
