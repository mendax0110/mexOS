#ifndef KERNEL_UHCI_H
#define KERNEL_UHCI_H

#include "../../../shared/types.h"
#include "usb_device.h"

/**
 * @brief Initializes the UHCI controller for the specified controller index and I/O base address.
 * @param controller_index The index of the UHCI controller to initialize (0 to UHCI_MAX_CONTROLLERS - 1).
 * @param io_base The I/O base address of the UHCI controller.
 * @return true if initialization is successful, false otherwise.
 */
bool uhci_init(uint32_t controller_index, uint16_t io_base);

/**
 * @brief Shuts down the UHCI controller for the specified controller index.
 * @param controller_index The index of the UHCI controller to shut down (0 to UHCI_MAX_CONTROLLERS - 1).
 */
void uhci_shutdown(uint32_t controller_index);

/**
 * @brief Performs a UHCI control transfer to the specified USB device using the provided setup packet and data buffer.
 * @param device The USB device to which the control transfer is directed.
 * @param setup The setup packet containing the request type, request, value, index, and length for the control transfer.
 * @param data The data buffer for the control transfer.
 * @param length The length of the data buffer.
 * @param actual_length A pointer to a variable that will store the actual length of the data transferred.
 * @return 0 on success, -1 on failure.
 */
int uhci_control_transfer(struct usb_device* device, const uint8_t setup[8], void* data, uint16_t length, uint16_t* actual_length);

/**
 * @brief Performs a UHCI bulk transfer to the specified USB device using the provided data buffer.
 * @param device The USB device to which the bulk transfer is directed.
 * @param endpoint_address The address of the endpoint to which the transfer is directed.
 * @param data The data buffer for the bulk transfer.
 * @param length The length of the data buffer.
 * @param actual_length A pointer to a variable that will store the actual length of the data transferred.
 * @return 0 on success, -1 on failure.
 */
int uhci_bulk_transfer(struct usb_device* device, uint8_t endpoint_address, void* data, uint32_t length, uint32_t* actual_length);

#endif
