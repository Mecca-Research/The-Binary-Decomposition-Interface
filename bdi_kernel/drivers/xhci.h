
/**
 * @file xhci.h
 * @brief xHCI (USB 3.0) driver with zero-copy I/O
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#ifndef BDI_XHCI_H
#define BDI_XHCI_H

#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <errno.h>

/* xHCI TRB types */
#define XHCI_TRB_NORMAL         1
#define XHCI_TRB_SETUP          2
#define XHCI_TRB_DATA           3
#define XHCI_TRB_STATUS         4
#define XHCI_TRB_LINK           6
#define XHCI_TRB_EVENT_DATA     7
#define XHCI_TRB_TRANSFER       32
#define XHCI_TRB_CMD_COMPLETE   33

/**
 * @brief xHCI transfer request block (TRB)
 */
struct xhci_trb {
    uint64_t parameter;
    uint32_t status;
    uint32_t control;
} __attribute__((packed));

/**
 * @brief xHCI transfer ring
 */
struct xhci_ring {
    struct xhci_trb *trbs;        /* Transfer request blocks */
    _Atomic uint32_t enqueue;     /* Enqueue pointer */
    _Atomic uint32_t dequeue;     /* Dequeue pointer */
    uint32_t num_trbs;            /* Number of TRBs */
    uint8_t cycle_state;          /* Cycle state */
};

/**
 * @brief xHCI controller
 */
struct xhci_ctrl {
    volatile void *bar;           /* Base address register (MMIO) */
    struct xhci_ring cmd_ring;    /* Command ring */
    struct xhci_ring event_ring;  /* Event ring */
    struct xhci_ring *transfer_rings; /* Transfer rings (per endpoint) */
    uint32_t num_slots;           /* Number of device slots */
    uint32_t max_ports;           /* Maximum ports */
};

/* Function prototypes */
int xhci_init(struct xhci_ctrl *ctrl, volatile void *bar);
int xhci_submit_trb(struct xhci_ring *ring, struct xhci_trb *trb);
int xhci_poll_event(struct xhci_ctrl *ctrl, struct xhci_trb *event);
int xhci_transfer(struct xhci_ctrl *ctrl, uint8_t slot, uint8_t ep, void *buf, size_t len);
void xhci_cleanup(struct xhci_ctrl *ctrl);

/* MMIO helpers */
static inline uint32_t xhci_read_reg32(volatile void *addr) {
    return atomic_load_explicit((_Atomic uint32_t *)addr, memory_order_acquire);
}

static inline void xhci_write_reg32(volatile void *addr, uint32_t val) {
    atomic_store_explicit((_Atomic uint32_t *)addr, val, memory_order_release);
}

#endif /* BDI_XHCI_H */
