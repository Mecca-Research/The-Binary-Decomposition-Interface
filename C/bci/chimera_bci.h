// Binary Counting Interface (BCI)
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
static_assert(sizeof(void*) >= 4, "BCI requires at least 32-bit pointers");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // CHIMERA_BCI_H
