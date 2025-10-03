
// ===================================================================
// DESC: xHCI (eXtensible Host Controller Interface) driver for BDI Kernel
//       Main xHCI controller implementation
// ===================================================================
// MODERNIZED: Phase 12 - C23 features (nullptr, [[nodiscard]], _Atomic)

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// xHCI Constants
#define XHCI_MAX_DEVICES        128
#define XHCI_MAX_ENDPOINTS      32
#define XHCI_MAX_SLOTS          256
#define XHCI_MAX_PORTS          127

// xHCI Capability Registers
#define XHCI_CAP_CAPLENGTH      0x00
#define XHCI_CAP_HCIVERSION     0x02
#define XHCI_CAP_HCSPARAMS1     0x04
#define XHCI_CAP_HCSPARAMS2     0x08
#define XHCI_CAP_HCSPARAMS3     0x0C
#define XHCI_CAP_HCCPARAMS1     0x10
#define XHCI_CAP_DBOFF          0x14
#define XHCI_CAP_RTSOFF         0x18
#define XHCI_CAP_HCCPARAMS2     0x1C

// xHCI Operational Registers
#define XHCI_OP_USBCMD          0x00
#define XHCI_OP_USBSTS          0x04
#define XHCI_OP_PAGESIZE        0x08
#define XHCI_OP_DNCTRL          0x14
#define XHCI_OP_CRCR            0x18
#define XHCI_OP_DCBAAP          0x30
#define XHCI_OP_CONFIG          0x38

// xHCI Command Register Bits
#define XHCI_CMD_RUN            0x00000001
#define XHCI_CMD_HCRST          0x00000002
#define XHCI_CMD_INTE           0x00000004
#define XHCI_CMD_HSEE           0x00000008

// xHCI Status Register Bits
#define XHCI_STS_HCH            0x00000001
#define XHCI_STS_HSE            0x00000004
#define XHCI_STS_EINT           0x00000008
#define XHCI_STS_PCD            0x00000010
#define XHCI_STS_SSS            0x00000100
#define XHCI_STS_RSS            0x00000200
#define XHCI_STS_SRE            0x00000400
#define XHCI_STS_CNR            0x00000800
#define XHCI_STS_HCE            0x00001000

// xHCI Port Status Bits
#define XHCI_PORTSC_CCS         0x00000001
#define XHCI_PORTSC_PED         0x00000002
#define XHCI_PORTSC_OCA         0x00000008
#define XHCI_PORTSC_PR          0x00000010
#define XHCI_PORTSC_PLS_MASK    0x000001E0
#define XHCI_PORTSC_PP          0x00000200
#define XHCI_PORTSC_SPEED_MASK  0x00003C00
#define XHCI_PORTSC_PIC_MASK    0x0000C000
#define XHCI_PORTSC_LWS         0x00010000
#define XHCI_PORTSC_CSC         0x00020000
#define XHCI_PORTSC_PEC         0x00040000
#define XHCI_PORTSC_WRC         0x00080000
#define XHCI_PORTSC_OCC         0x00100000
#define XHCI_PORTSC_PRC         0x00200000
#define XHCI_PORTSC_PLC         0x00400000
#define XHCI_PORTSC_CEC         0x00800000
#define XHCI_PORTSC_CAS         0x01000000
#define XHCI_PORTSC_WCE         0x02000000
#define XHCI_PORTSC_WDE         0x04000000
#define XHCI_PORTSC_WOE         0x08000000
#define XHCI_PORTSC_DR          0x40000000
#define XHCI_PORTSC_WPR         0x80000000

// xHCI Controller Structure
typedef struct {
    void *cap_regs;             // Capability registers base
    void *op_regs;              // Operational registers base
    void *runtime_regs;         // Runtime registers base
    void *doorbell_regs;        // Doorbell registers base
    uint32_t max_slots;         // Maximum device slots
    uint32_t max_ports;         // Maximum root hub ports
    uint32_t max_intrs;         // Maximum interrupters
    uint8_t context_size;       // Context size (32 or 64 bytes)
    uint8_t running;            // Controller running flag
    uint8_t initialized;        // Initialization complete flag
} xhci_controller_t;

