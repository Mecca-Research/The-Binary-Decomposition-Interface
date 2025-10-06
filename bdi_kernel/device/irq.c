
/**
 * @file irq.c
 * @brief Unified Interrupt Controller Interface Implementation
 */

#include "irq.h"
#include <stdio.h>
#include <string.h>

/* Global IRQ subsystem instance */
struct irq_subsystem g_irq_subsystem = {0};

/**
 * @brief Initialize the IRQ subsystem
 */
int irq_init(const struct irq_chip *chip) {
    if (chip == nullptr) {
        return -1;
    }
    
    if (atomic_load_explicit(&g_irq_subsystem.initialized, memory_order_acquire)) {
        return 0; /* Already initialized */
    }
    
    /* Initialize IRQ descriptors */
    for (uint32_t i = 0; i < IRQ_MAX_VECTORS; i++) {
        struct irq_desc *desc = &g_irq_subsystem.irq_descs[i];
        desc->irq = i;
        snprintf(desc->name, IRQ_NAME_MAX, "IRQ%u", i);
        desc->num_handlers = 0;
        atomic_store_explicit(&desc->flags, 0, memory_order_relaxed);
        atomic_store_explicit(&desc->pending_count, 0, memory_order_relaxed);
        atomic_store_explicit(&desc->total_count, 0, memory_order_relaxed);
        desc->affinity_cpu = 0;
        desc->affinity_mask = 0xFFFFFFFF;
        desc->device = nullptr;
        
        /* Initialize handlers */
        for (uint32_t j = 0; j < IRQ_MAX_HANDLERS; j++) {
            desc->handlers[j].active = false;
            atomic_store_explicit(&desc->handlers[j].count, 0, memory_order_relaxed);
        }
    }
    
    /* Set IRQ controller */
    g_irq_subsystem.chip = chip;
    
    /* Initialize statistics */
    atomic_store_explicit(&g_irq_subsystem.total_irqs, 0, memory_order_relaxed);
    atomic_store_explicit(&g_irq_subsystem.spurious_irqs, 0, memory_order_relaxed);
    atomic_store_explicit(&g_irq_subsystem.unhandled_irqs, 0, memory_order_relaxed);
    
    atomic_store_explicit(&g_irq_subsystem.initialized, true, memory_order_release);
    
    printf("[IRQ] Initialized with controller: %s\n", chip->name);
    return 0;
}

/**
 * @brief Shutdown the IRQ subsystem
 */
void irq_shutdown(void) {
    if (!atomic_load_explicit(&g_irq_subsystem.initialized, memory_order_acquire)) {
        return;
    }
    
    /* Disable all IRQs */
    for (uint32_t i = 0; i < IRQ_MAX_VECTORS; i++) {
        irq_disable(i);
    }
    
    atomic_store_explicit(&g_irq_subsystem.initialized, false, memory_order_release);
    printf("[IRQ] Shutdown complete\n");
}

/**
 * @brief Request an IRQ
 */
