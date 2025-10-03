
// ===================================================================
// DESC: xHCI Command Ring implementation for BDI Kernel
//       Handles xHCI command submission and completion
// ===================================================================
// MODERNIZED: Phase 12 - C23 features (nullptr, [[nodiscard]], _Atomic)

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// xHCI Command TRB Types
#define XHCI_TRB_NORMAL             1
#define XHCI_TRB_SETUP_STAGE        2
#define XHCI_TRB_DATA_STAGE         3
#define XHCI_TRB_STATUS_STAGE       4
#define XHCI_TRB_ISOCH              5
#define XHCI_TRB_LINK               6
#define XHCI_TRB_EVENT_DATA         7
#define XHCI_TRB_NO_OP              8
#define XHCI_TRB_ENABLE_SLOT        9
#define XHCI_TRB_DISABLE_SLOT       10
#define XHCI_TRB_ADDRESS_DEVICE     11
#define XHCI_TRB_CONFIGURE_EP       12
#define XHCI_TRB_EVALUATE_CONTEXT   13
#define XHCI_TRB_RESET_EP           14
#define XHCI_TRB_STOP_EP            15
#define XHCI_TRB_SET_TR_DEQUEUE     16
#define XHCI_TRB_RESET_DEVICE       17
#define XHCI_TRB_FORCE_EVENT        18
#define XHCI_TRB_NEGOTIATE_BW       19
#define XHCI_TRB_SET_LATENCY_TOL    20
#define XHCI_TRB_GET_PORT_BW        21
#define XHCI_TRB_FORCE_HEADER       22
#define XHCI_TRB_NO_OP_CMD          23

// xHCI TRB Completion Codes
#define XHCI_COMP_SUCCESS           1
#define XHCI_COMP_DATA_BUFFER_ERROR 2
#define XHCI_COMP_BABBLE_ERROR      3
#define XHCI_COMP_USB_TRANSACTION_ERROR 4
#define XHCI_COMP_TRB_ERROR         5
#define XHCI_COMP_STALL_ERROR       6
#define XHCI_COMP_RESOURCE_ERROR    7
#define XHCI_COMP_BANDWIDTH_ERROR   8
#define XHCI_COMP_NO_SLOTS_ERROR    9
#define XHCI_COMP_INVALID_STREAM_TYPE 10
#define XHCI_COMP_SLOT_NOT_ENABLED  11
#define XHCI_COMP_EP_NOT_ENABLED    12
#define XHCI_COMP_SHORT_PACKET      13
#define XHCI_COMP_RING_UNDERRUN     14
#define XHCI_COMP_RING_OVERRUN      15
#define XHCI_COMP_VF_RING_FULL      16
#define XHCI_COMP_PARAMETER_ERROR   17
#define XHCI_COMP_BANDWIDTH_OVERRUN 18
#define XHCI_COMP_CONTEXT_STATE_ERROR 19
#define XHCI_COMP_NO_PING_RESPONSE  20
#define XHCI_COMP_EVENT_RING_FULL   21
#define XHCI_COMP_INCOMPATIBLE_DEVICE 22
#define XHCI_COMP_MISSED_SERVICE    23
#define XHCI_COMP_COMMAND_RING_STOPPED 24
#define XHCI_COMP_COMMAND_ABORTED   25
#define XHCI_COMP_STOPPED           26
#define XHCI_COMP_STOPPED_LENGTH_INVALID 27
#define XHCI_COMP_MAX_EXIT_LATENCY  28

// TRB Control Flags
#define XHCI_TRB_CYCLE              0x00000001
#define XHCI_TRB_ENT                0x00000002
#define XHCI_TRB_ISP                0x00000004
#define XHCI_TRB_NS                 0x00000008
#define XHCI_TRB_CH                 0x00000010
#define XHCI_TRB_IOC                0x00000020
#define XHCI_TRB_IDT                0x00000040
#define XHCI_TRB_BEI                0x00000200

// xHCI TRB Structure
typedef struct {
    uint64_t parameter;         // Parameter field
    uint32_t status;            // Status field
    uint32_t control;           // Control field (includes type and flags)
} xhci_trb_t;

