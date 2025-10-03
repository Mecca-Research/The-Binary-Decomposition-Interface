
// ===================================================================
// DESC: xHCI Transfer Ring implementation for BDI Kernel
//       Handles xHCI transfer rings for endpoint data transfers
// ===================================================================
// MODERNIZED: Phase 12 - C23 features (nullptr, [[nodiscard]], _Atomic)

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// xHCI Transfer TRB Types
#define XHCI_TRB_NORMAL             1
#define XHCI_TRB_SETUP_STAGE        2
#define XHCI_TRB_DATA_STAGE         3
#define XHCI_TRB_STATUS_STAGE       4
#define XHCI_TRB_ISOCH              5
#define XHCI_TRB_LINK               6
#define XHCI_TRB_EVENT_DATA         7
#define XHCI_TRB_NO_OP              8

// TRB Control Flags
#define XHCI_TRB_CYCLE              0x00000001
#define XHCI_TRB_ENT                0x00000002
#define XHCI_TRB_ISP                0x00000004
#define XHCI_TRB_NS                 0x00000008
#define XHCI_TRB_CH                 0x00000010
#define XHCI_TRB_IOC                0x00000020
#define XHCI_TRB_IDT                0x00000040
#define XHCI_TRB_TBC_MASK           0x00000180
#define XHCI_TRB_BEI                0x00000200
#define XHCI_TRB_TLBPC_MASK         0x0000F000
#define XHCI_TRB_FRAME_ID_MASK      0x07FF0000
#define XHCI_TRB_SIA                0x80000000

// Transfer Ring Constants
#define XHCI_RING_MAX_SIZE          4096
#define XHCI_MAX_TRANSFER_RINGS     256

// xHCI TRB Structure
typedef struct {
    uint64_t parameter;         // Parameter field (buffer pointer, etc.)
    uint32_t status;            // Status field (transfer length, etc.)
    uint32_t control;           // Control field (type, flags, etc.)
} xhci_trb_t;

// Transfer Ring Structure
typedef struct {
    xhci_trb_t *trbs;          // TRB array
    uint32_t size;             // Ring size (number of TRBs)
    uint32_t enqueue;          // Enqueue pointer
    uint32_t dequeue;          // Dequeue pointer
    uint8_t cycle_state;       // Producer cycle state
    uint8_t consumer_cycle;    // Consumer cycle state
    uint8_t active;            // Ring active flag
    uint32_t slot_id;          // Associated slot ID
    uint32_t endpoint_id;      // Associated endpoint ID
    void *doorbell_reg;        // Doorbell register for this endpoint
} xhci_transfer_ring_t;

// Transfer Request Structure
typedef struct {
    uint64_t buffer_ptr;       // Data buffer pointer
    uint32_t transfer_length;  // Transfer length
    uint32_t td_size;          // TD size remaining
    uint8_t direction;         // Transfer direction (0=OUT, 1=IN)
    uint8_t setup_stage;       // Setup stage flag
    uint8_t data_stage;        // Data stage flag
    uint8_t status_stage;      // Status stage flag
    uint8_t interrupt_on_completion; // IOC flag
    uint8_t immediate_data;    // IDT flag
} xhci_transfer_request_t;

// Global transfer ring state
static xhci_transfer_ring_t g_transfer_rings[XHCI_MAX_TRANSFER_RINGS];
static uint32_t g_ring_count = 0;
static uint8_t g_rings_initialized = 0;

// Function prototypes
int xhci_ring_init(void);
int xhci_ring_create(uint32_t slot_id, uint32_t endpoint_id, uint32_t ring_size, 
                     void *ring_memory, void *doorbell_reg);
int xhci_ring_destroy(uint32_t slot_id, uint32_t endpoint_id);
xhci_transfer_ring_t *xhci_ring_get(uint32_t slot_id, uint32_t endpoint_id);
int xhci_ring_enqueue_trb(xhci_transfer_ring_t *ring, xhci_trb_t *trb);
int xhci_ring_dequeue_trb(xhci_transfer_ring_t *ring, xhci_trb_t *trb);
int xhci_ring_queue_transfer(uint32_t slot_id, uint32_t endpoint_id, 
                            xhci_transfer_request_t *request);
int xhci_ring_queue_control_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                    void *setup_packet, void *data_buffer, 
                                    uint32_t data_length, uint8_t direction);
