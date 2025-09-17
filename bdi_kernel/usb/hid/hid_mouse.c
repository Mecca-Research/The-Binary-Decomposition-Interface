
// ===================================================================
// DESC: HID Mouse driver implementation for BDI Kernel
//       Handles USB HID mouse devices and input events
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// HID Mouse Constants
#define HID_MOUSE_MAX_DEVICES   8
#define HID_MOUSE_REPORT_SIZE   4
#define HID_MOUSE_ENDPOINT      1

// HID Mouse Button Bits
#define HID_MOUSE_BTN_LEFT      0x01
#define HID_MOUSE_BTN_RIGHT     0x02
#define HID_MOUSE_BTN_MIDDLE    0x04
#define HID_MOUSE_BTN_SIDE      0x08
#define HID_MOUSE_BTN_EXTRA     0x10

// HID Mouse Report Structure
typedef struct {
    uint8_t buttons;        // Button state bitmask
    int8_t x;              // X movement delta
    int8_t y;              // Y movement delta
    int8_t wheel;          // Wheel movement delta
} hid_mouse_report_t;

// HID Mouse Device Structure
typedef struct {
    uint32_t device_id;     // USB device ID
    uint8_t interface;      // Interface number
    uint8_t endpoint;       // Interrupt endpoint
    uint16_t max_packet;    // Maximum packet size
    uint16_t interval;      // Polling interval
    uint8_t active;         // Device active flag
    hid_mouse_report_t last_report; // Last received report
    void (*callback)(uint32_t device_id, hid_mouse_report_t *report); // Event callback
} hid_mouse_device_t;

// Mouse Event Structure
typedef struct {
    uint32_t device_id;     // Device that generated event
    uint32_t timestamp;     // Event timestamp
    int16_t x_delta;        // X movement
    int16_t y_delta;        // Y movement
    int8_t wheel_delta;     // Wheel movement
    uint8_t buttons;        // Button state
    uint8_t button_changed; // Changed buttons mask
} hid_mouse_event_t;

// Global mouse state
static hid_mouse_device_t g_mouse_devices[HID_MOUSE_MAX_DEVICES];
static uint32_t g_device_count = 0;
static uint8_t g_mouse_initialized = 0;
static void (*g_global_callback)(hid_mouse_event_t *event) = NULL;

// Function prototypes
int hid_mouse_init(void);
int hid_mouse_register_device(uint32_t device_id, uint8_t interface, 
                             uint8_t endpoint, uint16_t max_packet, uint16_t interval);
int hid_mouse_unregister_device(uint32_t device_id);
int hid_mouse_process_report(uint32_t device_id, const uint8_t *data, uint16_t length);
int hid_mouse_set_callback(uint32_t device_id, 
                          void (*callback)(uint32_t device_id, hid_mouse_report_t *report));
int hid_mouse_set_global_callback(void (*callback)(hid_mouse_event_t *event));
hid_mouse_device_t *hid_mouse_get_device(uint32_t device_id);
uint32_t hid_mouse_get_device_count(void);
void hid_mouse_cleanup(void);

/**
 * Initialize HID mouse subsystem
 */
int hid_mouse_init(void) {
    if (g_mouse_initialized) {
        return 0;
    }
    
    // Clear device array
    memset(g_mouse_devices, 0, sizeof(g_mouse_devices));
    g_device_count = 0;
    g_global_callback = NULL;
    g_mouse_initialized = 1;
    
    return 0;
}

/**
 * Register a HID mouse device
 */
int hid_mouse_register_device(uint32_t device_id, uint8_t interface, 
                             uint8_t endpoint, uint16_t max_packet, uint16_t interval) {
    if (!g_mouse_initialized || g_device_count >= HID_MOUSE_MAX_DEVICES) {
        return -1;
    }
    
    // Check if device already registered
    for (uint32_t i = 0; i < g_device_count; i++) {
        if (g_mouse_devices[i].device_id == device_id) {
            return -1; // Already registered
        }
    }
    
    // Find free slot
    hid_mouse_device_t *device = NULL;
    for (uint32_t i = 0; i < HID_MOUSE_MAX_DEVICES; i++) {
        if (!g_mouse_devices[i].active) {
            device = &g_mouse_devices[i];
            break;
        }
    }
    
    if (!device) {
        return -1; // No free slots
    }
    
    // Initialize device structure
    device->device_id = device_id;
    device->interface = interface;
    device->endpoint = endpoint;
    device->max_packet = max_packet;
    device->interval = interval;
    device->active = 1;
    device->callback = NULL;
    memset(&device->last_report, 0, sizeof(hid_mouse_report_t));
    
    g_device_count++;
    return 0;
}

/**
 * Unregister a HID mouse device
 */
int hid_mouse_unregister_device(uint32_t device_id) {
    if (!g_mouse_initialized) {
        return -1;
    }
    
    // Find device
    for (uint32_t i = 0; i < HID_MOUSE_MAX_DEVICES; i++) {
        if (g_mouse_devices[i].active && g_mouse_devices[i].device_id == device_id) {
            // Clear device structure
            memset(&g_mouse_devices[i], 0, sizeof(hid_mouse_device_t));
            g_device_count--;
            return 0;
        }
    }
    
    return -1; // Device not found
}

