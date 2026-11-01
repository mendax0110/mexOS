#include "runtime.h"
#include "../../shared/usb_abi.h"

static void print_port_state(const struct usb_controller_info* controller, const uint32_t port)
{
    user_print("  port ");
    user_print_dec((int)port);
    user_print(": ");

    if (controller->connected_ports & (1u << port))
    {
        user_print("connected");
        if (controller->enabled_ports & (1u << port))
        {
            user_print(", enabled");
        }
        user_println(" (device not enumerated)");
        return;
    }

    user_println("empty");
}

int main(void)
{
    struct usb_system_info info;
    if (usb_get_info(&info) != 0)
    {
        user_println("usbinfo: failed to query USB status");
        return 1;
    }

    user_println("USB host-controller status");
    user_print("Controllers detected: ");
    user_print_dec((int)info.controller_count);
    user_print("\n");

    if (info.controller_count == 0)
    {
        user_println("No supported USB host controllers detected.");
        return 0;
    }

    for (uint32_t i = 0; i < info.controller_count; i++)
    {
        const struct usb_controller_info* controller = &info.controllers[i];

        user_print("Controller ");
        user_print_dec((int)controller->index);
        user_print(": ");

        if (controller->type == USB_CONTROLLER_UHCI)
        {
            user_println("UHCI");
        }
        else if (controller->type == USB_CONTROLLER_EHCI)
        {
            user_println("EHCI");
        }
        else
        {
            user_println("unknown");
            continue;
        }

        if (!(controller->flags & USB_CONTROLLER_PORT_STATUS_AVAILABLE))
        {
            user_println("  Port status polling is not implemented for this controller.");
            continue;
        }

        for (uint32_t port = 0; port < controller->port_count; port++)
        {
            print_port_state(controller, port);
        }
    }

    user_print("Connected ports: ");
    user_print_dec((int)info.connected_port_count);
    user_println("");
    user_println("USB device enumeration and mass-storage support are not implemented yet.");
    return 0;
}