int xhci_ring_queue_bulk_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                 void *data_buffer, uint32_t data_length, 
                                 uint8_t direction);
int xhci_ring_queue_interrupt_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                      void *data_buffer, uint32_t data_length, 
                                      uint8_t direction);
int xhci_ring_add_link_trb(xhci_transfer_ring_t *ring);
void xhci_ring_doorbell(xhci_transfer_ring_t *ring);
void xhci_ring_cleanup(void);

/**
 * Initialize transfer ring subsystem
 */
[[nodiscard]] int xhci_ring_init(void) {
    if (g_rings_initialized) {
        return 0;
    }
    
    // Clear transfer ring array
    memset(g_transfer_rings, 0, sizeof(g_transfer_rings));
    g_ring_count = 0;
    g_rings_initialized = 1;
    
    return 0;
}

/**
 * Create a transfer ring
 */
[[nodiscard]] int xhci_ring_create(uint32_t slot_id, uint32_t endpoint_id, uint32_t ring_size, 
                     void *ring_memory, void *doorbell_reg) {
    if (!g_rings_initialized || !ring_memory || ring_size == 0 || 
        ring_size > XHCI_RING_MAX_SIZE || slot_id == 0 || endpoint_id == 0) {
        return -1;
    }
    
    // Check if ring already exists
    if (xhci_ring_get(slot_id, endpoint_id) != nullptr) {
        return -1; // Ring already exists
    }
    
    // Find free ring slot
    xhci_transfer_ring_t *ring = nullptr;
    for (uint32_t i = 0; i < XHCI_MAX_TRANSFER_RINGS; i++) {
        if (!g_transfer_rings[i].active) {
            ring = &g_transfer_rings[i];
            break;
        }
    }
    
    if (!ring) {
        return -1; // No free slots
    }
    
    // Initialize ring structure
    ring->trbs = (xhci_trb_t *)ring_memory;
    ring->size = ring_size;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->cycle_state = 1;
    ring->consumer_cycle = 1;
    ring->active = 1;
    ring->slot_id = slot_id;
    ring->endpoint_id = endpoint_id;
    ring->doorbell_reg = doorbell_reg;
    
    // Clear ring memory
    memset(ring->trbs, 0, ring_size * sizeof(xhci_trb_t));
    
    g_ring_count++;
    return 0;
}

/**
 * Destroy a transfer ring
 */
[[nodiscard]] int xhci_ring_destroy(uint32_t slot_id, uint32_t endpoint_id) {
    if (!g_rings_initialized) {
        return -1;
    }
    
    // Find ring
    for (uint32_t i = 0; i < XHCI_MAX_TRANSFER_RINGS; i++) {
        xhci_transfer_ring_t *ring = &g_transfer_rings[i];
        if (ring->active && ring->slot_id == slot_id && ring->endpoint_id == endpoint_id) {
            // Clear ring structure
            memset(ring, 0, sizeof(xhci_transfer_ring_t));
            g_ring_count--;
            return 0;
        }
    }
    
    return -1; // Ring not found
}

/**
 * Get transfer ring by slot and endpoint ID
 */
xhci_transfer_ring_t *xhci_ring_get(uint32_t slot_id, uint32_t endpoint_id) {
    if (!g_rings_initialized) {
        return nullptr;
    }
    
    for (uint32_t i = 0; i < XHCI_MAX_TRANSFER_RINGS; i++) {
        xhci_transfer_ring_t *ring = &g_transfer_rings[i];
        if (ring->active && ring->slot_id == slot_id && ring->endpoint_id == endpoint_id) {
            return ring;
        }
    }
    
    return nullptr;
}

/**
 * Enqueue TRB to transfer ring
 */
[[nodiscard]] int xhci_ring_enqueue_trb(xhci_transfer_ring_t *ring, xhci_trb_t *trb) {
    if (!ring || !trb || !ring->active) {
        return -1;
    }
    
    // Check if ring is full
    uint32_t next_enqueue = (ring->enqueue + 1) % ring->size;
    if (next_enqueue == ring->dequeue) {
        return -1; // Ring full
    }
    
    // Set cycle bit
    trb->control &= ~XHCI_TRB_CYCLE;
    if (ring->cycle_state) {
        trb->control |= XHCI_TRB_CYCLE;
    }
    
    // Copy TRB to ring
    memcpy(&ring->trbs[ring->enqueue], trb, sizeof(xhci_trb_t));
    
    // Update enqueue pointer
    ring->enqueue = next_enqueue;
    if (ring->enqueue == 0) {
        ring->cycle_state = !ring->cycle_state;
    }
    
    return 0;
}

