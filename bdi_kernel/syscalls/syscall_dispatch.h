
/**
 * @file syscall_dispatch.h
 * @brief System Call Dispatch Interface
 */

#ifndef BDI_SYSCALL_DISPATCH_H
#define BDI_SYSCALL_DISPATCH_H

#include "syscalls.h"
#include <stdint.h>
#include <stdbool.h>

/* Syscall entry/exit functions */
[[nodiscard]] int64_t syscall_entry(uint32_t syscall_num, const syscall_args_t *args);
void syscall_exit(int64_t result);

/* 32-bit syscall support */
[[nodiscard]] int32_t syscall_entry_32(uint32_t syscall_num, const uint32_t *args32);

/* Syscall restart mechanism */
[[nodiscard]] bool syscall_should_restart(void);
[[nodiscard]] int64_t syscall_restart(void);
[[nodiscard]] uint32_t syscall_get_current(void);

#endif /* BDI_SYSCALL_DISPATCH_H */
