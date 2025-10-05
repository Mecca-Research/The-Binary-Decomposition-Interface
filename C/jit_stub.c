// Stub implementations for JIT functions to avoid linking errors
#include "vm/vm_jit_integration.h"

JITIntegratedVM* jit_vm_create(size_t heap_size) {
    (void)heap_size;  // Unused parameter
    return NULL;  // Stub implementation
}

void jit_vm_destroy(JITIntegratedVM* vm) {
    (void)vm;  // Stub implementation
}
