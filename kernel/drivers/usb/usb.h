#ifndef KERNEL_USB_H
#define KERNEL_USB_H

#include "../shared/types.h"

/**
 * @brief Initializes the USB subsystem
 */
void usb_init(void);

/**
 * @brief Shuts down the USB subsystem
 */
void usb_shutdown(void);

/**
 * @brief Checks if USB version 1 is supported
 * @return true if USB v1 is supported, false otherwise
 */
bool usb_is_v1_supported(void);

/**
 * @brief Checks if USB version 2 is supported
 * @return true if USB v2 is supported, false otherwise
 */
bool usb_is_v2_supported(void);

/**
 * @brief Prints information about all USB ports
 */
void usb_print_ports(void);

#endif // KERNEL_USB_H