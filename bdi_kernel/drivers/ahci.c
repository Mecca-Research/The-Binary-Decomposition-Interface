
/**
 * @file ahci.c
 * @brief AHCI (SATA) driver implementation
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#include "ahci.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* AHCI HBA registers */
#define AHCI_HBA_CAP        0x00
#define AHCI_HBA_GHC        0x04
#define AHCI_H BA_IS         0x08
#define AHCI_HBA_PI         0x0C
#define AHCI_HBA_VS         0x10

/* Global HBA control bits */
#define AHCI_GHC_AHCI_ENABLE (1 << 31)
#define AHCI_GHC_RESET       (1 << 0)

[[maybe_unused]] static int ahci_debug = 0;

/**
 * @brief Allocate command slot
 */
static int ahci_alloc_cmd_slot(struct ahci_port *port) {
    uint32_t slots = atomic_load_explicit(&port->cmd_slot, memory_order_acquire);
    
    for (uint32_t i = 0; i < port->num_slots; i++) {
        if (!(slots & (1 << i))) {
            /* Try to claim this slot */
            uint32_t new_slots = slots | (1 << i);
            if (atomic_compare_exchange_strong(&port->cmd_slot, &slots, new_slots)) {
                return i;
            }
        }
    }
    
    return -ENOSPC;
}

/**
 * @brief Free command slot
 */
static void ahci_free_cmd_slot(struct ahci_port *port, int slot) {
    uint32_t slots = atomic_load_explicit(&port->cmd_slot, memory_order_acquire);
    uint32_t new_slots = slots & ~(1 << slot);
    atomic_store_explicit(&port->cmd_slot, new_slots, memory_order_release);
}

/**
 * @brief Wait for port to be idle
 */
static int ahci_port_wait_idle(struct ahci_port *port) {
    int timeout = 1000;
    
    while (timeout-- > 0) {
        uint32_t cmd = ahci_read_reg32((volatile char *)port->regs + AHCI_PORT_CMD);
        if (!(cmd & (AHCI_PORT_CMD_ST | AHCI_PORT_CMD_CR))) {
            return 0;
        }
        usleep(1000);
    }
    
    return -ETIMEDOUT;
}

/**
 * @brief Start port
 */
static int ahci_port_start(struct ahci_port *port) {
    uint32_t cmd;
    
    /* Wait for port to be idle */
    if (ahci_port_wait_idle(port) < 0) {
        return -ETIMEDOUT;
    }
    
    /* Enable FIS receive */
    cmd = ahci_read_reg32((volatile char *)port->regs + AHCI_PORT_CMD);
    cmd |= AHCI_PORT_CMD_FRE;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CMD, cmd);
    
    /* Start command processing */
    cmd |= AHCI_PORT_CMD_ST;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CMD, cmd);
    
    return 0;
}

/**
 * @brief Stop port
 */
static int ahci_port_stop(struct ahci_port *port) {
    uint32_t cmd;
    
    /* Stop command processing */
    cmd = ahci_read_reg32((volatile char *)port->regs + AHCI_PORT_CMD);
    cmd &= ~AHCI_PORT_CMD_ST;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CMD, cmd);
    
    /* Wait for port to stop */
    if (ahci_port_wait_idle(port) < 0) {
        return -ETIMEDOUT;
    }
    
    /* Disable FIS receive */
    cmd &= ~AHCI_PORT_CMD_FRE;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CMD, cmd);
    
    return 0;
}

/**
 * @brief Initialize AHCI port
 */
