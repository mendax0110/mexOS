#include "usb_transfer.h"
#include "uhci.h"

int usb_transfer_control(struct usb_device* device, const struct usb_setup_packet* setup, void* data, const uint16_t length, uint16_t* actual_length)
{
    if (!device || !setup || setup->length != length)
    {
        return -1;
    }

    const uint8_t setup_bytes[8] = {
        setup->request_type,
        setup->request,
        (uint8_t)setup->value,
        (uint8_t)(setup->value >> 8),
        (uint8_t)setup->index,
        (uint8_t)(setup->index >> 8),
        (uint8_t)setup->length,
        (uint8_t)(setup->length >> 8),
    };

    return uhci_control_transfer(device, setup_bytes, data, length, actual_length);
}

int usb_bulk_transfer(struct usb_device* device, const uint8_t endpoint_address, void* data, const uint32_t length, uint32_t* actual_length)
{
    return uhci_bulk_transfer(device, endpoint_address, data, length, actual_length);
}
