#ifndef KERNEL_USB_H
#define KERNEL_USB_H

#include "../shared/types.h"

void usb_init(void);

void usb_shutdown(void);

bool usb_is_v1_supported(void);

bool usb_is_v2_supported(void);


#endif // KERNEL_USB_H