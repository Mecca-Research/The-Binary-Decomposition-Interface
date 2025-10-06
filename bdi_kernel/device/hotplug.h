
/**
 * @file hotplug.h
 * @brief Unified Hotplug Subsystem
 * 
 * Provides hotplug event system for device insertion/removal,
 * dynamic device discovery, and state transition management.
 */

#ifndef BDI_HOTPLUG_H
#define BDI_HOTPLUG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "device_manager.h"

/* Hotplug Event Types */
typedef enum {
    HOTPLUG_EVENT_DEVICE_ADDED = 0,
    HOTPLUG_EVENT_DEVICE_REMOVED,
    HOTPLUG_EVENT_DEVICE_CHANGED,
    HOTPLUG_EVENT_BUS_ADDED,
    HOTPLUG_EVENT_BUS_REMOVED,
    HOTPLUG_EVENT_MAX
} hotplug_event_type_t;

/* Hotplug Event Priorities */
#define HOTPLUG_PRIORITY_LOW        0
#define HOTPLUG_PRIORITY_NORMAL     50
#define HOTPLUG_PRIORITY_HIGH       100

/* Maximum hotplug handlers */
#define HOTPLUG_MAX_HANDLERS        64
#define HOTPLUG_EVENT_QUEUE_SIZE    256

/**
 * @brief Hotplug event structure
 */
struct hotplug_event {
    hotplug_event_type_t type;
    struct device *device;
    uint64_t timestamp;
    uint32_t flags;
    void *data;
};

/**
 * @brief Hotplug handler callback
 */
typedef int (*hotplug_handler_t)(const struct hotplug_event *event, void *user_data);

/**
 * @brief Hotplug handler registration
 */
struct hotplug_handler {
    hotplug_handler_t callback;
    void *user_data;
    uint32_t priority;
    device_type_t device_type;  /* DEVICE_TYPE_UNKNOWN for all types */
    bool active;
};

/**
 * @brief Hotplug subsystem state
 */
struct hotplug_subsystem {
    /* Event queue */
    struct hotplug_event event_queue[HOTPLUG_EVENT_QUEUE_SIZE];
    _Atomic uint32_t queue_head;
    _Atomic uint32_t queue_tail;
    
    /* Registered handlers */
    struct hotplug_handler handlers[HOTPLUG_MAX_HANDLERS];
    _Atomic uint32_t num_handlers;
    
    /* Statistics */
    _Atomic uint64_t total_events;
    _Atomic uint64_t processed_events;
    _Atomic uint64_t dropped_events;
    
    /* Subsystem state */
    _Atomic bool initialized;
    _Atomic bool processing;
};

/* Global hotplug subsystem instance */
extern struct hotplug_subsystem g_hotplug_subsystem;

/**
 * @brief Initialize the hotplug subsystem
 * 
 * @return 0 on success, negative error code on failure
 */
int hotplug_init(void);

/**
 * @brief Shutdown the hotplug subsystem
 */
void hotplug_shutdown(void);

/**
 * @brief Register a hotplug handler
 * 
 * @param callback Handler callback function
 * @param user_data User data passed to callback
 * @param priority Handler priority (higher = called first)
 * @param device_type Device type filter (DEVICE_TYPE_UNKNOWN for all)
 * @return Handler ID on success, negative error code on failure
 */
int hotplug_register_handler(hotplug_handler_t callback, void *user_data, 
                             uint32_t priority, device_type_t device_type);

/**
 * @brief Unregister a hotplug handler
 * 
 * @param handler_id Handler ID returned by hotplug_register_handler
 */
void hotplug_unregister_handler(int handler_id);

/**
 * @brief Notify device added
 * 
 * @param dev Device that was added
 * @return 0 on success, negative error code on failure
 */
int hotplug_notify_device_added(struct device *dev);

/**
 * @brief Notify device removed
 * 
 * @param dev Device that was removed
 * @return 0 on success, negative error code on failure
 */
int hotplug_notify_device_removed(struct device *dev);

/**
 * @brief Notify device changed
 * 
 * @param dev Device that changed
 * @return 0 on success, negative error code on failure
 */
int hotplug_notify_device_changed(struct device *dev);

/**
 * @brief Process pending hotplug events
 * 
 * @return Number of events processed
 */
uint32_t hotplug_process_events(void);

/**
 * @brief Get hotplug statistics
 * 
 * @param total_events Output for total events
 * @param processed_events Output for processed events
 * @param dropped_events Output for dropped events
 */
void hotplug_get_stats(uint64_t *total_events, uint64_t *processed_events, 
                      uint64_t *dropped_events);

#endif /* BDI_HOTPLUG_H */
