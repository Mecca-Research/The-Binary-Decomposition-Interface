
/**
 * @file ahci.h
 * @brief AHCI (SATA) driver with zero-copy I/O
 * 
 * Phase 5: Storage I/O Fast Paths
 * 
 * Key Features:
 * - SATA support via AHCI interface
 * - Direct I/O with zero-copy
 * - MMIO optimization with C23 atomics
 * - Command queuing (NCQ)
 * - I/O batching
 */

#ifndef BDI_AHCI_H
#define BDI_AHCI_H

#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <errno.h>

/* AHCI command opcodes (ATA commands) */
#define AHCI_CMD_READ_DMA_EXT   0x25
#define AHCI_CMD_WRITE_DMA_EXT  0x35
#define AHCI_CMD_IDENTIFY       0xEC
#define AHCI_CMD_FLUSH_CACHE    0xE7

/* AHCI port registers offsets */
#define AHCI_PORT_CLB   0x00  /* Command list base */
#define AHCI_PORT_CLBU  0x04  /* Command list base upper */
#define AHCI_PORT_FB    0x08  /* FIS base */
#define AHCI_PORT_FBU   0x0C  /* FIS base upper */
#define AHCI_PORT_IS    0x10  /* Interrupt status */
#define AHCI_PORT_IE    0x14  /* Interrupt enable */
#define AHCI_PORT_CMD   0x18  /* Command and status */
#define AHCI_PORT_TFD   0x20  /* Task file data */
#define AHCI_PORT_SIG   0x24  /* Signature */
#define AHCI_PORT_SSTS  0x28  /* SATA status */
#define AHCI_PORT_SCTL  0x2C  /* SATA control */
#define AHCI_PORT_SERR  0x30  /* SATA error */
#define AHCI_PORT_SACT  0x34  /* SATA active */
#define AHCI_PORT_CI    0x38  /* Command issue */

/* Port command bits */
#define AHCI_PORT_CMD_ST    (1 << 0)  /* Start */
#define AHCI_PORT_CMD_FRE   (1 << 4)  /* FIS receive enable */
#define AHCI_PORT_CMD_FR    (1 << 14) /* FIS receive running */
#define AHCI_PORT_CMD_CR    (1 << 15) /* Command list running */

/* FIS types */
#define FIS_TYPE_REG_H2D    0x27  /* Register FIS - host to device */
#define FIS_TYPE_REG_D2H    0x34  /* Register FIS - device to host */
#define FIS_TYPE_DMA_ACT    0x39  /* DMA activate FIS */
#define FIS_TYPE_DMA_SETUP  0x41  /* DMA setup FIS */

/**
 * @brief AHCI command header (32 bytes)
 */
struct ahci_cmd_header {
    uint16_t flags;
    uint16_t prdtl;               /* PRDT length */
    uint32_t prdbc;               /* PRD byte count */
    uint64_t ctba;                /* Command table base address */
    uint32_t reserved[4];
} __attribute__((packed));

/**
 * @brief AHCI PRDT entry (16 bytes)
 */
struct ahci_prdt_entry {
    uint64_t dba;                 /* Data base address */
    uint32_t reserved;
    uint32_t dbc;                 /* Data byte count (bit 0 must be 1) */
} __attribute__((packed));

/**
 * @brief AHCI command table
 */
struct ahci_cmd_table {
    uint8_t cfis[64];             /* Command FIS */
    uint8_t acmd[16];             /* ATAPI command */
    uint8_t reserved[48];
    struct ahci_prdt_entry prdt[8]; /* PRDT entries */
} __attribute__((packed));

/**
 * @brief AHCI FIS structure
 */
struct ahci_fis {
    uint8_t dsfis[32];            /* DMA setup FIS */
    uint8_t psfis[32];            /* PIO setup FIS */
    uint8_t rfis[24];             /* Register FIS */
    uint8_t sdbfis[8];            /* Set device bits FIS */
    uint8_t ufis[64];             /* Unknown FIS */
    uint8_t reserved[96];
} __attribute__((packed));

