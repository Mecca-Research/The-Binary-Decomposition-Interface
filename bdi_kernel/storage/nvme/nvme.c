
// ===================================================================
// DESC: NVMe driver implementation - Core controller functionality
// ===================================================================

#include "nvme.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- Register Access Functions ---

uint32_t nvme_read_reg32(nvme_controller_t* ctrl, uint32_t offset) {
    return *(volatile uint32_t*)(ctrl->mmio_base + offset);
}

uint64_t nvme_read_reg64(nvme_controller_t* ctrl, uint32_t offset) {
    return *(volatile uint64_t*)(ctrl->mmio_base + offset);
}

void nvme_write_reg32(nvme_controller_t* ctrl, uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)(ctrl->mmio_base + offset) = value;
}

void nvme_write_reg64(nvme_controller_t* ctrl, uint32_t offset, uint64_t value) {
    *(volatile uint64_t*)(ctrl->mmio_base + offset) = value;
}

// --- Controller Management ---

int nvme_init_controller(nvme_controller_t* ctrl, volatile uint8_t* mmio_base) {
    memset(ctrl, 0, sizeof(nvme_controller_t));
    ctrl->mmio_base = mmio_base;
    
    // Read controller capabilities
    ctrl->capabilities = nvme_read_reg64(ctrl, NVME_REG_CAP);
    ctrl->max_queue_entries = (ctrl->capabilities & NVME_CAP_MQES_MASK) + 1;
    ctrl->doorbell_stride = 4 << ((ctrl->capabilities & NVME_CAP_DSTRD_MASK) >> 32);
    
    printf("NVMe: Controller capabilities: 0x%llx\n", (unsigned long long)ctrl->capabilities);
    printf("NVMe: Max queue entries: %u\n", ctrl->max_queue_entries);
    printf("NVMe: Doorbell stride: %u\n", ctrl->doorbell_stride);
    
    // Check if controller is ready
    uint32_t csts = nvme_read_reg32(ctrl, NVME_REG_CSTS);
    if (!(csts & NVME_CSTS_RDY)) {
        printf("NVMe: Controller not ready, attempting reset\n");
        if (nvme_reset_controller(ctrl) != NVME_SUCCESS) {
            return NVME_ERROR_NOT_READY;
        }
    }
    
    // Setup admin queues
    if (nvme_setup_admin_queues(ctrl) != NVME_SUCCESS) {
        printf("NVMe: Failed to setup admin queues\n");
        return NVME_ERROR_INVALID;
    }
    
    // Enable controller
    uint32_t cc = nvme_read_reg32(ctrl, NVME_REG_CC);
    cc |= NVME_CC_EN;
    cc |= (6 << 16); // 64-byte SQ entries
    cc |= (4 << 20); // 16-byte CQ entries
    nvme_write_reg32(ctrl, NVME_REG_CC, cc);
    
    // Wait for controller to become ready
    int timeout = 1000;
    while (timeout-- > 0) {
        csts = nvme_read_reg32(ctrl, NVME_REG_CSTS);
        if (csts & NVME_CSTS_RDY) {
            break;
        }
        // Simple delay (in real implementation, use proper timer)
        for (volatile int i = 0; i < 100000; i++);
    }
    
    if (!(csts & NVME_CSTS_RDY)) {
        printf("NVMe: Controller failed to become ready\n");
        return NVME_ERROR_TIMEOUT;
    }
    
    printf("NVMe: Controller initialized successfully\n");
    ctrl->initialized = true;
    return NVME_SUCCESS;
}

