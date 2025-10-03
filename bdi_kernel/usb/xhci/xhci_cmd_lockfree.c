
// ===================================================================
// DESC: xHCI Command Ring - Lock-Free Implementation (Phase 12 Day 2)
//       Lock-free command submission using atomic operations
// ===================================================================
// MODERNIZED: Phase 12 - C23 features with lock-free algorithms

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>

// Lock-free command ring with atomic operations
typedef struct {
    xhci_trb_t *trbs;                    // TRB array (aligned to cache line)
    uint32_t size;                        // Ring size (power of 2)
    _Atomic uint32_t enqueue_idx;         // Atomic enqueue index
    _Atomic uint32_t dequeue_idx;         // Atomic dequeue index (for completion)
    _Atomic uint8_t cycle_state;          // Atomic cycle state
    _Atomic uint32_t pending_commands;    // Number of pending commands
    volatile uint32_t *doorbell_reg;      // Doorbell register
    _Atomic bool ring_running;            // Ring running state
} xhci_lockfree_cmd_ring_t;

// Command completion tracking
typedef struct {
    _Atomic uint64_t command_id;          // Unique command ID
    _Atomic uint32_t completion_code;     // Completion code
    _Atomic bool completed;               // Completion flag
    xhci_trb_t event_trb;                 // Event TRB data
} xhci_cmd_completion_t;

// Global command ring state
static xhci_lockfree_cmd_ring_t g_cmd_ring;
static xhci_cmd_completion_t g_cmd_completions[256];  // Track up to 256 pending commands
static _Atomic uint64_t g_next_cmd_id = 1;

/**
 * Initialize lock-free command ring
 * Uses atomic operations for all state management
 */
[[nodiscard]] int xhci_lockfree_cmd_ring_init(xhci_trb_t *ring_memory, uint32_t size, 
                                               volatile uint32_t *doorbell) {
    if (!ring_memory || size == 0 || (size & (size - 1)) != 0) {
        return -1;  // Size must be power of 2
    }
    
    // Initialize ring structure with atomic stores
    g_cmd_ring.trbs = ring_memory;
    g_cmd_ring.size = size;
    atomic_store_explicit(&g_cmd_ring.enqueue_idx, 0, memory_order_relaxed);
    atomic_store_explicit(&g_cmd_ring.dequeue_idx, 0, memory_order_relaxed);
    atomic_store_explicit(&g_cmd_ring.cycle_state, 1, memory_order_relaxed);
    atomic_store_explicit(&g_cmd_ring.pending_commands, 0, memory_order_relaxed);
    g_cmd_ring.doorbell_reg = doorbell;
    atomic_store_explicit(&g_cmd_ring.ring_running, true, memory_order_release);
    
    // Clear completion tracking
    memset(g_cmd_completions, 0, sizeof(g_cmd_completions));
    
    // Clear TRB ring
    memset(ring_memory, 0, size * sizeof(xhci_trb_t));
    
    return 0;
}

/**
 * Lock-free command submission using atomic compare-and-swap
 * Multiple threads can submit commands concurrently
 */
