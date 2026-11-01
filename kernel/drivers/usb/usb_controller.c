#include "usb_controller.h"
#include "arch/i686/arch.h"
#include "drivers/bus/pci.h"
#include "ipc/ipc.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/heap.h"
#include "uhci.h"

static usb_std_abs_ifc_t usb_controller_ifc;
static bool usb_port_mounted[NUMBER_OF_USB_PORTS][UHCI_PORTS_PER_CONTROLLER];

static char* usb_controller_make_name(const struct pci_device* dev, const char* kind)
{
    char* name = kmalloc(32);
    if (!name)
    {
        log_error("Failed to allocate USB controller name");
        return NULL;
    }

    snprintf(name, 32, "%s %d:%d.%d", kind, dev->bus, dev->device, dev->function);
    return name;
}

static bool usb_controller_record(char* slot_names[NUMBER_OF_USB_PORTS],
                                    uint32_t slot_numbers[NUMBER_OF_USB_PORTS],
                                    bool* slot_supported,
                                    const uint32_t index,
                                    struct pci_device* dev,
                                    const char* kind,
                                    const uint8_t bar_index)
{
    uint8_t is_io = 0;
    const uint32_t base = pci_get_bar(dev, bar_index, &is_io);

    if (base == 0 || (kind[0] == 'U' && !is_io))
    {
        log_error_fmt("USB %s controller has invalid BAR", kind);
        return false;
    }

    char* name = usb_controller_make_name(dev, kind);
    if (!name)
    {
        return false;
    }

    if (kind[0] == 'U')
    {
        uint16_t command = pci_config_read_word(dev->bus, dev->device, dev->function, PCI_REG_COMMAND);
        command |= 0x05;
        pci_config_write_word(dev->bus, dev->device, dev->function, PCI_REG_COMMAND, command);

        if (!uhci_init(index, (uint16_t)base))
        {
            log_error_fmt("Failed to initialize UHCI controller at %d:%d.%d",
                          dev->bus, dev->device, dev->function);
            kfree(name);
            return false;
        }
    }
    else
    {
        pci_enable_bus_mastering(dev);
    }

    slot_names[index] = name;
    slot_numbers[index] = base;
    *slot_supported = true;

    log_info_fmt("USB %s controller at %d:%d.%d, base 0x%x (%s)",
                    kind, dev->bus, dev->device, dev->function,
                    base, is_io ? "I/O" : "MMIO");
    return true;
}

static void usb_delay_ms(const uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 1000; i++)
    {
        io_wait();
    }
}

static bool usb_controller_slot_is_uhci(const uint32_t controller_index)
{
    const char* name = usb_controller_ifc.usb_version_1.usb_port_names[controller_index];
    return name &&
            name[0] == 'U' &&
            name[1] == 'H' &&
            name[2] == 'C' &&
            name[3] == 'I';
}

void usb_controller_init(void)
{
    log_info("Initializing USB controller subsystem");

    usb_std_abs_ifc_t ifc;
    memset(&ifc, 0, sizeof(ifc));

    uint32_t v1_count = 0;
    uint32_t v2_count = 0;

    struct pci_device* dev = pci_get_devices();

    while (dev)
    {
        if (dev->class_code == USB_PCI_CLASS_SERIAL_BUS &&
            dev->subclass == USB_PCI_SUBCLASS_USB)
        {
            switch (dev->prog_if)
            {
                case USB_PCI_PROGIF_UHCI:
                {
                    if (v1_count < NUMBER_OF_USB_PORTS)
                    {
                        if (usb_controller_record(ifc.usb_version_1.usb_port_names,
                                                ifc.usb_version_1.usb_port_num,
                                                &ifc.usb_version_1.is_supported,
                                                v1_count, dev, "UHCI", 4))
                        {
                            v1_count++;
                        }
                    }
                    else
                    {
                        log_warn_fmt("UHCI controller at %d:%d.%d skipped, port table full",
                                    dev->bus, dev->device, dev->function);
                    }
                    break;
                }
                case USB_PCI_PROGIF_EHCI:
                {
                    if (v2_count < NUMBER_OF_USB_PORTS)
                    {
                        if (usb_controller_record(ifc.usb_version_2.usb_port_names,
                                                ifc.usb_version_2.usb_port_num,
                                                &ifc.usb_version_2.is_supported,
                                                v2_count, dev, "EHCI", 0))
                        {
                            v2_count++;
                        }
                    }
                    else
                    {
                        log_warn_fmt("EHCI controller at %d:%d.%d skipped, port table full",
                                    dev->bus, dev->device, dev->function);
                    }
                    break;
                }
                default:
                    log_warn_fmt("Unknown USB controller prog_if 0x%x at %d:%d.%d",
                                dev->prog_if, dev->bus, dev->device, dev->function);
                    break;
            }
        }

        dev = dev->next;
    }

    if (v1_count == 0 && v2_count == 0)
    {
        log_warn("No USB host controllers detected");
    }

    usb_controller_enumerate(ifc);
}

