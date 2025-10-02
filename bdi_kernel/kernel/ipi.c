
/**
 * @file ipi.c
 * @brief Inter-processor interrupts for scheduler implementation
 * 
 * Phase 3: Scheduler & Lock-Free Concurrency
 */

#include "ipi.h"
#include "scheduler.h"
#include "smp.h"
#include <string.h>
#include <stdio.h>

/* Per-CPU IPI state */
struct cpu_ipi_state g_cpu_ipi_states[256] = {0};

/* Thread-local IPI batch */
_Thread_local struct ipi_batch g_ipi_batch = {0};

/* Forward declarations */
static void send_ipi_raw(uint32_t cpu_id, enum ipi_type type);
static void dispatch_ipi(struct ipi_message *msg);

/**
 * @brief Initialize IPI subsystem
 */
void ipi_init(void) {
    uint32_t num_cpus = get_num_cpus();
    
    /* Initialize per-CPU IPI state */
    for (uint32_t i = 0; i < num_cpus; i++) {
        struct cpu_ipi_state *state = &g_cpu_ipi_states[i];
        
        atomic_init(&state->pending_ipis, 0);
        atomic_init(&state->ipis_sent, 0);
        atomic_init(&state->ipis_received, 0);
        atomic_init(&state->reschedule_ipis, 0);
        atomic_init(&state->wakeup_ipis, 0);
        atomic_init(&state->tlb_flush_ipis, 0);
        
        state->cpu_id = i;
    }
    
    printf("[IPI] IPI subsystem initialized for %u CPUs\n", num_cpus);
}

/**
 * @brief Send IPI to specific CPU
 */
void send_ipi(uint32_t cpu_id, enum ipi_type type, void *data) {
    if (cpu_id >= get_num_cpus()) {
        return;
    }
    
    /* Create IPI message */
    struct ipi_message msg = {
        .type = type,
        .source_cpu = get_current_cpu_id(),
        .target_cpu = cpu_id
    };
    
    /* Copy type-specific data */
    switch (type) {
        case IPI_WAKEUP:
            if (data != NULL) {
                msg.data.wakeup = *(struct ipi_wakeup_data *)data;
            }
            break;
            
        case IPI_TLB_FLUSH:
            if (data != NULL) {
                msg.data.tlb_flush = *(struct ipi_tlb_flush_data *)data;
            }
            break;
            
        case IPI_CUSTOM:
            msg.data.custom_data = data;
            break;
            
        default:
            break;
    }
    
    /* Send IPI */
    send_ipi_raw(cpu_id, type);
    
    /* Update statistics */
    struct cpu_ipi_state *state = &g_cpu_ipi_states[get_current_cpu_id()];
    atomic_fetch_add_explicit(&state->ipis_sent, 1, memory_order_relaxed);
    
    /* Dispatch IPI (in real implementation, this would be handled by interrupt) */
    dispatch_ipi(&msg);
}

/**
 * @brief Send IPI to all CPUs except current
 */
void send_ipi_all_but_self(enum ipi_type type, void *data) {
    uint32_t current_cpu = get_current_cpu_id();
    uint32_t num_cpus = get_num_cpus();
    
    for (uint32_t i = 0; i < num_cpus; i++) {
        if (i != current_cpu) {
            send_ipi(i, type, data);
        }
    }
}

/**
 * @brief Send IPI to all CPUs
 */
void send_ipi_all(enum ipi_type type, void *data) {
    uint32_t num_cpus = get_num_cpus();
    
    for (uint32_t i = 0; i < num_cpus; i++) {
        send_ipi(i, type, data);
    }
}

/**
 * @brief Start IPI batch
 */
void ipi_batch_start(void) {
    g_ipi_batch.count = 0;
    g_ipi_batch.active = true;
}

/**
 * @brief Add IPI to batch
 */
