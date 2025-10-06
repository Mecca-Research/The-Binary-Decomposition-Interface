
/**
 * @file hotplug.c
 * @brief Unified Hotplug Subsystem Implementation
 */

#include "hotplug.h"
#include <stdio.h>
#include <string.h>

/* Global hotplug subsystem instance */
struct hotplug_subsystem g_hotplug_subsystem = {0};

/**
 * @brief Initialize the hotplug subsystem
 */
int hotplug_init(void) {
    if (atomic_load_explicit(&g_hotplug_subsystem.initialized, memory_order_acquire)) {
        return 0; /* Already initialized */
    }
    
    /* Initialize event queue */
    atomic_store_explicit(&g_hotplug_subsystem.queue_head, 0, memory_order_relaxed);
    atomic_store_explicit(&g_hotplug_subsystem.queue_tail, 0, memory_order_relaxed);
    
    /* Initialize handlers */
    memset(g_hotplug_subsystem.handlers, 0, sizeof(g_hotplug_subsystem.handlers));
    atomic_store_explicit(&g_hotplug_subsystem.num_handlers, 0, memory_order_relaxed);
    
    /* Initialize statistics */
    atomic_store_explicit(&g_hotplug_subsystem.total_events, 0, memory_order_relaxed);
    atomic_store_explicit(&g_hotplug_subsystem.processed_events, 0, memory_order_relaxed);
    atomic_store_explicit(&g_hotplug_subsystem.dropped_events, 0, memory_order_relaxed);
    
    atomic_store_explicit(&g_hotplug_subsystem.processing, false, memory_order_relaxed);
    atomic_store_explicit(&g_hotplug_subsystem.initialized, true, memory_order_release);
    
    printf("[Hotplug] Initialized successfully\n");
    return 0;
}

/**
 * @brief Shutdown the hotplug subsystem
 */
void hotplug_shutdown(void) {
    if (!atomic_load_explicit(&g_hotplug_subsystem.initialized, memory_order_acquire)) {
        return;
    }
    
    /* Process remaining events */
    hotplug_process_events();
    
    atomic_store_explicit(&g_hotplug_subsystem.initialized, false, memory_order_release);
    printf("[Hotplug] Shutdown complete\n");
}

/**
 * @brief Register a hotplug handler
 */
int hotplug_register_handler(hotplug_handler_t callback, void *user_data,
                             uint32_t priority, device_type_t device_type) {
    if (callback == nullptr) {
        return -1;
    }
    
    uint32_t num_handlers = atomic_load_explicit(&g_hotplug_subsystem.num_handlers, 
                                                  memory_order_acquire);
    
    if (num_handlers >= HOTPLUG_MAX_HANDLERS) {
        return -1; /* Too many handlers */
    }
    
    /* Find free slot */
    for (uint32_t i = 0; i < HOTPLUG_MAX_HANDLERS; i++) {
        if (!g_hotplug_subsystem.handlers[i].active) {
            g_hotplug_subsystem.handlers[i].callback = callback;
            g_hotplug_subsystem.handlers[i].user_data = user_data;
            g_hotplug_subsystem.handlers[i].priority = priority;
            g_hotplug_subsystem.handlers[i].device_type = device_type;
            g_hotplug_subsystem.handlers[i].active = true;
            
            atomic_fetch_add_explicit(&g_hotplug_subsystem.num_handlers, 1, 
                                     memory_order_release);
            
            printf("[Hotplug] Registered handler %u (priority: %u, type: %d)\n", 
                   i, priority, device_type);
            return (int)i;
        }
    }
    
    return -1;
}

/**
 * @brief Unregister a hotplug handler
 */
void hotplug_unregister_handler(int handler_id) {
    if (handler_id < 0 || handler_id >= HOTPLUG_MAX_HANDLERS) {
        return;
    }
    
    if (g_hotplug_subsystem.handlers[handler_id].active) {
        g_hotplug_subsystem.handlers[handler_id].active = false;
        atomic_fetch_sub_explicit(&g_hotplug_subsystem.num_handlers, 1, 
                                  memory_order_release);
        printf("[Hotplug] Unregistered handler %d\n", handler_id);
    }
}

/**
 * @brief Queue a hotplug event
 */
static int hotplug_queue_event(const struct hotplug_event *event) {
    uint32_t tail = atomic_load_explicit(&g_hotplug_subsystem.queue_tail, 
                                         memory_order_acquire);
    uint32_t head = atomic_load_explicit(&g_hotplug_subsystem.queue_head, 
                                         memory_order_acquire);
    uint32_t next_tail = (tail + 1) % HOTPLUG_EVENT_QUEUE_SIZE;
    
    if (next_tail == head) {
        /* Queue full */
        atomic_fetch_add_explicit(&g_hotplug_subsystem.dropped_events, 1, 
                                 memory_order_relaxed);
        return -1;
    }
    
    g_hotplug_subsystem.event_queue[tail] = *event;
    atomic_store_explicit(&g_hotplug_subsystem.queue_tail, next_tail, 
                         memory_order_release);
    
    atomic_fetch_add_explicit(&g_hotplug_subsystem.total_events, 1, 
                             memory_order_relaxed);
    
    return 0;
}

