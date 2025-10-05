// ===================================================================
// DESC: Defines the Virtual Machine (VM) that executes bytecode.
// ===================================================================
/**
 * @file bci_vm.h
 * @brief BCI Virtual Machine Implementation
 * @details This file provides the bci vm functionality for the BDI virtual machine execution environment.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BCI_VM_H
#define BCI_VM_H

#include "c23_compat.h"
#include "bci_chunk.h"

#define STACK_MAX 256

// --- Interpret Result Enumeration ---
// Represents the possible outcomes after the VM runs.
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// --- VM Execution Result Structure ---
// Contains the result of VM execution including status and return value.
typedef struct {
    InterpretResult status;     // Execution status (OK, error, etc.)
    double result_value;        // The computed result value (if any)
} BciVmResult;


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
// DEPRECATED: Use vm_interpret_with_result() for new code.
InterpretResult vm_interpret(VM* vm, Chunk* chunk);

// Enhanced VM entry point that returns both status and result value.
// This is the preferred method for capturing execution results.
BciVmResult vm_interpret_with_result(VM* vm, Chunk* chunk);

// Stack operations (for internal use, but declared for modularity).
void vm_stack_push(VM* vm, double value);
double vm_stack_pop(VM* vm);

// Reset VM state for reuse
void vm_reset(VM* vm);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "VM requires at least 32-bit pointers");
static_assert(sizeof(double) == 8, "VM requires 64-bit doubles");
static_assert(sizeof(InterpretResult) == sizeof(int), "InterpretResult must be int-sized");

#endif // BCI_VM_H