// xHCI Device Slot Structure
typedef struct {
    uint32_t slot_id;           // Slot ID
    uint32_t device_address;    // USB device address
    uint32_t port_id;           // Root hub port
    uint8_t speed;              // Device speed
    uint8_t state;              // Slot state
    void *device_context;       // Device context
    uint8_t active;             // Slot active flag
} xhci_device_slot_t;

// Global xHCI state
static xhci_controller_t g_xhci_controller;
static xhci_device_slot_t g_device_slots[XHCI_MAX_SLOTS];
static uint8_t g_xhci_initialized = 0;

// Function prototypes
int xhci_init(void *mmio_base);
int xhci_reset_controller(void);
int xhci_start_controller(void);
int xhci_stop_controller(void);
int xhci_enable_slot(uint32_t *slot_id);
int xhci_disable_slot(uint32_t slot_id);
int xhci_address_device(uint32_t slot_id, uint32_t port_id);
int xhci_configure_endpoint(uint32_t slot_id, void *input_context);
int xhci_reset_endpoint(uint32_t slot_id, uint32_t endpoint_id);
int xhci_stop_endpoint(uint32_t slot_id, uint32_t endpoint_id);
int xhci_set_tr_dequeue_pointer(uint32_t slot_id, uint32_t endpoint_id, uint64_t dequeue_ptr);
uint32_t xhci_read_cap_reg(uint32_t offset);
uint32_t xhci_read_op_reg(uint32_t offset);
void xhci_write_op_reg(uint32_t offset, uint32_t value);
uint32_t xhci_read_port_reg(uint32_t port, uint32_t offset);
void xhci_write_port_reg(uint32_t port, uint32_t offset, uint32_t value);
int xhci_handle_port_status_change(uint32_t port);
void xhci_cleanup(void);

/**
 * Initialize xHCI controller
 */
[[nodiscard]] int xhci_init(void *mmio_base) {
    if (!mmio_base || g_xhci_initialized) {
        return -1;
    }
    
    // Clear controller structure
    memset(&g_xhci_controller, 0, sizeof(xhci_controller_t));
    memset(g_device_slots, 0, sizeof(g_device_slots));
    
    // Set up register base addresses
    g_xhci_controller.cap_regs = mmio_base;
    
    // Read capability registers
    uint8_t cap_length = xhci_read_cap_reg(XHCI_CAP_CAPLENGTH) & 0xFF;
    uint32_t hcs_params1 = xhci_read_cap_reg(XHCI_CAP_HCSPARAMS1);
    uint32_t hcc_params1 = xhci_read_cap_reg(XHCI_CAP_HCCPARAMS1);
    
    // Calculate register offsets
    g_xhci_controller.op_regs = (uint8_t *)mmio_base + cap_length;
    g_xhci_controller.runtime_regs = (uint8_t *)mmio_base + 
                                    (xhci_read_cap_reg(XHCI_CAP_RTSOFF) & ~0x1F);
    g_xhci_controller.doorbell_regs = (uint8_t *)mmio_base + 
                                     (xhci_read_cap_reg(XHCI_CAP_DBOFF) & ~0x3);
    
    // Extract controller parameters
    g_xhci_controller.max_slots = hcs_params1 & 0xFF;
    g_xhci_controller.max_ports = (hcs_params1 >> 24) & 0xFF;
    g_xhci_controller.max_intrs = (hcs_params1 >> 8) & 0x7FF;
    g_xhci_controller.context_size = (hcc_params1 & 0x4) ? 64 : 32;
    
    // Reset controller
    if (xhci_reset_controller() != 0) {
        return -1;
    }
    
    // Initialize device context base address array
    // In a real implementation, this would allocate and set up DCBAAP
    
    // Set maximum device slots
    xhci_write_op_reg(XHCI_OP_CONFIG, g_xhci_controller.max_slots);
    
    // Start controller
    if (xhci_start_controller() != 0) {
        return -1;
    }
    
    g_xhci_controller.initialized = 1;
    g_xhci_initialized = 1;
    
    return 0;
}

/**
 * Reset xHCI controller
 */
