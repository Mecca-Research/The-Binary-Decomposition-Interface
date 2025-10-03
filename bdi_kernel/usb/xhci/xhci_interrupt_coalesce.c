
// ===================================================================
// DESC: xHCI Interrupt Coalescing (Phase 12 Day 2)
//       Optimize interrupt handling with coalescing and moderation
// ===================================================================
// MODERNIZED: Phase 12 - C23 features

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

// Interrupt coalescing configuration
typedef struct {
    _Atomic uint32_t imod_interval;        // Interrupt moderation interval (250ns units)
    _Atomic uint32_t imod_counter;         // Interrupt moderation counter
    _Atomic uint32_t events_per_interrupt; // Target events per interrupt
    _Atomic uint32_t total_interrupts;     // Total interrupt count
    _Atomic uint32_t total_events;         // Total event count
    _Atomic bool enabled;                  // Coalescing enabled
} xhci_interrupt_coalesce_t;

static xhci_interrupt_coalesce_t g_coalesce_config;

/**
 * Initialize interrupt coalescing
 * Default: 1ms interval (4000 * 250ns), 16 events per interrupt
 */
[[nodiscard]] int xhci_interrupt_coalesce_init(void) {
    atomic_store_explicit(&g_coalesce_config.imod_interval, 4000, memory_order_relaxed);
    atomic_store_explicit(&g_coalesce_config.imod_counter, 0, memory_order_relaxed);
    atomic_store_explicit(&g_coalesce_config.events_per_interrupt, 16, memory_order_relaxed);
    atomic_store_explicit(&g_coalesce_config.total_interrupts, 0, memory_order_relaxed);
    atomic_store_explicit(&g_coalesce_config.total_events, 0, memory_order_relaxed);
    atomic_store_explicit(&g_coalesce_config.enabled, true, memory_order_release);
    
    return 0;
}

/**
 * Configure interrupt moderation interval
 * @param interval_us: Interval in microseconds (1-4000)
 */
[[nodiscard]] int xhci_set_interrupt_interval(uint32_t interval_us) {
    if (interval_us == 0 || interval_us > 4000) {
        return -1;
    }
    
    // Convert microseconds to 250ns units
    uint32_t imod_value = interval_us * 4;
    atomic_store_explicit(&g_coalesce_config.imod_interval, imod_value, memory_order_release);
    
    return 0;
}

/**
 * Set target events per interrupt
 */
[[nodiscard]] int xhci_set_events_per_interrupt(uint32_t events) {
    if (events == 0 || events > 256) {
        return -1;
    }
    
    atomic_store_explicit(&g_coalesce_config.events_per_interrupt, events, memory_order_release);
    return 0;
}

/**
 * Record interrupt occurrence
 */
void xhci_record_interrupt(void) {
    atomic_fetch_add_explicit(&g_coalesce_config.total_interrupts, 1, memory_order_relaxed);
}

/**
 * Record event processing
 */
void xhci_record_event(void) {
    atomic_fetch_add_explicit(&g_coalesce_config.total_events, 1, memory_order_relaxed);
}

/**
 * Get interrupt statistics
 */
[[nodiscard]] uint32_t xhci_get_interrupt_count(void) {
    return atomic_load_explicit(&g_coalesce_config.total_interrupts, memory_order_acquire);
}

[[nodiscard]] uint32_t xhci_get_event_count(void) {
    return atomic_load_explicit(&g_coalesce_config.total_events, memory_order_acquire);
}

/**
 * Calculate average events per interrupt
 */
[[nodiscard]] float xhci_get_avg_events_per_interrupt(void) {
    uint32_t interrupts = atomic_load_explicit(&g_coalesce_config.total_interrupts, 
                                               memory_order_acquire);
    uint32_t events = atomic_load_explicit(&g_coalesce_config.total_events, 
                                          memory_order_acquire);
    
    if (interrupts == 0) return 0.0f;
    return (float)events / (float)interrupts;
}

/**
 * Enable/disable interrupt coalescing
 */
void xhci_set_coalescing_enabled(bool enabled) {
    atomic_store_explicit(&g_coalesce_config.enabled, enabled, memory_order_release);
}

[[nodiscard]] bool xhci_is_coalescing_enabled(void) {
    return atomic_load_explicit(&g_coalesce_config.enabled, memory_order_acquire);
}
