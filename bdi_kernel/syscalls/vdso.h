
/**
 * @file vdso.h
 * @brief vDSO Interface
 */

#ifndef BDI_VDSO_H
#define BDI_VDSO_H

#include "../process/process.h"
#include <stdint.h>
#include <time.h>

/* vDSO initialization */
[[nodiscard]] int vdso_init_complete(void);
[[nodiscard]] int vdso_map_into_process(ProcessControlBlock *pcb);

/* vDSO update functions */
void vdso_update_time_complete(void);
void vdso_update_process_info(ProcessId pid, ProcessId tid, ProcessId ppid, uint32_t cpu);

/* vDSO fast path implementations */
[[nodiscard]] int64_t vdso_getpid_impl(void);
[[nodiscard]] int64_t vdso_gettid_impl(void);
[[nodiscard]] int64_t vdso_getppid_impl(void);
[[nodiscard]] int64_t vdso_getcpu_impl(uint32_t *cpu, uint32_t *node);
[[nodiscard]] int64_t vdso_time_impl(time_t *tloc);

/* vDSO symbol resolution */
void *vdso_resolve_symbol(const char *name);
void *vdso_get_page_address(void);

#endif /* BDI_VDSO_H */