int ipi_batch_add(uint32_t cpu_id, enum ipi_type type, void *data) {
    if (!g_ipi_batch.active) {
        return -1;
    }
    
    if (g_ipi_batch.count >= IPI_MAX_BATCH) {
        return -1;  /* Batch full */
    }
    
    /* Create IPI message */
    struct ipi_message *msg = &g_ipi_batch.messages[g_ipi_batch.count];
    msg->type = type;
    msg->source_cpu = get_current_cpu_id();
    msg->target_cpu = cpu_id;
    
    /* Copy type-specific data */
    switch (type) {
        case IPI_WAKEUP:
            if (data != NULL) {
                msg->data.wakeup = *(struct ipi_wakeup_data *)data;
            }
            break;
            
        case IPI_TLB_FLUSH:
            if (data != NULL) {
                msg->data.tlb_flush = *(struct ipi_tlb_flush_data *)data;
            }
            break;
            
        case IPI_CUSTOM:
            msg->data.custom_data = data;
            break;
            
        default:
            break;
    }
    
    g_ipi_batch.count++;
    return 0;
}

/**
 * @brief Send all batched IPIs
 */
void ipi_batch_send(void) {
    if (!g_ipi_batch.active) {
        return;
    }
    
    /* Send all IPIs in batch */
    for (uint32_t i = 0; i < g_ipi_batch.count; i++) {
        struct ipi_message *msg = &g_ipi_batch.messages[i];
        
        /* Send raw IPI */
        send_ipi_raw(msg->target_cpu, msg->type);
        
        /* Dispatch IPI */
        dispatch_ipi(msg);
    }
    
    /* Update statistics */
    struct cpu_ipi_state *state = &g_cpu_ipi_states[get_current_cpu_id()];
    atomic_fetch_add_explicit(&state->ipis_sent, g_ipi_batch.count,
                              memory_order_relaxed);
    
    /* Clear batch */
    g_ipi_batch.count = 0;
    g_ipi_batch.active = false;
}

/**
 * @brief IPI interrupt handler
 */
void ipi_interrupt_handler(void) {
    uint32_t cpu_id = get_current_cpu_id();
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Update statistics */
    atomic_fetch_add_explicit(&state->ipis_received, 1, memory_order_relaxed);
    
    /* Load pending IPIs */
    uint32_t pending = atomic_load_explicit(&state->pending_ipis,
                                           memory_order_acquire);
    
    /* Handle each pending IPI type */
    if (pending & (1U << IPI_RESCHEDULE)) {
        ipi_handle_reschedule();
        ipi_clear_pending(cpu_id, IPI_RESCHEDULE);
    }
    
    if (pending & (1U << IPI_STOP)) {
        ipi_handle_stop();
        ipi_clear_pending(cpu_id, IPI_STOP);
    }
    
    /* Other IPI types handled by dispatch_ipi() */
}

/**
 * @brief Handle reschedule IPI
 */
void ipi_handle_reschedule(void) {
    uint32_t cpu_id = get_current_cpu_id();
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Update statistics */
    atomic_fetch_add_explicit(&state->reschedule_ipis, 1,
                              memory_order_relaxed);
    
    /* Force reschedule */
    scheduler_resched();
}

/**
 * @brief Handle wakeup IPI
 */
void ipi_handle_wakeup(struct ipi_wakeup_data *data) {
    if (data == NULL || data->task == NULL) {
        return;
    }
    
    uint32_t cpu_id = get_current_cpu_id();
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Update statistics */
    atomic_fetch_add_explicit(&state->wakeup_ipis, 1, memory_order_relaxed);
    
    /* Unblock task */
    task_unblock(data->task);
}

/**
 * @brief Handle TLB flush IPI
 */
void ipi_handle_tlb_flush(struct ipi_tlb_flush_data *data) {
    if (data == NULL) {
        return;
    }
    
    uint32_t cpu_id = get_current_cpu_id();
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Update statistics */
    atomic_fetch_add_explicit(&state->tlb_flush_ipis, 1, memory_order_relaxed);
    
    /* TODO: Flush TLB entries in specified range */
    /* For x86_64: invlpg instruction */
    /* For ARM64: tlbi instruction */
    (void)data;
}

/**
 * @brief Handle stop IPI
 */
