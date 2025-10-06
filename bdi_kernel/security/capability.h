
/**
 * @file capability.h
 * @brief Capability-Based Security Interface
 */

#ifndef BDI_CAPABILITY_H
#define BDI_CAPABILITY_H

#include "../process/process.h"
#include <stdint.h>
#include <stdbool.h>

/* Capability initialization */
[[nodiscard]] int capability_init(void);

/* Capability checking */
[[nodiscard]] bool capability_check_syscall(uint32_t syscall_num);

/* Capability auditing */
void capability_audit(uint32_t syscall_num, bool allowed);
void capability_audit_enable(void);
void capability_audit_disable(void);

/* Capability management */
[[nodiscard]] int capability_grant(ProcessControlBlock *pcb, uint64_t cap);
[[nodiscard]] int capability_revoke(ProcessControlBlock *pcb, uint64_t cap);
[[nodiscard]] int capability_inherit(ProcessControlBlock *parent, ProcessControlBlock *child);

#endif /* BDI_CAPABILITY_H */
