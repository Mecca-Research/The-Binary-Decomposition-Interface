
// ===================================================================
// DESC: AHCI SATA driver implementation - SATA storage fallback
// ===================================================================

#include "ahci.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- Register Access Functions ---

uint32_t ahci_read_reg32(ahci_controller_t* ctrl, uint32_t offset) {
    return *(volatile uint32_t*)(ctrl->mmio_base + offset);
}

void ahci_write_reg32(ahci_controller_t* ctrl, uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)(ctrl->mmio_base + offset) = value;
}

uint32_t ahci_port_read_reg32(ahci_controller_t* ctrl, uint32_t port_num, uint32_t offset) {
    return *(volatile uint32_t*)(ctrl->mmio_base + 0x100 + (port_num * 0x80) + offset);
}

void ahci_port_write_reg32(ahci_controller_t* ctrl, uint32_t port_num, uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)(ctrl->mmio_base + 0x100 + (port_num * 0x80) + offset) = value;
}

// --- Controller Management ---

int ahci_init_controller(ahci_controller_t* ctrl, volatile uint8_t* mmio_base) {
    memset(ctrl, 0, sizeof(ahci_controller_t));
    ctrl->mmio_base = mmio_base;
    
    // Read capabilities
    uint32_t cap = ahci_read_reg32(ctrl, AHCI_GHC_CAP);
    ctrl->num_command_slots = ((cap >> 8) & 0x1F) + 1;
    ctrl->ports_implemented = ahci_read_reg32(ctrl, AHCI_GHC_PI);
    
    printf("AHCI: Controller capabilities: 0x%x\n", cap);
    printf("AHCI: Command slots: %u\n", ctrl->num_command_slots);
    printf("AHCI: Ports implemented: 0x%x\n", ctrl->ports_implemented);
    
    // Enable AHCI mode
    uint32_t ghc = ahci_read_reg32(ctrl, AHCI_GHC_GHC);
    ghc |= AHCI_GHC_AE;
    ahci_write_reg32(ctrl, AHCI_GHC_GHC, ghc);
    
    // Reset HBA
    ghc |= AHCI_GHC_HR;
    ahci_write_reg32(ctrl, AHCI_GHC_GHC, ghc);
    
    // Wait for reset to complete
    int timeout = 1000;
    while (timeout-- > 0) {
        ghc = ahci_read_reg32(ctrl, AHCI_GHC_GHC);
        if (!(ghc & AHCI_GHC_HR)) {
            break;
        }
        for (volatile int i = 0; i < 100000; i++);
    }
    
    if (ghc & AHCI_GHC_HR) {
        printf("AHCI: HBA reset timeout\n");
        return AHCI_ERROR_TIMEOUT;
    }
    
    // Re-enable AHCI mode after reset
    ghc = ahci_read_reg32(ctrl, AHCI_GHC_GHC);
    ghc |= AHCI_GHC_AE;
    ahci_write_reg32(ctrl, AHCI_GHC_GHC, ghc);
    
    // Enable interrupts
    ghc |= AHCI_GHC_IE;
    ahci_write_reg32(ctrl, AHCI_GHC_GHC, ghc);
    
    // Initialize ports
    for (int i = 0; i < 32; i++) {
        if (ctrl->ports_implemented & (1 << i)) {
            if (ahci_init_port(ctrl, i) == AHCI_SUCCESS) {
                printf("AHCI: Port %d initialized\n", i);
            }
        }
    }
    
    ctrl->initialized = true;
    printf("AHCI: Controller initialized successfully\n");
    return AHCI_SUCCESS;
}

// --- Port Management ---

