// Binary Counting Interface (BCI)
#ifndef CHIMERA_BCI_H
#define CHIMERA_BCI_H


#include <stdint.h>
#include <stdio.h>


// Print binary representation of an integer
void print_binary(uint32_t value);


// Print binary representation of a float (IEEE-754)
void print_binary_float(float f);


#endif // CHIMERA_BCI_H


// ----------- HEADER: chimera_btl.h -------------
#ifndef CHIMERA_BTL_H
#define CHIMERA_BTL_H


#include <stdint.h>


const char* get_isa_mnemonic(uint8_t opcode);
const char* decode_flag_state(uint8_t flags);


#endif // CHIMERA_BTL_H
