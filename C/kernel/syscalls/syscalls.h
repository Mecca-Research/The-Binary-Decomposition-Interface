// ===================================================================
// DESC: Defines the standard Service IDs and Operation Codes for the
//       Aeon kernel's system call interface.
// ===================================================================
#include "c23_compat.h"
/**
 * @file syscalls.h
 * @brief Syscalls API
 * @details This file provides the syscalls functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
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

// Compile-time invariants
static_assert(sizeof(int) >= 4, "Syscall codes require at least 32-bit int");

#endif // AEON_SYSCALLS_H