int ahci_init_port(ahci_controller_t* ctrl, uint32_t port_num) {
    if (port_num >= 32) {
        return AHCI_ERROR_INVALID;
    }
    
    ahci_port_t* port = &ctrl->ports[port_num];
    port->port_num = port_num;
    port->port_base = ctrl->mmio_base + 0x100 + (port_num * 0x80);
    
    // Check if device is present
    uint32_t ssts = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_SSTS);
    uint32_t det = ssts & AHCI_PORT_SSTS_DET_MASK;
    
    if (det != AHCI_PORT_SSTS_DET_ESTABLISHED) {
        printf("AHCI: No device on port %u (SSTS=0x%x)\n", port_num, ssts);
        return AHCI_ERROR_NOT_READY;
    }
    
    port->device_present = true;
    
    // Stop port before configuration
    ahci_stop_port(ctrl, port_num);
    
    // Allocate command list (1KB aligned)
    port->cmd_list = (ahci_cmd_header_t*)malloc(1024);
    if (!port->cmd_list) {
        return AHCI_ERROR_NO_MEMORY;
    }
    memset(port->cmd_list, 0, 1024);
    
    // Allocate FIS receive area (256 bytes aligned)
    port->fis_base = (uint8_t*)malloc(256);
    if (!port->fis_base) {
        free(port->cmd_list);
        return AHCI_ERROR_NO_MEMORY;
    }
    memset(port->fis_base, 0, 256);
    
    // Allocate command tables
    for (int i = 0; i < ctrl->num_command_slots; i++) {
        port->cmd_tables[i] = (ahci_cmd_table_t*)malloc(sizeof(ahci_cmd_table_t) + sizeof(ahci_prd_t) * 16);
        if (!port->cmd_tables[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                free(port->cmd_tables[j]);
            }
            free(port->cmd_list);
            free(port->fis_base);
            return AHCI_ERROR_NO_MEMORY;
        }
        memset(port->cmd_tables[i], 0, sizeof(ahci_cmd_table_t) + sizeof(ahci_prd_t) * 16);
        
        // Set command table address in command list
        port->cmd_list[i].ctba = (uint64_t)(uintptr_t)port->cmd_tables[i];
    }
    
    // Set command list and FIS base addresses
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CLB, (uint32_t)(uintptr_t)port->cmd_list);
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CLBU, (uint32_t)((uintptr_t)port->cmd_list >> 32));
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_FB, (uint32_t)(uintptr_t)port->fis_base);
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_FBU, (uint32_t)((uintptr_t)port->fis_base >> 32));
    
    // Clear error status
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_SERR, 0xFFFFFFFF);
    
    // Enable FIS receive
    uint32_t cmd = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CMD);
    cmd |= AHCI_PORT_CMD_FRE;
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CMD, cmd);
    
    // Start port
    ahci_start_port(ctrl, port_num);
    
    // Identify device
    uint8_t* identify_buffer = (uint8_t*)malloc(512);
    if (identify_buffer) {
        if (ahci_identify_device(ctrl, port_num, identify_buffer) == AHCI_SUCCESS) {
            // Parse identify data
            uint64_t* sectors_ptr = (uint64_t*)(identify_buffer + 200);
            port->sectors = *sectors_ptr;
            port->sector_size = 512; // Standard sector size
            
            printf("AHCI: Port %u device: %llu sectors, %u bytes/sector\n", 
                   port_num, (unsigned long long)port->sectors, port->sector_size);
        }
        free(identify_buffer);
    }
    
    return AHCI_SUCCESS;
}

int ahci_start_port(ahci_controller_t* ctrl, uint32_t port_num) {
    uint32_t cmd = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CMD);
    cmd |= AHCI_PORT_CMD_ST;
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CMD, cmd);
    return AHCI_SUCCESS;
}

int ahci_stop_port(ahci_controller_t* ctrl, uint32_t port_num) {
    uint32_t cmd = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CMD);
    cmd &= ~AHCI_PORT_CMD_ST;
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CMD, cmd);
    
    // Wait for command list to stop
    int timeout = 1000;
    while (timeout-- > 0) {
        cmd = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CMD);
        if (!(cmd & AHCI_PORT_CMD_CR)) {
            break;
        }
        for (volatile int i = 0; i < 1000; i++);
    }
    
    return (timeout > 0) ? AHCI_SUCCESS : AHCI_ERROR_TIMEOUT;
}

bool ahci_port_has_device(ahci_controller_t* ctrl, uint32_t port_num) {
    if (port_num >= 32) {
        return false;
    }
    return ctrl->ports[port_num].device_present;
}

// --- Command Operations ---

