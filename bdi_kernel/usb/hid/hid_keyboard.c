
// ===================================================================
// DESC: HID Keyboard driver implementation
// ===================================================================
// MODERNIZED: Phase 12 - C23 features (nullptr, [[nodiscard]], _Atomic)

#include "hid_keyboard.h"
#include <string.h>
#include <stdio.h>

// --- HID Scancode to ASCII Mapping ---
const uint8_t hid_scancode_to_ascii[256] = {
    0,   0,   0,   0,   'a', 'b', 'c', 'd', // 0x00-0x07
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', // 0x08-0x0F
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', // 0x10-0x17
    'u', 'v', 'w', 'x', 'y', 'z', '1', '2', // 0x18-0x1F
    '3', '4', '5', '6', '7', '8', '9', '0', // 0x20-0x27
    '\n', 0x1B, '\b', '\t', ' ', '-', '=', '[', // 0x28-0x2F (Enter, Esc, Backspace, Tab, Space)
    ']', '\\', 0,   ';', '\'', '`', ',', '.', // 0x30-0x37
    '/', 0,   0,   0,   0,   0,   0,   0,   // 0x38-0x3F (Slash, Caps Lock, F1-F6)
    0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x47 (F7-F12, Print Screen, Scroll Lock)
    0,   0,   0,   0,   0x7F, 0,   0,   0,   // 0x48-0x4F (Pause, Insert, Home, Page Up, Delete)
    0,   0,   0,   0,   '/', '*', '-', '+', // 0x50-0x57 (Arrows, Num Lock, KP operations)
    '\n', '1', '2', '3', '4', '5', '6', '7', // 0x58-0x5F (KP Enter, KP 1-7)
    '8', '9', '0', '.', 0,   0,   0,   0,   // 0x60-0x67 (KP 8-0, KP Period)
    // Rest filled with zeros
};

const uint8_t hid_scancode_to_ascii_shift[256] = {
    0,   0,   0,   0,   'A', 'B', 'C', 'D', // 0x00-0x07
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', // 0x08-0x0F
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', // 0x10-0x17
    'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@', // 0x18-0x1F
    '#', '$', '%', '^', '&', '*', '(', ')', // 0x20-0x27
    '\n', 0x1B, '\b', '\t', ' ', '_', '+', '{', // 0x28-0x2F
    '}', '|', 0,   ':', '"', '~', '<', '>', // 0x30-0x37
    '?', 0,   0,   0,   0,   0,   0,   0,   // 0x38-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x47
    0,   0,   0,   0,   0x7F, 0,   0,   0,   // 0x48-0x4F
    0,   0,   0,   0,   '/', '*', '-', '+', // 0x50-0x57
    '\n', '1', '2', '3', '4', '5', '6', '7', // 0x58-0x5F
    '8', '9', '0', '.', 0,   0,   0,   0,   // 0x60-0x67
    // Rest filled with zeros
};

// --- Keyboard Management ---

[[nodiscard]] int hid_keyboard_init(hid_keyboard_t* kbd, uint8_t slot_id, uint8_t ep_id, uint8_t interface_num) {
    memset(kbd, 0, sizeof(hid_keyboard_t));
    
    kbd->slot_id = slot_id;
    kbd->ep_id = ep_id;
    kbd->interface_num = interface_num;
    kbd->event_head = 0;
    kbd->event_tail = 0;
    kbd->event_count = 0;
    kbd->led_state = 0;
    
    printf("HID: Keyboard initialized (slot=%u, ep=%u, interface=%u)\n", 
           slot_id, ep_id, interface_num);
    
    kbd->initialized = true;
    return 0;
}

[[nodiscard]] int hid_keyboard_shutdown(hid_keyboard_t* kbd) {
    if (!kbd->initialized) {
        return 0;
    }
    
    memset(kbd, 0, sizeof(hid_keyboard_t));
    printf("HID: Keyboard shutdown completed\n");
    return 0;
}

// --- Input Processing ---