[[nodiscard]] int xhci_reset_controller(void) {
    // Stop controller if running
    xhci_write_op_reg(XHCI_OP_USBCMD, 0);
    
    // Wait for controller to halt
    uint32_t timeout = 1000;
    while (timeout-- > 0) {
        if (xhci_read_op_reg(XHCI_OP_USBSTS) & XHCI_STS_HCH) {
            break;
        }
        // In a real implementation, this would be a proper delay
    }
    
    if (timeout == 0) {
        return -1; // Timeout waiting for halt
    }
    
    // Reset controller
    xhci_write_op_reg(XHCI_OP_USBCMD, XHCI_CMD_HCRST);
    
    // Wait for reset to complete
    timeout = 1000;
    while (timeout-- > 0) {
        uint32_t cmd = xhci_read_op_reg(XHCI_OP_USBCMD);
        uint32_t sts = xhci_read_op_reg(XHCI_OP_USBSTS);
        if (!(cmd & XHCI_CMD_HCRST) && !(sts & XHCI_STS_CNR)) {
            break;
        }
        // In a real implementation, this would be a proper delay
    }
    
    if (timeout == 0) {
        return -1; // Timeout waiting for reset
    }
    
    return 0;
}

/**
 * Start xHCI controller
 */
[[nodiscard]] int xhci_start_controller(void) {
    // Enable interrupts and start controller
    uint32_t cmd = XHCI_CMD_RUN | XHCI_CMD_INTE;
    xhci_write_op_reg(XHCI_OP_USBCMD, cmd);
    
    // Wait for controller to start
    uint32_t timeout = 1000;
    while (timeout-- > 0) {
        if (!(xhci_read_op_reg(XHCI_OP_USBSTS) & XHCI_STS_HCH)) {
            break;
        }
        // In a real implementation, this would be a proper delay
    }
    
    if (timeout == 0) {
        return -1; // Timeout waiting for start
    }
    
    g_xhci_controller.running = 1;
    return 0;
}

/**
 * Stop xHCI controller
 */
[[nodiscard]] int xhci_stop_controller(void) {
    // Stop controller
    xhci_write_op_reg(XHCI_OP_USBCMD, 0);
    
    // Wait for controller to halt
    uint32_t timeout = 1000;
    while (timeout-- > 0) {
        if (xhci_read_op_reg(XHCI_OP_USBSTS) & XHCI_STS_HCH) {
            break;
        }
        // In a real implementation, this would be a proper delay
    }
    
    if (timeout == 0) {
        return -1; // Timeout waiting for halt
    }
    
    g_xhci_controller.running = 0;
    return 0;
}

/**
 * Enable device slot
 */
[[nodiscard]] int xhci_enable_slot(uint32_t *slot_id) {
    if (!slot_id || !g_xhci_controller.running) {
        return -1;
    }
    
    // Find free slot
    for (uint32_t i = 1; i <= g_xhci_controller.max_slots; i++) {
        if (!g_device_slots[i].active) {
            g_device_slots[i].slot_id = i;
            g_device_slots[i].active = 1;
            g_device_slots[i].state = 1; // Enabled state
            *slot_id = i;
            
            // In a real implementation, this would:
            // 1. Issue Enable Slot command via command ring
            // 2. Wait for command completion
            // 3. Allocate device context
            
            return 0;
        }
    }
    
    return -1; // No free slots
}

/**
 * Disable device slot
 */
[[nodiscard]] int xhci_disable_slot(uint32_t slot_id) {
    if (slot_id == 0 || slot_id > g_xhci_controller.max_slots) {
        return -1;
    }
    
    xhci_device_slot_t *slot = &g_device_slots[slot_id];
    if (!slot->active) {
        return -1; // Slot not active
    }
    
    // In a real implementation, this would:
    // 1. Issue Disable Slot command via command ring
    // 2. Wait for command completion
    // 3. Free device context
    
    // Clear slot
    memset(slot, 0, sizeof(xhci_device_slot_t));
    
    return 0;
}

/**
 * Address device
 */
[[nodiscard]] int xhci_address_device(uint32_t slot_id, uint32_t port_id) {
    if (slot_id == 0 || slot_id > g_xhci_controller.max_slots || 
        port_id == 0 || port_id > g_xhci_controller.max_ports) {
        return -1;
    }
    
    xhci_device_slot_t *slot = &g_device_slots[slot_id];
    if (!slot->active) {
        return -1; // Slot not active
    }
    
    // In a real implementation, this would:
    // 1. Set up input context with slot and endpoint 0 contexts
    // 2. Issue Address Device command via command ring
    // 3. Wait for command completion
    // 4. Update device context with assigned address
    
    slot->port_id = port_id;
    slot->device_address = slot_id; // Simple assignment for simulation
    slot->state = 2; // Addressed state
    
    return 0;
}