/**
 * Process HID mouse report
 */
int hid_mouse_process_report(uint32_t device_id, const uint8_t *data, uint16_t length) {
    if (!g_mouse_initialized || !data || length < HID_MOUSE_REPORT_SIZE) {
        return -1;
    }
    
    // Find device
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return -1;
    }
    
    // Parse report data
    hid_mouse_report_t report;
    report.buttons = data[0];
    report.x = (int8_t)data[1];
    report.y = (int8_t)data[2];
    report.wheel = (length > 3) ? (int8_t)data[3] : 0;
    
    // Create mouse event
    hid_mouse_event_t event;
    event.device_id = device_id;
    event.timestamp = 0; // Would be filled by system timer
    event.x_delta = report.x;
    event.y_delta = report.y;
    event.wheel_delta = report.wheel;
    event.buttons = report.buttons;
    event.button_changed = report.buttons ^ device->last_report.buttons;
    
    // Call device-specific callback
    if (device->callback) {
        device->callback(device_id, &report);
    }
    
    // Call global callback
    if (g_global_callback) {
        g_global_callback(&event);
    }
    
    // Update last report
    device->last_report = report;
    
    return 0;
}

/**
 * Set device-specific callback
 */
int hid_mouse_set_callback(uint32_t device_id, 
                          void (*callback)(uint32_t device_id, hid_mouse_report_t *report)) {
    if (!g_mouse_initialized) {
        return -1;
    }
    
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return -1;
    }
    
    device->callback = callback;
    return 0;
}

/**
 * Set global mouse event callback
 */
int hid_mouse_set_global_callback(void (*callback)(hid_mouse_event_t *event)) {
    if (!g_mouse_initialized) {
        return -1;
    }
    
    g_global_callback = callback;
    return 0;
}

/**
 * Get mouse device by ID
 */
hid_mouse_device_t *hid_mouse_get_device(uint32_t device_id) {
    if (!g_mouse_initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < HID_MOUSE_MAX_DEVICES; i++) {
        if (g_mouse_devices[i].active && g_mouse_devices[i].device_id == device_id) {
            return &g_mouse_devices[i];
        }
    }
    
    return NULL;
}

/**
 * Get number of registered mouse devices
 */
uint32_t hid_mouse_get_device_count(void) {
    return g_device_count;
}

/**
 * Get mouse button state
 */
uint8_t hid_mouse_get_buttons(uint32_t device_id) {
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return 0;
    }
    
    return device->last_report.buttons;
}

/**
 * Check if specific button is pressed
 */
int hid_mouse_is_button_pressed(uint32_t device_id, uint8_t button) {
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return 0;
    }
    
    return (device->last_report.buttons & button) != 0;
}

/**
 * Get last mouse movement
 */
int hid_mouse_get_movement(uint32_t device_id, int8_t *x, int8_t *y) {
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return -1;
    }
    
    if (x) {
        *x = device->last_report.x;
    }
    if (y) {
        *y = device->last_report.y;
    }
    
    return 0;
}

/**
 * Get last wheel movement
 */
int8_t hid_mouse_get_wheel(uint32_t device_id) {
    hid_mouse_device_t *device = hid_mouse_get_device(device_id);
    if (!device) {
        return 0;
    }
    
    return device->last_report.wheel;
}

/**
 * Example mouse event handler
 */
void hid_mouse_example_handler(hid_mouse_event_t *event) {
    if (!event) {
        return;
    }
    
    // Example: Print mouse events (in a real system, this would
    // update cursor position, handle clicks, etc.)
    
    if (event->x_delta != 0 || event->y_delta != 0) {
        // Mouse moved
        // update_cursor_position(event->x_delta, event->y_delta);
    }
    
    if (event->wheel_delta != 0) {
        // Wheel scrolled
        // handle_scroll(event->wheel_delta);
    }
    
    if (event->button_changed) {
        // Button state changed
        if (event->button_changed & HID_MOUSE_BTN_LEFT) {
            if (event->buttons & HID_MOUSE_BTN_LEFT) {
                // Left button pressed
                // handle_left_click_down();
            } else {
                // Left button released
                // handle_left_click_up();
            }
        }
        
        if (event->button_changed & HID_MOUSE_BTN_RIGHT) {
            if (event->buttons & HID_MOUSE_BTN_RIGHT) {
                // Right button pressed
                // handle_right_click_down();
            } else {
                // Right button released
                // handle_right_click_up();
            }
        }
        
        if (event->button_changed & HID_MOUSE_BTN_MIDDLE) {
            if (event->buttons & HID_MOUSE_BTN_MIDDLE) {
                // Middle button pressed
                // handle_middle_click_down();
            } else {
                // Middle button released
                // handle_middle_click_up();
            }
        }
    }
}

/**
 * Cleanup HID mouse subsystem
 */
void hid_mouse_cleanup(void) {
    // Clear all devices
    memset(g_mouse_devices, 0, sizeof(g_mouse_devices));
    g_device_count = 0;
    g_global_callback = NULL;
    g_mouse_initialized = 0;
}