static void hid_keyboard_add_event(hid_keyboard_t* kbd, uint8_t scancode, bool pressed) {
    if (kbd->event_count >= HID_KEYBOARD_BUFFER_SIZE) {
        // Buffer full, drop oldest event
        kbd->event_head = (kbd->event_head + 1) % HID_KEYBOARD_BUFFER_SIZE;
        kbd->event_count--;
    }
    
    hid_key_event_t* event = &kbd->event_buffer[kbd->event_tail];
    event->scancode = scancode;
    event->pressed = pressed;
    event->modifiers = kbd->modifiers;
    event->ascii = hid_scancode_to_ascii(scancode, 
                                        (kbd->modifiers & (HID_MOD_LEFT_SHIFT | HID_MOD_RIGHT_SHIFT)) != 0,
                                        (kbd->led_state & HID_LED_CAPS_LOCK) != 0);
    
    kbd->event_tail = (kbd->event_tail + 1) % HID_KEYBOARD_BUFFER_SIZE;
    kbd->event_count++;
}

[[nodiscard]] int hid_keyboard_process_report(hid_keyboard_t* kbd, const uint8_t* report_data, uint32_t length) {
    if (!kbd->initialized || length < HID_KEYBOARD_REPORT_SIZE) {
        return -1;
    }
    
    // Copy previous report
    memcpy(&kbd->previous_report, &kbd->current_report, sizeof(hid_keyboard_report_t));
    
    // Parse new report
    kbd->current_report.modifiers = report_data[0];
    kbd->current_report.reserved = report_data[1];
    memcpy(kbd->current_report.keys, &report_data[2], HID_KEYBOARD_MAX_KEYS);
    
    // Update modifier state
    kbd->modifiers = kbd->current_report.modifiers;
    
    // Check for modifier changes
    uint8_t modifier_changes = kbd->current_report.modifiers ^ kbd->previous_report.modifiers;
    if (modifier_changes) {
        for (int i = 0; i < 8; i++) {
            if (modifier_changes & (1 << i)) {
                uint8_t modifier_scancode = HID_KEY_LEFT_CTRL + i;
                bool pressed = (kbd->current_report.modifiers & (1 << i)) != 0;
                hid_keyboard_add_event(kbd, modifier_scancode, pressed);
            }
        }
    }
    
    // Check for key releases (keys in previous but not in current)
    for (int i = 0; i < HID_KEYBOARD_MAX_KEYS; i++) {
        uint8_t prev_key = kbd->previous_report.keys[i];
        if (prev_key == 0) continue;
        
        bool found = false;
        for (int j = 0; j < HID_KEYBOARD_MAX_KEYS; j++) {
            if (kbd->current_report.keys[j] == prev_key) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            hid_keyboard_add_event(kbd, prev_key, false);
        }
    }
    
    // Check for key presses (keys in current but not in previous)
    for (int i = 0; i < HID_KEYBOARD_MAX_KEYS; i++) {
        uint8_t curr_key = kbd->current_report.keys[i];
        if (curr_key == 0) continue;
        
        bool found = false;
        for (int j = 0; j < HID_KEYBOARD_MAX_KEYS; j++) {
            if (kbd->previous_report.keys[j] == curr_key) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            hid_keyboard_add_event(kbd, curr_key, true);
            
            // Handle special keys
            if (curr_key == HID_KEY_CAPS_LOCK) {
                kbd->led_state ^= HID_LED_CAPS_LOCK;
                hid_keyboard_set_leds(kbd, kbd->led_state);
            } else if (curr_key == HID_KEY_NUM_LOCK) {
                kbd->led_state ^= HID_LED_NUM_LOCK;
                hid_keyboard_set_leds(kbd, kbd->led_state);
            } else if (curr_key == HID_KEY_SCROLL_LOCK) {
                kbd->led_state ^= HID_LED_SCROLL_LOCK;
                hid_keyboard_set_leds(kbd, kbd->led_state);
            }
        }
    }
    
    return 0;
}

[[nodiscard]] int hid_keyboard_get_event(hid_keyboard_t* kbd, hid_key_event_t* event) {
    if (!kbd->initialized || kbd->event_count == 0) {
        return -1;
    }
    
    memcpy(event, &kbd->event_buffer[kbd->event_head], sizeof(hid_key_event_t));
    kbd->event_head = (kbd->event_head + 1) % HID_KEYBOARD_BUFFER_SIZE;
    kbd->event_count--;
    
    return 0;
}

[[nodiscard]] bool hid_keyboard_has_events(hid_keyboard_t* kbd) {
    return kbd->initialized && kbd->event_count > 0;
}

// --- LED Control ---

