#include "chimera_btl.h"


const char* get_isa_mnemonic(uint8_t opcode) {
switch (opcode) {
case 0x01: return "ADD";
case 0x29: return "SUB";
case 0x31: return "XOR";
case 0x39: return "CMP";
case 0x74: return "JE";
default: return "UNKNOWN";
}
}


const char* decode_flag_state(uint8_t flags) {
if (flags & 0b0100) return "ZF (Zero Flag Set)";
if (flags & 0b0001) return "CF (Carry Flag Set)";
if (flags & 0b1000) return "SF (Sign Flag Set)";
return "No Relevant Flags Set";
}
