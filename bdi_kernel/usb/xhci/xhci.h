
// ===================================================================
// DESC: xHCI USB host controller driver header
//       Supporting USB 3.x, 2.0, and 1.x devices with ring structures
// MODERNIZED: Phase 12 - C23 features (nullptr, [[nodiscard]], _Atomic)
#ifndef AEON_XHCI_H
#define AEON_XHCI_H
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>  // C23: Atomic operations
// --- xHCI Register Offsets ---
#define XHCI_CAP_CAPLENGTH      0x00    // Capability Register Length
#define XHCI_CAP_HCIVERSION     0x02    // Interface Version Number
#define XHCI_CAP_HCSPARAMS1     0x04    // Structural Parameters 1
#define XHCI_CAP_HCSPARAMS2     0x08    // Structural Parameters 2
#define XHCI_CAP_HCSPARAMS3     0x0C    // Structural Parameters 3
#define XHCI_CAP_HCCPARAMS1     0x10    // Capability Parameters 1
#define XHCI_CAP_DBOFF          0x14    // Doorbell Offset
#define XHCI_CAP_RTSOFF         0x18    // Runtime Register Space Offset
#define XHCI_CAP_HCCPARAMS2     0x1C    // Capability Parameters 2
// --- Operational Registers (Base + CAPLENGTH) ---
#define XHCI_OP_USBCMD          0x00    // USB Command
#define XHCI_OP_USBSTS          0x04    // USB Status
#define XHCI_OP_PAGESIZE        0x08    // Page Size
#define XHCI_OP_DNCTRL          0x14    // Device Notification Control
#define XHCI_OP_CRCR            0x18    // Command Ring Control Register
#define XHCI_OP_DCBAAP          0x30    // Device Context Base Address Array Pointer
#define XHCI_OP_CONFIG          0x38    // Configure
// --- USB Command Register Bits ---
#define XHCI_CMD_RUN            (1 << 0)    // Run/Stop
#define XHCI_CMD_HCRST          (1 << 1)    // Host Controller Reset
#define XHCI_CMD_INTE           (1 << 2)    // Interrupter Enable
#define XHCI_CMD_HSEE           (1 << 3)    // Host System Error Enable
#define XHCI_CMD_LHCRST         (1 << 7)    // Light Host Controller Reset
#define XHCI_CMD_CSS            (1 << 8)    // Controller Save State
#define XHCI_CMD_CRS            (1 << 9)    // Controller Restore State
#define XHCI_CMD_EWE            (1 << 10)   // Enable Wrap Event
#define XHCI_CMD_EU3S           (1 << 11)   // Enable U3 MFINDEX Stop
// --- USB Status Register Bits ---
#define XHCI_STS_HCH            (1 << 0)    // HC Halted
#define XHCI_STS_HSE            (1 << 2)    // Host System Error
#define XHCI_STS_EINT           (1 << 3)    // Event Interrupt
#define XHCI_STS_PCD            (1 << 4)    // Port Change Detect
#define XHCI_STS_SSS            (1 << 8)    // Save State Status
#define XHCI_STS_RSS            (1 << 9)    // Restore State Status
#define XHCI_STS_SRE            (1 << 10)   // Save/Restore Error
#define XHCI_STS_CNR            (1 << 11)   // Controller Not Ready
#define XHCI_STS_HCE            (1 << 12)   // Host Controller Error
// --- TRB Types ---
#define TRB_TYPE_NORMAL         1
#define TRB_TYPE_SETUP_STAGE    2
#define TRB_TYPE_DATA_STAGE     3
#define TRB_TYPE_STATUS_STAGE   4
#define TRB_TYPE_ISOCH          5
#define TRB_TYPE_LINK           6
#define TRB_TYPE_EVENT_DATA     7
#define TRB_TYPE_NO_OP          8
#define TRB_TYPE_ENABLE_SLOT    9
#define TRB_TYPE_DISABLE_SLOT   10
#define TRB_TYPE_ADDRESS_DEVICE 11
#define TRB_TYPE_CONFIG_EP      12
#define TRB_TYPE_EVALUATE_CTX   13
#define TRB_TYPE_RESET_EP       14
#define TRB_TYPE_STOP_EP        15
#define TRB_TYPE_SET_TR_DEQUEUE 16
#define TRB_TYPE_RESET_DEVICE   17
#define TRB_TYPE_FORCE_EVENT    18
#define TRB_TYPE_NEGOTIATE_BW   19
#define TRB_TYPE_SET_LATENCY    20
#define TRB_TYPE_GET_PORT_BW    21
#define TRB_TYPE_FORCE_HEADER   22
#define TRB_TYPE_NO_OP_CMD      23
// --- Event TRB Types ---
#define TRB_TYPE_TRANSFER       32
#define TRB_TYPE_CMD_COMPLETION 33
#define TRB_TYPE_PORT_STATUS    34
#define TRB_TYPE_BANDWIDTH_REQ  35
#define TRB_TYPE_DOORBELL       36
#define TRB_TYPE_HOST_CTRL      37
#define TRB_TYPE_DEVICE_NOTIFY  38
#define TRB_TYPE_MFINDEX_WRAP   39
// --- TRB Structure (16 bytes) ---
typedef struct {
    uint64_t parameter;     // Parameter or data buffer pointer
    uint32_t status;        // Status field
    uint32_t control;       // Control field (includes type and cycle bit)
} __attribute__((packed)) xhci_trb_t;
// C23: Verify TRB structure is exactly 16 bytes
_Static_assert(sizeof(xhci_trb_t) == 16, "xHCI TRB must be 16 bytes");
// --- TRB Control Field Bits ---
#define TRB_CYCLE_BIT           (1 << 0)
#define TRB_ENT                 (1 << 1)    // Evaluate Next TRB
#define TRB_ISP                 (1 << 2)    // Interrupt on Short Packet
#define TRB_NS                  (1 << 3)    // No Snoop
#define TRB_CH                  (1 << 4)    // Chain bit
#define TRB_IOC                 (1 << 5)    // Interrupt On Completion
#define TRB_IDT                 (1 << 6)    // Immediate Data
#define TRB_BEI                 (1 << 9)    // Block Event Interrupt
#define TRB_TYPE_SHIFT          10
#define TRB_TYPE_MASK           (0x3F << TRB_TYPE_SHIFT)
// --- Ring Structures ---
    xhci_trb_t* trbs;       // TRB array
    uint32_t size;          // Number of TRBs
    _Atomic uint32_t enqueue;       // Enqueue pointer
    _Atomic uint32_t dequeue;       // Dequeue pointer
    _Atomic uint8_t cycle_state;    // Current cycle state
    bool producer;          // True for command/transfer rings, false for event rings
} xhci_ring_t;
// --- Slot Context ---
    uint32_t dev_info;      // Device info
    uint32_t dev_info2;     // Device info 2
    uint32_t tt_info;       // TT Hub Slot ID and Port Number
    uint32_t dev_state;     // Device State
    uint32_t reserved[4];   // Reserved
} __attribute__((packed)) xhci_slot_context_t;
_Static_assert(sizeof(xhci_slot_context_t) == 32, "Slot context must be 32 bytes");
// --- Endpoint Context ---
    uint32_t ep_info;       // Endpoint info
    uint32_t ep_info2;      // Endpoint info 2
    uint64_t dequeue_ptr;   // TR Dequeue Pointer
    uint32_t tx_info;       // Transfer info
    uint32_t reserved[3];   // Reserved
} __attribute__((packed)) xhci_endpoint_context_t;
_Static_assert(sizeof(xhci_endpoint_context_t) == 32, "Endpoint context must be 32 bytes");
// --- Device Context ---
    xhci_slot_context_t slot;
    xhci_endpoint_context_t endpoints[31]; // EP0 to EP30
} __attribute__((packed)) xhci_device_context_t;
// --- Input Context ---
    uint32_t drop_flags;    // Drop Context flags
    uint32_t add_flags;     // Add Context flags
    uint32_t reserved[6];   // Reserved
    xhci_endpoint_context_t endpoints[31];
} __attribute__((packed)) xhci_input_context_t;
// --- Port Registers (Base + 0x400 + port * 0x10) ---
#define XHCI_PORT_SC            0x00    // Port Status and Control
#define XHCI_PORT_PMSC          0x04    // Port Power Management Status and Control
#define XHCI_PORT_LI            0x08    // Port Link Info
#define XHCI_PORT_HLC           0x0C    // Port Hardware LPM Control
// --- Port Status and Control Bits ---
#define XHCI_PORT_CCS           (1 << 0)    // Current Connect Status
#define XHCI_PORT_PED           (1 << 1)    // Port Enabled/Disabled
#define XHCI_PORT_OCA           (1 << 3)    // Over-current Active
#define XHCI_PORT_PR            (1 << 4)    // Port Reset
#define XHCI_PORT_PLS_MASK      (0xF << 5)  // Port Link State
#define XHCI_PORT_PP            (1 << 9)    // Port Power
#define XHCI_PORT_SPEED_MASK    (0xF << 10) // Port Speed
#define XHCI_PORT_PIC_MASK      (3 << 14)   // Port Indicator Control
#define XHCI_PORT_LWS           (1 << 16)   // Port Link State Write Strobe
#define XHCI_PORT_CSC           (1 << 17)   // Connect Status Change
#define XHCI_PORT_PEC           (1 << 18)   // Port Enabled/Disabled Change
#define XHCI_PORT_WRC           (1 << 19)   // Warm Port Reset Change
#define XHCI_PORT_OCC           (1 << 20)   // Over-current Change
#define XHCI_PORT_PRC           (1 << 21)   // Port Reset Change
#define XHCI_PORT_PLC           (1 << 22)   // Port Link State Change
#define XHCI_PORT_CEC           (1 << 23)   // Port Config Error Change
#define XHCI_PORT_CAS           (1 << 24)   // Cold Attach Status
#define XHCI_PORT_WCE           (1 << 25)   // Wake on Connect Enable
#define XHCI_PORT_WDE           (1 << 26)   // Wake on Disconnect Enable
#define XHCI_PORT_WOE           (1 << 27)   // Wake on Over-current Enable
#define XHCI_PORT_DR            (1 << 30)   // Device Removable
#define XHCI_PORT_WPR           (1 << 31)   // Warm Port Reset
// --- USB Device Structure ---
    uint8_t slot_id;
    uint8_t port_num;
    uint8_t speed;
    uint8_t address;
    xhci_device_context_t* device_context;
    xhci_ring_t* ep_rings[31]; // Endpoint transfer rings
    bool configured;
} xhci_device_t;
// --- xHCI Controller Structure ---
    volatile uint8_t* mmio_base;
    volatile uint8_t* cap_regs;     // Capability registers
    volatile uint8_t* op_regs;      // Operational registers
    volatile uint8_t* runtime_regs; // Runtime registers
    volatile uint32_t* doorbell_regs; // Doorbell registers
    
    // Controller capabilities
    uint8_t cap_length;
    uint16_t hci_version;
    uint32_t hcs_params1;
    uint32_t hcs_params2;
    uint32_t hcs_params3;
    uint32_t hcc_params1;
    // Derived values
    uint8_t max_slots;
    uint8_t max_intrs;
    uint8_t max_ports;
    uint16_t max_scratchpad_bufs;
    // Rings
    xhci_ring_t* command_ring;
    xhci_ring_t* event_ring;
    // Device Context Base Address Array
    uint64_t* dcbaa;
    // Connected devices
    xhci_device_t devices[256]; // Max 255 devices + slot 0
    uint8_t next_address;
    // Scratchpad buffers
    void** scratchpad_bufs;
    bool initialized;
} xhci_controller_t;
// --- Function Declarations ---
// Controller management
[[nodiscard]] int xhci_init_controller(xhci_controller_t* ctrl, volatile uint8_t* mmio_base);
[[nodiscard]] int xhci_shutdown_controller(xhci_controller_t* ctrl);
[[nodiscard]] int xhci_reset_controller(xhci_controller_t* ctrl);
[[nodiscard]] int xhci_start_controller(xhci_controller_t* ctrl);
[[nodiscard]] int xhci_stop_controller(xhci_controller_t* ctrl);
// Ring management
[[nodiscard]] xhci_ring_t* xhci_create_ring(uint32_t size, bool producer);
void xhci_free_ring(xhci_ring_t* ring);
[[nodiscard]] int xhci_enqueue_trb(xhci_ring_t* ring, xhci_trb_t* trb);
[[nodiscard]] int xhci_dequeue_trb(xhci_ring_t* ring, xhci_trb_t* trb);
// Command operations
[[nodiscard]] int xhci_send_command(xhci_controller_t* ctrl, xhci_trb_t* cmd_trb, xhci_trb_t* event_trb);
[[nodiscard]] int xhci_enable_slot(xhci_controller_t* ctrl, uint8_t* slot_id);
[[nodiscard]] int xhci_disable_slot(xhci_controller_t* ctrl, uint8_t slot_id);
[[nodiscard]] int xhci_address_device(xhci_controller_t* ctrl, uint8_t slot_id, bool bsr);
[[nodiscard]] int xhci_configure_endpoint(xhci_controller_t* ctrl, uint8_t slot_id);
[[nodiscard]] int xhci_reset_endpoint(xhci_controller_t* ctrl, uint8_t slot_id, uint8_t ep_id);
// Device management
[[nodiscard]] int xhci_setup_device(xhci_controller_t* ctrl, uint8_t port_num);
[[nodiscard]] int xhci_configure_device(xhci_controller_t* ctrl, uint8_t slot_id);
[[nodiscard]] xhci_device_t* xhci_get_device(xhci_controller_t* ctrl, uint8_t slot_id);
// Transfer operations
[[nodiscard]] int xhci_control_transfer(xhci_controller_t* ctrl, uint8_t slot_id, 
                         uint8_t request_type, uint8_t request, 
                         uint16_t value, uint16_t index, 
                         void* data, uint16_t length);