void usb_controller_shutdown(void)
{
    log_info_fmt("Shutting down USB controller subsystem (v1 supported: %d, v2 supported: %d",
                usb_controller_ifc.usb_version_1.is_supported,
                usb_controller_ifc.usb_version_2.is_supported);

    for (uint32_t i = 0; i < NUMBER_OF_USB_PORTS; i++)
    {
        if (usb_controller_ifc.usb_version_1.usb_port_names[i])
        {
            uhci_shutdown(i);
            kfree(usb_controller_ifc.usb_version_1.usb_port_names[i]);
        }
        if (usb_controller_ifc.usb_version_2.usb_port_names[i])
        {
            kfree(usb_controller_ifc.usb_version_2.usb_port_names[i]);
        }
    }

    memset(&usb_controller_ifc, 0, sizeof(usb_controller_ifc));
    memset(usb_port_mounted, 0, sizeof(usb_port_mounted));
}

void usb_controller_enumerate(const usb_std_abs_ifc_t usb_ifc)
{
    usb_controller_ifc = usb_ifc;

    log_warn_fmt("USB enumeration complete: v1=%s v2=%s",
                usb_controller_ifc.usb_version_1.is_supported ? "YES" : "NO",
                usb_controller_ifc.usb_version_2.is_supported ? "YES" : "NO");
}

bool usb_controller_supports_usb_v1(void)
{
    return usb_controller_ifc.usb_version_1.is_supported;
}

bool usb_controller_supports_usb_v2(void)
{
    return usb_controller_ifc.usb_version_2.is_supported;
}

bool usb_controller_detect_bus_change(void)
{
    bool change_detected = false;

    for (uint32_t i = 0; i < NUMBER_OF_USB_PORTS; i++)
    {
        if (!usb_controller_slot_is_uhci(i))
        {
            continue;
        }

        if (!usb_controller_ifc.usb_version_1.usb_port_names[i])
        {
            continue;
        }

        const uint16_t io_base = (uint16_t)usb_controller_ifc.usb_version_1.usb_port_num[i];

        for (uint8_t port = 0; port < UHCI_PORTS_PER_CONTROLLER; port++)
        {
            const uint16_t portsc_addr = (uint16_t)(io_base + UHCI_PORTSC_BASE_OFFSET + (port * 2));
            const uint16_t status = inw(portsc_addr);

            if (status & UHCI_PORTSC_CONNECT_STATUS_CHANGE)
            {
                const bool connected = (status & UHCI_PORTSC_CURRENT_CONNECT_STATUS) != 0;

                if (!connected && i < NUMBER_OF_USB_PORTS)
                {
                    usb_port_mounted[i][port] = false;
                }

                log_info_fmt("USB bus change: controller '%s' port %d device %s",
                            usb_controller_ifc.usb_version_1.usb_port_names[i],
                            port, connected ? "connected" : "disconnected");

                outw(portsc_addr, (uint16_t)(status & (UHCI_PORTSC_CONNECT_STATUS_CHANGE | UHCI_PORTSC_PORT_ENABLE_CHANGE)));
                change_detected = true;
            }
        }
    }

    if (usb_controller_ifc.usb_version_2.is_supported)
    {
        log_warn("USB bus-change polling for EHCI (USB v2) controllers is not yet implemented(needs mapped MMIO access)");
    }

    return change_detected;
}

bool usb_controller_eject_device(const uint32_t controller_index, const uint8_t port_index)
{
    if (controller_index >= NUMBER_OF_USB_PORTS ||
        !usb_controller_ifc.usb_version_1.usb_port_names[controller_index])
    {
        log_error_fmt("Invalid controller index %u", controller_index);
        return false;
    }

    if (!usb_controller_slot_is_uhci(controller_index))
    {
        log_error_fmt("Controller '%s' is not UHCI, unsupported",
                        usb_controller_ifc.usb_version_1.usb_port_names[controller_index]);
        return false;
    }

    if (port_index >= UHCI_PORTS_PER_CONTROLLER)
    {
        log_error_fmt("Invalid port index %u", port_index);
        return false;
    }

    const uint16_t io_base = (uint16_t)usb_controller_ifc.usb_version_1.usb_port_num[controller_index];
    const uint16_t portsc_addr = (uint16_t)(io_base + UHCI_PORTSC_BASE_OFFSET + (port_index * 2));

    const uint16_t status = inw(portsc_addr);

    if (!(status & UHCI_PORTSC_CURRENT_CONNECT_STATUS))
    {
        log_warn_fmt("No device present on controller '%s' port %u",
                    usb_controller_ifc.usb_version_1.usb_port_names[controller_index], port_index);
        return false;
    }

    outw(portsc_addr, (uint16_t)((status & UHCI_PORTSC_PORT_ENABLED) |
                                 (status & (UHCI_PORTSC_CONNECT_STATUS_CHANGE |
                                            UHCI_PORTSC_PORT_ENABLE_CHANGE))));

    usb_port_mounted[controller_index][port_index] = false;
    log_info_fmt("USB device on controller '%s' port %u ejected (port diabled)",
                usb_controller_ifc.usb_version_1.usb_port_names[controller_index]);
    return true;
}