int nvme_reset_controller(nvme_controller_t* ctrl) {
    // Disable controller
    uint32_t cc = nvme_read_reg32(ctrl, NVME_REG_CC);
    cc &= ~NVME_CC_EN;
    nvme_write_reg32(ctrl, NVME_REG_CC, cc);
    
    // Wait for controller to become not ready
    int timeout = 1000;
    while (timeout-- > 0) {
        uint32_t csts = nvme_read_reg32(ctrl, NVME_REG_CSTS);
        if (!(csts & NVME_CSTS_RDY)) {
            break;
        }
        for (volatile int i = 0; i < 100000; i++);
    }
    
    if (timeout <= 0) {
        printf("NVMe: Controller reset timeout\n");
        return NVME_ERROR_TIMEOUT;
    }
    
    printf("NVMe: Controller reset completed\n");
    return NVME_SUCCESS;
}

// --- Queue Management ---

int nvme_setup_admin_queues(nvme_controller_t* ctrl) {
    // Allocate memory for admin queues
    ctrl->admin_sq.commands = (nvme_command_t*)malloc(NVME_ADMIN_QUEUE_SIZE * sizeof(nvme_command_t));
    ctrl->admin_cq.completions = (nvme_completion_t*)malloc(NVME_ADMIN_QUEUE_SIZE * sizeof(nvme_completion_t));
    
    if (!ctrl->admin_sq.commands || !ctrl->admin_cq.completions) {
        return NVME_ERROR_NO_MEMORY;
    }
    
    memset(ctrl->admin_sq.commands, 0, NVME_ADMIN_QUEUE_SIZE * sizeof(nvme_command_t));
    memset(ctrl->admin_cq.completions, 0, NVME_ADMIN_QUEUE_SIZE * sizeof(nvme_completion_t));
    
    // Initialize queue structures
    ctrl->admin_sq.head = 0;
    ctrl->admin_sq.tail = 0;
    ctrl->admin_sq.size = NVME_ADMIN_QUEUE_SIZE;
    ctrl->admin_sq.queue_id = 0;
    ctrl->admin_sq.doorbell = (volatile uint32_t*)(ctrl->mmio_base + NVME_REG_DBS);
    
    ctrl->admin_cq.head = 0;
    ctrl->admin_cq.tail = 0;
    ctrl->admin_cq.size = NVME_ADMIN_QUEUE_SIZE;
    ctrl->admin_cq.queue_id = 0;
    ctrl->admin_cq.phase = 1;
    ctrl->admin_cq.doorbell = (volatile uint32_t*)(ctrl->mmio_base + NVME_REG_DBS + ctrl->doorbell_stride);
    
    // Configure admin queue attributes
    uint32_t aqa = ((NVME_ADMIN_QUEUE_SIZE - 1) << 16) | (NVME_ADMIN_QUEUE_SIZE - 1);
    nvme_write_reg32(ctrl, NVME_REG_AQA, aqa);
    
    // Set admin queue base addresses
    nvme_write_reg64(ctrl, NVME_REG_ASQ, (uint64_t)(uintptr_t)ctrl->admin_sq.commands);
    nvme_write_reg64(ctrl, NVME_REG_ACQ, (uint64_t)(uintptr_t)ctrl->admin_cq.completions);
    
    printf("NVMe: Admin queues setup completed\n");
    return NVME_SUCCESS;
}

// --- Command Submission ---