int irq_request(uint32_t irq, irq_handler_t handler, uint32_t flags,
               const char *name, void *dev_id) {
    if (irq >= IRQ_MAX_VECTORS || handler == nullptr) {
        return -1;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    
    /* Check for exclusive IRQ */
    if ((flags & IRQ_FLAG_EXCLUSIVE) && desc->num_handlers > 0) {
        printf("[IRQ] IRQ %u already has handlers (exclusive requested)\n", irq);
        return -1;
    }
    
    if (desc->num_handlers > 0) {
        uint32_t desc_flags = atomic_load_explicit(&desc->flags, memory_order_acquire);
        if (desc_flags & IRQ_FLAG_EXCLUSIVE) {
            printf("[IRQ] IRQ %u is exclusive\n", irq);
            return -1;
        }
        if (!(flags & IRQ_FLAG_SHARED)) {
            printf("[IRQ] IRQ %u requires shared flag\n", irq);
            return -1;
        }
    }
    
    /* Find free handler slot */
    for (uint32_t i = 0; i < IRQ_MAX_HANDLERS; i++) {
        if (!desc->handlers[i].active) {
            desc->handlers[i].handler = handler;
            desc->handlers[i].dev_id = dev_id;
            desc->handlers[i].flags = flags;
            desc->handlers[i].active = true;
            
            if (name != nullptr) {
                snprintf(desc->handlers[i].name, IRQ_NAME_MAX, "%s", name);
            } else {
                snprintf(desc->handlers[i].name, IRQ_NAME_MAX, "handler%u", i);
            }
            
            desc->num_handlers++;
            
            /* Update descriptor flags */
            atomic_fetch_or_explicit(&desc->flags, flags, memory_order_release);
            
            printf("[IRQ] Registered handler for IRQ %u: %s\n", irq, desc->handlers[i].name);
            
            /* Enable IRQ if this is the first handler */
            if (desc->num_handlers == 1) {
                irq_enable(irq);
            }
            
            return 0;
        }
    }
    
    printf("[IRQ] No free handler slots for IRQ %u\n", irq);
    return -1;
}

/**
 * @brief Free an IRQ
 */
void irq_free(uint32_t irq, void *dev_id) {
    if (irq >= IRQ_MAX_VECTORS) {
        return;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    
    /* Find and remove handler */
    for (uint32_t i = 0; i < IRQ_MAX_HANDLERS; i++) {
        if (desc->handlers[i].active && desc->handlers[i].dev_id == dev_id) {
            printf("[IRQ] Freeing handler for IRQ %u: %s\n", irq, desc->handlers[i].name);
            
            desc->handlers[i].active = false;
            desc->num_handlers--;
            
            /* Disable IRQ if no more handlers */
            if (desc->num_handlers == 0) {
                irq_disable(irq);
                atomic_store_explicit(&desc->flags, 0, memory_order_release);
            }
            
            return;
        }
    }
}

/**
 * @brief Enable an IRQ
 */
void irq_enable(uint32_t irq) {
    if (irq >= IRQ_MAX_VECTORS) {
        return;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    atomic_fetch_and_explicit(&desc->flags, ~IRQ_FLAG_DISABLED, memory_order_release);
    
    if (g_irq_subsystem.chip != nullptr && g_irq_subsystem.chip->enable != nullptr) {
        g_irq_subsystem.chip->enable(irq);
    }
}

/**
 * @brief Disable an IRQ
 */
void irq_disable(uint32_t irq) {
    if (irq >= IRQ_MAX_VECTORS) {
        return;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    atomic_fetch_or_explicit(&desc->flags, IRQ_FLAG_DISABLED, memory_order_release);
    
    if (g_irq_subsystem.chip != nullptr && g_irq_subsystem.chip->disable != nullptr) {
        g_irq_subsystem.chip->disable(irq);
    }
}

/**
 * @brief Set IRQ affinity
 */
int irq_set_affinity(uint32_t irq, uint32_t cpu_mask) {
    if (irq >= IRQ_MAX_VECTORS) {
        return -1;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    desc->affinity_mask = cpu_mask;
    
    if (g_irq_subsystem.chip != nullptr && g_irq_subsystem.chip->set_affinity != nullptr) {
        return g_irq_subsystem.chip->set_affinity(irq, cpu_mask);
    }
    
    return 0;
}

/**
 * @brief Handle an IRQ
 */
void irq_handle(uint32_t irq) {
    if (irq >= IRQ_MAX_VECTORS) {
        return;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    
    /* Check if IRQ is disabled */
    uint32_t flags = atomic_load_explicit(&desc->flags, memory_order_acquire);
    if (flags & IRQ_FLAG_DISABLED) {
        atomic_fetch_add_explicit(&g_irq_subsystem.spurious_irqs, 1, memory_order_relaxed);
        return;
    }
    
    /* Update statistics */
    atomic_fetch_add_explicit(&desc->total_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_irq_subsystem.total_irqs, 1, memory_order_relaxed);
    
    /* Acknowledge IRQ */
    if (g_irq_subsystem.chip != nullptr && g_irq_subsystem.chip->ack != nullptr) {
        g_irq_subsystem.chip->ack(irq);
    }
    
    /* Call handlers */
    bool handled = false;
    for (uint32_t i = 0; i < IRQ_MAX_HANDLERS; i++) {
        if (desc->handlers[i].active) {
            int result = desc->handlers[i].handler(irq, desc->handlers[i].dev_id);
            
            if (result == IRQ_HANDLED || result == IRQ_WAKE_THREAD) {
                handled = true;
                atomic_fetch_add_explicit(&desc->handlers[i].count, 1, memory_order_relaxed);
            }
            
            /* For shared IRQs, continue calling other handlers */
            if (!(flags & IRQ_FLAG_SHARED) && handled) {
                break;
            }
        }
    }
    
    if (!handled) {
        atomic_fetch_add_explicit(&g_irq_subsystem.unhandled_irqs, 1, memory_order_relaxed);
    }
    
    /* End of interrupt */
    if (g_irq_subsystem.chip != nullptr && g_irq_subsystem.chip->eoi != nullptr) {
        g_irq_subsystem.chip->eoi(irq);
    }
}

/**
 * @brief Setup MSI interrupt
 */
int irq_setup_msi(uint32_t irq, uint64_t *address, uint32_t *data) {
    if (irq >= IRQ_MAX_VECTORS || address == nullptr || data == nullptr) {
        return -1;
    }
    
    if (g_irq_subsystem.chip == nullptr || g_irq_subsystem.chip->setup_msi == nullptr) {
        return -1;
    }
    
    int result = g_irq_subsystem.chip->setup_msi(irq, address, data);
    
    if (result == 0) {
        struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
        desc->msi_address = *address;
        desc->msi_data = *data;
        atomic_fetch_or_explicit(&desc->flags, IRQ_FLAG_MSI, memory_order_release);
        
        printf("[IRQ] Setup MSI for IRQ %u: addr=0x%lx, data=0x%x\n", 
               irq, *address, *data);
    }
    
    return result;
}

/**
 * @brief Teardown MSI interrupt
 */
int irq_teardown_msi(uint32_t irq) {
    if (irq >= IRQ_MAX_VECTORS) {
        return -1;
    }
    
    if (g_irq_subsystem.chip == nullptr || g_irq_subsystem.chip->teardown_msi == nullptr) {
        return -1;
    }
    
    int result = g_irq_subsystem.chip->teardown_msi(irq);
    
    if (result == 0) {
        struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
        atomic_fetch_and_explicit(&desc->flags, ~IRQ_FLAG_MSI, memory_order_release);
        printf("[IRQ] Teardown MSI for IRQ %u\n", irq);
    }
    
    return result;
}

/**
 * @brief Get IRQ statistics
 */
int irq_get_stats(uint32_t irq, uint64_t *count) {
    if (irq >= IRQ_MAX_VECTORS || count == nullptr) {
        return -1;
    }
    
    struct irq_desc *desc = &g_irq_subsystem.irq_descs[irq];
    *count = atomic_load_explicit(&desc->total_count, memory_order_relaxed);
    
    return 0;
}

/**
 * @brief Get global IRQ statistics
 */
void irq_get_global_stats(uint64_t *total_irqs, uint64_t *spurious_irqs,
                         uint64_t *unhandled_irqs) {
    if (total_irqs != nullptr) {
        *total_irqs = atomic_load_explicit(&g_irq_subsystem.total_irqs, 
                                          memory_order_relaxed);
    }
    if (spurious_irqs != nullptr) {
        *spurious_irqs = atomic_load_explicit(&g_irq_subsystem.spurious_irqs,
                                             memory_order_relaxed);
    }
    if (unhandled_irqs != nullptr) {
        *unhandled_irqs = atomic_load_explicit(&g_irq_subsystem.unhandled_irqs,
                                              memory_order_relaxed);
    }
}
