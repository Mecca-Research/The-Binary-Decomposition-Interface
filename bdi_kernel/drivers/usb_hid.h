
/**
 * @file usb_hid.h
 * @brief USB HID driver (keyboard, mouse, etc.)
 * 
 * Phase 5: Storage I/O Fast Paths
 */

#ifndef BDI_USB_HID_H
#define BDI_USB_HID_H

#include <stdint.h>
#include <stddef.h>
#include "xhci.h"

/* USB HID class codes */
#define USB_CLASS_HID       0x03
#define USB_SUBCLASS_BOOT   0x01
#define USB_PROTOCOL_KBD    0x01
#define USB_PROTOCOL_MOUSE  0x02

/**
 * @brief USB HID device
 */
struct usb_hid_dev {
    struct xhci_ctrl *xhci;       /* xHCI controller */
    uint8_t slot_id;              /* Device slot ID */
    uint8_t interface;            /* Interface number */
    uint8_t endpoint;             /* Endpoint address */
    uint16_t max_packet_size;     /* Max packet size */
    uint8_t *report_desc;         /* Report descriptor */
    size_t report_desc_len;       /* Report descriptor length */
    uint8_t protocol;             /* Protocol (keyboard/mouse) */
};

/* Function prototypes */
int usb_hid_init(struct usb_hid_dev *dev, struct xhci_ctrl *xhci, uint8_t slot);
int usb_hid_read_report(struct usb_hid_dev *dev, void *buf, size_t len);
int usb_hid_parse_report(struct usb_hid_dev *dev, const void *report, size_t len);
void usb_hid_cleanup(struct usb_hid_dev *dev);

#endif /* BDI_USB_HID_H */
