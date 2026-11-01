#ifndef KERNEL_USB_TRANSFER_H
#define KERNEL_USB_TRANSFER_H

#include "usb_device.h"

/**
 * @brief Represents a USB setup packet used for control transfers \struct usb_setup_packet
 */
struct usb_setup_packet
{
	uint8_t request_type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} PACKED;

/**
 * @brief Performs a USB control transfer to the specified device using the provided setup packet and data buffer.
 * @param device The USB device to which the control transfer is directed.
 * @param setup The setup packet containing the request type, request, value, index, and length for the control transfer.
 * @param data The data buffer for the control transfer.
 * @param length The length of the data buffer.
 * @param actual_length A pointer to a variable that will store the actual length of the data transferred.
 * @return The result of the control transfer.
 */
int usb_transfer_control(struct usb_device* device, const struct usb_setup_packet* setup, void* data, uint16_t length, uint16_t* actual_length);

/**
 * @brief Performs a USB bulk transfer to the specified device using the provided endpoint address and data buffer.
 * @param device The USB device to which the bulk transfer is directed.
 * @param endpoint_address The address of the endpoint to which the bulk transfer is directed.
 * @param data The data buffer for the bulk transfer.
 * @param length The length of the data buffer.
 * @param actual_length A pointer to a variable that will store the actual length of the data transferred.
 * @return The result of the bulk transfer.
 */
int usb_bulk_transfer(struct usb_device* device, uint8_t endpoint_address, void* data, uint32_t length, uint32_t* actual_length);

#endif // KERNEL_USB_TRANSFER_H