[[nodiscard]] int hid_keyboard_set_leds(hid_keyboard_t* kbd, uint8_t led_state) {
    if (!kbd->initialized) {
        return -1;
    }
    
    kbd->led_state = led_state;
    
    // In a real implementation, this would send a SET_REPORT request
    // to the USB device to update the LED state
    printf("HID: Setting keyboard LEDs: 0x%02x\n", led_state);
    
    return 0;
}

[[nodiscard]] uint8_t hid_keyboard_get_leds(hid_keyboard_t* kbd) {
    return kbd->initialized ? kbd->led_state : 0;
}

// --- Utility Functions ---

[[nodiscard]] uint8_t hid_scancode_to_ascii(uint8_t scancode, bool shift, bool caps_lock) {
    if (scancode >= 256) {
        return 0;
    }
    
    uint8_t ascii;
    if (shift) {
        ascii = hid_scancode_to_ascii_shift[scancode];
    } else {
        ascii = hid_scancode_to_ascii[scancode];
    }
    
    // Apply caps lock to letters
    if (caps_lock && ascii >= 'a' && ascii <= 'z') {
        ascii = ascii - 'a' + 'A';
    } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
        ascii = ascii - 'A' + 'a';
    }
    
    return ascii;
}

[[nodiscard]] bool hid_is_modifier_key(uint8_t scancode) {
    return (scancode >= HID_KEY_LEFT_CTRL && scancode <= HID_KEY_RIGHT_GUI);
}

const char* hid_scancode_to_name(uint8_t scancode) {
    switch (scancode) {
        case HID_KEY_A: return "A";
        case HID_KEY_B: return "B";
        case HID_KEY_C: return "C";
        case HID_KEY_D: return "D";
        case HID_KEY_E: return "E";
        case HID_KEY_F: return "F";
        case HID_KEY_G: return "G";
        case HID_KEY_H: return "H";
        case HID_KEY_I: return "I";
        case HID_KEY_J: return "J";
        case HID_KEY_K: return "K";
        case HID_KEY_L: return "L";
        case HID_KEY_M: return "M";
        case HID_KEY_N: return "N";
        case HID_KEY_O: return "O";
        case HID_KEY_P: return "P";
        case HID_KEY_Q: return "Q";
        case HID_KEY_R: return "R";
        case HID_KEY_S: return "S";
        case HID_KEY_T: return "T";
        case HID_KEY_U: return "U";
        case HID_KEY_V: return "V";
        case HID_KEY_W: return "W";
        case HID_KEY_X: return "X";
        case HID_KEY_Y: return "Y";
        case HID_KEY_Z: return "Z";
        case HID_KEY_1: return "1";
        case HID_KEY_2: return "2";
        case HID_KEY_3: return "3";
        case HID_KEY_4: return "4";
        case HID_KEY_5: return "5";
        case HID_KEY_6: return "6";
        case HID_KEY_7: return "7";
        case HID_KEY_8: return "8";
        case HID_KEY_9: return "9";
        case HID_KEY_0: return "0";
        case HID_KEY_ENTER: return "Enter";
        case HID_KEY_ESCAPE: return "Escape";
        case HID_KEY_BACKSPACE: return "Backspace";
        case HID_KEY_TAB: return "Tab";
        case HID_KEY_SPACE: return "Space";
        case HID_KEY_CAPS_LOCK: return "Caps Lock";
        case HID_KEY_F1: return "F1";
        case HID_KEY_F2: return "F2";
        case HID_KEY_F3: return "F3";
        case HID_KEY_F4: return "F4";
        case HID_KEY_F5: return "F5";
        case HID_KEY_F6: return "F6";
        case HID_KEY_F7: return "F7";
        case HID_KEY_F8: return "F8";
        case HID_KEY_F9: return "F9";
        case HID_KEY_F10: return "F10";
        case HID_KEY_F11: return "F11";
        case HID_KEY_F12: return "F12";
        case HID_KEY_LEFT_CTRL: return "Left Ctrl";
        case HID_KEY_LEFT_SHIFT: return "Left Shift";
        case HID_KEY_LEFT_ALT: return "Left Alt";
        case HID_KEY_LEFT_GUI: return "Left GUI";
        case HID_KEY_RIGHT_CTRL: return "Right Ctrl";
        case HID_KEY_RIGHT_SHIFT: return "Right Shift";
        case HID_KEY_RIGHT_ALT: return "Right Alt";
        case HID_KEY_RIGHT_GUI: return "Right GUI";
        default: return "Unknown";
    }
}