void ipi_handle_stop(void) {
    /* TODO: Stop CPU (enter halt state) */
    /* For now, just enter idle loop */
    while (1) {
        __asm__ volatile("pause");
    }
}

/**
 * @brief Get IPI statistics for CPU
 */
void ipi_get_stats(uint32_t cpu_id,
                  uint64_t *ipis_sent,
                  uint64_t *ipis_received,
                  uint64_t *reschedule_ipis,
                  uint64_t *wakeup_ipis,
                  uint64_t *tlb_flush_ipis) {
    if (cpu_id >= get_num_cpus()) {
        return;
    }
    
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    if (ipis_sent != NULL) {
        *ipis_sent = atomic_load_explicit(&state->ipis_sent,
                                         memory_order_relaxed);
    }
    
    if (ipis_received != NULL) {
        *ipis_received = atomic_load_explicit(&state->ipis_received,
                                             memory_order_relaxed);
    }
    
    if (reschedule_ipis != NULL) {
        *reschedule_ipis = atomic_load_explicit(&state->reschedule_ipis,
                                               memory_order_relaxed);
    }
    
    if (wakeup_ipis != NULL) {
        *wakeup_ipis = atomic_load_explicit(&state->wakeup_ipis,
                                           memory_order_relaxed);
    }
    
    if (tlb_flush_ipis != NULL) {
        *tlb_flush_ipis = atomic_load_explicit(&state->tlb_flush_ipis,
                                              memory_order_relaxed);
    }
}

/**
 * @brief Check if IPI is pending
 */
bool ipi_is_pending(uint32_t cpu_id, enum ipi_type type) {
    if (cpu_id >= get_num_cpus()) {
        return false;
    }
    
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    uint32_t pending = atomic_load_explicit(&state->pending_ipis,
                                           memory_order_acquire);
    
    return (pending & (1U << type)) != 0;
}

/**
 * @brief Clear pending IPI
 */
void ipi_clear_pending(uint32_t cpu_id, enum ipi_type type) {
    if (cpu_id >= get_num_cpus()) {
        return;
    }
    
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Clear bit atomically */
    uint32_t mask = ~(1U << type);
    atomic_fetch_and_explicit(&state->pending_ipis, mask,
                              memory_order_release);
}

/**
 * @brief Wait for IPI acknowledgment
 */
bool ipi_wait_ack(uint32_t cpu_id, uint64_t timeout_us) {
    /* TODO: Implement IPI acknowledgment mechanism */
    /* For now, just return true (assume immediate ack) */
    (void)cpu_id;
    (void)timeout_us;
    return true;
}

/**
 * @brief Send raw IPI to CPU
 * 
 * Platform-specific IPI sending.
 */
static void send_ipi_raw(uint32_t cpu_id, enum ipi_type type) {
    if (cpu_id >= get_num_cpus()) {
        return;
    }
    
    struct cpu_ipi_state *state = &g_cpu_ipi_states[cpu_id];
    
    /* Set pending bit atomically */
    uint32_t mask = (1U << type);
    atomic_fetch_or_explicit(&state->pending_ipis, mask,
                            memory_order_release);
    
    /* TODO: Send actual hardware IPI */
    /* For x86_64: Write to APIC ICR register */
    /* For ARM64: Write to GIC GICD_SGIR register */
}

/**
 * @brief Dispatch IPI to handler
 */
static void dispatch_ipi(struct ipi_message *msg) {
    if (msg == NULL) {
        return;
    }
    
    /* In real implementation, this would be called from interrupt handler */
    /* For now, we call handlers directly for simulation */
    
    switch (msg->type) {
        case IPI_RESCHEDULE:
            /* Handled by interrupt handler */
            break;
            
        case IPI_WAKEUP:
            ipi_handle_wakeup(&msg->data.wakeup);
            break;
            
        case IPI_TLB_FLUSH:
            ipi_handle_tlb_flush(&msg->data.tlb_flush);
            break;
            
        case IPI_STOP:
            /* Handled by interrupt handler */
            break;
            
        case IPI_CUSTOM:
            /* User-defined handler */
            break;
            
        default:
            break;
    }
}
