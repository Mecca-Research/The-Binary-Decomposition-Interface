// ===================================================================
// DESC: Defines the Virtual Machine (VM) that executes bytecode.
// ===================================================================
#ifndef BCI_VM_H
#define BCI_VM_H

#include "bci_chunk.h"

#define STACK_MAX 256

// --- Interpret Result Enumeration ---
// Represents the possible outcomes after the VM runs.
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;


// --- Virtual Machine Structure ---
// The state of the virtual machine.
typedef struct {
    Chunk* chunk;               // The chunk of bytecode to be executed.
    uint8_t* ip;                // Instruction Pointer: points to the next instruction to execute.
    double stack[STACK_MAX];    // The value stack.
    double* stack_top;          // Points to the next available slot in the stack.
} VM;


// --- VM Public API ---

// Initializes the virtual machine.
void vm_init(VM* vm);

// Frees all resources used by the virtual machine.
void vm_free(VM* vm);

// The main entry point to run the VM. It takes a chunk of bytecode
// and executes it. Returns the result of the interpretation.
InterpretResult vm_interpret(VM* vm, Chunk* chunk);

// Stack operations (for internal use, but declared for modularity).
void vm_stack_push(VM* vm, double value);
double vm_stack_pop(VM* vm);

#endif // BCI_VM_H