/**
 * @brief Notify device added
 */
int hotplug_notify_device_added(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    struct hotplug_event event = {
        .type = HOTPLUG_EVENT_DEVICE_ADDED,
        .device = dev,
        .timestamp = 0, /* TODO: Get actual timestamp */
        .flags = 0,
        .data = nullptr
    };
    
    device_get(dev); /* Increment reference for event */
    
    int result = hotplug_queue_event(&event);
    if (result != 0) {
        device_put(dev);
    }
    
    return result;
}

/**
 * @brief Notify device removed
 */
int hotplug_notify_device_removed(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    struct hotplug_event event = {
        .type = HOTPLUG_EVENT_DEVICE_REMOVED,
        .device = dev,
        .timestamp = 0,
        .flags = 0,
        .data = nullptr
    };
    
    device_get(dev);
    
    int result = hotplug_queue_event(&event);
    if (result != 0) {
        device_put(dev);
    }
    
    return result;
}

/**
 * @brief Notify device changed
 */
int hotplug_notify_device_changed(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    struct hotplug_event event = {
        .type = HOTPLUG_EVENT_DEVICE_CHANGED,
        .device = dev,
        .timestamp = 0,
        .flags = 0,
        .data = nullptr
    };
    
    device_get(dev);
    
    int result = hotplug_queue_event(&event);
    if (result != 0) {
        device_put(dev);
    }
    
    return result;
}

/**
 * @brief Process pending hotplug events
 */
uint32_t hotplug_process_events(void) {
    /* Check if already processing */
    bool expected = false;
    if (!atomic_compare_exchange_strong_explicit(&g_hotplug_subsystem.processing,
                                                  &expected, true,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
        return 0; /* Already processing */
    }
    
    uint32_t processed = 0;
    
    while (true) {
        uint32_t head = atomic_load_explicit(&g_hotplug_subsystem.queue_head,
                                             memory_order_acquire);
        uint32_t tail = atomic_load_explicit(&g_hotplug_subsystem.queue_tail,
                                             memory_order_acquire);
        
        if (head == tail) {
            break; /* Queue empty */
        }
        
        struct hotplug_event event = g_hotplug_subsystem.event_queue[head];
        
        /* Call handlers in priority order */
        for (uint32_t priority = HOTPLUG_PRIORITY_HIGH; 
             priority >= HOTPLUG_PRIORITY_LOW && priority <= HOTPLUG_PRIORITY_HIGH; 
             priority--) {
            for (uint32_t i = 0; i < HOTPLUG_MAX_HANDLERS; i++) {
                struct hotplug_handler *handler = &g_hotplug_subsystem.handlers[i];
                
                if (!handler->active || handler->priority != priority) {
                    continue;
                }
                
                /* Check device type filter */
                if (handler->device_type != DEVICE_TYPE_UNKNOWN &&
                    event.device != nullptr &&
                    handler->device_type != event.device->type) {
                    continue;
                }
                
                handler->callback(&event, handler->user_data);
            }
        }
        
        /* Release device reference */
        if (event.device != nullptr) {
            device_put(event.device);
        }
        
        /* Move to next event */
        uint32_t next_head = (head + 1) % HOTPLUG_EVENT_QUEUE_SIZE;
        atomic_store_explicit(&g_hotplug_subsystem.queue_head, next_head,
                             memory_order_release);
        
        processed++;
        atomic_fetch_add_explicit(&g_hotplug_subsystem.processed_events, 1,
                                 memory_order_relaxed);
    }
    
    atomic_store_explicit(&g_hotplug_subsystem.processing, false,
                         memory_order_release);
    
    return processed;
}

/**
 * @brief Get hotplug statistics
 */
void hotplug_get_stats(uint64_t *total_events, uint64_t *processed_events,
                      uint64_t *dropped_events) {
    if (total_events != nullptr) {
        *total_events = atomic_load_explicit(&g_hotplug_subsystem.total_events,
                                            memory_order_relaxed);
    }
    if (processed_events != nullptr) {
        *processed_events = atomic_load_explicit(&g_hotplug_subsystem.processed_events,
                                                 memory_order_relaxed);
    }
    if (dropped_events != nullptr) {
        *dropped_events = atomic_load_explicit(&g_hotplug_subsystem.dropped_events,
                                              memory_order_relaxed);
    }
}