/**
 * Configure endpoint
 */
[[nodiscard]] int xhci_configure_endpoint(uint32_t slot_id, void *input_context) {
    if (slot_id == 0 || slot_id > g_xhci_controller.max_slots || !input_context) {
        return -1;
    }
    
    xhci_device_slot_t *slot = &g_device_slots[slot_id];
    if (!slot->active) {
        return -1; // Slot not active
    }
    
    // In a real implementation, this would:
    // 1. Validate input context
    // 2. Issue Configure Endpoint command via command ring
    // 3. Wait for command completion
    // 4. Update device context
    
    slot->state = 3; // Configured state
    
    return 0;
}

/**
 * Read capability register
 */
[[nodiscard]] uint32_t xhci_read_cap_reg(uint32_t offset) {
    if (!g_xhci_controller.cap_regs) {
        return 0;
    }
    
    // In a real implementation, this would read from MMIO
    // For simulation, return reasonable values
    switch (offset) {
        case XHCI_CAP_CAPLENGTH:
            return 0x20; // 32-byte capability length
        case XHCI_CAP_HCIVERSION:
            return 0x0110; // xHCI version 1.1
        case XHCI_CAP_HCSPARAMS1:
            return (127 << 24) | (16 << 8) | 64; // 127 ports, 16 intrs, 64 slots
        case XHCI_CAP_HCSPARAMS2:
            return 0x00000004; // 4 scratchpad buffers
        case XHCI_CAP_HCCPARAMS1:
            return 0x00000004; // 64-byte context size
        case XHCI_CAP_DBOFF:
            return 0x2000; // Doorbell offset
        case XHCI_CAP_RTSOFF:
            return 0x1000; // Runtime offset
        default:
            return 0;
    }
}

/**
 * Read operational register
 */
[[nodiscard]] uint32_t xhci_read_op_reg(uint32_t offset) {
    if (!g_xhci_controller.op_regs) {
        return 0;
    }
    
    // In a real implementation, this would read from MMIO
    // For simulation, return reasonable values
    switch (offset) {
        case XHCI_OP_USBSTS:
            return g_xhci_controller.running ? 0 : XHCI_STS_HCH;
        case XHCI_OP_PAGESIZE:
            return 0x00000001; // 4KB page size
        default:
            return 0;
    }
}

/**
 * Write operational register
 */
void xhci_write_op_reg(uint32_t offset, uint32_t value) {
    if (!g_xhci_controller.op_regs) {
        return;
    }
    
    // In a real implementation, this would write to MMIO
    // For simulation, just track some state changes
    (void)offset;
    (void)value;
}

/**
 * Handle port status change
 */
[[nodiscard]] int xhci_handle_port_status_change(uint32_t port) {
    if (port == 0 || port > g_xhci_controller.max_ports) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Read port status register
    // 2. Check for connect/disconnect events
    // 3. Handle device enumeration
    // 4. Clear status change bits
    
    return 0;
}

/**
 * Get controller information
 */
[[nodiscard]] int xhci_get_controller_info(uint32_t *max_slots, uint32_t *max_ports, uint8_t *context_size) {
    if (!g_xhci_initialized) {
        return -1;
    }
    
    if (max_slots) {
        *max_slots = g_xhci_controller.max_slots;
    }
    if (max_ports) {
        *max_ports = g_xhci_controller.max_ports;
    }
    if (context_size) {
        *context_size = g_xhci_controller.context_size;
    }
    
    return 0;
}

/**
 * Check if controller is running
 */
[[nodiscard]] int xhci_is_running(void) {
    return g_xhci_controller.running;
}

/**
 * Get device slot information
 */
xhci_device_slot_t *xhci_get_device_slot(uint32_t slot_id) {
    if (slot_id == 0 || slot_id > g_xhci_controller.max_slots) {
        return nullptr;
    }
    
    return &g_device_slots[slot_id];
}

/**
 * Cleanup xHCI controller
 */
void xhci_cleanup(void) {
    if (g_xhci_initialized) {
        // Stop controller
        xhci_stop_controller();
        
        // Clear all device slots
        memset(g_device_slots, 0, sizeof(g_device_slots));
        
        // Clear controller structure
        memset(&g_xhci_controller, 0, sizeof(xhci_controller_t));
        
        g_xhci_initialized = 0;
    }
}