static int ahci_init_port(struct ahci_port *port, volatile void *port_regs) {
    port->regs = port_regs;
    port->num_slots = 32;
    atomic_init(&port->cmd_slot, 0);
    
    /* Allocate command list (1KB, 32 entries) */
    port->cmd_list = aligned_alloc(1024, 1024);
    if (!port->cmd_list) {
        return -ENOMEM;
    }
    memset(port->cmd_list, 0, 1024);
    
    /* Allocate FIS receive area (256 bytes) */
    port->fis = aligned_alloc(256, 256);
    if (!port->fis) {
        free(port->cmd_list);
        return -ENOMEM;
    }
    memset(port->fis, 0, 256);
    
    /* Allocate command tables */
    for (int i = 0; i < 32; i++) {
        port->cmd_tables[i] = aligned_alloc(128, sizeof(struct ahci_cmd_table));
        if (!port->cmd_tables[i]) {
            for (int j = 0; j < i; j++) {
                free(port->cmd_tables[j]);
            }
            free(port->fis);
            free(port->cmd_list);
            return -ENOMEM;
        }
        memset(port->cmd_tables[i], 0, sizeof(struct ahci_cmd_table));
        
        /* Set command table address in command list */
        port->cmd_list[i].ctba = (uint64_t)port->cmd_tables[i];
    }
    
    /* Stop port */
    ahci_port_stop(port);
    
    /* Set command list and FIS base addresses */
    ahci_write_reg32((volatile char *)port_regs + AHCI_PORT_CLB, (uint32_t)(uint64_t)port->cmd_list);
    ahci_write_reg32((volatile char *)port_regs + AHCI_PORT_CLBU, (uint32_t)((uint64_t)port->cmd_list >> 32));
    ahci_write_reg32((volatile char *)port_regs + AHCI_PORT_FB, (uint32_t)(uint64_t)port->fis);
    ahci_write_reg32((volatile char *)port_regs + AHCI_PORT_FBU, (uint32_t)((uint64_t)port->fis >> 32));
    
    /* Clear interrupt status */
    ahci_write_reg32((volatile char *)port_regs + AHCI_PORT_IS, 0xFFFFFFFF);
    
    /* Start port */
    return ahci_port_start(port);
}

/**
 * @brief Initialize AHCI controller
 */
int ahci_init(struct ahci_ctrl *ctrl, volatile void *bar) {
    uint32_t cap, ghc, pi;
    int ret;
    
    if (!ctrl || !bar) {
        return -EINVAL;
    }
    
    memset(ctrl, 0, sizeof(*ctrl));
    ctrl->bar = bar;
    
    /* Read capabilities */
    cap = ahci_read_reg32((volatile char *)bar + AHCI_HBA_CAP);
    ctrl->cap = cap;
    
    /* Enable AHCI mode */
    ghc = ahci_read_reg32((volatile char *)bar + AHCI_HBA_GHC);
    ghc |= AHCI_GHC_AHCI_ENABLE;
    ahci_write_reg32((volatile char *)bar + AHCI_HBA_GHC, ghc);
    
    /* Get ports implemented */
    pi = ahci_read_reg32((volatile char *)bar + AHCI_HBA_PI);
    ctrl->ports_impl = pi;
    
    /* Initialize implemented ports */
    ctrl->num_ports = 0;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            volatile void *port_regs = (volatile char *)bar + 0x100 + (i * 0x80);
            ret = ahci_init_port(&ctrl->ports[i], port_regs);
            if (ret < 0) {
                /* Cleanup already initialized ports */
                for (int j = 0; j < i; j++) {
                    if (pi & (1 << j)) {
                        ahci_port_stop(&ctrl->ports[j]);
                        free(ctrl->ports[j].cmd_list);
                        free(ctrl->ports[j].fis);
                        for (int k = 0; k < 32; k++) {
                            free(ctrl->ports[j].cmd_tables[k]);
                        }
                    }
                }
                return ret;
            }
            ctrl->num_ports++;
        }
    }
    
    return 0;
}

/**
 * @brief Read from AHCI port
 */
int ahci_read(struct ahci_port *port, uint64_t lba, void *buf, size_t count) {
    int slot;
    struct ahci_cmd_header *cmd_hdr;
    struct ahci_cmd_table *cmd_tbl;
    struct fis_reg_h2d *fis;
    uint32_t ci;
    int timeout;
    
    if (!port || !buf || count == 0) {
        return -EINVAL;
    }
    
    /* Allocate command slot */
    slot = ahci_alloc_cmd_slot(port);
    if (slot < 0) {
        return slot;
    }
    
    cmd_hdr = &port->cmd_list[slot];
    cmd_tbl = port->cmd_tables[slot];
    
    /* Setup command header */
    cmd_hdr->flags = (sizeof(struct fis_reg_h2d) / 4) | (0 << 5); /* FIS length, read */
    cmd_hdr->prdtl = 1; /* One PRDT entry */
    cmd_hdr->prdbc = 0;
    
    /* Setup PRDT */
    cmd_tbl->prdt[0].dba = (uint64_t)buf;
    cmd_tbl->prdt[0].dbc = (count * 512) - 1; /* Byte count - 1 */
    
    /* Setup command FIS */
    fis = (struct fis_reg_h2d *)cmd_tbl->cfis;
    memset(fis, 0, sizeof(*fis));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1; /* Command */
    fis->command = AHCI_CMD_READ_DMA_EXT;
    fis->lba0 = lba & 0xFF;
    fis->lba1 = (lba >> 8) & 0xFF;
    fis->lba2 = (lba >> 16) & 0xFF;
    fis->lba3 = (lba >> 24) & 0xFF;
    fis->lba4 = (lba >> 32) & 0xFF;
    fis->lba5 = (lba >> 40) & 0xFF;
    fis->device = 1 << 6; /* LBA mode */
    fis->count = count & 0xFFFF;
    
    /* Issue command */
    ci = 1 << slot;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CI, ci);
    
    /* Wait for completion */
    timeout = 1000;
    while (timeout-- > 0) {
        ci = ahci_read_reg32((volatile char *)port->regs + AHCI_PORT_CI);
        if (!(ci & (1 << slot))) {
            break;
        }
        usleep(1000);
    }
    
    /* Free command slot */
    ahci_free_cmd_slot(port, slot);
    
    if (timeout <= 0) {
        return -ETIMEDOUT;
    }
    
    return 0;
}