// Command Ring Structure
typedef struct {
    xhci_trb_t *trbs;          // TRB array
    uint32_t size;             // Ring size
    uint32_t enqueue;          // Enqueue pointer
    uint32_t dequeue;          // Dequeue pointer
    uint8_t cycle_state;       // Cycle state
    uint8_t running;           // Ring running flag
    void *doorbell_reg;        // Doorbell register
} xhci_command_ring_t;

// Event Ring Structure
typedef struct {
    xhci_trb_t *trbs;          // TRB array
    uint32_t size;             // Ring size
    uint32_t dequeue;          // Dequeue pointer
    uint8_t cycle_state;       // Cycle state
    void *erdp_reg;            // Event ring dequeue pointer register
} xhci_event_ring_t;

// Command Completion Structure
typedef struct {
    uint32_t command_id;       // Command identifier
    uint32_t completion_code;  // Completion code
    uint64_t command_trb;      // Command TRB pointer
    uint32_t slot_id;          // Slot ID (for relevant commands)
    uint8_t completed;         // Completion flag
} xhci_command_completion_t;

// Global command ring state
static xhci_command_ring_t g_command_ring;
static xhci_event_ring_t g_event_ring;
static xhci_command_completion_t g_pending_commands[64];
static uint32_t g_command_id = 1;
static uint8_t g_cmd_initialized = 0;

// Function prototypes
int xhci_cmd_init(void *cmd_ring_mem, uint32_t cmd_ring_size, 
                  void *event_ring_mem, uint32_t event_ring_size,
                  void *doorbell_reg, void *erdp_reg);
int xhci_cmd_submit(xhci_trb_t *trb);
int xhci_cmd_wait_completion(uint32_t command_id, xhci_command_completion_t *completion);
int xhci_cmd_enable_slot(uint32_t *slot_id);
int xhci_cmd_disable_slot(uint32_t slot_id);
int xhci_cmd_address_device(uint32_t slot_id, uint64_t input_context_ptr, uint8_t bsr);
int xhci_cmd_configure_endpoint(uint32_t slot_id, uint64_t input_context_ptr);
int xhci_cmd_evaluate_context(uint32_t slot_id, uint64_t input_context_ptr);
int xhci_cmd_reset_endpoint(uint32_t slot_id, uint32_t endpoint_id);
int xhci_cmd_stop_endpoint(uint32_t slot_id, uint32_t endpoint_id);
int xhci_cmd_set_tr_dequeue_pointer(uint32_t slot_id, uint32_t endpoint_id, uint64_t dequeue_ptr);
int xhci_cmd_reset_device(uint32_t slot_id);
int xhci_cmd_no_op(void);
int xhci_cmd_process_events(void);
void xhci_cmd_ring_doorbell(void);
void xhci_cmd_cleanup(void);

/**
 * Initialize command ring
 */
[[nodiscard]] int xhci_cmd_init(void *cmd_ring_mem, uint32_t cmd_ring_size, 
                  void *event_ring_mem, uint32_t event_ring_size,
                  void *doorbell_reg, void *erdp_reg) {
    if (!cmd_ring_mem || !event_ring_mem || cmd_ring_size == 0 || event_ring_size == 0) {
        return -1;
    }
    
    // Initialize command ring
    g_command_ring.trbs = (xhci_trb_t *)cmd_ring_mem;
    g_command_ring.size = cmd_ring_size;
    g_command_ring.enqueue = 0;
    g_command_ring.dequeue = 0;
    g_command_ring.cycle_state = 1;
    g_command_ring.running = 1;
    g_command_ring.doorbell_reg = doorbell_reg;
    
    // Clear command ring
    memset(g_command_ring.trbs, 0, cmd_ring_size * sizeof(xhci_trb_t));
    
    // Initialize event ring
    g_event_ring.trbs = (xhci_trb_t *)event_ring_mem;
    g_event_ring.size = event_ring_size;
    g_event_ring.dequeue = 0;
    g_event_ring.cycle_state = 1;
    g_event_ring.erdp_reg = erdp_reg;
    
    // Clear event ring
    memset(g_event_ring.trbs, 0, event_ring_size * sizeof(xhci_trb_t));
    
    // Clear pending commands
    memset(g_pending_commands, 0, sizeof(g_pending_commands));
    g_command_id = 1;
    
    g_cmd_initialized = 1;
    return 0;
}