[[nodiscard]] int xhci_lockfree_submit_command(xhci_trb_t *cmd_trb, xhci_trb_t *event_trb_out) {
    if (!cmd_trb || !atomic_load_explicit(&g_cmd_ring.ring_running, memory_order_acquire)) {
        return -1;
    }
    
    // Allocate command ID for tracking
    uint64_t cmd_id = atomic_fetch_add_explicit(&g_next_cmd_id, 1, memory_order_relaxed);
    uint32_t completion_slot = cmd_id % 256;
    
    // Initialize completion tracking
    atomic_store_explicit(&g_cmd_completions[completion_slot].command_id, cmd_id, memory_order_relaxed);
    atomic_store_explicit(&g_cmd_completions[completion_slot].completed, false, memory_order_relaxed);
    atomic_store_explicit(&g_cmd_completions[completion_slot].completion_code, 0, memory_order_relaxed);
    
    // Lock-free enqueue using atomic compare-and-swap
    uint32_t enqueue_idx, next_idx;
    uint8_t cycle;
    xhci_trb_t *trb_slot;
    
    do {
        // Load current enqueue index and cycle state
        enqueue_idx = atomic_load_explicit(&g_cmd_ring.enqueue_idx, memory_order_acquire);
        cycle = atomic_load_explicit(&g_cmd_ring.cycle_state, memory_order_acquire);
        
        // Calculate next index
        next_idx = (enqueue_idx + 1) % g_cmd_ring.size;
        
        // Check if ring is full (would catch up to dequeue)
        uint32_t dequeue_idx = atomic_load_explicit(&g_cmd_ring.dequeue_idx, memory_order_acquire);
        if (next_idx == dequeue_idx) {
            return -2;  // Ring full
        }
        
        // Try to claim this slot using CAS
        if (atomic_compare_exchange_weak_explicit(&g_cmd_ring.enqueue_idx, 
                                                   &enqueue_idx, next_idx,
                                                   memory_order_acq_rel, 
                                                   memory_order_acquire)) {
            // Successfully claimed slot
            break;
        }
        // CAS failed, retry with new enqueue_idx value
    } while (true);
    
    // We now own the slot at enqueue_idx
    trb_slot = &g_cmd_ring.trbs[enqueue_idx];
    
    // Copy command TRB data
    trb_slot->parameter = cmd_trb->parameter;
    trb_slot->status = cmd_trb->status;
    
    // Set control field with cycle bit (atomic store with release semantics)
    // This makes the TRB visible to hardware
    uint32_t control = cmd_trb->control | (cycle ? 0x1 : 0x0);
    atomic_store_explicit((_Atomic uint32_t *)&trb_slot->control, control, memory_order_release);
    
    // Increment pending commands counter
    atomic_fetch_add_explicit(&g_cmd_ring.pending_commands, 1, memory_order_release);
    
    // Ring doorbell (wait-free operation)
    atomic_store_explicit((_Atomic uint32_t *)g_cmd_ring.doorbell_reg, 0, memory_order_release);
    
    // Wait for completion (with timeout)
    uint32_t timeout = 1000000;  // 1 second timeout
    while (!atomic_load_explicit(&g_cmd_completions[completion_slot].completed, memory_order_acquire)) {
        if (--timeout == 0) {
            return -3;  // Timeout
        }
        // Yield CPU or spin
        __asm__ volatile("pause" ::: "memory");
    }
    
    // Copy event TRB if requested
    if (event_trb_out) {
        *event_trb_out = g_cmd_completions[completion_slot].event_trb;
    }
    
    // Get completion code
    uint32_t comp_code = atomic_load_explicit(&g_cmd_completions[completion_slot].completion_code, 
                                              memory_order_acquire);
    
    return (comp_code == 1) ? 0 : -4;  // 1 = SUCCESS
}

/**
 * Lock-free command completion handler
 * Called from interrupt context
 */
void xhci_lockfree_handle_command_completion(xhci_trb_t *event_trb) {
    if (!event_trb) return;
    
    // Extract command TRB pointer from event
    uint64_t cmd_trb_ptr = event_trb->parameter;
    uint32_t completion_code = (event_trb->status >> 24) & 0xFF;
    
    // Find which slot this command was in
    uint32_t slot_idx = ((uint32_t)(cmd_trb_ptr - (uint64_t)g_cmd_ring.trbs)) / sizeof(xhci_trb_t);
    
    if (slot_idx >= g_cmd_ring.size) {
        return;  // Invalid slot
    }
    
    // Find completion tracking entry (simple linear search for now)
    // In production, use command ID embedded in TRB
    for (uint32_t i = 0; i < 256; i++) {
        if (!atomic_load_explicit(&g_cmd_completions[i].completed, memory_order_acquire)) {
            // Store completion data
            g_cmd_completions[i].event_trb = *event_trb;
            atomic_store_explicit(&g_cmd_completions[i].completion_code, completion_code, 
                                 memory_order_release);
            atomic_store_explicit(&g_cmd_completions[i].completed, true, memory_order_release);
            break;
        }
    }
    
    // Decrement pending commands
    atomic_fetch_sub_explicit(&g_cmd_ring.pending_commands, 1, memory_order_release);
    
    // Advance dequeue pointer
    uint32_t dequeue_idx = atomic_load_explicit(&g_cmd_ring.dequeue_idx, memory_order_acquire);
    uint32_t next_dequeue = (dequeue_idx + 1) % g_cmd_ring.size;
    atomic_store_explicit(&g_cmd_ring.dequeue_idx, next_dequeue, memory_order_release);
}

/**
 * Get pending command count (lock-free read)
 */
[[nodiscard]] uint32_t xhci_lockfree_get_pending_count(void) {
    return atomic_load_explicit(&g_cmd_ring.pending_commands, memory_order_acquire);
}

/**
 * Stop command ring (atomic operation)
 */
void xhci_lockfree_cmd_ring_stop(void) {
    atomic_store_explicit(&g_cmd_ring.ring_running, false, memory_order_release);
}
