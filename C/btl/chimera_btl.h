// Boolean Translation Layer (BTL)
#ifndef CHIMERA_BTL_H
#define CHIMERA_BTL_H


#include "c23_compat.h"
#include <stdint.h>


const char* get_isa_mnemonic(uint8_t opcode);
const char* decode_flag_state(uint8_t flags);



// Compile-time invariants
static_assert(sizeof(void*) >= 4, "BTL requires at least 32-bit pointers");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");

#endif // CHIMERA_BTL_H
