#ifndef SHARED_USB_ABI_H
#define SHARED_USB_ABI_H

#include "types.h"

#define USB_INFO_MAX_CONTROLLERS 8

#define USB_CONTROLLER_UHCI 1
#define USB_CONTROLLER_EHCI 2

#define USB_CONTROLLER_PORT_STATUS_AVAILABLE (1u << 0)

/**
 * @brief Represents information about a USB controller \struct usb_controller_info
 */
struct usb_controller_info
{
    uint32_t type;
    uint32_t index;
    uint32_t port_count;
    uint32_t connected_ports;
    uint32_t enabled_ports;
    uint32_t flags;
};

/**
 * @brief Represents information about the USB system \struct usb_system_info
 */
struct usb_system_info
{
    uint32_t controller_count;
    uint32_t connected_port_count;
    uint32_t enumerated_device_count;
    struct usb_controller_info controllers[USB_INFO_MAX_CONTROLLERS];
};

#endif