/**
 * Dequeue TRB from transfer ring
 */
[[nodiscard]] int xhci_ring_dequeue_trb(xhci_transfer_ring_t *ring, xhci_trb_t *trb) {
    if (!ring || !trb || !ring->active) {
        return -1;
    }
    
    // Check if ring is empty
    if (ring->dequeue == ring->enqueue) {
        return -1; // Ring empty
    }
    
    // Check cycle bit
    xhci_trb_t *current_trb = &ring->trbs[ring->dequeue];
    uint8_t cycle_bit = (current_trb->control & XHCI_TRB_CYCLE) ? 1 : 0;
    if (cycle_bit != ring->consumer_cycle) {
        return -1; // No new TRBs
    }
    
    // Copy TRB from ring
    memcpy(trb, current_trb, sizeof(xhci_trb_t));
    
    // Update dequeue pointer
    ring->dequeue = (ring->dequeue + 1) % ring->size;
    if (ring->dequeue == 0) {
        ring->consumer_cycle = !ring->consumer_cycle;
    }
    
    return 0;
}

/**
 * Queue a generic transfer
 */
[[nodiscard]] int xhci_ring_queue_transfer(uint32_t slot_id, uint32_t endpoint_id, 
                            xhci_transfer_request_t *request) {
    if (!request) {
        return -1;
    }
    
    xhci_transfer_ring_t *ring = xhci_ring_get(slot_id, endpoint_id);
    if (!ring) {
        return -1;
    }
    
    // Create Normal TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.parameter = request->buffer_ptr;
    trb.status = request->transfer_length & 0x1FFFF; // Transfer length (17 bits)
    trb.status |= (request->td_size & 0x1F) << 17;   // TD size (5 bits)
    trb.control = (XHCI_TRB_NORMAL << 10);           // TRB type
    
    if (request->interrupt_on_completion) {
        trb.control |= XHCI_TRB_IOC;
    }
    if (request->immediate_data) {
        trb.control |= XHCI_TRB_IDT;
    }
    
    // Enqueue TRB
    if (xhci_ring_enqueue_trb(ring, &trb) != 0) {
        return -1;
    }
    
    // Ring doorbell
    xhci_ring_doorbell(ring);
    
    return 0;
}

/**
 * Queue a control transfer
 */
[[nodiscard]] int xhci_ring_queue_control_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                    void *setup_packet, void *data_buffer, 
                                    uint32_t data_length, uint8_t direction) {
    xhci_transfer_ring_t *ring = xhci_ring_get(slot_id, endpoint_id);
    if (!ring || !setup_packet) {
        return -1;
    }
    
    xhci_trb_t trb;
    
    // 1. Setup Stage TRB
    memset(&trb, 0, sizeof(trb));
    trb.parameter = *((uint64_t *)setup_packet);
    trb.status = 8; // Setup packet is always 8 bytes
    trb.control = (XHCI_TRB_SETUP_STAGE << 10) | XHCI_TRB_IDT;
    if (data_length > 0) {
        trb.control |= (direction ? (3 << 16) : (2 << 16)); // TRT field
    }
    
    if (xhci_ring_enqueue_trb(ring, &trb) != 0) {
        return -1;
    }
    
    // 2. Data Stage TRB (if data phase exists)
    if (data_buffer && data_length > 0) {
        memset(&trb, 0, sizeof(trb));
        trb.parameter = (uint64_t)data_buffer;
        trb.status = data_length & 0x1FFFF;
        trb.control = (XHCI_TRB_DATA_STAGE << 10);
        if (direction) {
            trb.control |= (1 << 16); // DIR bit for IN
        }
        
        if (xhci_ring_enqueue_trb(ring, &trb) != 0) {
            return -1;
        }
    }
    
    // 3. Status Stage TRB
    memset(&trb, 0, sizeof(trb));
    trb.control = (XHCI_TRB_STATUS_STAGE << 10) | XHCI_TRB_IOC;
    if (data_length == 0 || !direction) {
        trb.control |= (1 << 16); // DIR bit
    }
    
    if (xhci_ring_enqueue_trb(ring, &trb) != 0) {
        return -1;
    }
    
    // Ring doorbell
    xhci_ring_doorbell(ring);
    
    return 0;
}

