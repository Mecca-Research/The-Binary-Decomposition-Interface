// ===================================================================
// DESC: Defines the "Chunk," which is a container for bytecode,
//       and the Opcodes for the virtual machine.
// ===================================================================
#ifndef BCI_CHUNK_H
#define BCI_CHUNK_H

#include "bci_types.h"

// --- Opcode Enumeration ---
// These are the instructions for our stack-based virtual machine.
typedef enum {
    OP_RETURN,      // Return from the current function.
    OP_CONSTANT,    // Push a constant value onto the stack.
    OP_NEGATE,      // Negate the top of the stack.
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
} OpCode;


// --- Value Array ---
// A dynamic array for storing constant values.
typedef BciVec(double) ValueArray; // For now, our VM will use doubles.

// --- Chunk Structure ---
// A dynamic array of bytecode instructions.
typedef struct {
    int count;          // Number of bytes in use.
    int capacity;       // Number of bytes allocated.
    uint8_t* code;      // The bytecode instructions.
    int* lines;         // Line number corresponding to each byte of code.
    ValueArray constants; // Pool of constant values.
} Chunk;


// --- Chunk Public API ---

// Initializes a new, empty chunk.
void chunk_init(Chunk* chunk);

// Frees all memory associated with a chunk.
void chunk_free(Chunk* chunk);

// Appends a byte (an opcode or an operand) to the end of the chunk.
void chunk_write(Chunk* chunk, uint8_t byte, int line);

// Adds a constant to the chunk's constant pool and returns its index.
int chunk_add_constant(Chunk* chunk, double value);

#endif // BCI_CHUNK_H
