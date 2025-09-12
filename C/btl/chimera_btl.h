// Boolean Translation Layer (BTL)
#ifndef CHIMERA_BTL_H
#define CHIMERA_BTL_H


#include <stdint.h>


const char* get_isa_mnemonic(uint8_t opcode);
const char* decode_flag_state(uint8_t flags);


#endif // CHIMERA_BTL_H
