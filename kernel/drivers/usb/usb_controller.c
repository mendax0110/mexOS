#include "usb_controller.h"
#include "drivers/bus/pci.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/heap.h"

usb_std_abs_ifc_t usb_controller_ifc;

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

static void usb_controller_record(char* slot_names[NUMBER_OF_USB_PORTS],
                                    uint32_t slot_numbers[NUMBER_OF_USB_PORTS],
                                    bool* slot_supported,
                                    const uint32_t index,
                                    struct pci_device* dev,
                                    const char* kind,
                                    const uint8_t bar_index)
{
    uint8_t is_io = 0;
    const uint32_t base = pci_get_bar(dev, bar_index, &is_io);

    slot_names[index] = usb_controller_make_name(dev, kind);
    slot_numbers[index] = base;
    *slot_supported = true;

    pci_enable_bus_mastering(dev);

    log_info_fmt("USB %s controller at %d:%d.%d, base 0x%x (%s)",
                    kind, dev->bus, dev->device, dev->function,
                    base, is_io ? "I/O" : "MMIO");
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
                        usb_controller_record(ifc.usb_version_1.usb_port_names,
                                                ifc.usb_version_1.usb_port_num,
                                                &ifc.usb_version_1.is_supported,
                                                v1_count, dev, "UHCI", 4);
                        v1_count++;
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
                        usb_controller_record(ifc.usb_version_2.usb_port_names,
                                                ifc.usb_version_2.usb_port_num,
                                                &ifc.usb_version_2.is_supported,
                                                v2_count, dev, "EHCI", 0);
                        v2_count++;
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
            kfree(usb_controller_ifc.usb_version_1.usb_port_names[i]);
        }
        if (usb_controller_ifc.usb_version_2.usb_port_names[i])
        {
            kfree(usb_controller_ifc.usb_version_2.usb_port_names[i]);
        }
    }

    memset(&usb_controller_ifc, 0, sizeof(usb_controller_ifc));
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