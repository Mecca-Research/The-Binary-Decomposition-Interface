
/**
 * @file ipi.h
 * @brief Inter-processor interrupts for scheduler
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 * 
 * This header defines IPI (Inter-Processor Interrupt) support for the
 * scheduler, enabling cross-CPU operations like remote task wakeup and
 * forced rescheduling.
 * 
 * Key Features:
 * - Remote task wakeup
 * - Scheduler IPI for preemption
 * - TLB shootdown for scheduler
 * - IPI batching for efficiency
 */

#ifndef BDI_IPI_H
#define BDI_IPI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"
#include "task.h"

/* IPI constants */
#define IPI_MAX_BATCH           32      /* Maximum IPIs in batch */
#define IPI_VECTOR_BASE         0xF0    /* Base vector for IPIs */

/**
 * @brief IPI types
 */
enum ipi_type {
    IPI_RESCHEDULE = 0,     /* Force reschedule on target CPU */
    IPI_WAKEUP,             /* Wake up specific task */
    IPI_TLB_FLUSH,          /* Flush TLB entries */
    IPI_STOP,               /* Stop CPU */
    IPI_CUSTOM              /* Custom IPI (user-defined) */
};

/**
 * @brief IPI data for wakeup
 */
struct ipi_wakeup_data {
    struct task *task;      /* Task to wake up */
};

/**
 * @brief IPI data for TLB flush
 */
struct ipi_tlb_flush_data {
    uint64_t start_addr;    /* Start address to flush */
    uint64_t end_addr;      /* End address to flush */
};

/**
 * @brief IPI message
 */
struct ipi_message {
    enum ipi_type type;     /* IPI type */
    uint32_t source_cpu;    /* Source CPU ID */
    uint32_t target_cpu;    /* Target CPU ID */
    
    /* IPI-specific data */
    union {
        struct ipi_wakeup_data wakeup;
        struct ipi_tlb_flush_data tlb_flush;
        void *custom_data;
    } data;
};

/**
 * @brief IPI batch for efficient sending
 */
struct ipi_batch {
    struct ipi_message messages[IPI_MAX_BATCH];
    uint32_t count;
    bool active;
};

/**
 * @brief Per-CPU IPI state
 */
struct cpu_ipi_state {
    /* Pending IPI flags (atomic) */
    _Atomic uint32_t pending_ipis;
    
    /* IPI statistics */
    _Atomic uint64_t ipis_sent;
    _Atomic uint64_t ipis_received;
    _Atomic uint64_t reschedule_ipis;
    _Atomic uint64_t wakeup_ipis;
    _Atomic uint64_t tlb_flush_ipis;
    
    /* CPU ID */
    uint32_t cpu_id;
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(_Atomic uint32_t) + 
                          sizeof(_Atomic uint64_t) * 5 + 
                          sizeof(uint32_t)) % 64)];
} __attribute__((aligned(64)));

/* Per-CPU IPI state */
extern struct cpu_ipi_state g_cpu_ipi_states[256];

/* Thread-local IPI batch */
extern _Thread_local struct ipi_batch g_ipi_batch;

/**
 * @brief Initialize IPI subsystem
 */
void ipi_init(void);

/**
 * @brief Send IPI to specific CPU
 * 
 * @param cpu_id Target CPU ID
 * @param type IPI type
 * @param data IPI-specific data (can be NULL)
 */
void send_ipi(uint32_t cpu_id, enum ipi_type type, void *data);

/**
 * @brief Send IPI to all CPUs except current
 * 
 * @param type IPI type
 * @param data IPI-specific data (can be NULL)
 */
void send_ipi_all_but_self(enum ipi_type type, void *data);

/**
 * @brief Send IPI to all CPUs
 * 
 * @param type IPI type
 * @param data IPI-specific data (can be NULL)
 */
void send_ipi_all(enum ipi_type type, void *data);

/**
 * @brief Start IPI batch
 * 
 * Begins batching IPIs for efficient sending.
 */
void ipi_batch_start(void);

/**
 * @brief Add IPI to batch
 * 
 * @param cpu_id Target CPU ID
 * @param type IPI type
 * @param data IPI-specific data (can be NULL)
 * @return 0 on success, -1 if batch is full
 */
int ipi_batch_add(uint32_t cpu_id, enum ipi_type type, void *data);

/**
 * @brief Send all batched IPIs
 * 
 * Sends all IPIs in the current batch and clears the batch.
 */
void ipi_batch_send(void);

/**
 * @brief IPI interrupt handler
 * 
 * Called when IPI is received.
 * Dispatches to appropriate handler based on IPI type.
 */
void ipi_interrupt_handler(void);

/**
 * @brief Handle reschedule IPI
 * 
 * Forces reschedule on current CPU.
 */
void ipi_handle_reschedule(void);

/**
 * @brief Handle wakeup IPI
 * 
 * Wakes up specified task.
 * 
 * @param data Wakeup data containing task pointer
 */
void ipi_handle_wakeup(struct ipi_wakeup_data *data);

/**
 * @brief Handle TLB flush IPI
 * 
 * Flushes TLB entries in specified range.
 * 
 * @param data TLB flush data containing address range
 */
void ipi_handle_tlb_flush(struct ipi_tlb_flush_data *data);

/**
 * @brief Handle stop IPI
 * 
 * Stops current CPU.
 */
void ipi_handle_stop(void);

/**
 * @brief Get IPI statistics for CPU
 * 
 * @param cpu_id CPU ID
 * @param ipis_sent Output: number of IPIs sent
 * @param ipis_received Output: number of IPIs received
 * @param reschedule_ipis Output: number of reschedule IPIs
 * @param wakeup_ipis Output: number of wakeup IPIs
 * @param tlb_flush_ipis Output: number of TLB flush IPIs
 */
void ipi_get_stats(uint32_t cpu_id,
                  uint64_t *ipis_sent,
                  uint64_t *ipis_received,
                  uint64_t *reschedule_ipis,
                  uint64_t *wakeup_ipis,
                  uint64_t *tlb_flush_ipis);

/**
 * @brief Check if IPI is pending
 * 
 * @param cpu_id CPU ID
 * @param type IPI type to check
 * @return true if IPI of specified type is pending
 */
[[nodiscard]] bool ipi_is_pending(uint32_t cpu_id, enum ipi_type type);

/**
 * @brief Clear pending IPI
 * 
 * @param cpu_id CPU ID
 * @param type IPI type to clear
 */
void ipi_clear_pending(uint32_t cpu_id, enum ipi_type type);

/**
 * @brief Wait for IPI acknowledgment
 * 
 * Waits until target CPU acknowledges IPI.
 * 
 * @param cpu_id Target CPU ID
 * @param timeout_us Timeout in microseconds (0 = no timeout)
 * @return true if acknowledged, false if timeout
 */
bool ipi_wait_ack(uint32_t cpu_id, uint64_t timeout_us);

#endif /* BDI_IPI_H */
