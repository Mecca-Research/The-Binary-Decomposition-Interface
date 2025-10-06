
/**
 * @file syscall_trace.h
 * @brief System Call Tracing Interface
 */

#ifndef BDI_SYSCALL_TRACE_H
#define BDI_SYSCALL_TRACE_H

#include "../syscalls/syscalls.h"
#include <stdint.h>
#include <stdbool.h>

/* Trace initialization */
[[nodiscard]] int syscall_trace_init(void);

/* Trace entry/exit */
void syscall_trace_entry(uint32_t syscall_num, const syscall_args_t *args);
void syscall_trace_exit(uint32_t syscall_num, int64_t result);

/* Trace control */
void syscall_trace_enable(void);
void syscall_trace_disable(void);
void syscall_trace_set_filter(uint32_t syscall_num, bool enable);
void syscall_trace_set_sample_rate(uint32_t rate);

/* Trace export */
void syscall_trace_print(void);
void syscall_trace_clear(void);

#endif /* BDI_SYSCALL_TRACE_H */