[[nodiscard]] int xhci_bulk_transfer(xhci_controller_t* ctrl, uint8_t slot_id, uint8_t ep_id,
                      void* data, uint32_t length, bool in);
[[nodiscard]] int xhci_interrupt_transfer(xhci_controller_t* ctrl, uint8_t slot_id, uint8_t ep_id,
                           void* data, uint32_t length, bool in);
// Port management
[[nodiscard]] int xhci_scan_ports(xhci_controller_t* ctrl);
[[nodiscard]] int xhci_reset_port(xhci_controller_t* ctrl, uint8_t port_num);
[[nodiscard]] uint32_t xhci_read_port_reg(xhci_controller_t* ctrl, uint8_t port_num, uint32_t offset);
void xhci_write_port_reg(xhci_controller_t* ctrl, uint8_t port_num, uint32_t offset, uint32_t value);
// Event handling
[[nodiscard]] int xhci_handle_events(xhci_controller_t* ctrl);
[[nodiscard]] int xhci_handle_port_status_event(xhci_controller_t* ctrl, xhci_trb_t* event);
[[nodiscard]] int xhci_handle_transfer_event(xhci_controller_t* ctrl, xhci_trb_t* event);
[[nodiscard]] int xhci_handle_command_completion(xhci_controller_t* ctrl, xhci_trb_t* event);
// Utility functions
[[nodiscard]] uint32_t xhci_read_cap_reg32(xhci_controller_t* ctrl, uint32_t offset);
[[nodiscard]] uint32_t xhci_read_op_reg32(xhci_controller_t* ctrl, uint32_t offset);
void xhci_write_op_reg32(xhci_controller_t* ctrl, uint32_t offset, uint32_t value);
[[nodiscard]] uint64_t xhci_read_op_reg64(xhci_controller_t* ctrl, uint32_t offset);
void xhci_write_op_reg64(xhci_controller_t* ctrl, uint32_t offset, uint64_t value);
// Error codes
#define XHCI_SUCCESS            0
#define XHCI_ERROR_TIMEOUT      -1
#define XHCI_ERROR_INVALID      -2
#define XHCI_ERROR_NO_MEMORY    -3
#define XHCI_ERROR_IO           -4
#define XHCI_ERROR_NOT_READY    -5
#define XHCI_ERROR_STALL        -6
#endif // AEON_XHCI_H