int nvme_submit_admin_command(nvme_controller_t* ctrl, nvme_command_t* cmd, nvme_completion_t* completion) {
    if (!ctrl->initialized) {
        return NVME_ERROR_NOT_READY;
    }
    
    // Assign command ID
    static uint16_t command_id = 1;
    cmd->command_id = command_id++;
    
    // Copy command to submission queue
    uint32_t tail = ctrl->admin_sq.tail;
    memcpy(&ctrl->admin_sq.commands[tail], cmd, sizeof(nvme_command_t));
    
    // Update tail pointer
    ctrl->admin_sq.tail = (tail + 1) % ctrl->admin_sq.size;
    
    // Ring doorbell
    *ctrl->admin_sq.doorbell = ctrl->admin_sq.tail;
    
    // Poll for completion
    int timeout = 10000;
    while (timeout-- > 0) {
        nvme_completion_t* cqe = &ctrl->admin_cq.completions[ctrl->admin_cq.head];
        
        // Check phase bit
        if (((cqe->status >> 0) & 1) == ctrl->admin_cq.phase) {
            if (cqe->command_id == cmd->command_id) {
                // Found our completion
                if (completion) {
                    memcpy(completion, cqe, sizeof(nvme_completion_t));
                }
                
                // Update head pointer
                ctrl->admin_cq.head = (ctrl->admin_cq.head + 1) % ctrl->admin_cq.size;
                if (ctrl->admin_cq.head == 0) {
                    ctrl->admin_cq.phase = !ctrl->admin_cq.phase;
                }
                
                // Ring completion doorbell
                *ctrl->admin_cq.doorbell = ctrl->admin_cq.head;
                
                // Check status
                if ((cqe->status >> 1) & 0x7FF) {
                    printf("NVMe: Command failed with status 0x%x\n", cqe->status);
                    return NVME_ERROR_IO;
                }
                
                return NVME_SUCCESS;
            }
        }
        
        // Simple delay
        for (volatile int i = 0; i < 1000; i++);
    }
    
    printf("NVMe: Admin command timeout\n");
    return NVME_ERROR_TIMEOUT;
}

// --- High-Level Operations ---

int nvme_identify_controller(nvme_controller_t* ctrl, void* data) {
    nvme_command_t cmd = {0};
    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.nsid = 0;
    cmd.prp1 = (uint64_t)(uintptr_t)data;
    cmd.cdw10 = 1; // Controller identify
    
    return nvme_submit_admin_command(ctrl, &cmd, NULL);
}

int nvme_identify_namespace(nvme_controller_t* ctrl, uint32_t nsid, void* data) {
    nvme_command_t cmd = {0};
    cmd.opcode = NVME_ADMIN_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)(uintptr_t)data;
    cmd.cdw10 = 0; // Namespace identify
    
    return nvme_submit_admin_command(ctrl, &cmd, NULL);
}

int nvme_create_io_queue_pair(nvme_controller_t* ctrl, uint16_t queue_id, uint16_t queue_size) {
    if (queue_id >= NVME_MAX_QUEUES) {
        return NVME_ERROR_INVALID;
    }
    
    // Allocate memory for I/O queues
    ctrl->io_sq[queue_id].commands = (nvme_command_t*)malloc(queue_size * sizeof(nvme_command_t));
    ctrl->io_cq[queue_id].completions = (nvme_completion_t*)malloc(queue_size * sizeof(nvme_completion_t));
    
    if (!ctrl->io_sq[queue_id].commands || !ctrl->io_cq[queue_id].completions) {
        return NVME_ERROR_NO_MEMORY;
    }
    
    memset(ctrl->io_sq[queue_id].commands, 0, queue_size * sizeof(nvme_command_t));
    memset(ctrl->io_cq[queue_id].completions, 0, queue_size * sizeof(nvme_completion_t));
    
    // Initialize queue structures
    ctrl->io_sq[queue_id].head = 0;
    ctrl->io_sq[queue_id].tail = 0;
    ctrl->io_sq[queue_id].size = queue_size;
    ctrl->io_sq[queue_id].queue_id = queue_id;
    ctrl->io_sq[queue_id].doorbell = (volatile uint32_t*)(ctrl->mmio_base + NVME_REG_DBS + 
                                                          (2 * queue_id * ctrl->doorbell_stride));
    
    ctrl->io_cq[queue_id].head = 0;
    ctrl->io_cq[queue_id].tail = 0;
    ctrl->io_cq[queue_id].size = queue_size;
    ctrl->io_cq[queue_id].queue_id = queue_id;
    ctrl->io_cq[queue_id].phase = 1;
    ctrl->io_cq[queue_id].doorbell = (volatile uint32_t*)(ctrl->mmio_base + NVME_REG_DBS + 
                                                          ((2 * queue_id + 1) * ctrl->doorbell_stride));
    
    // Create completion queue first
    nvme_command_t cmd = {0};
    cmd.opcode = NVME_ADMIN_CREATE_CQ;
    cmd.prp1 = (uint64_t)(uintptr_t)ctrl->io_cq[queue_id].completions;
    cmd.cdw10 = ((queue_size - 1) << 16) | queue_id;
    cmd.cdw11 = 1; // Physically contiguous
    
    if (nvme_submit_admin_command(ctrl, &cmd, NULL) != NVME_SUCCESS) {
        printf("NVMe: Failed to create completion queue %u\n", queue_id);
        return NVME_ERROR_IO;
    }
    
    // Create submission queue
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_ADMIN_CREATE_SQ;
    cmd.prp1 = (uint64_t)(uintptr_t)ctrl->io_sq[queue_id].commands;
    cmd.cdw10 = ((queue_size - 1) << 16) | queue_id;
    cmd.cdw11 = (queue_id << 16) | 1; // Associated CQ ID and physically contiguous
    
    if (nvme_submit_admin_command(ctrl, &cmd, NULL) != NVME_SUCCESS) {
        printf("NVMe: Failed to create submission queue %u\n", queue_id);
        return NVME_ERROR_IO;
    }
    
    printf("NVMe: Created I/O queue pair %u\n", queue_id);
    return NVME_SUCCESS;
}

