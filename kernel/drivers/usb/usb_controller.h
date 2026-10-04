#ifndef KERNEL_USB_CONTROLLER_H
#define KERNEL_USB_CONTROLLER_H

#include "../shared/types.h"

#define NUMBER_OF_USB_PORTS 4

#define USB_PCI_CLASS_SERIAL_BUS 0x0C
#define USB_PCI_SUBCLASS_USB 0x03

#define USB_PCI_PROGIF_UHCI 0x00
#define USB_PCI_PROGIF_OHCI 0x10
#define USB_PCI_PROGIF_EHCI 0x20
#define USB_PCI_PROGIF_XHCI 0x30

#define UHCI_PORTS_PER_CONTROLLER 2
#define UHCI_PORTSC_BASE_OFFSET 0x10

#define UHCI_PORTSC_CURRENT_CONNECT_STATUS (1 << 0)
#define UHCI_PORTSC_CONNECT_STATUS_CHANGE (1 << 1)
#define UHCI_PORTSC_PORT_ENABLED (1 << 2)
#define UHCI_PORTSC_PORT_ENABLE_CHANGE (1 << 3)
#define UHCI_PORTSC_RESET (1 << 9)
#define UHCI_PORTSC_LOW_SPEED_DEVICE (1 << 8)

/**
 * @brief Represents the USB standard interface for version 1 (UHCI/OHCI) \struct usb_std_1_t
 */
typedef struct
{
    bool is_supported;
    char* usb_port_names[NUMBER_OF_USB_PORTS];
    uint32_t usb_port_num[NUMBER_OF_USB_PORTS];
} usb_std_1_t;

/**
 * @brief Represents the USB standard interface for version 2 (EHCI) \struct usb_std_2_t
 */
typedef struct
{
    bool is_supported;
    char* usb_port_names[NUMBER_OF_USB_PORTS];
    uint32_t usb_port_num[NUMBER_OF_USB_PORTS];
} usb_std_2_t;

/**
 * @brief Interface for USB standard abstraction, combining both version 1 and version 2 interfaces \struct usb_std_abs_ifc_t
 */
typedef struct
{
    usb_std_1_t usb_version_1;
    usb_std_2_t usb_version_2;
} usb_std_abs_ifc_t;

/**
 * @brief Initializes the USB controller
 */
void usb_controller_init(void);

/**
 * @brief Shuts down the USB controller
 */
void usb_controller_shutdown(void);

/**
 * @brief Enumerates the USB controller with the provided interface
 * @param usb_ifc The USB standard abstraction interface to be used for enumeration
 */
void usb_controller_enumerate(usb_std_abs_ifc_t usb_ifc);

/**
 * @brief Detects changes in the USB bus, such as device connections or disconnections
 * @return true if a change was detected, false otherwise
 */
bool usb_controller_detect_bus_change(void);

/**
 * @brief Ejects a USB device from the specified controller and port
 * @param controller_index The index of the USB controller
 * @param port_index The index of the USB port
 * @return true if the device was successfully ejected, false otherwise
 */
bool usb_controller_eject_device(uint32_t controller_index, uint8_t port_index);

/**
 * @brief Checks if the USB controller supports USB version 1
 * @return true if supported, false otherwise
 */
bool usb_controller_supports_usb_v1(void);

/**
 * @brief Checks if the USB controller supports USB version 2
 * @return true if supported, false otherwise
 */
bool usb_controller_supports_usb_v2(void);

/**
 * @brief Mounts a USB device on the specified controller and port
 * @param controller_index The index of the USB controller
 * @param port_index The index of the USB port
 * @return true if the device was successfully mounted, false otherwise
 */
bool usb_controller_mount_device(uint32_t controller_index, uint8_t port_index);

/**
 * @brief Checks if a USB device is mounted on the specified controller and port
 * @param controller_index The index of the USB controller
 * @param port_index The index of the USB port
 * @return true if the device is mounted, false otherwise
 */
bool usb_controller_is_mounted(uint32_t controller_index, uint8_t port_index);

#endif // KERNEL_USB_CONTROLLER_H