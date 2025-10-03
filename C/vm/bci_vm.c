// ===================================================================
// DESC: Implementation of the stack-based Virtual Machine.
// ===================================================================

#include "c23_compat.h"
#include "bci_vm.h"
#include <stdio.h>

// --- VM Public API Implementation ---

void vm_init(VM* vm) {
    vm->stack_top = vm->stack;
    vm->chunk = nullptr;
    vm->ip = nullptr;
}

void vm_free(VM* vm) {
    // Nothing to do for now, as VM does not own the chunk.
}

void vm_stack_push(VM* vm, double value) {
    *vm->stack_top = value;
    vm->stack_top++;
}

double vm_stack_pop(VM* vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

// --- Core VM Execution Logic ---

// The main execution loop for the VM.
static InterpretResult run(VM* vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.data[READ_BYTE()])

// Macro to handle binary operations to reduce code duplication.
#define BINARY_OP(op) \
    do { \
        double b = vm_stack_pop(vm); \
        double a = vm_stack_pop(vm); \
        vm_stack_push(vm, a op b); \
    } while (false)


    for (;;) {
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                printf("Result: %g\n", vm_stack_pop(vm));
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                double constant = READ_CONSTANT();
                vm_stack_push(vm, constant);
                break;
            }
            case OP_NEGATE: {
                vm_stack_push(vm, -vm_stack_pop(vm));
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            
            default:
                // Handle unknown opcode
                return INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult vm_interpret(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = vm->chunk->code;
    return run(vm);
}
