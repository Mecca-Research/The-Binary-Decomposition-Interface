
/**
 * @file device_manager.c
 * @brief Device Manager Core Implementation
 */

#include "device_manager.h"
#include "hotplug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global device manager instance */
struct device_manager g_device_manager = {0};

/* Helper: Hash function for device IDs */
static inline uint32_t device_hash(uint64_t id) {
    return (uint32_t)(id % DEVICE_HASH_BUCKETS);
}

/* Helper: Allocate a new device ID */
static uint64_t device_alloc_id(void) {
    return atomic_fetch_add_explicit(&g_device_manager.next_device_id, 1, memory_order_relaxed);
}

/**
 * @brief Initialize the device manager
 */
int device_manager_init(void) {
    if (atomic_load_explicit(&g_device_manager.initialized, memory_order_acquire)) {
        return 0; /* Already initialized */
    }
    
    /* Initialize hash table */
    memset(g_device_manager.device_hash, 0, sizeof(g_device_manager.device_hash));
    
    /* Initialize device lists */
    memset(g_device_manager.devices_by_type, 0, sizeof(g_device_manager.devices_by_type));
    
    /* Initialize driver list */
    g_device_manager.drivers = nullptr;
    
    /* Initialize statistics */
    atomic_store_explicit(&g_device_manager.total_devices, 0, memory_order_relaxed);
    atomic_store_explicit(&g_device_manager.active_devices, 0, memory_order_relaxed);
    atomic_store_explicit(&g_device_manager.total_probes, 0, memory_order_relaxed);
    atomic_store_explicit(&g_device_manager.failed_probes, 0, memory_order_relaxed);
    
    /* Start device IDs from 1 */
    atomic_store_explicit(&g_device_manager.next_device_id, 1, memory_order_relaxed);
    
    /* Mark as initialized */
    atomic_store_explicit(&g_device_manager.initialized, true, memory_order_release);
    
    printf("[DeviceManager] Initialized successfully\n");
    return 0;
}

/**
 * @brief Shutdown the device manager
 */
void device_manager_shutdown(void) {
    if (!atomic_load_explicit(&g_device_manager.initialized, memory_order_acquire)) {
        return;
    }
    
    /* Remove all devices */
    for (uint32_t i = 0; i < DEVICE_HASH_BUCKETS; i++) {
        struct device *dev = g_device_manager.device_hash[i];
        while (dev != nullptr) {
            struct device *next = dev->hash_next;
            device_remove(dev);
            dev = next;
        }
    }
    
    atomic_store_explicit(&g_device_manager.initialized, false, memory_order_release);
    printf("[DeviceManager] Shutdown complete\n");
}

/**
 * @brief Register a new device
 */
int device_register(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    /* Allocate device ID if not set */
    if (dev->id == 0) {
        dev->id = device_alloc_id();
    }
    
    /* Initialize atomic fields */
    atomic_store_explicit(&dev->state, DEVICE_STATE_REGISTERED, memory_order_release);
    atomic_store_explicit(&dev->refcount, 1, memory_order_relaxed);
    atomic_store_explicit(&dev->access_count, 0, memory_order_relaxed);
    
    /* Add to hash table */
    uint32_t hash = device_hash(dev->id);
    dev->hash_next = g_device_manager.device_hash[hash];
    g_device_manager.device_hash[hash] = dev;
    
    /* Add to type list */
    if (dev->type < DEVICE_TYPE_MAX) {
        dev->next = g_device_manager.devices_by_type[dev->type];
        if (dev->next != nullptr) {
            dev->next->prev = dev;
        }
        dev->prev = nullptr;
        g_device_manager.devices_by_type[dev->type] = dev;
    }
    
    /* Update statistics */
    atomic_fetch_add_explicit(&g_device_manager.total_devices, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_device_manager.active_devices, 1, memory_order_relaxed);
    
    printf("[DeviceManager] Registered device: %s (ID: %lu, Type: %d)\n", 
           dev->name, dev->id, dev->type);
    
    /* Try to bind a driver */
    device_bind_driver(dev);
    
    /* Send hotplug event */
    hotplug_notify_device_added(dev);
    
    return 0;
}