int nvme_read_blocks(nvme_controller_t* ctrl, uint32_t nsid, uint64_t lba, uint32_t count, void* buffer) {
    // For simplicity, use admin queue for now (in real implementation, use I/O queues)
    nvme_command_t cmd = {0};
    cmd.opcode = NVME_CMD_READ;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)(uintptr_t)buffer;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = count - 1; // 0-based
    
    return nvme_submit_admin_command(ctrl, &cmd, NULL);
}

int nvme_write_blocks(nvme_controller_t* ctrl, uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer) {
    nvme_command_t cmd = {0};
    cmd.opcode = NVME_CMD_WRITE;
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)(uintptr_t)buffer;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = count - 1; // 0-based
    
    return nvme_submit_admin_command(ctrl, &cmd, NULL);
}

int nvme_shutdown_controller(nvme_controller_t* ctrl) {
    if (!ctrl->initialized) {
        return NVME_SUCCESS;
    }
    
    // Shutdown notification
    uint32_t cc = nvme_read_reg32(ctrl, NVME_REG_CC);
    cc |= (1 << 14); // Normal shutdown
    nvme_write_reg32(ctrl, NVME_REG_CC, cc);
    
    // Wait for shutdown to complete
    int timeout = 1000;
    while (timeout-- > 0) {
        uint32_t csts = nvme_read_reg32(ctrl, NVME_REG_CSTS);
        if ((csts & NVME_CSTS_SHST_MASK) == (2 << 2)) { // Shutdown complete
            break;
        }
        for (volatile int i = 0; i < 100000; i++);
    }
    
    // Disable controller
    cc = nvme_read_reg32(ctrl, NVME_REG_CC);
    cc &= ~NVME_CC_EN;
    nvme_write_reg32(ctrl, NVME_REG_CC, cc);
    
    // Free allocated memory
    if (ctrl->admin_sq.commands) {
        free(ctrl->admin_sq.commands);
    }
    if (ctrl->admin_cq.completions) {
        free(ctrl->admin_cq.completions);
    }
    
    for (int i = 0; i < NVME_MAX_QUEUES; i++) {
        if (ctrl->io_sq[i].commands) {
            free(ctrl->io_sq[i].commands);
        }
        if (ctrl->io_cq[i].completions) {
            free(ctrl->io_cq[i].completions);
        }
    }
    
    ctrl->initialized = false;
    printf("NVMe: Controller shutdown completed\n");
    return NVME_SUCCESS;
}
