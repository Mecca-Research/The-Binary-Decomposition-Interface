
// ===================================================================
// DESC: USB Integration Layer (Phase 12 Day 4)
//       Integration with Phase 8 Device Management
// ===================================================================
// MODERNIZED: Phase 12 - C23 features

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>

// Integration with Phase 8 Device Management
// This file provides the glue between USB subsystem and kernel device tree

// Device class definitions
#define USB_CLASS_AUDIO             0x01
#define USB_CLASS_COMM              0x02
#define USB_CLASS_HID               0x03
#define USB_CLASS_PHYSICAL          0x05
#define USB_CLASS_IMAGE             0x06
#define USB_CLASS_PRINTER           0x07
#define USB_CLASS_MASS_STORAGE      0x08
#define USB_CLASS_HUB               0x09
#define USB_CLASS_CDC_DATA          0x0A
#define USB_CLASS_SMART_CARD        0x0B
#define USB_CLASS_CONTENT_SECURITY  0x0D
#define USB_CLASS_VIDEO             0x0E
#define USB_CLASS_HEALTHCARE        0x0F
#define USB_CLASS_DIAGNOSTIC        0xDC
#define USB_CLASS_WIRELESS          0xE0
#define USB_CLASS_MISC              0xEF
#define USB_CLASS_APP_SPECIFIC      0xFE
#define USB_CLASS_VENDOR_SPECIFIC   0xFF

// USB device attributes for sysfs-like interface
typedef struct {
    char name[64];                      // Attribute name
    char value[256];                    // Attribute value
    _Atomic bool readable;              // Readable flag
    _Atomic bool writable;              // Writable flag
} usb_device_attribute_t;

// USB device node in device tree
typedef struct {
    uint8_t slot_id;                    // USB slot ID
    uint8_t bus_num;                    // Bus number
    uint8_t port_num;                   // Port number
    uint8_t device_class;               // Device class
    
    char device_path[256];              // Device path (/dev/usb/...)
    char sysfs_path[256];               // Sysfs path (/sys/bus/usb/...)
    
    usb_device_attribute_t *attributes; // Device attributes
    uint32_t num_attributes;            // Number of attributes
    
    _Atomic bool registered;            // Registration state
    _Atomic uint32_t ref_count;         // Reference count
} usb_device_node_t;

// Global device registry
#define USB_MAX_DEVICE_NODES 256
static usb_device_node_t g_device_nodes[USB_MAX_DEVICE_NODES];
static _Atomic uint32_t g_node_count = 0;

/**
 * Initialize USB integration layer
 */
[[nodiscard]] int usb_integration_init(void) {
    memset(g_device_nodes, 0, sizeof(g_device_nodes));
    atomic_store_explicit(&g_node_count, 0, memory_order_release);
    return 0;
}

/**
 * Register USB device in device tree
 * Integrates with Phase 8 device management
 */
