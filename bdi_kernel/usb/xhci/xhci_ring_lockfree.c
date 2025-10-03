
// ===================================================================
// DESC: xHCI Transfer Ring - Lock-Free Implementation (Phase 12 Day 2)
//       Lock-free transfer ring using atomic operations
// ===================================================================
// MODERNIZED: Phase 12 - C23 features with lock-free algorithms

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>

// Lock-free transfer ring structure
typedef struct {
    xhci_trb_t *trbs;                     // TRB array (cache-aligned)
    uint32_t size;                         // Ring size (power of 2)
    _Atomic uint32_t enqueue_idx;          // Atomic enqueue index
    _Atomic uint32_t dequeue_idx;          // Atomic dequeue index
    _Atomic uint8_t producer_cycle;        // Producer cycle state
    _Atomic uint8_t consumer_cycle;        // Consumer cycle state
    _Atomic bool active;                   // Ring active flag
    uint32_t slot_id;                      // USB slot ID
    uint32_t endpoint_id;                  // Endpoint ID
    volatile uint32_t *doorbell_reg;       // Doorbell register
    _Atomic uint32_t pending_transfers;    // Pending transfer count
} xhci_lockfree_transfer_ring_t;

// Transfer completion tracking
typedef struct {
    _Atomic uint64_t transfer_id;          // Unique transfer ID
    _Atomic uint32_t bytes_transferred;    // Bytes transferred
    _Atomic uint32_t completion_code;      // Completion code
    _Atomic bool completed;                // Completion flag
} xhci_transfer_completion_t;

// Global transfer ring pool
#define MAX_TRANSFER_RINGS 256
static xhci_lockfree_transfer_ring_t g_transfer_rings[MAX_TRANSFER_RINGS];
static xhci_transfer_completion_t g_transfer_completions[1024];  // Track up to 1024 transfers
static _Atomic uint32_t g_ring_count = 0;
static _Atomic uint64_t g_next_transfer_id = 1;

/**
 * Initialize lock-free transfer ring subsystem
 */
[[nodiscard]] int xhci_lockfree_ring_init(void) {
    // Clear all rings
    memset(g_transfer_rings, 0, sizeof(g_transfer_rings));
    memset(g_transfer_completions, 0, sizeof(g_transfer_completions));
    
    atomic_store_explicit(&g_ring_count, 0, memory_order_release);
    atomic_store_explicit(&g_next_transfer_id, 1, memory_order_release);
    
    return 0;
}

/**
 * Create a lock-free transfer ring
 */
[[nodiscard]] int xhci_lockfree_ring_create(uint32_t slot_id, uint32_t endpoint_id,
                                            xhci_trb_t *ring_memory, uint32_t size,
                                            volatile uint32_t *doorbell) {
    if (!ring_memory || size == 0 || (size & (size - 1)) != 0) {
        return -1;  // Size must be power of 2
    }
    
    // Find free ring slot using atomic operations
    uint32_t ring_idx = 0;
    bool found = false;
    
    for (uint32_t i = 0; i < MAX_TRANSFER_RINGS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong_explicit(&g_transfer_rings[i].active,
                                                     &expected, true,
                                                     memory_order_acq_rel,
                                                     memory_order_acquire)) {
            ring_idx = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        return -2;  // No free slots
    }
    
    // Initialize ring
    xhci_lockfree_transfer_ring_t *ring = &g_transfer_rings[ring_idx];
    ring->trbs = ring_memory;
    ring->size = size;
    atomic_store_explicit(&ring->enqueue_idx, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->dequeue_idx, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->producer_cycle, 1, memory_order_relaxed);
    atomic_store_explicit(&ring->consumer_cycle, 1, memory_order_relaxed);
    ring->slot_id = slot_id;
    ring->endpoint_id = endpoint_id;
    ring->doorbell_reg = doorbell;
    atomic_store_explicit(&ring->pending_transfers, 0, memory_order_release);
    
    // Clear TRB ring
    memset(ring_memory, 0, size * sizeof(xhci_trb_t));
    
    atomic_fetch_add_explicit(&g_ring_count, 1, memory_order_release);
    
    return ring_idx;
}

/**
 * Lock-free TRB enqueue operation
 * Uses atomic CAS for concurrent enqueue operations
 */
