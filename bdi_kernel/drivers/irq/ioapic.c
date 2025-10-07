
#include "../../include/bdi/drivers/ioapic.h"
#include <string.h>
#include <stdio.h>

static ioapic_t ioapics[MAX_IOAPICS];
static uint32_t num_ioapics = 0;
static irq_route_t irq_routes[256];

/* Read IOAPIC register */
uint32_t ioapic_read_reg(ioapic_t *ioapic, uint8_t reg) {
    volatile uint32_t *regsel = (volatile uint32_t *)((uint8_t *)ioapic->base_address + IOAPIC_REGSEL);
    volatile uint32_t *iowin = (volatile uint32_t *)((uint8_t *)ioapic->base_address + IOAPIC_IOWIN);
    
    *regsel = reg;
    return *iowin;
}

/* Write IOAPIC register */
void ioapic_write_reg(ioapic_t *ioapic, uint8_t reg, uint32_t value) {
    volatile uint32_t *regsel = (volatile uint32_t *)((uint8_t *)ioapic->base_address + IOAPIC_REGSEL);
    volatile uint32_t *iowin = (volatile uint32_t *)((uint8_t *)ioapic->base_address + IOAPIC_IOWIN);
    
    *regsel = reg;
    *iowin = value;
}

/* Read redirection table entry */
uint64_t ioapic_read_redir_entry(ioapic_t *ioapic, uint8_t pin) {
    if (pin >= ioapic->num_entries) {
        return 0;
    }
    
    uint8_t reg = IOAPIC_REDTBL_BASE + (pin * 2);
    uint32_t low = ioapic_read_reg(ioapic, reg);
    uint32_t high = ioapic_read_reg(ioapic, reg + 1);
    
    return ((uint64_t)high << 32) | low;
}

/* Write redirection table entry */
void ioapic_write_redir_entry(ioapic_t *ioapic, uint8_t pin, uint64_t value) {
    if (pin >= ioapic->num_entries) {
        return;
    }
    
    uint8_t reg = IOAPIC_REDTBL_BASE + (pin * 2);
    
    /* Write high dword first, then low dword */
    ioapic_write_reg(ioapic, reg + 1, (uint32_t)(value >> 32));
    ioapic_write_reg(ioapic, reg, (uint32_t)value);
}

/* Initialize IOAPIC subsystem */
int ioapic_init(void) {
    memset(ioapics, 0, sizeof(ioapics));
    memset(irq_routes, 0, sizeof(irq_routes));
    num_ioapics = 0;
    
    printf("IOAPIC: Subsystem initialized\n");
    
    return 0;
}

/* Register an IOAPIC */
int ioapic_register(uint32_t id, uintptr_t base_addr, uint32_t gsi_base) {
    if (num_ioapics >= MAX_IOAPICS) {
        return -1;
    }
    
    ioapic_t *ioapic = &ioapics[num_ioapics++];
    
    ioapic->id = id;
    ioapic->base_address = (volatile void *)base_addr;
    ioapic->gsi_base = gsi_base;
    
    /* Read version register */
    uint32_t ver = ioapic_read_reg(ioapic, IOAPIC_VER);
    ioapic->version = ver & 0xFF;
    ioapic->max_redir_entry = (ver >> 16) & 0xFF;
    ioapic->num_entries = ioapic->max_redir_entry + 1;
    
    /* Mask all interrupts initially */
    for (uint32_t i = 0; i < ioapic->num_entries; i++) {
        uint64_t entry = ioapic_read_redir_entry(ioapic, i);
        entry |= IOAPIC_MASK;
        ioapic_write_redir_entry(ioapic, i, entry);
    }
    
    ioapic->initialized = true;
    
    printf("IOAPIC: Registered ID %u at 0x%lx, GSI base %u, %u entries\n",
           id, base_addr, gsi_base, ioapic->num_entries);
    
    return 0;
}

/* Get IOAPIC by ID */
ioapic_t *ioapic_get(uint32_t id) {
    for (uint32_t i = 0; i < num_ioapics; i++) {
        if (ioapics[i].id == id) {
            return &ioapics[i];
        }
    }
    return NULL;
}

/* Get IOAPIC by GSI */
ioapic_t *ioapic_get_by_gsi(uint32_t gsi) {
    for (uint32_t i = 0; i < num_ioapics; i++) {
        ioapic_t *ioapic = &ioapics[i];
        if (gsi >= ioapic->gsi_base && gsi < ioapic->gsi_base + ioapic->num_entries) {
            return ioapic;
        }
    }
    return NULL;
}

