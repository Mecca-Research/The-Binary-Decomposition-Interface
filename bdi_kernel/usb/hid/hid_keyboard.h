
// ===================================================================
// DESC: HID Keyboard driver header
// ===================================================================
#ifndef AEON_HID_KEYBOARD_H
#define AEON_HID_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// --- HID Keyboard Constants ---
#define HID_KEYBOARD_REPORT_SIZE    8
#define HID_KEYBOARD_MAX_KEYS       6
#define HID_KEYBOARD_BUFFER_SIZE    256

// --- Modifier Key Bits ---
#define HID_MOD_LEFT_CTRL           (1 << 0)
#define HID_MOD_LEFT_SHIFT          (1 << 1)
#define HID_MOD_LEFT_ALT            (1 << 2)
#define HID_MOD_LEFT_GUI            (1 << 3)
#define HID_MOD_RIGHT_CTRL          (1 << 4)
#define HID_MOD_RIGHT_SHIFT         (1 << 5)
#define HID_MOD_RIGHT_ALT           (1 << 6)
#define HID_MOD_RIGHT_GUI           (1 << 7)

// --- HID Keyboard Report ---
typedef struct {
    uint8_t modifiers;              // Modifier keys
    uint8_t reserved;               // Reserved byte
    uint8_t keys[HID_KEYBOARD_MAX_KEYS]; // Pressed keys
} __attribute__((packed)) hid_keyboard_report_t;

// --- Key Event ---
typedef struct {
    uint8_t scancode;               // HID scancode
    uint8_t ascii;                  // ASCII character (if printable)
    uint8_t modifiers;              // Modifier state
    bool pressed;                   // True if pressed, false if released
} hid_key_event_t;

// --- Keyboard State ---
typedef struct {
    uint8_t slot_id;                // USB slot ID
    uint8_t ep_id;                  // Interrupt endpoint ID
    uint8_t interface_num;          // Interface number
    
    // Current state
    hid_keyboard_report_t current_report;
    hid_keyboard_report_t previous_report;
    uint8_t modifiers;
    
    // Key event buffer
    hid_key_event_t event_buffer[HID_KEYBOARD_BUFFER_SIZE];
    uint32_t event_head;
    uint32_t event_tail;
    uint32_t event_count;
    
    // LED state
    uint8_t led_state;              // Num Lock, Caps Lock, Scroll Lock
    
    bool initialized;
} hid_keyboard_t;

// --- HID Scancodes to ASCII mapping ---
extern const uint8_t hid_scancode_to_ascii[256];
extern const uint8_t hid_scancode_to_ascii_shift[256];

// --- Function Declarations ---

// Keyboard management
int hid_keyboard_init(hid_keyboard_t* kbd, uint8_t slot_id, uint8_t ep_id, uint8_t interface_num);
int hid_keyboard_shutdown(hid_keyboard_t* kbd);

// Input processing
int hid_keyboard_process_report(hid_keyboard_t* kbd, const uint8_t* report_data, uint32_t length);
int hid_keyboard_get_event(hid_keyboard_t* kbd, hid_key_event_t* event);
bool hid_keyboard_has_events(hid_keyboard_t* kbd);

// LED control
int hid_keyboard_set_leds(hid_keyboard_t* kbd, uint8_t led_state);
uint8_t hid_keyboard_get_leds(hid_keyboard_t* kbd);

// Utility functions
uint8_t hid_scancode_to_ascii(uint8_t scancode, bool shift, bool caps_lock);
bool hid_is_modifier_key(uint8_t scancode);
const char* hid_scancode_to_name(uint8_t scancode);

// LED bits
#define HID_LED_NUM_LOCK            (1 << 0)
#define HID_LED_CAPS_LOCK           (1 << 1)
#define HID_LED_SCROLL_LOCK         (1 << 2)
#define HID_LED_COMPOSE             (1 << 3)
#define HID_LED_KANA                (1 << 4)

// Common HID scancodes
#define HID_KEY_A                   0x04
#define HID_KEY_B                   0x05
#define HID_KEY_C                   0x06
#define HID_KEY_D                   0x07
#define HID_KEY_E                   0x08
#define HID_KEY_F                   0x09
#define HID_KEY_G                   0x0A
#define HID_KEY_H                   0x0B
#define HID_KEY_I                   0x0C
#define HID_KEY_J                   0x0D
#define HID_KEY_K                   0x0E
#define HID_KEY_L                   0x0F
#define HID_KEY_M                   0x10
#define HID_KEY_N                   0x11
#define HID_KEY_O                   0x12
#define HID_KEY_P                   0x13
#define HID_KEY_Q                   0x14
#define HID_KEY_R                   0x15
#define HID_KEY_S                   0x16
#define HID_KEY_T                   0x17
#define HID_KEY_U                   0x18
#define HID_KEY_V                   0x19
#define HID_KEY_W                   0x1A
#define HID_KEY_X                   0x1B
#define HID_KEY_Y                   0x1C
#define HID_KEY_Z                   0x1D