[[nodiscard]] int xhci_lockfree_enqueue_trb(uint32_t ring_idx, xhci_trb_t *trb) {
    if (ring_idx >= MAX_TRANSFER_RINGS || !trb) {
        return -1;
    }
    
    xhci_lockfree_transfer_ring_t *ring = &g_transfer_rings[ring_idx];
    
    if (!atomic_load_explicit(&ring->active, memory_order_acquire)) {
        return -2;  // Ring not active
    }
    
    // Lock-free enqueue using CAS
    uint32_t enqueue_idx, next_idx;
    uint8_t cycle;
    xhci_trb_t *trb_slot;
    
    do {
        // Load current state
        enqueue_idx = atomic_load_explicit(&ring->enqueue_idx, memory_order_acquire);
        cycle = atomic_load_explicit(&ring->producer_cycle, memory_order_acquire);
        
        // Calculate next index
        next_idx = (enqueue_idx + 1) % ring->size;
        
        // Check for ring full condition
        uint32_t dequeue_idx = atomic_load_explicit(&ring->dequeue_idx, memory_order_acquire);
        if (next_idx == dequeue_idx) {
            return -3;  // Ring full
        }
        
        // Try to claim slot
        if (atomic_compare_exchange_weak_explicit(&ring->enqueue_idx,
                                                   &enqueue_idx, next_idx,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire)) {
            // Check if we wrapped around
            if (next_idx == 0) {
                // Toggle cycle bit atomically
                uint8_t old_cycle = cycle;
                atomic_compare_exchange_strong_explicit(&ring->producer_cycle,
                                                        &old_cycle, (uint8_t)(cycle ^ 1),
                                                        memory_order_release,
                                                        memory_order_relaxed);
            }
            break;
        }
    } while (true);
    
    // We own the slot at enqueue_idx
    trb_slot = &ring->trbs[enqueue_idx];
    
    // Copy TRB data
    trb_slot->parameter = trb->parameter;
    trb_slot->status = trb->status;
    
    // Set control with cycle bit (atomic store with release)
    uint32_t control = trb->control | (cycle ? 0x1 : 0x0);
    atomic_store_explicit((_Atomic uint32_t *)&trb_slot->control, control, memory_order_release);
    
    // Increment pending transfers
    atomic_fetch_add_explicit(&ring->pending_transfers, 1, memory_order_release);
    
    return 0;
}

/**
 * Wait-free doorbell ring operation
 */
void xhci_lockfree_ring_doorbell(uint32_t ring_idx) {
    if (ring_idx >= MAX_TRANSFER_RINGS) return;
    
    xhci_lockfree_transfer_ring_t *ring = &g_transfer_rings[ring_idx];
    
    if (!atomic_load_explicit(&ring->active, memory_order_acquire)) {
        return;
    }
    
    // Ring doorbell (wait-free atomic store)
    uint32_t doorbell_value = (ring->endpoint_id << 16) | ring->slot_id;
    atomic_store_explicit((_Atomic uint32_t *)ring->doorbell_reg, doorbell_value, 
                         memory_order_release);
}

/**
 * Lock-free event ring processing
 * Called from interrupt handler
 */
void xhci_lockfree_process_transfer_event(xhci_trb_t *event_trb) {
    if (!event_trb) return;
    
    uint32_t completion_code = (event_trb->status >> 24) & 0xFF;
    uint32_t bytes_transferred = event_trb->status & 0xFFFFFF;
    
    // Find corresponding transfer completion entry
    // In production, use transfer ID from TRB
    for (uint32_t i = 0; i < 1024; i++) {
        if (!atomic_load_explicit(&g_transfer_completions[i].completed, memory_order_acquire)) {
            atomic_store_explicit(&g_transfer_completions[i].bytes_transferred, 
                                 bytes_transferred, memory_order_relaxed);
            atomic_store_explicit(&g_transfer_completions[i].completion_code,
                                 completion_code, memory_order_relaxed);
            atomic_store_explicit(&g_transfer_completions[i].completed, true,
                                 memory_order_release);
            break;
        }
    }
}

/**
 * Get ring statistics (lock-free reads)
 */
[[nodiscard]] uint32_t xhci_lockfree_get_pending_transfers(uint32_t ring_idx) {
    if (ring_idx >= MAX_TRANSFER_RINGS) return 0;
    
    return atomic_load_explicit(&g_transfer_rings[ring_idx].pending_transfers, 
                               memory_order_acquire);
}

/**
 * Destroy transfer ring (atomic operation)
 */
void xhci_lockfree_ring_destroy(uint32_t ring_idx) {
    if (ring_idx >= MAX_TRANSFER_RINGS) return;
    
    atomic_store_explicit(&g_transfer_rings[ring_idx].active, false, memory_order_release);
    atomic_fetch_sub_explicit(&g_ring_count, 1, memory_order_release);
}
