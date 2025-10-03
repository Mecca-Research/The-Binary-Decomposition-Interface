
// ===================================================================
// DESC: USB Core Management (Phase 12 Day 3)
//       Core USB device enumeration and management
// ===================================================================
// MODERNIZED: Phase 12 - C23 features

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// USB Device Descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// USB Configuration Descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB Interface Descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

// USB Endpoint Descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// USB Device State
typedef enum {
    USB_STATE_DETACHED = 0,
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED,
    USB_STATE_SUSPENDED
} usb_device_state_t;

// USB Device Structure
typedef struct {
    uint8_t slot_id;                       // xHCI slot ID
    uint8_t port_num;                      // Port number
    uint8_t address;                       // USB address
    uint8_t speed;                         // Device speed
    _Atomic usb_device_state_t state;      // Device state
    
    usb_device_descriptor_t descriptor;    // Device descriptor
    usb_config_descriptor_t *config_desc;  // Configuration descriptor
    
    uint8_t num_interfaces;                // Number of interfaces
    uint8_t current_config;                // Current configuration
    
    _Atomic bool enumerated;               // Enumeration complete
    _Atomic uint32_t ref_count;            // Reference count
    
    char manufacturer[128];                // Manufacturer string
    char product[128];                     // Product string
    char serial[128];                      // Serial number string
} usb_device_t;

// USB Device Tree
#define USB_MAX_DEVICES 256
static usb_device_t g_usb_devices[USB_MAX_DEVICES];
static _Atomic uint32_t g_device_count = 0;
static _Atomic bool g_usb_core_initialized = false;

/**
 * Initialize USB core subsystem
 */
[[nodiscard]] int usb_core_init(void) {
    if (atomic_load_explicit(&g_usb_core_initialized, memory_order_acquire)) {
        return 0;
    }
    
    memset(g_usb_devices, 0, sizeof(g_usb_devices));
    atomic_store_explicit(&g_device_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_usb_core_initialized, true, memory_order_release);
    
    return 0;
}

/**
 * Allocate USB device slot
 */
[[nodiscard]] usb_device_t* usb_alloc_device(uint8_t port_num) {
    if (!atomic_load_explicit(&g_usb_core_initialized, memory_order_acquire)) {
        return nullptr;
    }
    
    // Find free device slot using atomic operations
    for (uint32_t i = 0; i < USB_MAX_DEVICES; i++) {
        usb_device_t *device = &g_usb_devices[i];
        usb_device_state_t expected = USB_STATE_DETACHED;
        
        if (atomic_compare_exchange_strong_explicit(&device->state,
                                                     &expected, USB_STATE_ATTACHED,
                                                     memory_order_acq_rel,
                                                     memory_order_acquire)) {
            // Successfully allocated device
            device->port_num = port_num;
            atomic_store_explicit(&device->ref_count, 1, memory_order_relaxed);
            atomic_store_explicit(&device->enumerated, false, memory_order_relaxed);
            
            atomic_fetch_add_explicit(&g_device_count, 1, memory_order_relaxed);
            return device;
        }
    }
    
    return nullptr;  // No free slots
}

/**
 * Free USB device
 */
void usb_free_device(usb_device_t *device) {
    if (!device) return;
    
    uint32_t old_ref = atomic_fetch_sub_explicit(&device->ref_count, 1, memory_order_release);
    
    if (old_ref == 1) {
        // Last reference
        if (device->config_desc) {
            free(device->config_desc);
            device->config_desc = nullptr;
        }
        
        atomic_store_explicit(&device->state, USB_STATE_DETACHED, memory_order_release);
        atomic_fetch_sub_explicit(&g_device_count, 1, memory_order_relaxed);
    }
}

/**
 * Enumerate USB device
 * Reads device descriptor and configures device
 */
[[nodiscard]] int usb_enumerate_device(usb_device_t *device) {
    if (!device) return -1;
    
    // Set device to DEFAULT state
    atomic_store_explicit(&device->state, USB_STATE_DEFAULT, memory_order_release);
    
    // Read device descriptor (simplified - would use control transfer)
    // In real implementation: usb_control_transfer(GET_DESCRIPTOR, DEVICE, ...)
    
    // Assign USB address
    device->address = (uint8_t)(device->slot_id);
    atomic_store_explicit(&device->state, USB_STATE_ADDRESS, memory_order_release);
    
    // Read configuration descriptor
    // In real implementation: usb_control_transfer(GET_DESCRIPTOR, CONFIGURATION, ...)
    
    // Set configuration
    device->current_config = 1;
    atomic_store_explicit(&device->state, USB_STATE_CONFIGURED, memory_order_release);
    
    atomic_store_explicit(&device->enumerated, true, memory_order_release);
    
    return 0;
}

/**
 * Get device by slot ID
 */
[[nodiscard]] usb_device_t* usb_get_device(uint8_t slot_id) {
    for (uint32_t i = 0; i < USB_MAX_DEVICES; i++) {
        usb_device_t *device = &g_usb_devices[i];
        if (device->slot_id == slot_id && 
            atomic_load_explicit(&device->state, memory_order_acquire) != USB_STATE_DETACHED) {
            // Increment reference count
            atomic_fetch_add_explicit(&device->ref_count, 1, memory_order_relaxed);
            return device;
        }
    }
    return nullptr;
}

/**
 * Get device count
 */
[[nodiscard]] uint32_t usb_get_device_count(void) {
    return atomic_load_explicit(&g_device_count, memory_order_acquire);
}

/**
 * Device hotplug notification
 */
void usb_device_connected(uint8_t port_num, uint8_t speed) {
    usb_device_t *device = usb_alloc_device(port_num);
    if (!device) return;
    
    device->speed = speed;
    atomic_store_explicit(&device->state, USB_STATE_POWERED, memory_order_release);
    
    // Trigger enumeration
    usb_enumerate_device(device);
}

/**
 * Device disconnection notification
 */
void usb_device_disconnected(uint8_t slot_id) {
    usb_device_t *device = usb_get_device(slot_id);
    if (!device) return;
    
    atomic_store_explicit(&device->state, USB_STATE_DETACHED, memory_order_release);
    usb_free_device(device);  // Release our reference
}

/**
 * Cleanup USB core
 */
void usb_core_cleanup(void) {
    if (!atomic_load_explicit(&g_usb_core_initialized, memory_order_acquire)) {
        return;
    }
    
    // Free all devices
    for (uint32_t i = 0; i < USB_MAX_DEVICES; i++) {
        usb_device_t *device = &g_usb_devices[i];
        if (atomic_load_explicit(&device->state, memory_order_acquire) != USB_STATE_DETACHED) {
            if (device->config_desc) {
                free(device->config_desc);
            }
        }
    }
    
    atomic_store_explicit(&g_usb_core_initialized, false, memory_order_release);
}