/**
 * Submit command TRB
 */
[[nodiscard]] int xhci_cmd_submit(xhci_trb_t *trb) {
    if (!g_cmd_initialized || !trb || !g_command_ring.running) {
        return -1;
    }
    
    // Check if ring is full
    uint32_t next_enqueue = (g_command_ring.enqueue + 1) % g_command_ring.size;
    if (next_enqueue == g_command_ring.dequeue) {
        return -1; // Ring full
    }
    
    // Set cycle bit
    trb->control &= ~XHCI_TRB_CYCLE;
    if (g_command_ring.cycle_state) {
        trb->control |= XHCI_TRB_CYCLE;
    }
    
    // Copy TRB to ring
    memcpy(&g_command_ring.trbs[g_command_ring.enqueue], trb, sizeof(xhci_trb_t));
    
    // Store command for completion tracking
    uint32_t cmd_id = g_command_id++;
    for (uint32_t i = 0; i < 64; i++) {
        if (!g_pending_commands[i].completed) {
            g_pending_commands[i].command_id = cmd_id;
            g_pending_commands[i].command_trb = (uint64_t)&g_command_ring.trbs[g_command_ring.enqueue];
            g_pending_commands[i].completed = 0;
            break;
        }
    }
    
    // Update enqueue pointer
    g_command_ring.enqueue = next_enqueue;
    if (g_command_ring.enqueue == 0) {
        g_command_ring.cycle_state = !g_command_ring.cycle_state;
    }
    
    // Ring doorbell
    xhci_cmd_ring_doorbell();
    
    return cmd_id;
}

/**
 * Wait for command completion
 */
[[nodiscard]] int xhci_cmd_wait_completion(uint32_t command_id, xhci_command_completion_t *completion) {
    if (!g_cmd_initialized || !completion) {
        return -1;
    }
    
    // In a real implementation, this would wait for interrupt or poll
    // For simulation, process events and mark command as completed
    xhci_cmd_process_events();
    
    // Find command in pending list
    for (uint32_t i = 0; i < 64; i++) {
        if (g_pending_commands[i].command_id == command_id) {
            // Simulate successful completion
            g_pending_commands[i].completion_code = XHCI_COMP_SUCCESS;
            g_pending_commands[i].completed = 1;
            
            // Copy completion data
            memcpy(completion, &g_pending_commands[i], sizeof(xhci_command_completion_t));
            
            // Clear pending command
            memset(&g_pending_commands[i], 0, sizeof(xhci_command_completion_t));
            
            return 0;
        }
    }
    
    return -1; // Command not found
}

/**
 * Enable Slot command
 */
[[nodiscard]] int xhci_cmd_enable_slot(uint32_t *slot_id) {
    if (!slot_id) {
        return -1;
    }
    
    // Prepare Enable Slot TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_ENABLE_SLOT << 10) | XHCI_TRB_IOC;
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    if (completion.completion_code != XHCI_COMP_SUCCESS) {
        return -1;
    }
    
    // Extract slot ID from completion (simulated)
    *slot_id = 1; // For simulation, always return slot 1
    
    return 0;
}

/**
 * Disable Slot command
 */
