
#ifndef BDI_IOAPIC_H
#define BDI_IOAPIC_H

#include <stdint.h>
#include <stdbool.h>

/* IOAPIC Register Offsets */
#define IOAPIC_REGSEL       0x00
#define IOAPIC_IOWIN        0x10

/* IOAPIC Register Indices */
#define IOAPIC_ID           0x00
#define IOAPIC_VER          0x01
#define IOAPIC_ARB          0x02
#define IOAPIC_REDTBL_BASE  0x10

/* Redirection Table Entry bits */
#define IOAPIC_VECTOR_MASK          0xFF
#define IOAPIC_DELMOD_FIXED         (0 << 8)
#define IOAPIC_DELMOD_LOWPRI        (1 << 8)
#define IOAPIC_DELMOD_SMI           (2 << 8)
#define IOAPIC_DELMOD_NMI           (4 << 8)
#define IOAPIC_DELMOD_INIT          (5 << 8)
#define IOAPIC_DELMOD_EXTINT        (7 << 8)
#define IOAPIC_DESTMOD_PHYSICAL     (0 << 11)
#define IOAPIC_DESTMOD_LOGICAL      (1 << 11)
#define IOAPIC_DELIVS_IDLE          (0 << 12)
#define IOAPIC_DELIVS_PENDING       (1 << 12)
#define IOAPIC_INTPOL_HIGH          (0 << 13)
#define IOAPIC_INTPOL_LOW           (1 << 13)
#define IOAPIC_REMOTE_IRR           (1 << 14)
#define IOAPIC_TRIGGER_EDGE         (0 << 15)
#define IOAPIC_TRIGGER_LEVEL        (1 << 15)
#define IOAPIC_MASK                 (1 << 16)

/* Maximum number of IOAPICs and IRQs */
#define MAX_IOAPICS         8
#define MAX_IOAPIC_IRQS     24

/* Delivery modes */
typedef enum {
    IOAPIC_DELIVERY_FIXED = 0,
    IOAPIC_DELIVERY_LOWPRI = 1,
    IOAPIC_DELIVERY_SMI = 2,
    IOAPIC_DELIVERY_NMI = 4,
    IOAPIC_DELIVERY_INIT = 5,
    IOAPIC_DELIVERY_EXTINT = 7
} ioapic_delivery_mode_t;

/* Destination modes */
typedef enum {
    IOAPIC_DEST_PHYSICAL = 0,
    IOAPIC_DEST_LOGICAL = 1
} ioapic_dest_mode_t;

/* Trigger modes */
typedef enum {
    IOAPIC_TRIGGER_EDGE = 0,
    IOAPIC_TRIGGER_LEVEL = 1
} ioapic_trigger_mode_t;

/* Polarity */
typedef enum {
    IOAPIC_POLARITY_HIGH = 0,
    IOAPIC_POLARITY_LOW = 1
} ioapic_polarity_t;

/* Redirection table entry */
typedef struct ioapic_redir_entry {
    uint64_t vector         : 8;
    uint64_t delivery_mode  : 3;
    uint64_t dest_mode      : 1;
    uint64_t delivery_status: 1;
    uint64_t polarity       : 1;
    uint64_t remote_irr     : 1;
    uint64_t trigger_mode   : 1;
    uint64_t mask           : 1;
    uint64_t reserved       : 39;
    uint64_t destination    : 8;
} __attribute__((packed)) ioapic_redir_entry_t;

/* IOAPIC device structure */
typedef struct ioapic {
    uint32_t id;
    volatile void *base_address;
    uint32_t gsi_base;  /* Global System Interrupt base */
    uint32_t num_entries;
    uint8_t version;
    uint8_t max_redir_entry;
    bool initialized;
} ioapic_t;

/* IRQ routing information */
typedef struct irq_route {
    uint32_t gsi;           /* Global System Interrupt */
    uint32_t ioapic_id;
    uint32_t pin;
    uint8_t vector;
    ioapic_trigger_mode_t trigger;
    ioapic_polarity_t polarity;
    bool masked;
    void (*handler)(uint32_t irq, void *data);
    void *handler_data;
} irq_route_t;

/* IOAPIC API */
int ioapic_init(void);
int ioapic_register(uint32_t id, uintptr_t base_addr, uint32_t gsi_base);
ioapic_t *ioapic_get(uint32_t id);
ioapic_t *ioapic_get_by_gsi(uint32_t gsi);

/* Register access */
uint32_t ioapic_read_reg(ioapic_t *ioapic, uint8_t reg);
void ioapic_write_reg(ioapic_t *ioapic, uint8_t reg, uint32_t value);
uint64_t ioapic_read_redir_entry(ioapic_t *ioapic, uint8_t pin);
void ioapic_write_redir_entry(ioapic_t *ioapic, uint8_t pin, uint64_t value);

/* IRQ management */
int ioapic_set_irq(uint32_t gsi, uint8_t vector, uint8_t dest,
                   ioapic_delivery_mode_t delivery_mode,
                   ioapic_dest_mode_t dest_mode,
                   ioapic_trigger_mode_t trigger_mode,
                   ioapic_polarity_t polarity);
int ioapic_mask_irq(uint32_t gsi);
int ioapic_unmask_irq(uint32_t gsi);
int ioapic_set_irq_handler(uint32_t gsi, void (*handler)(uint32_t, void *), void *data);

/* Legacy IRQ support */
int ioapic_setup_legacy_irq(uint32_t irq, uint8_t vector);
int ioapic_enable_legacy_mode(void);
int ioapic_disable_legacy_mode(void);

/* IRQ affinity */
int ioapic_set_irq_affinity(uint32_t gsi, uint32_t cpu_mask);

/* Debugging */
void ioapic_dump_info(ioapic_t *ioapic);
void ioapic_dump_redir_table(ioapic_t *ioapic);

#endif /* BDI_IOAPIC_H */
