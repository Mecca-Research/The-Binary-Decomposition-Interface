// Boolean Translation Layer (BTL)
#ifndef CHIMERA_BTL_H
#define CHIMERA_BTL_H


#include <stdint.h>


const char* get_isa_mnemonic(uint8_t opcode);
const char* decode_flag_state(uint8_t flags);


#endif // CHIMERA_BTL_H


// ----------- SOURCE: chimera_bci.c -------------
#include "chimera_bci.h"


void print_binary(uint32_t value) {
for (int i = 31; i >= 0; i--) {
printf("%d", (value >> i) & 1);
if (i % 8 == 0) printf(" ");
}
printf("\n");
}


void print_binary_float(float f) {
union {
float f;
uint32_t i;
} u;
u.f = f;
print_binary(u.i);
}