/**
 * @brief Unregister a device
 */
void device_unregister(struct device *dev) {
    if (dev == nullptr) {
        return;
    }
    
    /* Unbind driver */
    device_unbind_driver(dev);
    
    /* Remove from hash table */
    uint32_t hash = device_hash(dev->id);
    struct device **prev_ptr = &g_device_manager.device_hash[hash];
    while (*prev_ptr != nullptr) {
        if (*prev_ptr == dev) {
            *prev_ptr = dev->hash_next;
            break;
        }
        prev_ptr = &(*prev_ptr)->hash_next;
    }
    
    /* Remove from type list */
    if (dev->prev != nullptr) {
        dev->prev->next = dev->next;
    } else if (dev->type < DEVICE_TYPE_MAX) {
        g_device_manager.devices_by_type[dev->type] = dev->next;
    }
    
    if (dev->next != nullptr) {
        dev->next->prev = dev->prev;
    }
    
    /* Update state */
    atomic_store_explicit(&dev->state, DEVICE_STATE_REMOVED, memory_order_release);
    
    /* Update statistics */
    atomic_fetch_sub_explicit(&g_device_manager.active_devices, 1, memory_order_relaxed);
    
    /* Send hotplug event */
    hotplug_notify_device_removed(dev);
    
    printf("[DeviceManager] Unregistered device: %s (ID: %lu)\n", dev->name, dev->id);
    
    /* Release reference */
    device_put(dev);
}

/**
 * @brief Find a device by ID
 */
struct device *device_find_by_id(uint64_t id) {
    uint32_t hash = device_hash(id);
    struct device *dev = g_device_manager.device_hash[hash];
    
    while (dev != nullptr) {
        if (dev->id == id) {
            device_get(dev);
            return dev;
        }
        dev = dev->hash_next;
    }
    
    return nullptr;
}

/**
 * @brief Find a device by name
 */
struct device *device_find_by_name(const char *name) {
    if (name == nullptr) {
        return nullptr;
    }
    
    for (uint32_t i = 0; i < DEVICE_HASH_BUCKETS; i++) {
        struct device *dev = g_device_manager.device_hash[i];
        while (dev != nullptr) {
            if (strcmp(dev->name, name) == 0) {
                device_get(dev);
                return dev;
            }
            dev = dev->hash_next;
        }
    }
    
    return nullptr;
}

/**
 * @brief Find a device by path
 */
struct device *device_find_by_path(const char *path) {
    if (path == nullptr) {
        return nullptr;
    }
    
    for (uint32_t i = 0; i < DEVICE_HASH_BUCKETS; i++) {
        struct device *dev = g_device_manager.device_hash[i];
        while (dev != nullptr) {
            if (strcmp(dev->path, path) == 0) {
                device_get(dev);
                return dev;
            }
            dev = dev->hash_next;
        }
    }
    
    return nullptr;
}

/**
 * @brief Get devices by type
 */
size_t device_get_by_type(device_type_t type, struct device **devices, size_t max_devices) {
    if (type >= DEVICE_TYPE_MAX || devices == nullptr || max_devices == 0) {
        return 0;
    }
    
    size_t count = 0;
    struct device *dev = g_device_manager.devices_by_type[type];
    
    while (dev != nullptr && count < max_devices) {
        devices[count++] = dev;
        device_get(dev);
        dev = dev->next;
    }
    
    return count;
}

/**
 * @brief Increment device reference count
 */
void device_get(struct device *dev) {
    if (dev != nullptr) {
        atomic_fetch_add_explicit(&dev->refcount, 1, memory_order_relaxed);
    }
}

/**
 * @brief Decrement device reference count
 */
void device_put(struct device *dev) {
    if (dev == nullptr) {
        return;
    }
    
    uint32_t old_refcount = atomic_fetch_sub_explicit(&dev->refcount, 1, memory_order_release);
    
    if (old_refcount == 1) {
        /* Last reference released, free the device */
        atomic_thread_fence(memory_order_acquire);
        printf("[DeviceManager] Freeing device: %s (ID: %lu)\n", dev->name, dev->id);
        free(dev);
    }
}