/**
 * Queue a bulk transfer
 */
[[nodiscard]] int xhci_ring_queue_bulk_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                 void *data_buffer, uint32_t data_length, 
                                 uint8_t direction) {
    if (!data_buffer || data_length == 0) {
        return -1;
    }
    
    xhci_transfer_request_t request;
    memset(&request, 0, sizeof(request));
    request.buffer_ptr = (uint64_t)data_buffer;
    request.transfer_length = data_length;
    request.direction = direction;
    request.interrupt_on_completion = 1;
    
    return xhci_ring_queue_transfer(slot_id, endpoint_id, &request);
}

/**
 * Queue an interrupt transfer
 */
[[nodiscard]] int xhci_ring_queue_interrupt_transfer(uint32_t slot_id, uint32_t endpoint_id,
                                      void *data_buffer, uint32_t data_length, 
                                      uint8_t direction) {
    if (!data_buffer || data_length == 0) {
        return -1;
    }
    
    xhci_transfer_request_t request;
    memset(&request, 0, sizeof(request));
    request.buffer_ptr = (uint64_t)data_buffer;
    request.transfer_length = data_length;
    request.direction = direction;
    request.interrupt_on_completion = 1;
    
    return xhci_ring_queue_transfer(slot_id, endpoint_id, &request);
}

/**
 * Add Link TRB to ring
 */
[[nodiscard]] int xhci_ring_add_link_trb(xhci_transfer_ring_t *ring) {
    if (!ring || !ring->active) {
        return -1;
    }
    
    // Create Link TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.parameter = (uint64_t)ring->trbs; // Point back to beginning
    trb.control = (XHCI_TRB_LINK << 10) | XHCI_TRB_CH; // Chain bit
    
    // Enqueue Link TRB
    return xhci_ring_enqueue_trb(ring, &trb);
}

/**
 * Ring doorbell for transfer ring
 */
void xhci_ring_doorbell(xhci_transfer_ring_t *ring) {
    if (!ring || !ring->doorbell_reg || !ring->active) {
        return;
    }
    
    // In a real implementation, this would write to the doorbell register
    // to notify the controller of new transfers
    // *((uint32_t *)ring->doorbell_reg) = ring->endpoint_id;
}

/**
 * Get ring statistics
 */
[[nodiscard]] int xhci_ring_get_stats(uint32_t slot_id, uint32_t endpoint_id, 
                       uint32_t *enqueue, uint32_t *dequeue, uint8_t *cycle_state) {
    xhci_transfer_ring_t *ring = xhci_ring_get(slot_id, endpoint_id);
    if (!ring) {
        return -1;
    }
    
    if (enqueue) {
        *enqueue = ring->enqueue;
    }
    if (dequeue) {
        *dequeue = ring->dequeue;
    }
    if (cycle_state) {
        *cycle_state = ring->cycle_state;
    }
    
    return 0;
}

/**
 * Check if ring is empty
 */
[[nodiscard]] int xhci_ring_is_empty(uint32_t slot_id, uint32_t endpoint_id) {
    xhci_transfer_ring_t *ring = xhci_ring_get(slot_id, endpoint_id);
    if (!ring) {
        return 1; // Consider non-existent ring as empty
    }
    
    return ring->enqueue == ring->dequeue;
}

/**
 * Check if ring is full
 */
[[nodiscard]] int xhci_ring_is_full(uint32_t slot_id, uint32_t endpoint_id) {
    xhci_transfer_ring_t *ring = xhci_ring_get(slot_id, endpoint_id);
    if (!ring) {
        return 0; // Non-existent ring is not full
    }
    
    uint32_t next_enqueue = (ring->enqueue + 1) % ring->size;
    return next_enqueue == ring->dequeue;
}

/**
 * Get number of active rings
 */
[[nodiscard]] uint32_t xhci_ring_get_count(void) {
    return g_ring_count;
}

/**
 * Cleanup transfer ring subsystem
 */
void xhci_ring_cleanup(void) {
    // Clear all transfer rings
    memset(g_transfer_rings, 0, sizeof(g_transfer_rings));
    g_ring_count = 0;
    g_rings_initialized = 0;
}
