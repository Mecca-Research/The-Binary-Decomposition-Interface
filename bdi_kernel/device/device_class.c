
/**
 * @file device_class.c
 * @brief Device Class Framework Implementation
 */

#include "device_class.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations of class-specific operations */
static int block_class_init(struct device_class *class);
static int char_class_init(struct device_class *class);
static int network_class_init(struct device_class *class);
static int input_class_init(struct device_class *class);
static int display_class_init(struct device_class *class);
static int timer_class_init(struct device_class *class);
static int power_class_init(struct device_class *class);

static void generic_class_shutdown(struct device_class *class);
static int generic_add_device(struct device_class *class, struct device *dev);
static void generic_remove_device(struct device_class *class, struct device *dev);
static size_t generic_enumerate(struct device_class *class, struct device **devices, size_t max_devices);

/* Generic class operations */
static const struct device_class_ops generic_class_ops = {
    .init = nullptr,
    .shutdown = generic_class_shutdown,
    .add_device = generic_add_device,
    .remove_device = generic_remove_device,
    .enumerate = generic_enumerate
};

/* Block device class */
struct device_class block_device_class = {
    .name = "block",
    .type = DEVICE_TYPE_BLOCK,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Character device class */
struct device_class char_device_class = {
    .name = "char",
    .type = DEVICE_TYPE_CHAR,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Network device class */
struct device_class network_device_class = {
    .name = "network",
    .type = DEVICE_TYPE_NETWORK,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Input device class */
struct device_class input_device_class = {
    .name = "input",
    .type = DEVICE_TYPE_INPUT,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Display device class */
struct device_class display_device_class = {
    .name = "display",
    .type = DEVICE_TYPE_DISPLAY,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Timer device class */
struct device_class timer_device_class = {
    .name = "timer",
    .type = DEVICE_TYPE_TIMER,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Power device class */
struct device_class power_device_class = {
    .name = "power",
    .type = DEVICE_TYPE_POWER,
    .ops = &generic_class_ops,
    .num_devices = 0,
    .class_data = nullptr,
    .fops = nullptr
};

/* Array of all device classes */
static struct device_class *all_classes[] = {
    &block_device_class,
    &char_device_class,
    &network_device_class,
    &input_device_class,
    &display_device_class,
    &timer_device_class,
    &power_device_class
};

/**
 * @brief Initialize all device classes
 */
int device_classes_init(void) {
    printf("[DeviceClass] Initializing device classes...\n");
    
    for (size_t i = 0; i < sizeof(all_classes) / sizeof(all_classes[0]); i++) {
        struct device_class *class = all_classes[i];
        
        if (class->ops != nullptr && class->ops->init != nullptr) {
            int result = class->ops->init(class);
            if (result != 0) {
                printf("[DeviceClass] Failed to initialize class: %s\n", class->name);
                return result;
            }
        }
        
        printf("[DeviceClass] Initialized class: %s (type: %d)\n", class->name, class->type);
    }
    
    return 0;
}

/**
 * @brief Shutdown all device classes
 */
void device_classes_shutdown(void) {
    printf("[DeviceClass] Shutting down device classes...\n");
    
    for (size_t i = 0; i < sizeof(all_classes) / sizeof(all_classes[0]); i++) {
        struct device_class *class = all_classes[i];
        
        if (class->ops != nullptr && class->ops->shutdown != nullptr) {
            class->ops->shutdown(class);
        }
        
        printf("[DeviceClass] Shutdown class: %s\n", class->name);
    }
}

/**
 * @brief Get device class by type
 */
struct device_class *device_class_get(device_type_t type) {
    for (size_t i = 0; i < sizeof(all_classes) / sizeof(all_classes[0]); i++) {
        if (all_classes[i]->type == type) {
            return all_classes[i];
        }
    }
    return nullptr;
}

/**
 * @brief Add device to its class
 */
int device_class_add_device(struct device *dev) {
    if (dev == nullptr) {
        return -1;
    }
    
    struct device_class *class = device_class_get(dev->type);
    if (class == nullptr) {
        return -1;
    }
    
    dev->class = class;
    
    if (class->ops != nullptr && class->ops->add_device != nullptr) {
        return class->ops->add_device(class, dev);
    }
    
    return 0;
}

/**
 * @brief Remove device from its class
 */
void device_class_remove_device(struct device *dev) {
    if (dev == nullptr || dev->class == nullptr) {
        return;
    }
    
    struct device_class *class = dev->class;
    
    if (class->ops != nullptr && class->ops->remove_device != nullptr) {
        class->ops->remove_device(class, dev);
    }
    
    dev->class = nullptr;
}

/**
 * @brief Generic class shutdown
 */
static void generic_class_shutdown(struct device_class *class) {
    if (class == nullptr) {
        return;
    }
    
    /* Remove all devices from class */
    for (uint32_t i = 0; i < class->num_devices; i++) {
        if (class->devices[i] != nullptr) {
            class->devices[i]->class = nullptr;
        }
    }
    
    class->num_devices = 0;
}

/**
 * @brief Generic add device to class
 */
static int generic_add_device(struct device_class *class, struct device *dev) {
    if (class == nullptr || dev == nullptr) {
        return -1;
    }
    
    if (class->num_devices >= CLASS_MAX_DEVICES) {
        printf("[DeviceClass] Class %s is full\n", class->name);
        return -1;
    }
    
    class->devices[class->num_devices++] = dev;
    printf("[DeviceClass] Added device %s to class %s\n", dev->name, class->name);
    
    return 0;
}

/**
 * @brief Generic remove device from class
 */
static void generic_remove_device(struct device_class *class, struct device *dev) {
    if (class == nullptr || dev == nullptr) {
        return;
    }
    
    for (uint32_t i = 0; i < class->num_devices; i++) {
        if (class->devices[i] == dev) {
            /* Shift remaining devices */
            for (uint32_t j = i; j < class->num_devices - 1; j++) {
                class->devices[j] = class->devices[j + 1];
            }
            class->num_devices--;
            printf("[DeviceClass] Removed device %s from class %s\n", dev->name, class->name);
            return;
        }
    }
}

/**
 * @brief Generic enumerate devices in class
 */
static size_t generic_enumerate(struct device_class *class, struct device **devices, size_t max_devices) {
    if (class == nullptr || devices == nullptr || max_devices == 0) {
        return 0;
    }
    
    size_t count = 0;
    for (uint32_t i = 0; i < class->num_devices && count < max_devices; i++) {
        devices[count++] = class->devices[i];
    }
    
    return count;
}