/**
 * @brief Probe a device
 */
int device_probe(struct device *dev) {
    if (dev == nullptr || dev->ops == nullptr || dev->ops->probe == nullptr) {
        return -1;
    }
    
    device_state_t expected = DEVICE_STATE_REGISTERED;
    if (!atomic_compare_exchange_strong_explicit(&dev->state, &expected, 
                                                  DEVICE_STATE_PROBING,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
        return -1; /* Already probing or in wrong state */
    }
    
    atomic_fetch_add_explicit(&g_device_manager.total_probes, 1, memory_order_relaxed);
    
    printf("[DeviceManager] Probing device: %s\n", dev->name);
    
    int result = dev->ops->probe(dev);
    
    if (result == 0) {
        atomic_store_explicit(&dev->state, DEVICE_STATE_READY, memory_order_release);
        printf("[DeviceManager] Device probe successful: %s\n", dev->name);
    } else {
        atomic_store_explicit(&dev->state, DEVICE_STATE_ERROR, memory_order_release);
        atomic_fetch_add_explicit(&g_device_manager.failed_probes, 1, memory_order_relaxed);
        printf("[DeviceManager] Device probe failed: %s (error: %d)\n", dev->name, result);
    }
    
    return result;
}

/**
 * @brief Remove a device
 */
int device_remove(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    atomic_store_explicit(&dev->state, DEVICE_STATE_REMOVING, memory_order_release);
    
    if (dev->ops != nullptr && dev->ops->remove != nullptr) {
        dev->ops->remove(dev);
    }
    
    device_unregister(dev);
    return 0;
}

/**
 * @brief Attach a device to its parent
 */
int device_attach(struct device *dev, struct device *parent) {
    if (dev == nullptr || parent == nullptr) {
        return -1;
    }
    
    if (parent->num_children >= DEVICE_MAX_CHILDREN) {
        return -1; /* Parent has too many children */
    }
    
    dev->parent = parent;
    parent->children[parent->num_children++] = dev;
    device_get(dev); /* Increment reference for parent */
    
    printf("[DeviceManager] Attached device %s to parent %s\n", dev->name, parent->name);
    return 0;
}

/**
 * @brief Detach a device from its parent
 */
void device_detach(struct device *dev) {
    if (dev == nullptr || dev->parent == nullptr) {
        return;
    }
    
    struct device *parent = dev->parent;
    
    /* Remove from parent's children list */
    for (uint32_t i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == dev) {
            /* Shift remaining children */
            for (uint32_t j = i; j < parent->num_children - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->num_children--;
            break;
        }
    }
    
    dev->parent = nullptr;
    device_put(dev); /* Release parent's reference */
    
    printf("[DeviceManager] Detached device %s from parent %s\n", dev->name, parent->name);
}

/**
 * @brief Register a device driver
 */
int device_driver_register(struct device_driver *driver) {
    if (driver == nullptr) {
        return -1;
    }
    
    /* Add to driver list */
    driver->next = g_device_manager.drivers;
    g_device_manager.drivers = driver;
    
    printf("[DeviceManager] Registered driver: %s\n", driver->name);
    
    /* Try to bind to existing devices */
    if (driver->type < DEVICE_TYPE_MAX) {
        struct device *dev = g_device_manager.devices_by_type[driver->type];
        while (dev != nullptr) {
            if (dev->driver == nullptr && driver->match != nullptr && driver->match(dev) == 0) {
                device_bind_driver(dev);
            }
            dev = dev->next;
        }
    }
    
    return 0;
}

/**
 * @brief Unregister a device driver
 */
void device_driver_unregister(struct device_driver *driver) {
    if (driver == nullptr) {
        return;
    }
    
    /* Unbind from all devices */
    for (uint32_t i = 0; i < DEVICE_TYPE_MAX; i++) {
        struct device *dev = g_device_manager.devices_by_type[i];
        while (dev != nullptr) {
            if (dev->driver == driver) {
                device_unbind_driver(dev);
            }
            dev = dev->next;
        }
    }
    
    /* Remove from driver list */
    struct device_driver **prev_ptr = &g_device_manager.drivers;
    while (*prev_ptr != nullptr) {
        if (*prev_ptr == driver) {
            *prev_ptr = driver->next;
            break;
        }
        prev_ptr = &(*prev_ptr)->next;
    }
    
    printf("[DeviceManager] Unregistered driver: %s\n", driver->name);
}

/**
 * @brief Match and bind a driver to a device
 */
int device_bind_driver(struct device *dev) {
    if (dev == nullptr || dev->driver != nullptr) {
        return -1; /* Already has a driver */
    }
    
    struct device_driver *driver = g_device_manager.drivers;
    while (driver != nullptr) {
        if (driver->type == dev->type && driver->match != nullptr && driver->match(dev) == 0) {
            dev->driver = driver;
            dev->ops = driver->ops;
            
            if (driver->bind != nullptr) {
                int result = driver->bind(dev);
                if (result != 0) {
                    dev->driver = nullptr;
                    dev->ops = nullptr;
                    return result;
                }
            }
            
            printf("[DeviceManager] Bound driver %s to device %s\n", driver->name, dev->name);
            
            /* Probe the device */
            device_probe(dev);
            return 0;
        }
        driver = driver->next;
    }
    
    return -1; /* No matching driver found */
}

/**
 * @brief Unbind a driver from a device
 */
void device_unbind_driver(struct device *dev) {
    if (dev == nullptr || dev->driver == nullptr) {
        return;
    }
    
    struct device_driver *driver = dev->driver;
    
    if (driver->unbind != nullptr) {
        driver->unbind(dev);
    }
    
    printf("[DeviceManager] Unbound driver %s from device %s\n", driver->name, dev->name);
    
    dev->driver = nullptr;
    dev->ops = nullptr;
}

/**
 * @brief Add a resource to a device
 */
int device_add_resource(struct device *dev, const struct device_resource *resource) {
    if (dev == nullptr || resource == nullptr) {
        return -1;
    }
    
    if (dev->num_resources >= DEVICE_MAX_RESOURCES) {
        return -1; /* Too many resources */
    }
    
    dev->resources[dev->num_resources++] = *resource;
    return 0;
}

/**
 * @brief Get a resource from a device
 */
struct device_resource *device_get_resource(struct device *dev, resource_type_t type, uint32_t index) {
    if (dev == nullptr) {
        return nullptr;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < dev->num_resources; i++) {
        if (dev->resources[i].type == type) {
            if (count == index) {
                return &dev->resources[i];
            }
            count++;
        }
    }
    
    return nullptr;
}

/**
 * @brief Suspend a device
 */
int device_suspend(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    if (dev->ops != nullptr && dev->ops->suspend != nullptr) {
        int result = dev->ops->suspend(dev);
        if (result == 0) {
            atomic_fetch_or_explicit(&dev->flags, DEVICE_FLAG_SUSPENDED, memory_order_release);
            atomic_store_explicit(&dev->state, DEVICE_STATE_SUSPENDED, memory_order_release);
        }
        return result;
    }
    
    return 0;
}

/**
 * @brief Resume a device
 */
int device_resume(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    if (dev->ops != nullptr && dev->ops->resume != nullptr) {
        int result = dev->ops->resume(dev);
        if (result == 0) {
            atomic_fetch_and_explicit(&dev->flags, ~DEVICE_FLAG_SUSPENDED, memory_order_release);
            atomic_store_explicit(&dev->state, DEVICE_STATE_READY, memory_order_release);
        }
        return result;
    }
    
    return 0;
}

/**
 * @brief Get device statistics
 */
void device_manager_get_stats(uint64_t *total_devices, uint64_t *active_devices) {
    if (total_devices != nullptr) {
        *total_devices = atomic_load_explicit(&g_device_manager.total_devices, memory_order_relaxed);
    }
    if (active_devices != nullptr) {
        *active_devices = atomic_load_explicit(&g_device_manager.active_devices, memory_order_relaxed);
    }
}