[[nodiscard]] int usb_register_device(uint8_t slot_id, uint8_t bus_num, 
                                      uint8_t port_num, uint8_t device_class) {
    // Find free device node
    usb_device_node_t *node = nullptr;
    for (uint32_t i = 0; i < USB_MAX_DEVICE_NODES; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong_explicit(&g_device_nodes[i].registered,
                                                     &expected, true,
                                                     memory_order_acq_rel,
                                                     memory_order_acquire)) {
            node = &g_device_nodes[i];
            break;
        }
    }
    
    if (!node) {
        return -1;  // No free slots
    }
    
    // Initialize device node
    node->slot_id = slot_id;
    node->bus_num = bus_num;
    node->port_num = port_num;
    node->device_class = device_class;
    
    // Create device path
    snprintf(node->device_path, sizeof(node->device_path),
             "/dev/usb/bus%d/port%d", bus_num, port_num);
    
    // Create sysfs path
    snprintf(node->sysfs_path, sizeof(node->sysfs_path),
             "/sys/bus/usb/devices/%d-%d", bus_num, port_num);
    
    // Allocate attributes
    node->num_attributes = 8;
    node->attributes = calloc(8, sizeof(usb_device_attribute_t));
    if (!node->attributes) {
        atomic_store_explicit(&node->registered, false, memory_order_release);
        return -2;
    }
    
    // Initialize standard attributes
    usb_device_attribute_t *attr;
    
    // Attribute: idVendor
    attr = &node->attributes[0];
    strncpy(attr->name, "idVendor", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: idProduct
    attr = &node->attributes[1];
    strncpy(attr->name, "idProduct", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: bDeviceClass
    attr = &node->attributes[2];
    strncpy(attr->name, "bDeviceClass", 63);
    snprintf(attr->value, 255, "0x%02x", device_class);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: manufacturer
    attr = &node->attributes[3];
    strncpy(attr->name, "manufacturer", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: product
    attr = &node->attributes[4];
    strncpy(attr->name, "product", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: serial
    attr = &node->attributes[5];
    strncpy(attr->name, "serial", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: speed
    attr = &node->attributes[6];
    strncpy(attr->name, "speed", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, false, memory_order_relaxed);
    
    // Attribute: power
    attr = &node->attributes[7];
    strncpy(attr->name, "power", 63);
    atomic_store_explicit(&attr->readable, true, memory_order_relaxed);
    atomic_store_explicit(&attr->writable, true, memory_order_relaxed);
    
    atomic_store_explicit(&node->ref_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_node_count, 1, memory_order_release);
    
    return 0;
}

/**
 * Unregister USB device from device tree
 */
void usb_unregister_device(uint8_t slot_id) {
    for (uint32_t i = 0; i < USB_MAX_DEVICE_NODES; i++) {
        usb_device_node_t *node = &g_device_nodes[i];
        
        if (node->slot_id == slot_id && 
            atomic_load_explicit(&node->registered, memory_order_acquire)) {
            
            // Free attributes
            if (node->attributes) {
                free(node->attributes);
                node->attributes = nullptr;
            }
            
            atomic_store_explicit(&node->registered, false, memory_order_release);
            atomic_fetch_sub_explicit(&g_node_count, 1, memory_order_release);
            break;
        }
    }
}

/**
 * Get device node by slot ID
 */
[[nodiscard]] usb_device_node_t* usb_get_device_node(uint8_t slot_id) {
    for (uint32_t i = 0; i < USB_MAX_DEVICE_NODES; i++) {
        usb_device_node_t *node = &g_device_nodes[i];
        
        if (node->slot_id == slot_id && 
            atomic_load_explicit(&node->registered, memory_order_acquire)) {
            atomic_fetch_add_explicit(&node->ref_count, 1, memory_order_relaxed);
            return node;
        }
    }
    return nullptr;
}

/**
 * Release device node reference
 */
void usb_put_device_node(usb_device_node_t *node) {
    if (!node) return;
    atomic_fetch_sub_explicit(&node->ref_count, 1, memory_order_release);
}

/**
 * Read device attribute
 */
[[nodiscard]] int usb_read_attribute(usb_device_node_t *node, const char *name,
                                     char *value, size_t size) {
    if (!node || !name || !value) return -1;
    
    for (uint32_t i = 0; i < node->num_attributes; i++) {
        usb_device_attribute_t *attr = &node->attributes[i];
        
        if (strcmp(attr->name, name) == 0) {
            if (!atomic_load_explicit(&attr->readable, memory_order_acquire)) {
                return -2;  // Not readable
            }
            
            strncpy(value, attr->value, size - 1);
            value[size - 1] = '\0';
            return 0;
        }
    }
    
    return -3;  // Attribute not found
}

/**
 * Write device attribute
 */
[[nodiscard]] int usb_write_attribute(usb_device_node_t *node, const char *name,
                                      const char *value) {
    if (!node || !name || !value) return -1;
    
    for (uint32_t i = 0; i < node->num_attributes; i++) {
        usb_device_attribute_t *attr = &node->attributes[i];
        
        if (strcmp(attr->name, name) == 0) {
            if (!atomic_load_explicit(&attr->writable, memory_order_acquire)) {
                return -2;  // Not writable
            }
            
            strncpy(attr->value, value, sizeof(attr->value) - 1);
            attr->value[sizeof(attr->value) - 1] = '\0';
            return 0;
        }
    }
    
    return -3;  // Attribute not found
}

/**
 * Get registered device count
 */
[[nodiscard]] uint32_t usb_get_registered_count(void) {
    return atomic_load_explicit(&g_node_count, memory_order_acquire);
}

/**
 * Power management: Suspend device
 */
[[nodiscard]] int usb_suspend_device(uint8_t slot_id) {
    usb_device_node_t *node = usb_get_device_node(slot_id);
    if (!node) return -1;
    
    // Set power attribute to "suspended"
    usb_write_attribute(node, "power", "suspended");
    
    usb_put_device_node(node);
    return 0;
}

/**
 * Power management: Resume device
 */
[[nodiscard]] int usb_resume_device(uint8_t slot_id) {
    usb_device_node_t *node = usb_get_device_node(slot_id);
    if (!node) return -1;
    
    // Set power attribute to "active"
    usb_write_attribute(node, "power", "active");
    
    usb_put_device_node(node);
    return 0;
}

/**
 * Cleanup integration layer
 */
void usb_integration_cleanup(void) {
    for (uint32_t i = 0; i < USB_MAX_DEVICE_NODES; i++) {
        usb_device_node_t *node = &g_device_nodes[i];
        
        if (atomic_load_explicit(&node->registered, memory_order_acquire)) {
            if (node->attributes) {
                free(node->attributes);
            }
        }
    }
    
    atomic_store_explicit(&g_node_count, 0, memory_order_release);
}
