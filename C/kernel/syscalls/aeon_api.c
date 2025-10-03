// ===================================================================
// DESC: Implements the user-facing C API for Aeon system services.
//       These functions wrap the low-level BDI OS_SERVICE_CALL mechanism.
// ===================================================================
#include "c23_compat.h"
#include "syscalls.h"
#include <stdint.h>
#include <stdio.h>

// This is a placeholder for the actual BDI instruction or inline assembly
// that would trigger the OS_SERVICE_CALL.
static uint64_t bdi_os_service_call(uint64_t service_id, uint64_t op_code, void* args) {
    printf("AEON_API: Triggering OS_SERVICE_CALL -> Service: %llu, Op: %llu\n",
           (unsigned long long)service_id, (unsigned long long)op_code);
    // In a real implementation, this would trap into the BDIVM/kernel.
    // The VM would then execute the corresponding service graph.
    // We'll return a dummy success value.
    return 0;
}

// --- Public API Functions ---

uint64_t aeon_read(int fd, void* buf, size_t count) {
    // Arguments would be packed into a struct or array for the service call.
    void* args[] = {&fd, buf, &count};
    return bdi_os_service_call(SERVICE_FS, FS_OP_READ, args);
}

uint64_t aeon_fork() {
    return bdi_os_service_call(SERVICE_PROCESS_MANAGER, PROC_OP_FORK, nullptr);
}
