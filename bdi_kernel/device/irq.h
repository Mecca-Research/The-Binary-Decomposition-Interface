
/**
 * @file irq.h
 * @brief Unified Interrupt Controller Interface
 * 
 * Abstraction layer for interrupt controllers with support for
 * MSI/MSI-X, interrupt routing, affinity management, and
 * shared/exclusive interrupts.
 */

#ifndef BDI_IRQ_H
#define BDI_IRQ_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"

/* IRQ Constants */
#define IRQ_MAX_VECTORS         256     /* Maximum interrupt vectors */
#define IRQ_MAX_HANDLERS        16      /* Max handlers per IRQ */
#define IRQ_NAME_MAX            32      /* Maximum IRQ name length */

/* IRQ Flags */
#define IRQ_FLAG_SHARED         (1U << 0)  /* Shared interrupt */
#define IRQ_FLAG_EXCLUSIVE      (1U << 1)  /* Exclusive interrupt */
#define IRQ_FLAG_EDGE           (1U << 2)  /* Edge-triggered */
#define IRQ_FLAG_LEVEL          (1U << 3)  /* Level-triggered */
#define IRQ_FLAG_MSI            (1U << 4)  /* MSI interrupt */
#define IRQ_FLAG_MSIX           (1U << 5)  /* MSI-X interrupt */
#define IRQ_FLAG_DISABLED       (1U << 6)  /* IRQ disabled */
#define IRQ_FLAG_PENDING        (1U << 7)  /* IRQ pending */

/* IRQ Return Values */
#define IRQ_HANDLED             1       /* IRQ was handled */
#define IRQ_NOT_HANDLED         0       /* IRQ was not handled */
#define IRQ_WAKE_THREAD         2       /* Wake threaded handler */

/* Forward declarations */
struct device;

/**
 * @brief IRQ handler callback
 * 
 * @param irq IRQ number
 * @param dev_id Device ID passed during registration
 * @return IRQ_HANDLED, IRQ_NOT_HANDLED, or IRQ_WAKE_THREAD
 */
typedef int (*irq_handler_t)(uint32_t irq, void *dev_id);

/**
 * @brief IRQ handler descriptor
 */
struct irq_handler {
    irq_handler_t handler;
    void *dev_id;
    char name[IRQ_NAME_MAX];
    uint32_t flags;
    _Atomic uint64_t count;     /* Number of times called */
    bool active;
};

/**
 * @brief IRQ descriptor
 */
struct irq_desc {
    uint32_t irq;
    char name[IRQ_NAME_MAX];
    
    /* Handlers */
    struct irq_handler handlers[IRQ_MAX_HANDLERS];
    uint32_t num_handlers;
    
    /* IRQ state */
    _Atomic uint32_t flags;
    _Atomic uint32_t pending_count;
    _Atomic uint64_t total_count;
    
    /* Affinity */
    uint32_t affinity_cpu;      /* CPU affinity */
    uint32_t affinity_mask;     /* CPU affinity mask */
    
    /* MSI/MSI-X data */
    uint64_t msi_address;
    uint32_t msi_data;
    
    /* Device association */
    struct device *device;
};

/**
 * @brief IRQ controller operations
 */
struct irq_chip {
    const char *name;
    
    /* Enable/disable IRQ */
    void (*enable)(uint32_t irq);
    void (*disable)(uint32_t irq);
    
    /* Mask/unmask IRQ */
    void (*mask)(uint32_t irq);
    void (*unmask)(uint32_t irq);
    
    /* Acknowledge IRQ */
    void (*ack)(uint32_t irq);
    void (*eoi)(uint32_t irq);
    
    /* Set IRQ type */
    int (*set_type)(uint32_t irq, uint32_t type);
    
    /* Set IRQ affinity */
    int (*set_affinity)(uint32_t irq, uint32_t cpu_mask);
    
    /* MSI/MSI-X support */
    int (*setup_msi)(uint32_t irq, uint64_t *address, uint32_t *data);
    int (*teardown_msi)(uint32_t irq);
};

/**
 * @brief IRQ subsystem state
 */
struct irq_subsystem {
    /* IRQ descriptors */
    struct irq_desc irq_descs[IRQ_MAX_VECTORS];
    
    /* IRQ controller */
    const struct irq_chip *chip;
    
    /* Statistics */
    _Atomic uint64_t total_irqs;
    _Atomic uint64_t spurious_irqs;
    _Atomic uint64_t unhandled_irqs;
    
    /* Subsystem state */
    _Atomic bool initialized;
};

/* Global IRQ subsystem instance */
extern struct irq_subsystem g_irq_subsystem;

/**
 * @brief Initialize the IRQ subsystem
 * 
 * @param chip IRQ controller chip
 * @return 0 on success, negative error code on failure
 */
int irq_init(const struct irq_chip *chip);

/**
 * @brief Shutdown the IRQ subsystem
 */
void irq_shutdown(void);

/**
 * @brief Request an IRQ
 * 
 * @param irq IRQ number
 * @param handler IRQ handler callback
 * @param flags IRQ flags (IRQ_FLAG_*)
 * @param name IRQ name
 * @param dev_id Device ID passed to handler
 * @return 0 on success, negative error code on failure
 */
int irq_request(uint32_t irq, irq_handler_t handler, uint32_t flags,
               const char *name, void *dev_id);

/**
 * @brief Free an IRQ
 * 
 * @param irq IRQ number
 * @param dev_id Device ID used during request
 */
void irq_free(uint32_t irq, void *dev_id);

/**
 * @brief Enable an IRQ
 * 
 * @param irq IRQ number
 */
void irq_enable(uint32_t irq);

/**
 * @brief Disable an IRQ
 * 
 * @param irq IRQ number
 */
void irq_disable(uint32_t irq);

/**
 * @brief Set IRQ affinity
 * 
 * @param irq IRQ number
 * @param cpu_mask CPU affinity mask
 * @return 0 on success, negative error code on failure
 */
int irq_set_affinity(uint32_t irq, uint32_t cpu_mask);

/**
 * @brief Handle an IRQ
 * 
 * @param irq IRQ number
 */
void irq_handle(uint32_t irq);

/**
 * @brief Setup MSI interrupt
 * 
 * @param irq IRQ number
 * @param address Output for MSI address
 * @param data Output for MSI data
 * @return 0 on success, negative error code on failure
 */
int irq_setup_msi(uint32_t irq, uint64_t *address, uint32_t *data);

/**
 * @brief Teardown MSI interrupt
 * 
 * @param irq IRQ number
 * @return 0 on success, negative error code on failure
 */
int irq_teardown_msi(uint32_t irq);

/**
 * @brief Get IRQ statistics
 * 
 * @param irq IRQ number
 * @param count Output for IRQ count
 * @return 0 on success, negative error code on failure
 */
int irq_get_stats(uint32_t irq, uint64_t *count);

/**
 * @brief Get global IRQ statistics
 * 
 * @param total_irqs Output for total IRQs
 * @param spurious_irqs Output for spurious IRQs
 * @param unhandled_irqs Output for unhandled IRQs
 */
void irq_get_global_stats(uint64_t *total_irqs, uint64_t *spurious_irqs,
                         uint64_t *unhandled_irqs);

#endif /* BDI_IRQ_H */
