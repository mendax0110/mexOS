#ifndef KERNEL_USB_DEVICE_H
#define KERNEL_USB_DEVICE_H

#include "../../../shared/types.h"

/**
 * @brief Represents a USB device with its properties and state \struct usb_device
 */
struct usb_device
{
	uint8_t address;
	uint8_t endpoint0_max_packet_size;
	bool low_speed;
	uint32_t controller_index;
	uint8_t bulk_in_endpoint;
	uint8_t bulk_out_endpoint;
	uint8_t bulk_in_max_packet_size;
	uint8_t bulk_out_max_packet_size;
	uint8_t bulk_in_toggle;
	uint8_t bulk_out_toggle;
};

#endif // KERNEL_USB_DEVICE_H