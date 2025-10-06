
/**
 * @file device_manager.h
 * @brief Device Manager Core - Registration, Discovery, and Lifecycle Management
 * 
 * Comprehensive device management system for the BDI kernel.
 * Provides device registration, discovery, hierarchy management,
 * and lifecycle control with C23 atomic operations.
 * 
 * Key Features:
 * - Device registration and discovery APIs
 * - Device tree/hierarchy management
 * - Lifecycle management (probe, attach, detach, remove)
 * - Reference counting and cleanup
 * - Device matching and driver binding
 * - NUMA-aware device placement
 * - Hotplug support integration
 */

#ifndef BDI_DEVICE_MANAGER_H
#define BDI_DEVICE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "c23_compat.h"

/* Device Manager Constants */
#define DEVICE_NAME_MAX         64      /* Maximum device name length */
#define DEVICE_PATH_MAX         256     /* Maximum device path length */
#define DEVICE_MAX_RESOURCES    16      /* Maximum resources per device */
#define DEVICE_MAX_CHILDREN     32      /* Maximum child devices */
#define DEVICE_HASH_BUCKETS     256     /* Device hash table size */

/* Device Types */
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_BLOCK,          /* Block devices (storage) */
    DEVICE_TYPE_CHAR,           /* Character devices (serial, terminals) */
    DEVICE_TYPE_NETWORK,        /* Network devices (NICs) */
    DEVICE_TYPE_INPUT,          /* Input devices (keyboard, mouse) */
    DEVICE_TYPE_DISPLAY,        /* Display devices (framebuffer, GPU) */
    DEVICE_TYPE_TIMER,          /* Timer devices (RTC, HPET) */
    DEVICE_TYPE_POWER,          /* Power devices (ACPI, battery) */
    DEVICE_TYPE_BUS,            /* Bus controllers (PCI, USB) */
    DEVICE_TYPE_ACCELERATOR,    /* Compute accelerators (GPU, FPGA, BPU) */
    DEVICE_TYPE_MAX
} device_type_t;

/* Device States */
typedef enum {
    DEVICE_STATE_UNINITIALIZED = 0,
    DEVICE_STATE_REGISTERED,    /* Device registered but not probed */
    DEVICE_STATE_PROBING,       /* Device being probed */
    DEVICE_STATE_READY,         /* Device ready for use */
    DEVICE_STATE_SUSPENDED,     /* Device suspended */
    DEVICE_STATE_ERROR,         /* Device in error state */
    DEVICE_STATE_REMOVING,      /* Device being removed */
    DEVICE_STATE_REMOVED        /* Device removed */
} device_state_t;

/* Device Flags */
#define DEVICE_FLAG_HOTPLUG     (1U << 0)  /* Supports hotplug */
#define DEVICE_FLAG_REMOVABLE   (1U << 1)  /* Removable device */
#define DEVICE_FLAG_NUMA_LOCAL  (1U << 2)  /* NUMA-local device */
#define DEVICE_FLAG_DMA_CAPABLE (1U << 3)  /* DMA capable */
#define DEVICE_FLAG_MSI_CAPABLE (1U << 4)  /* MSI/MSI-X capable */
#define DEVICE_FLAG_POWER_MGMT  (1U << 5)  /* Power management support */
#define DEVICE_FLAG_SUSPENDED   (1U << 6)  /* Currently suspended */

/* Resource Types */
typedef enum {
    RESOURCE_TYPE_NONE = 0,
    RESOURCE_TYPE_IO,           /* I/O port range */
    RESOURCE_TYPE_MEM,          /* Memory range */
    RESOURCE_TYPE_IRQ,          /* Interrupt line */
    RESOURCE_TYPE_DMA,          /* DMA channel */
    RESOURCE_TYPE_BUS           /* Bus address */
} resource_type_t;

/**
 * @brief Device resource descriptor
 */
struct device_resource {
    resource_type_t type;
    uint64_t start;             /* Start address/number */
    uint64_t end;               /* End address/number */
    uint32_t flags;
    char name[32];
};

/* Forward declarations */
struct device;
struct device_driver;
struct device_class;

/**
 * @brief Device operations
 */
struct device_ops {
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    int (*shutdown)(struct device *dev);
};

/**
 * @brief Device structure
 * 
 * Core device representation with atomic state management
 * and reference counting.
 */
struct device {
    /* Device identification */
    uint64_t id;                        /* Unique device ID */
    char name[DEVICE_NAME_MAX];         /* Device name */
    char path[DEVICE_PATH_MAX];         /* Device path (/dev/...) */
    device_type_t type;                 /* Device type */
    
    /* Device state (atomic) */
    _Atomic device_state_t state;
    _Atomic uint32_t flags;
    _Atomic uint32_t refcount;          /* Reference count */
    
    /* Device hierarchy */
    struct device *parent;              /* Parent device */
    struct device *children[DEVICE_MAX_CHILDREN];
    uint32_t num_children;
    
    /* Driver binding */
    struct device_driver *driver;       /* Bound driver */
    struct device_class *class;         /* Device class */
    void *driver_data;                  /* Driver private data */
    
    /* Resources */
    struct device_resource resources[DEVICE_MAX_RESOURCES];
    uint32_t num_resources;
    
    /* NUMA information */
    uint32_t numa_node;
    
    /* Device operations */
    const struct device_ops *ops;
    