bool usb_controller_mount_device(const uint32_t controller_index, const uint8_t port_index)
{
    if (controller_index >= NUMBER_OF_USB_PORTS ||
        !usb_controller_ifc.usb_version_1.usb_port_names[controller_index])
    {
        log_error_fmt("Invalid controller index %u", controller_index);
        return false;
    }

    if (!usb_controller_slot_is_uhci(controller_index))
    {
        log_error_fmt("Controller '%s' is not UHCI, unsupported",
                        usb_controller_ifc.usb_version_1.usb_port_names[controller_index]);
        return false;
    }

    if (port_index >= UHCI_PORTS_PER_CONTROLLER)
    {
        log_error_fmt("Invalid port index %u", port_index);
        return false;
    }

    if (usb_port_mounted[controller_index][port_index])
    {
        log_warn_fmt("Controller %u port %u already mounted", controller_index, port_index);
        return true;
    }

    const uint16_t io_base = (uint16_t)usb_controller_ifc.usb_version_1.usb_port_num[controller_index];
    const uint16_t portsc = (uint16_t)(io_base + UHCI_PORTSC_BASE_OFFSET + (port_index * 2));
    const uint16_t status = inw(portsc);

    if (!(status & UHCI_PORTSC_CURRENT_CONNECT_STATUS))
    {
        log_warn_fmt("No device on controller '%s' port %u",
                    usb_controller_ifc.usb_version_1.usb_port_names[controller_index], port_index);
        return false;
    }

    outw(portsc, UHCI_PORTSC_RESET);
    usb_delay_ms(50);

    outw(portsc, 0);
    usb_delay_ms(10);

    outw(portsc, UHCI_PORTSC_PORT_ENABLED | UHCI_PORTSC_CONNECT_STATUS_CHANGE | UHCI_PORTSC_PORT_ENABLE_CHANGE);

    bool enabled = false;
    for (uint32_t i = 0; i < 100; i++)
    {
        const uint16_t now = inw(portsc);
        if (!(now & UHCI_PORTSC_CURRENT_CONNECT_STATUS))
        {
            log_warn("Device vanished during reset");
            return false;
        }

        if (now & UHCI_PORTSC_PORT_ENABLED)
        {
            enabled = true;
            break;
        }
        usb_delay_ms(1);
    }

    if (!enabled)
    {
        log_warn_fmt("Port %u failed to enabled after reset", port_index);
        return false;
    }

    usb_port_mounted[controller_index][port_index] = true;

    log_info_fmt("USB device mounted on controller '%s' port %u (%s speed)",
                usb_controller_ifc.usb_version_1.usb_port_names[controller_index], port_index,
                (inw(portsc) & UHCI_PORTSC_LOW_SPEED_DEVICE) ? "low" : "full");

    return true;
}

bool usb_controller_is_mounted(const uint32_t controller_index, const uint8_t port_index)
{
    if (controller_index >= NUMBER_OF_USB_PORTS || port_index >= UHCI_PORTS_PER_CONTROLLER)
    {
        return false;
    }

    return usb_port_mounted[controller_index][port_index];
}

int usb_controller_get_info(struct usb_system_info* out_info)
{
    if (!out_info)
    {
        return -1;
    }

    memset(out_info, 0, sizeof(*out_info));

    for (uint32_t i = 0; i < NUMBER_OF_USB_PORTS; i++)
    {
        if (usb_controller_ifc.usb_version_1.usb_port_names[i])
        {
            if (out_info->controller_count >= USB_INFO_MAX_CONTROLLERS)
            {
                return -1;
            }

            struct usb_controller_info* info = &out_info->controllers[out_info->controller_count++];
            info->type = USB_CONTROLLER_UHCI;
            info->index = i;
            info->port_count = UHCI_PORTS_PER_CONTROLLER;
            info->flags = USB_CONTROLLER_PORT_STATUS_AVAILABLE;

            const uint16_t io_base = (uint16_t)usb_controller_ifc.usb_version_1.usb_port_num[i];
            for (uint32_t port = 0; port < UHCI_PORTS_PER_CONTROLLER; port++)
            {
                const uint16_t portsc_addr = (uint16_t)(io_base + UHCI_PORTSC_BASE_OFFSET + (port * 2));
                const uint16_t status = inw(portsc_addr);
                if (status & UHCI_PORTSC_CURRENT_CONNECT_STATUS)
                {
                    info->connected_ports |= 1u << port;
                    out_info->connected_port_count++;
                }
                if (status & UHCI_PORTSC_PORT_ENABLED)
                {
                    info->enabled_ports |= 1u << port;
                }
            }
        }

        if (usb_controller_ifc.usb_version_2.usb_port_names[i])
        {
            if (out_info->controller_count >= USB_INFO_MAX_CONTROLLERS)
            {
                return -1;
            }

            struct usb_controller_info* info =
                &out_info->controllers[out_info->controller_count++];
            info->type = USB_CONTROLLER_EHCI;
            info->index = i;
        }
    }

    return 0;
}