#include "usb.h"
#include "usb_controller.h"
#include "lib/log.h"

void usb_init(void)
{
    log_info("Initalizing USB subsystem");
    usb_controller_init();
}

void usb_shutdown(void)
{
    log_info("Shutting down USB subsystem");
    usb_controller_shutdown();
}

bool usb_is_v1_supported(void)
{
    return usb_controller_supports_usb_v1();
}

bool usb_is_v2_supported(void)
{
    return usb_controller_supports_usb_v2();
}