/**
 * @brief Write to AHCI port
 */
int ahci_write(struct ahci_port *port, uint64_t lba, const void *buf, size_t count) {
    int slot;
    struct ahci_cmd_header *cmd_hdr;
    struct ahci_cmd_table *cmd_tbl;
    struct fis_reg_h2d *fis;
    uint32_t ci;
    int timeout;
    
    if (!port || !buf || count == 0) {
        return -EINVAL;
    }
    
    /* Allocate command slot */
    slot = ahci_alloc_cmd_slot(port);
    if (slot < 0) {
        return slot;
    }
    
    cmd_hdr = &port->cmd_list[slot];
    cmd_tbl = port->cmd_tables[slot];
    
    /* Setup command header */
    cmd_hdr->flags = (sizeof(struct fis_reg_h2d) / 4) | (1 << 6); /* FIS length, write */
    cmd_hdr->prdtl = 1;
    cmd_hdr->prdbc = 0;
    
    /* Setup PRDT */
    cmd_tbl->prdt[0].dba = (uint64_t)buf;
    cmd_tbl->prdt[0].dbc = (count * 512) - 1;
    
    /* Setup command FIS */
    fis = (struct fis_reg_h2d *)cmd_tbl->cfis;
    memset(fis, 0, sizeof(*fis));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = AHCI_CMD_WRITE_DMA_EXT;
    fis->lba0 = lba & 0xFF;
    fis->lba1 = (lba >> 8) & 0xFF;
    fis->lba2 = (lba >> 16) & 0xFF;
    fis->lba3 = (lba >> 24) & 0xFF;
    fis->lba4 = (lba >> 32) & 0xFF;
    fis->lba5 = (lba >> 40) & 0xFF;
    fis->device = 1 << 6;
    fis->count = count & 0xFFFF;
    
    /* Issue command */
    ci = 1 << slot;
    ahci_write_reg32((volatile char *)port->regs + AHCI_PORT_CI, ci);
    
    /* Wait for completion */
    timeout = 1000;
    while (timeout-- > 0) {
        ci = ahci_read_reg32((volatile char *)port->regs + AHCI_PORT_CI);
        if (!(ci & (1 << slot))) {
            break;
        }
        usleep(1000);
    }
    
    ahci_free_cmd_slot(port, slot);
    
    if (timeout <= 0) {
        return -ETIMEDOUT;
    }
    
    return 0;
}

/**
 * @brief Submit batch of commands
 */
int ahci_submit_batch(struct ahci_port *port, struct ahci_cmd_header *cmds, size_t count) {
    /* TODO: Implement batch submission */
    /* This would involve setting up multiple command slots and issuing them together */
    return -ENOSYS;
}

/**
 * @brief Cleanup AHCI controller
 */
void ahci_cleanup(struct ahci_ctrl *ctrl) {
    if (!ctrl) {
        return;
    }
    
    for (uint32_t i = 0; i < 32; i++) {
        if (ctrl->ports_impl & (1 << i)) {
            ahci_port_stop(&ctrl->ports[i]);
            free(ctrl->ports[i].cmd_list);
            free(ctrl->ports[i].fis);
            for (int j = 0; j < 32; j++) {
                free(ctrl->ports[i].cmd_tables[j]);
            }
        }
    }
    
    memset(ctrl, 0, sizeof(*ctrl));
}
