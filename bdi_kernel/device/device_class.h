
/**
 * @file device_class.h
 * @brief Device Class Framework
 * 
 * Provides device class framework for organizing devices by type
 * with class-specific operations and attributes.
 */

#ifndef BDI_DEVICE_CLASS_H
#define BDI_DEVICE_CLASS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "device_manager.h"
#include "driver_interface.h"

/* Maximum devices per class */
#define CLASS_MAX_DEVICES       256

/**
 * @brief Device class operations
 */
struct device_class_ops {
    /* Initialize class */
    int (*init)(struct device_class *class);
    
    /* Shutdown class */
    void (*shutdown)(struct device_class *class);
    
    /* Add device to class */
    int (*add_device)(struct device_class *class, struct device *dev);
    
    /* Remove device from class */
    void (*remove_device)(struct device_class *class, struct device *dev);
    
    /* Enumerate devices */
    size_t (*enumerate)(struct device_class *class, struct device **devices, size_t max_devices);
};

/**
 * @brief Device class structure
 */
struct device_class {
    char name[DEVICE_NAME_MAX];
    device_type_t type;
    
    /* Class operations */
    const struct device_class_ops *ops;
    
    /* Devices in this class */
    struct device *devices[CLASS_MAX_DEVICES];
    uint32_t num_devices;
    
    /* Class-specific data */
    void *class_data;
    
    /* File operations for this class */
    const struct file_operations *fops;
};

/* Block device class */
extern struct device_class block_device_class;

/* Character device class */
extern struct device_class char_device_class;

/* Network device class */
extern struct device_class network_device_class;

/* Input device class */
extern struct device_class input_device_class;

/* Display device class */
extern struct device_class display_device_class;

/* Timer device class */
extern struct device_class timer_device_class;

/* Power device class */
extern struct device_class power_device_class;

/**
 * @brief Initialize all device classes
 * 
 * @return 0 on success, negative error code on failure
 */
int device_classes_init(void);

/**
 * @brief Shutdown all device classes
 */
void device_classes_shutdown(void);

/**
 * @brief Get device class by type
 * 
 * @param type Device type
 * @return Device class or nullptr if not found
 */
struct device_class *device_class_get(device_type_t type);

/**
 * @brief Add device to its class
 * 
 * @param dev Device
 * @return 0 on success, negative error code on failure
 */
int device_class_add_device(struct device *dev);

/**
 * @brief Remove device from its class
 * 
 * @param dev Device
 */
void device_class_remove_device(struct device *dev);

#endif /* BDI_DEVICE_CLASS_H */