    /* Hash table linkage */
    struct device *hash_next;
    
    /* List linkage */
    struct device *next;
    struct device *prev;
    
    /* Device-specific data */
    void *platform_data;
    
    /* Statistics */
    uint64_t probe_time_ns;
    uint64_t last_access_time;
    _Atomic uint64_t access_count;
};

/**
 * @brief Device driver structure
 */
struct device_driver {
    char name[DEVICE_NAME_MAX];
    device_type_t type;
    
    /* Driver operations */
    const struct device_ops *ops;
    
    /* Device matching */
    int (*match)(struct device *dev);
    int (*bind)(struct device *dev);
    void (*unbind)(struct device *dev);
    
    /* Driver private data */
    void *driver_data;
    
    /* List linkage */
    struct device_driver *next;
};

/**
 * @brief Device manager state
 */
struct device_manager {
    /* Device hash table */
    struct device *device_hash[DEVICE_HASH_BUCKETS];
    
    /* Device lists by type */
    struct device *devices_by_type[DEVICE_TYPE_MAX];
    
    /* Registered drivers */
    struct device_driver *drivers;
    
    /* Statistics */
    _Atomic uint64_t total_devices;
    _Atomic uint64_t active_devices;
    _Atomic uint64_t total_probes;
    _Atomic uint64_t failed_probes;
    
    /* Next device ID */
    _Atomic uint64_t next_device_id;
    
    /* Manager state */
    _Atomic bool initialized;
};

/* Global device manager instance */
extern struct device_manager g_device_manager;

/**
 * @brief Initialize the device manager
 * 
 * @return 0 on success, negative error code on failure
 */
int device_manager_init(void);

/**
 * @brief Shutdown the device manager
 */
void device_manager_shutdown(void);

/**
 * @brief Register a new device
 * 
 * @param dev Device to register
 * @return 0 on success, negative error code on failure
 */
int device_register(struct device *dev);

/**
 * @brief Unregister a device
 * 
 * @param dev Device to unregister
 */
void device_unregister(struct device *dev);

/**
 * @brief Find a device by ID
 * 
 * @param id Device ID
 * @return Device pointer or nullptr if not found
 */
struct device *device_find_by_id(uint64_t id);

/**
 * @brief Find a device by name
 * 
 * @param name Device name
 * @return Device pointer or nullptr if not found
 */
struct device *device_find_by_name(const char *name);

/**
 * @brief Find a device by path
 * 
 * @param path Device path
 * @return Device pointer or nullptr if not found
 */
struct device *device_find_by_path(const char *path);

/**
 * @brief Get devices by type
 * 
 * @param type Device type
 * @param devices Output array for devices
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found
 */
size_t device_get_by_type(device_type_t type, struct device **devices, size_t max_devices);

/**
 * @brief Increment device reference count
 * 
 * @param dev Device
 */
void device_get(struct device *dev);

/**
 * @brief Decrement device reference count
 * 
 * @param dev Device
 */
void device_put(struct device *dev);

/**
 * @brief Probe a device
 * 
 * @param dev Device to probe
 * @return 0 on success, negative error code on failure
 */
int device_probe(struct device *dev);

/**
 * @brief Remove a device
 * 
 * @param dev Device to remove
 * @return 0 on success, negative error code on failure
 */
int device_remove(struct device *dev);

/**
 * @brief Attach a device to its parent
 * 
 * @param dev Device to attach
 * @param parent Parent device
 * @return 0 on success, negative error code on failure
 */
int device_attach(struct device *dev, struct device *parent);

/**
 * @brief Detach a device from its parent
 * 
 * @param dev Device to detach
 */
void device_detach(struct device *dev);

/**
 * @brief Register a device driver
 * 
 * @param driver Driver to register
 * @return 0 on success, negative error code on failure
 */
int device_driver_register(struct device_driver *driver);

/**
 * @brief Unregister a device driver
 * 
 * @param driver Driver to unregister
 */
void device_driver_unregister(struct device_driver *driver);

/**
 * @brief Match and bind a driver to a device
 * 
 * @param dev Device
 * @return 0 on success, negative error code on failure
 */
int device_bind_driver(struct device *dev);

/**
 * @brief Unbind a driver from a device
 * 
 * @param dev Device
 */
void device_unbind_driver(struct device *dev);

/**
 * @brief Add a resource to a device
 * 
 * @param dev Device
 * @param resource Resource to add
 * @return 0 on success, negative error code on failure
 */
int device_add_resource(struct device *dev, const struct device_resource *resource);

/**
 * @brief Get a resource from a device
 * 
 * @param dev Device
 * @param type Resource type
 * @param index Resource index
 * @return Resource pointer or nullptr if not found
 */
struct device_resource *device_get_resource(struct device *dev, resource_type_t type, uint32_t index);

/**
 * @brief Suspend a device
 * 
 * @param dev Device to suspend
 * @return 0 on success, negative error code on failure
 */
int device_suspend(struct device *dev);

/**
 * @brief Resume a device
 * 
 * @param dev Device to resume
 * @return 0 on success, negative error code on failure
 */
int device_resume(struct device *dev);

/**
 * @brief Get device statistics
 * 
 * @param total_devices Output for total devices
 * @param active_devices Output for active devices
 */
void device_manager_get_stats(uint64_t *total_devices, uint64_t *active_devices);

#endif /* BDI_DEVICE_MANAGER_H */