static int ahci_send_command(ahci_controller_t* ctrl, uint32_t port_num, uint8_t command, 
                            uint64_t lba, uint16_t count, void* buffer, bool write) {
    ahci_port_t* port = &ctrl->ports[port_num];
    
    // Find free command slot
    uint32_t ci = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CI);
    int slot = -1;
    for (int i = 0; i < ctrl->num_command_slots; i++) {
        if (!(ci & (1 << i))) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return AHCI_ERROR_IO;
    }
    
    // Setup command header
    ahci_cmd_header_t* cmd_header = &port->cmd_list[slot];
    memset(cmd_header, 0, sizeof(ahci_cmd_header_t));
    cmd_header->cfl = sizeof(fis_reg_h2d_t) / 4; // FIS length in DWORDs
    cmd_header->w = write ? 1 : 0;
    cmd_header->prdtl = buffer ? 1 : 0;
    
    // Setup command table
    ahci_cmd_table_t* cmd_table = port->cmd_tables[slot];
    memset(cmd_table, 0, sizeof(ahci_cmd_table_t));
    
    // Setup command FIS
    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)cmd_table->cfis;
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1; // Command
    fis->command = command;
    fis->device = 0x40; // LBA mode
    
    if (command == ATA_CMD_READ_DMA_EXT || command == ATA_CMD_WRITE_DMA_EXT) {
        fis->lba0 = (uint8_t)lba;
        fis->lba1 = (uint8_t)(lba >> 8);
        fis->lba2 = (uint8_t)(lba >> 16);
        fis->lba3 = (uint8_t)(lba >> 24);
        fis->lba4 = (uint8_t)(lba >> 32);
        fis->lba5 = (uint8_t)(lba >> 40);
        fis->countl = (uint8_t)count;
        fis->counth = (uint8_t)(count >> 8);
    }
    
    // Setup PRD if buffer provided
    if (buffer) {
        cmd_table->prdt[0].dba = (uint64_t)(uintptr_t)buffer;
        cmd_table->prdt[0].dbc = (count * 512) - 1; // 0-based byte count
        cmd_table->prdt[0].i = 1; // Interrupt on completion
    }
    
    // Issue command
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_CI, 1 << slot);
    
    // Wait for completion
    int timeout = 10000;
    while (timeout-- > 0) {
        ci = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_CI);
        if (!(ci & (1 << slot))) {
            break;
        }
        for (volatile int i = 0; i < 1000; i++);
    }
    
    if (ci & (1 << slot)) {
        printf("AHCI: Command timeout on port %u\n", port_num);
        return AHCI_ERROR_TIMEOUT;
    }
    
    // Check for errors
    uint32_t is = ahci_port_read_reg32(ctrl, port_num, AHCI_PORT_IS);
    if (is & 0x40000000) { // Task file error
        printf("AHCI: Task file error on port %u\n", port_num);
        ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_IS, is);
        return AHCI_ERROR_IO;
    }
    
    // Clear interrupt status
    ahci_port_write_reg32(ctrl, port_num, AHCI_PORT_IS, is);
    
    return AHCI_SUCCESS;
}

int ahci_identify_device(ahci_controller_t* ctrl, uint32_t port_num, void* buffer) {
    return ahci_send_command(ctrl, port_num, ATA_CMD_IDENTIFY, 0, 1, buffer, false);
}

int ahci_read_sectors(ahci_controller_t* ctrl, uint32_t port_num, uint64_t lba, uint32_t count, void* buffer) {
    return ahci_send_command(ctrl, port_num, ATA_CMD_READ_DMA_EXT, lba, count, buffer, false);
}

int ahci_write_sectors(ahci_controller_t* ctrl, uint32_t port_num, uint64_t lba, uint32_t count, const void* buffer) {
    return ahci_send_command(ctrl, port_num, ATA_CMD_WRITE_DMA_EXT, lba, count, (void*)buffer, true);
}

int ahci_flush_cache(ahci_controller_t* ctrl, uint32_t port_num) {
    return ahci_send_command(ctrl, port_num, ATA_CMD_FLUSH_CACHE_EXT, 0, 0, NULL, false);
}

int ahci_shutdown_controller(ahci_controller_t* ctrl) {
    if (!ctrl->initialized) {
        return AHCI_SUCCESS;
    }
    
    // Stop all ports
    for (int i = 0; i < 32; i++) {
        if (ctrl->ports_implemented & (1 << i)) {
            ahci_stop_port(ctrl, i);
            
            // Free allocated memory
            ahci_port_t* port = &ctrl->ports[i];
            if (port->cmd_list) {
                free(port->cmd_list);
            }
            if (port->fis_base) {
                free(port->fis_base);
            }
            for (int j = 0; j < ctrl->num_command_slots; j++) {
                if (port->cmd_tables[j]) {
                    free(port->cmd_tables[j]);
                }
            }
        }
    }
    
    // Disable interrupts
    uint32_t ghc = ahci_read_reg32(ctrl, AHCI_GHC_GHC);
    ghc &= ~AHCI_GHC_IE;
    ahci_write_reg32(ctrl, AHCI_GHC_GHC, ghc);
    
    ctrl->initialized = false;
    printf("AHCI: Controller shutdown completed\n");
    return AHCI_SUCCESS;
}