/**
 * @brief Register FIS - Host to Device
 */
struct fis_reg_h2d {
    uint8_t fis_type;             /* FIS_TYPE_REG_H2D */
    uint8_t pmport:4;             /* Port multiplier */
    uint8_t rsv0:3;               /* Reserved */
    uint8_t c:1;                  /* 1: Command, 0: Control */
    uint8_t command;              /* Command register */
    uint8_t featurel;             /* Feature register, 7:0 */
    
    uint8_t lba0;                 /* LBA low register, 7:0 */
    uint8_t lba1;                 /* LBA mid register, 15:8 */
    uint8_t lba2;                 /* LBA high register, 23:16 */
    uint8_t device;               /* Device register */
    
    uint8_t lba3;                 /* LBA register, 31:24 */
    uint8_t lba4;                 /* LBA register, 39:32 */
    uint8_t lba5;                 /* LBA register, 47:40 */
    uint8_t featureh;             /* Feature register, 15:8 */
    
    uint16_t count;               /* Count register */
    uint8_t icc;                  /* Isochronous command completion */
    uint8_t control;              /* Control register */
    
    uint32_t rsv1;                /* Reserved */
} __attribute__((packed));

/**
 * @brief AHCI port structure
 */
struct ahci_port {
    volatile void *regs;          /* Port registers (MMIO) */
    struct ahci_cmd_header *cmd_list; /* Command list (32 entries) */
    struct ahci_cmd_table *cmd_tables[32]; /* Command tables */
    struct ahci_fis *fis;         /* FIS receive area */
    _Atomic uint32_t cmd_slot;    /* Command slot bitmap */
    uint32_t num_slots;           /* Number of command slots */
};

/**
 * @brief AHCI controller structure
 */
struct ahci_ctrl {
    volatile void *bar;           /* Base address register (MMIO) */
    struct ahci_port ports[32];   /* Ports */
    uint32_t num_ports;           /* Number of ports */
    uint32_t cap;                 /* Capabilities */
    uint32_t ports_impl;          /* Ports implemented */
};

/* Function prototypes */

/**
 * @brief Initialize AHCI controller
 * @param ctrl Controller structure to initialize
 * @param bar Base address register (MMIO)
 * @return 0 on success, negative error code on failure
 */
int ahci_init(struct ahci_ctrl *ctrl, volatile void *bar);

/**
 * @brief Read from AHCI port (zero-copy)
 * @param port Port to read from
 * @param lba Logical block address
 * @param buf Buffer to read into
 * @param count Number of sectors to read
 * @return 0 on success, negative error code on failure
 */
int ahci_read(struct ahci_port *port, uint64_t lba, void *buf, size_t count);

/**
 * @brief Write to AHCI port (zero-copy)
 * @param port Port to write to
 * @param lba Logical block address
 * @param buf Buffer to write from
 * @param count Number of sectors to write
 * @return 0 on success, negative error code on failure
 */
int ahci_write(struct ahci_port *port, uint64_t lba, const void *buf, size_t count);

/**
 * @brief Submit batch of commands
 * @param port Port to submit to
 * @param cmds Array of command headers
 * @param count Number of commands
 * @return 0 on success, negative error code on failure
 */
int ahci_submit_batch(struct ahci_port *port, struct ahci_cmd_header *cmds, size_t count);

/**
 * @brief Cleanup AHCI controller
 * @param ctrl Controller to cleanup
 */
void ahci_cleanup(struct ahci_ctrl *ctrl);

/* MMIO helper functions using C23 atomics */

/**
 * @brief Read 32-bit register with acquire semantics
 */
static inline uint32_t ahci_read_reg32(volatile void *addr) {
    return atomic_load_explicit((_Atomic uint32_t *)addr, memory_order_acquire);
}

/**
 * @brief Write 32-bit register with release semantics
 */
static inline void ahci_write_reg32(volatile void *addr, uint32_t val) {
    atomic_store_explicit((_Atomic uint32_t *)addr, val, memory_order_release);
}

#endif /* BDI_AHCI_H */
