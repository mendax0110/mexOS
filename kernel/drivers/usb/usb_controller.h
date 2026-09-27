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

typedef struct
{
    bool is_supported;
    char* usb_port_names[NUMBER_OF_USB_PORTS];
    uint32_t usb_port_num[NUMBER_OF_USB_PORTS];
} usb_std_1_t;

typedef struct
{
    bool is_supported;
    char* usb_port_names[NUMBER_OF_USB_PORTS];
    uint32_t usb_port_num[NUMBER_OF_USB_PORTS];
} usb_std_2_t;

typedef struct
{
    usb_std_1_t usb_version_1;
    usb_std_2_t usb_version_2;
} usb_std_abs_ifc_t;

void usb_controller_init(void);

void usb_controller_shutdown(void);

void usb_controller_enumerate(usb_std_abs_ifc_t usb_ifc);

bool usb_controller_supports_usb_v1(void);

bool usb_controller_supports_usb_v2(void);

#endif // KERNEL_USB_CONTROLLER_H