#define HID_KEY_1                   0x1E
#define HID_KEY_2                   0x1F
#define HID_KEY_3                   0x20
#define HID_KEY_4                   0x21
#define HID_KEY_5                   0x22
#define HID_KEY_6                   0x23
#define HID_KEY_7                   0x24
#define HID_KEY_8                   0x25
#define HID_KEY_9                   0x26
#define HID_KEY_0                   0x27

#define HID_KEY_ENTER               0x28
#define HID_KEY_ESCAPE              0x29
#define HID_KEY_BACKSPACE           0x2A
#define HID_KEY_TAB                 0x2B
#define HID_KEY_SPACE               0x2C
#define HID_KEY_MINUS               0x2D
#define HID_KEY_EQUAL               0x2E
#define HID_KEY_LEFT_BRACKET        0x2F
#define HID_KEY_RIGHT_BRACKET       0x30
#define HID_KEY_BACKSLASH           0x31
#define HID_KEY_SEMICOLON           0x33
#define HID_KEY_APOSTROPHE          0x34
#define HID_KEY_GRAVE               0x35
#define HID_KEY_COMMA               0x36
#define HID_KEY_PERIOD              0x37
#define HID_KEY_SLASH               0x38

#define HID_KEY_CAPS_LOCK           0x39
#define HID_KEY_F1                  0x3A
#define HID_KEY_F2                  0x3B
#define HID_KEY_F3                  0x3C
#define HID_KEY_F4                  0x3D
#define HID_KEY_F5                  0x3E
#define HID_KEY_F6                  0x3F
#define HID_KEY_F7                  0x40
#define HID_KEY_F8                  0x41
#define HID_KEY_F9                  0x42
#define HID_KEY_F10                 0x43
#define HID_KEY_F11                 0x44
#define HID_KEY_F12                 0x45

#define HID_KEY_PRINT_SCREEN        0x46
#define HID_KEY_SCROLL_LOCK         0x47
#define HID_KEY_PAUSE               0x48
#define HID_KEY_INSERT              0x49
#define HID_KEY_HOME                0x4A
#define HID_KEY_PAGE_UP             0x4B
#define HID_KEY_DELETE              0x4C
#define HID_KEY_END                 0x4D
#define HID_KEY_PAGE_DOWN           0x4E
#define HID_KEY_RIGHT_ARROW         0x4F
#define HID_KEY_LEFT_ARROW          0x50
#define HID_KEY_DOWN_ARROW          0x51
#define HID_KEY_UP_ARROW            0x52

#define HID_KEY_NUM_LOCK            0x53
#define HID_KEY_KP_DIVIDE           0x54
#define HID_KEY_KP_MULTIPLY         0x55
#define HID_KEY_KP_SUBTRACT         0x56
#define HID_KEY_KP_ADD              0x57
#define HID_KEY_KP_ENTER            0x58
#define HID_KEY_KP_1                0x59
#define HID_KEY_KP_2                0x5A
#define HID_KEY_KP_3                0x5B
#define HID_KEY_KP_4                0x5C
#define HID_KEY_KP_5                0x5D
#define HID_KEY_KP_6                0x5E
#define HID_KEY_KP_7                0x5F
#define HID_KEY_KP_8                0x60
#define HID_KEY_KP_9                0x61
#define HID_KEY_KP_0                0x62
#define HID_KEY_KP_PERIOD           0x63

#define HID_KEY_LEFT_CTRL           0xE0
#define HID_KEY_LEFT_SHIFT          0xE1
#define HID_KEY_LEFT_ALT            0xE2
#define HID_KEY_LEFT_GUI            0xE3
#define HID_KEY_RIGHT_CTRL          0xE4
#define HID_KEY_RIGHT_SHIFT         0xE5
#define HID_KEY_RIGHT_ALT           0xE6
#define HID_KEY_RIGHT_GUI           0xE7

#endif // AEON_HID_KEYBOARD_H