[[nodiscard]] int xhci_cmd_disable_slot(uint32_t slot_id) {
    if (slot_id == 0) {
        return -1;
    }
    
    // Prepare Disable Slot TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_DISABLE_SLOT << 10) | XHCI_TRB_IOC | (slot_id << 24);
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Address Device command
 */
[[nodiscard]] int xhci_cmd_address_device(uint32_t slot_id, uint64_t input_context_ptr, uint8_t bsr) {
    if (slot_id == 0 || input_context_ptr == 0) {
        return -1;
    }
    
    // Prepare Address Device TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.parameter = input_context_ptr;
    trb.control = (XHCI_TRB_ADDRESS_DEVICE << 10) | XHCI_TRB_IOC | (slot_id << 24);
    if (bsr) {
        trb.control |= (1 << 9); // BSR bit
    }
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Configure Endpoint command
 */
[[nodiscard]] int xhci_cmd_configure_endpoint(uint32_t slot_id, uint64_t input_context_ptr) {
    if (slot_id == 0 || input_context_ptr == 0) {
        return -1;
    }
    
    // Prepare Configure Endpoint TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.parameter = input_context_ptr;
    trb.control = (XHCI_TRB_CONFIGURE_EP << 10) | XHCI_TRB_IOC | (slot_id << 24);
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Reset Endpoint command
 */
[[nodiscard]] int xhci_cmd_reset_endpoint(uint32_t slot_id, uint32_t endpoint_id) {
    if (slot_id == 0 || endpoint_id == 0) {
        return -1;
    }
    
    // Prepare Reset Endpoint TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_RESET_EP << 10) | XHCI_TRB_IOC | 
                  (slot_id << 24) | (endpoint_id << 16);
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Stop Endpoint command
 */
[[nodiscard]] int xhci_cmd_stop_endpoint(uint32_t slot_id, uint32_t endpoint_id) {
    if (slot_id == 0 || endpoint_id == 0) {
        return -1;
    }
    
    // Prepare Stop Endpoint TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_STOP_EP << 10) | XHCI_TRB_IOC | 
                  (slot_id << 24) | (endpoint_id << 16);
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Set TR Dequeue Pointer command
 */
[[nodiscard]] int xhci_cmd_set_tr_dequeue_pointer(uint32_t slot_id, uint32_t endpoint_id, uint64_t dequeue_ptr) {
    if (slot_id == 0 || endpoint_id == 0 || dequeue_ptr == 0) {
        return -1;
    }
    
    // Prepare Set TR Dequeue Pointer TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.parameter = dequeue_ptr;
    trb.control = (XHCI_TRB_SET_TR_DEQUEUE << 10) | XHCI_TRB_IOC | 
                  (slot_id << 24) | (endpoint_id << 16);
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * No-Op command
 */
[[nodiscard]] int xhci_cmd_no_op(void) {
    // Prepare No-Op TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_NO_OP_CMD << 10) | XHCI_TRB_IOC;
    
    // Submit command
    int cmd_id = xhci_cmd_submit(&trb);
    if (cmd_id < 0) {
        return -1;
    }
    
    // Wait for completion
    xhci_command_completion_t completion;
    if (xhci_cmd_wait_completion(cmd_id, &completion) != 0) {
        return -1;
    }
    
    return completion.completion_code == XHCI_COMP_SUCCESS ? 0 : -1;
}

/**
 * Process event ring
 */
[[nodiscard]] int xhci_cmd_process_events(void) {
    if (!g_cmd_initialized) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Check event ring for new events
    // 2. Process command completion events
    // 3. Update pending command status
    // 4. Update event ring dequeue pointer
    
    // For simulation, just return success
    return 0;
}

/**
 * Ring command doorbell
 */
void xhci_cmd_ring_doorbell(void) {
    if (!g_cmd_initialized || !g_command_ring.doorbell_reg) {
        return;
    }
    
    // In a real implementation, this would write to the doorbell register
    // to notify the controller of new commands
    // *((uint32_t *)g_command_ring.doorbell_reg) = 0;
}

/**
 * Get command ring status
 */
[[nodiscard]] int xhci_cmd_get_ring_status(uint32_t *enqueue, uint32_t *dequeue, uint8_t *cycle_state) {
    if (!g_cmd_initialized) {
        return -1;
    }
    
    if (enqueue) {
        *enqueue = g_command_ring.enqueue;
    }
    if (dequeue) {
        *dequeue = g_command_ring.dequeue;
    }
    if (cycle_state) {
        *cycle_state = g_command_ring.cycle_state;
    }
    
    return 0;
}

/**
 * Check if command ring is running
 */
[[nodiscard]] int xhci_cmd_is_running(void) {
    return g_cmd_initialized && g_command_ring.running;
}

/**
 * Cleanup command ring
 */
void xhci_cmd_cleanup(void) {
    // Clear command ring
    memset(&g_command_ring, 0, sizeof(xhci_command_ring_t));
    
    // Clear event ring
    memset(&g_event_ring, 0, sizeof(xhci_event_ring_t));
    
    // Clear pending commands
    memset(g_pending_commands, 0, sizeof(g_pending_commands));
    
    g_command_id = 1;
    g_cmd_initialized = 0;
}