/* Set IRQ routing */
int ioapic_set_irq(uint32_t gsi, uint8_t vector, uint8_t dest,
                   ioapic_delivery_mode_t delivery_mode,
                   ioapic_dest_mode_t dest_mode,
                   ioapic_trigger_mode_t trigger_mode,
                   ioapic_polarity_t polarity) {
    ioapic_t *ioapic = ioapic_get_by_gsi(gsi);
    if (!ioapic) {
        return -1;
    }
    
    uint32_t pin = gsi - ioapic->gsi_base;
    
    /* Build redirection entry */
    uint64_t entry = 0;
    entry |= vector & IOAPIC_VECTOR_MASK;
    entry |= (delivery_mode << 8);
    entry |= (dest_mode << 11);
    entry |= (polarity << 13);
    entry |= (trigger_mode << 15);
    entry |= ((uint64_t)dest << 56);
    
    /* Write entry */
    ioapic_write_redir_entry(ioapic, pin, entry);
    
    /* Update routing table */
    irq_routes[gsi].gsi = gsi;
    irq_routes[gsi].ioapic_id = ioapic->id;
    irq_routes[gsi].pin = pin;
    irq_routes[gsi].vector = vector;
    irq_routes[gsi].trigger = trigger_mode;
    irq_routes[gsi].polarity = polarity;
    irq_routes[gsi].masked = false;
    
    return 0;
}

/* Mask IRQ */
int ioapic_mask_irq(uint32_t gsi) {
    ioapic_t *ioapic = ioapic_get_by_gsi(gsi);
    if (!ioapic) {
        return -1;
    }
    
    uint32_t pin = gsi - ioapic->gsi_base;
    uint64_t entry = ioapic_read_redir_entry(ioapic, pin);
    entry |= IOAPIC_MASK;
    ioapic_write_redir_entry(ioapic, pin, entry);
    
    irq_routes[gsi].masked = true;
    
    return 0;
}

/* Unmask IRQ */
int ioapic_unmask_irq(uint32_t gsi) {
    ioapic_t *ioapic = ioapic_get_by_gsi(gsi);
    if (!ioapic) {
        return -1;
    }
    
    uint32_t pin = gsi - ioapic->gsi_base;
    uint64_t entry = ioapic_read_redir_entry(ioapic, pin);
    entry &= ~IOAPIC_MASK;
    ioapic_write_redir_entry(ioapic, pin, entry);
    
    irq_routes[gsi].masked = false;
    
    return 0;
}

/* Set IRQ handler */
int ioapic_set_irq_handler(uint32_t gsi, void (*handler)(uint32_t, void *), void *data) {
    if (gsi >= 256) {
        return -1;
    }
    
    irq_routes[gsi].handler = handler;
    irq_routes[gsi].handler_data = data;
    
    return 0;
}

/* Setup legacy IRQ */
int ioapic_setup_legacy_irq(uint32_t irq, uint8_t vector) {
    /* Legacy IRQs map to GSI 0-15 */
    if (irq >= 16) {
        return -1;
    }
    
    return ioapic_set_irq(irq, vector, 0,
                         IOAPIC_DELIVERY_FIXED,
                         IOAPIC_DEST_PHYSICAL,
                         IOAPIC_TRIGGER_EDGE,
                         IOAPIC_POLARITY_HIGH);
}

/* Enable legacy mode */
int ioapic_enable_legacy_mode(void) {
    /* Setup standard ISA IRQs */
    for (uint32_t irq = 0; irq < 16; irq++) {
        ioapic_setup_legacy_irq(irq, 0x20 + irq);
    }
    
    return 0;
}

/* Disable legacy mode */
int ioapic_disable_legacy_mode(void) {
    /* Mask all legacy IRQs */
    for (uint32_t irq = 0; irq < 16; irq++) {
        ioapic_mask_irq(irq);
    }
    
    return 0;
}

/* Set IRQ affinity */
int ioapic_set_irq_affinity(uint32_t gsi, uint32_t cpu_mask) {
    ioapic_t *ioapic = ioapic_get_by_gsi(gsi);
    if (!ioapic) {
        return -1;
    }
    
    uint32_t pin = gsi - ioapic->gsi_base;
    uint64_t entry = ioapic_read_redir_entry(ioapic, pin);
    
    /* Update destination field */
    entry &= ~(0xFFULL << 56);
    entry |= ((uint64_t)(cpu_mask & 0xFF) << 56);
    
    ioapic_write_redir_entry(ioapic, pin, entry);
    
    return 0;
}

/* Dump IOAPIC information */
void ioapic_dump_info(ioapic_t *ioapic) {
    if (!ioapic) {
        return;
    }
    
    printf("IOAPIC ID %u:\n", ioapic->id);
    printf("  Base: 0x%lx\n", (uintptr_t)ioapic->base_address);
    printf("  GSI Base: %u\n", ioapic->gsi_base);
    printf("  Version: 0x%02x\n", ioapic->version);
    printf("  Entries: %u\n", ioapic->num_entries);
}

/* Dump redirection table */
void ioapic_dump_redir_table(ioapic_t *ioapic) {
    if (!ioapic) {
        return;
    }
    
    printf("IOAPIC %u Redirection Table:\n", ioapic->id);
    for (uint32_t i = 0; i < ioapic->num_entries; i++) {
        uint64_t entry = ioapic_read_redir_entry(ioapic, i);
        printf("  Pin %2u: Vector=0x%02llx Dest=0x%02llx %s %s %s\n",
               i,
               entry & 0xFF,
               (entry >> 56) & 0xFF,
               (entry & IOAPIC_MASK) ? "MASKED" : "UNMASKED",
               (entry & IOAPIC_TRIGGER_LEVEL) ? "LEVEL" : "EDGE",
               (entry & IOAPIC_INTPOL_LOW) ? "LOW" : "HIGH");
    }
}
