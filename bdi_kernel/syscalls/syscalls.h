// ===================================================================
// DESC: Defines the standard Service IDs and Operation Codes for the
//       Aeon kernel's system call interface.
// ===================================================================
#ifndef AEON_SYSCALLS_H
#define AEON_SYSCALLS_H

// --- Standard Service IDs ---
#define SERVICE_PROCESS_MANAGER 1
#define SERVICE_FS              2 // File System
#define SERVICE_HAM             3 // Memory Manager

// --- Operation Codes for the File System Service ---
#define FS_OP_READ   1
#define FS_OP_WRITE  2
#define FS_OP_LOOKUP 3

// --- Operation Codes for the Process Manager Service ---
#define PROC_OP_FORK 1
#define PROC_OP_EXIT 2
#define PROC_OP_WAIT 3

#endif // AEON_SYSCALLS_H
